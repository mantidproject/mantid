// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
 * NexusGeometrySave::saveInstrument :
 * Save methods to save geometry and metadata from memory
 * to disk in Nexus file format for Instrument 2.0.
 *
 * @author Takudzwa Makoni, RAL (UKRI), ISIS
 * @date 07/08/2019
 */

#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"

#include "MantidGeometry/Rendering/ShapeInfo.h"

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidNexusGeometry/H5ForwardCompatibility.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include <H5Cpp.h>
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <cmath>
#include <memory>
#include <string>

namespace Mantid {
namespace NexusGeometry {
namespace NexusGeometrySave {

/*
 * Function toStdVector (Overloaded). Store data in Mantid::Kernel::V3D vector
 * into std::vector<double> vector. Used by saveInstrument to write array-type
 * datasets to file.
 *
 * @param data : Mantid::Kernel::V3D vector containing data values
 * @return std::vector<double> vector containing data values in
 * Mantid::Kernel::V3D format.
 */
inline std::vector<double> toStdVector(const V3D &data) {
  std::vector<double> stdVector;
  stdVector.reserve(3);
  stdVector.push_back(data.X());
  stdVector.push_back(data.Y());
  stdVector.push_back(data.Z());
  return stdVector;
}

/*
 * Function toStdVector (Overloaded). Store data in Eigen::Vector3d vector
 * into std::vector<double> vector. Used by saveInstrument to write array-type
 * datasets to file.
 *
 * @param data : Eigen::Vector3d vector containing data values
 * @return std::vector<double> vector containing data values in
 * Eigen::Vector3d format
 */
inline std::vector<double> toStdVector(const Eigen::Vector3d &data) {
  return toStdVector(Kernel::toV3D(data));
}

/*
 * Function: isApproxZero. returns true if all values in an variable-sized
 * std-vector container evaluate to zero with a given level of precision. Used
 * by SaveInstrument methods to determine whether or not to write a dataset to
 * file.
 *
 * @param data : std::vector<double> data
 * @param precision : double precision specifier
 * @return true if all elements are approx zero, else false.
 */
inline bool isApproxZero(const std::vector<double> &data,
                         const double &precision) {

  return std::all_of(data.begin(), data.end(),
                     [&precision](const double &element) {
                       return std::abs(element) < precision;
                     });
}

// overload. return true if vector is approx to zero
inline bool isApproxZero(const Eigen::Vector3d &data, const double &precision) {
  return data.isApprox(Eigen::Vector3d(0, 0, 0), precision);
}

// overload. returns true is angle is approx to zero
inline bool isApproxZero(const Eigen::Quaterniond &data,
                         const double &precision) {
  return data.isApprox(Eigen::Quaterniond(1, 0, 0, 0), precision);
}

/*
 * Function: strTypeOfSize
 * Produces the HDF StrType of size equal to that of the
 * input string.
 *
 * @param str : std::string
 * @return string datatype of size = length of input string
 */
inline H5::StrType strTypeOfSize(const std::string &str) {
  H5::StrType stringType(H5::PredType::C_S1, str.size());
  return stringType;
}

/*
 * Function: writeStrDataset
 * writes a StrType HDF dataset and dataset value to a HDF group.
 *
 * @param grp : HDF group object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
inline void writeStrDataset(H5::Group &grp, const std::string &dSetName,
                            const std::string &dSetVal,
                            const H5::DataSpace &dataSpace = SCALAR) {
  H5::StrType dataType = strTypeOfSize(dSetVal);
  H5::DataSet dSet = grp.createDataSet(dSetName, dataType, dataSpace);
  dSet.write(dSetVal, dataType);
}

/*
 * Function: writeStrAttribute
 * writes a StrType HDF attribute and attribute value to a HDF group.
 *
 * @param grp : HDF group object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
inline void writeStrAttribute(H5::Group &grp, const std::string &attrName,
                              const std::string &attrVal,
                              const H5::DataSpace &dataSpace = SCALAR) {
  H5::StrType dataType = strTypeOfSize(attrVal);
  H5::Attribute attribute = grp.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

/*
 * Function: writeStrAttribute
 * Overload function which writes a StrType HDF attribute and attribute value
 * to a HDF dataset.
 *
 * @param dSet : HDF dataset object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
inline void writeStrAttribute(H5::DataSet &dSet, const std::string &attrName,
                              const std::string &attrVal,
                              const H5::DataSpace &dataSpace = SCALAR) {
  H5::StrType dataType = strTypeOfSize(attrVal);
  auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

// function to create a simple sub-group that has a nexus class attribute,
// inside a parent group.
inline H5::Group simpleNXSubGroup(H5::Group &parent, const std::string &name,
                                  const std::string &nexusAttribute) {
  H5::Group subGroup = parent.createGroup(name);
  writeStrAttribute(subGroup, NX_CLASS, nexusAttribute);
  return subGroup;
}

/*
TODO: DOCUMENTATION
*/
inline void writePixelShape(H5::Group &grp, size_t nCylinders,
                            const std::vector<double> &vertexCoordinates) {

  H5::DataSet cylinders, vertices;
  // prepare the data
  std::vector<int> cylinderData(nCylinders * 3);
  for (size_t i = 0; i < nCylinders * 3; i += 3) {
    for (int j = 0; j < 3; ++j) {
      cylinderData[i + j] = static_cast<int>(i + j); // testing output
    }
  }

  int crank = 2;
  hsize_t cdims[static_cast<hsize_t>(2)];
  cdims[0] = static_cast<hsize_t>(nCylinders); // nCylinders;
  cdims[1] = static_cast<hsize_t>(3);
  H5::DataSpace cspace = H5Screate_simple(crank, cdims, nullptr);

  cylinders = grp.createDataSet(CYLINDERS, H5::PredType::NATIVE_INT, cspace);
  cylinders.write(cylinderData.data(), H5::PredType::NATIVE_INT, cspace);

  int vrank = 2;
  hsize_t vdims[static_cast<hsize_t>(2)];
  vdims[0] = static_cast<hsize_t>(3 * nCylinders);
  vdims[1] = static_cast<hsize_t>(3);
  H5::DataSpace vspace = H5Screate_simple(vrank, vdims, nullptr);

  vertices = grp.createDataSet(VERTICES, H5::PredType::NATIVE_DOUBLE, vspace);
  vertices.write(vertexCoordinates.data(), H5::PredType::NATIVE_DOUBLE, vspace);
  writeStrAttribute(vertices, UNITS, METRES);
}

/*
 * Function: writePixelData
 * write the x, y, and z offset of the pixels from the parent detector bank as
 * HDF5 datasets to HDF5 group. If all of the pixel offsets in either x, y, or z
 * are approximately zero, skips writing that dataset to file. Also write the
 * pixel shape group to file, if there are valid* shapes.
 *
 * *current implementation is that only 'CYLINDER' type shape is considered
 * 'valid'.
 *
 * @param grp : HDF5 parent group
 * @param compInfo : Component Info Instrument cache
 * @param idx : index of bank in cache.
 */
inline void writePixelData(H5::Group &grp,
                           const Geometry::ComponentInfo &compInfo,
                           const size_t idx) {

  // write pixel offset

  H5::DataSet xPixelOffset, yPixelOffset, zPixelOffset; // pixel offset datasets
  auto pixels =
      compInfo.detectorsInSubtree(idx); // indices of all pixels in bank
  const auto nPixels = pixels.size();   // number of pixels

  // prevents undefined behaviour when passing empty container into std::all_of.
  // If children detectors empty, there is no data to write to bank. exits here.
  if (pixels.empty())
    return;

  bool cylindersExist{false};       // for checking if any cylinder types exist
  bool shapesAreHomogeneous{false}; // all shapes equal

  // shape type of the first detector in children detectors
  auto &firstShape = compInfo.shape(pixels.front());
  auto firstShapeInfo = firstShape.shapeInfo();
  auto fType = firstShapeInfo.shape();
  auto fHeight = firstShapeInfo.height();
  auto fRadius = firstShapeInfo.radius();

  // return true if all shape types, heights and radii are equal to the first
  // shape in children detectors.
  shapesAreHomogeneous =
      std::all_of(pixels.begin(), pixels.end(), [&](const size_t &idx) {
        auto &shapeObj = compInfo.shape(idx);

        if (auto *meshObject =
                dynamic_cast<const Geometry::MeshObject *>(&firstShape)) {
          // current implementation only considers solid shapes
          throw std::runtime_error("MeshObject Type pixel shape is not "
                                   "implemented for a detector bank");
        }
        if (auto *meshObject2d =
                dynamic_cast<const Geometry::MeshObject2D *>(&firstShape)) {
          // current implementation only considers solid shapes
          throw std::runtime_error("MeshObject2D Type pixel shape is not "
                                   "implemented for a detector bank");
        }

        auto shapeInfo = shapeObj.shapeInfo();
        auto type = shapeInfo.shape();
        auto height = shapeInfo.height();
        auto radius = shapeInfo.radius();
        // if at least one cylinder is found, change flag to true.
        if (static_cast<int>(type) == 4 /*CYLINDER*/)
          cylindersExist = true;
        return (type == fType && height == fHeight && radius);
      });

  std::vector<double> posx;
  std::vector<double> posy;
  std::vector<double> posz;

  posx.reserve(nPixels);
  posy.reserve(nPixels);
  posz.reserve(nPixels);

  if (!cylindersExist) {
    // if no cylinders exist do not write pixel shape data
    for (std::vector<size_t>::iterator it = pixels.begin(); it != pixels.end();
         ++it) {

      auto offset = Geometry::ComponentInfoBankHelpers::offsetFromAncestor(
          compInfo, idx, *it);

      posx.push_back(offset[0]); // x pixel offset
      posy.push_back(offset[1]); // y pixel offset
      posz.push_back(offset[2]); // z pixel offset
    }
  } else
      // else, if cylinders exist and shapes are homogenous, write cylinder data
      // to pixel group only once.
      if (shapesAreHomogeneous) {

    H5::Group pixelShapeGroup = simpleNXSubGroup(grp, PIXEL_SHAPE, NX_CYLINDER);
    for (std::vector<size_t>::iterator it = pixels.begin(); it != pixels.end();
         ++it) {

      auto offset = Geometry::ComponentInfoBankHelpers::offsetFromAncestor(
          compInfo, idx, *it);

      posx.push_back(offset[0]);
      posy.push_back(offset[1]);
      posz.push_back(offset[2]);
    }

    auto geometry = firstShapeInfo.cylinderGeometry();
    auto &base = geometry.centreOfBottomBase;
    auto &axis = geometry.axis;
    auto &height = geometry.height;
    auto &radius = geometry.radius;

    auto top = Kernel::toVector3d(base) + (height * Kernel::toVector3d(axis));
    Eigen::Affine3d rotation(Eigen::Quaterniond(0, 1, 1, 1));
    // creat an arbitrary noncollinear vector
    auto nonCollinear = rotation * Kernel::toVector3d(axis);

    // get vector that is othogonal to the cylinder axis
    auto orthogonalV = Kernel::toVector3d(axis).cross(nonCollinear);
    auto edge = top + (radius * orthogonalV.normalized());

    std::vector<double> vertices{base[0], base[1], base[2], top[0], top[1],
                                 top[2],  edge[0], edge[1], edge[2]};
    writePixelShape(pixelShapeGroup, 1, vertices);

  } else {

    // Implicitly, if shapesAreHomogeneous is false and cylindersExist is true,
    // there is at least 1 valid* shape (that is 'CYLINDER' under current
    // implementation). Then iterate through pixels to find those with valid
    // shapes

    // create pixel shape group
    H5::Group pixelShapeGroup = simpleNXSubGroup(grp, PIXEL_SHAPE, NX_CYLINDER);
    size_t nCylinders = 0;
    std::vector<double> vertices;

    for (std::vector<size_t>::iterator it = pixels.begin(); it != pixels.end();
         ++it) {

      auto offset = Geometry::ComponentInfoBankHelpers::offsetFromAncestor(
          compInfo, idx, *it);

      posx.push_back(offset[0]);
      posy.push_back(offset[1]);
      posz.push_back(offset[2]);

      // TODO: get pixel verices for each pixel
      if (compInfo.hasValidShape(*it)) {
        auto pixelShapeInfo = compInfo.shape(*it).shapeInfo();
        auto type = pixelShapeInfo.shape();
        // only write if shape is cylinder
        if (static_cast<int>(type) == 4 /*CYLINDER*/) {
          auto geometry = pixelShapeInfo.cylinderGeometry();
          auto &base = geometry.centreOfBottomBase;
          auto &axis = geometry.axis;
          auto &height = geometry.height;
          auto &radius = geometry.radius;

          auto top =
              Kernel::toVector3d(base) + (height * Kernel::toVector3d(axis));
          Eigen::Affine3d rotation(Eigen::Quaterniond(0, 1, 1, 1));
          // creat an arbitrary noncollinear vector
          auto nonCollinear = rotation * Kernel::toVector3d(axis);

          // get vector that is othogonal to the cylinder axis
          auto orthogonalV = Kernel::toVector3d(axis).cross(nonCollinear);
          auto edge = top + (radius * orthogonalV.normalized());

          vertices.push_back(base[0]);
          vertices.push_back(base[1]);
          vertices.push_back(base[2]);
          vertices.push_back(top[0]);
          vertices.push_back(top[1]);
          vertices.push_back(top[2]);
          vertices.push_back(edge[0]);
          vertices.push_back(edge[1]);
          vertices.push_back(edge[2]);

          nCylinders++;
        }
      }
    }
    writePixelShape(pixelShapeGroup, nCylinders, vertices);
  }

  // write pixel offset data

  bool xIsZero = isApproxZero(posx, PRECISION);
  bool yIsZero = isApproxZero(posy, PRECISION);
  bool zIsZero = isApproxZero(posz, PRECISION);

  auto bankName = compInfo.name(idx);
  const auto nDetectorsInBank = static_cast<hsize_t>(posx.size());

  int rank = 1;
  hsize_t dims[static_cast<hsize_t>(1)];
  dims[0] = nDetectorsInBank;

  H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

  if (!xIsZero) {
    xPixelOffset =
        grp.createDataSet(X_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
    xPixelOffset.write(posx.data(), H5::PredType::NATIVE_DOUBLE, space);
    writeStrAttribute(xPixelOffset, UNITS, METRES);
  }

  if (!yIsZero) {
    yPixelOffset =
        grp.createDataSet(Y_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
    yPixelOffset.write(posy.data(), H5::PredType::NATIVE_DOUBLE);
    writeStrAttribute(yPixelOffset, UNITS, METRES);
  }

  if (!zIsZero) {
    zPixelOffset =
        grp.createDataSet(Z_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
    zPixelOffset.write(posz.data(), H5::PredType::NATIVE_DOUBLE);
    writeStrAttribute(zPixelOffset, UNITS, METRES);
  }
}

/*
 * Function: writeNXDetectorNumber
 * For use with NXdetector group. Writes the detector numbers for all detector
 * pixels in compInfo to a new dataset in the group.
 *
 * @param detectorIDs : std::vector<int> container of all detectorIDs to be
 * stored into dataset 'detector_number'.
 * @param compInfo : instrument cache with component info.
 * @idx : size_t index of bank in compInfo.
 */
void writeNXDetectorNumber(H5::Group &grp,
                           const Geometry::ComponentInfo &compInfo,
                           const std::vector<int> &detectorIDs,
                           const size_t idx) {

  H5::DataSet detectorNumber;

  std::vector<int> bankDetIDs; // IDs of detectors beloning to bank
  std::vector<size_t> bankDetectors =
      compInfo.detectorsInSubtree(idx); // Indexes of children detectors in bank
  bankDetIDs.reserve(bankDetectors.size());

  // write the ID for each child detector to std::vector to be written to
  // dataset
  std::for_each(bankDetectors.begin(), bankDetectors.end(),
                [&bankDetIDs, &detectorIDs](const size_t index) {
                  bankDetIDs.push_back(detectorIDs[index]);
                });

  const auto nDetectorsInBank = static_cast<hsize_t>(bankDetIDs.size());

  int rank = 1;
  hsize_t dims[static_cast<hsize_t>(1)];
  dims[0] = nDetectorsInBank;

  H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

  detectorNumber =
      grp.createDataSet(DETECTOR_IDS, H5::PredType::NATIVE_INT, space);
  detectorNumber.write(bankDetIDs.data(), H5::PredType::NATIVE_INT, space);
}

/*
 * Function: writeNXMonitorNumber
 * For use with NXmonitor group. write 'detector_id's of an NXmonitor, which
 * is a specific type of pixel, to its group.
 *
 * @param grp : NXmonitor group (HDF group)
 * @param monitorID : monitor ID to be
 * stored into dataset 'detector_id' (or 'detector_number'. naming convention
 * inconsistency?).
 */
void writeNXMonitorNumber(H5::Group &grp, const int monitorID) {

  // these DataSets are duplicates of each other. written to the NXmonitor
  // group to handle the naming inconsistency. probably temporary.
  H5::DataSet detectorNumber, detector_id;

  int rank = 1;
  hsize_t dims[static_cast<hsize_t>(1)];
  dims[0] = static_cast<hsize_t>(1);

  H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

  // these DataSets are duplicates of each other. written to the group to
  // handle the naming inconsistency. probably temporary.
  detectorNumber =
      grp.createDataSet(DETECTOR_IDS, H5::PredType::NATIVE_INT, space);
  detectorNumber.write(&monitorID, H5::PredType::NATIVE_INT, space);

  detector_id = grp.createDataSet(DETECTOR_ID, H5::PredType::NATIVE_INT, space);
  detector_id.write(&monitorID, H5::PredType::NATIVE_INT, space);
}

/*
 * Function: writeLocation
 * For use with NXdetector group. Writes absolute position of detector bank to
 * dataset and metadata as attributes.
 *
 * @param grp : NXdetector group : (HDF group)
 * @param position : Eigen::Vector3d position of component in instrument cache.
 */
inline void writeLocation(H5::Group &grp, const Eigen::Vector3d &position) {

  std::string dependency = NO_DEPENDENCY; // self dependent

  double norm;

  H5::DataSet location;
  H5::DataSpace dspace;
  H5::DataSpace aspace;

  H5::Attribute vector;
  H5::Attribute units;
  H5::Attribute transformationType;
  H5::Attribute dependsOn;

  H5::StrType strSize;

  int drank = 1;                          // rank of dataset
  hsize_t ddims[static_cast<hsize_t>(1)]; // dimensions of dataset
  ddims[0] = static_cast<hsize_t>(1);     // datapoints in dataset dimension 0

  norm = position.norm();               // norm od the position vector
  auto unitVec = position.normalized(); // unit vector of the position vector
  std::vector<double> stdNormPos =
      toStdVector(unitVec); // convert to std::vector

  dspace = H5Screate_simple(drank, ddims, nullptr); // dataspace for dataset
  location = grp.createDataSet(LOCATION, H5::PredType::NATIVE_DOUBLE,
                               dspace); // dataset location
  location.write(&norm, H5::PredType::NATIVE_DOUBLE,
                 dspace); // write norm to location

  int arank = 1;                          // rank of attribute
  hsize_t adims[static_cast<hsize_t>(3)]; // dimensions of attribute
  adims[0] = 3;                           // datapoints in attribute dimension 0

  aspace = H5Screate_simple(arank, adims, nullptr); // dataspace for attribute
  vector = location.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE,
                                    aspace); // attribute vector
  vector.write(H5::PredType::NATIVE_DOUBLE,
               stdNormPos.data()); // write unit vector to vector

  // units attribute
  strSize = strTypeOfSize(METRES);
  units = location.createAttribute(UNITS, strSize, SCALAR);
  units.write(strSize, METRES);

  // transformation-type attribute
  strSize = strTypeOfSize(TRANSLATION);
  transformationType =
      location.createAttribute(TRANSFORMATION_TYPE, strSize, SCALAR);
  transformationType.write(strSize, TRANSLATION);

  // dependency attribute
  strSize = strTypeOfSize(dependency);
  dependsOn = location.createAttribute(DEPENDS_ON, strSize, SCALAR);
  dependsOn.write(strSize, dependency);
}

/*
 * Function: writeOrientation
 * For use with NXdetector group. Writes the absolute rotation of detector
 * bank to dataset and metadata as attributes.
 *
 * @param grp : NXdetector group : (HDF group)
 * @param rotation : Eigen::Quaterniond rotation of component in instrument
 * cache.
 * @param dependency : dependency of the orientation dataset:
 * Compliant to the Mantid Instrument Definition file, if a translation
 * exists, it precedes a rotation.
 * https://docs.mantidproject.org/nightly/concepts/InstrumentDefinitionFile.html
 */
inline void writeOrientation(H5::Group &grp, const Eigen::Quaterniond &rotation,
                             const std::string &dependency) {

  // dependency for orientation defaults to self-dependent. If Location
  // dataset exists, the orientation will depend on it instead.

  double angle;

  H5::DataSet orientation;
  H5::DataSpace dspace;
  H5::DataSpace aspace;

  H5::Attribute vector;
  H5::Attribute units;
  H5::Attribute transformationType;
  H5::Attribute dependsOn;

  H5::StrType strSize;

  int drank = 1;                          // rank of dataset
  hsize_t ddims[static_cast<hsize_t>(1)]; // dimensions of dataset
  ddims[0] = static_cast<hsize_t>(1);     // datapoints in dataset dimension 0

  angle = std::acos(rotation.w()) * (360.0 / PI); // angle magnitude
  Eigen::Vector3d axisOfRotation = rotation.vec().normalized(); // angle axis
  std::vector<double> stdNormAxis =
      toStdVector(axisOfRotation); // convert to std::vector

  dspace = H5Screate_simple(drank, ddims, nullptr); // dataspace for dataset
  orientation = grp.createDataSet(ORIENTATION, H5::PredType::NATIVE_DOUBLE,
                                  dspace); // dataset orientation
  orientation.write(&angle, H5::PredType::NATIVE_DOUBLE,
                    dspace); // write angle magnitude to orientation

  int arank = 1;                          // rank of attribute
  hsize_t adims[static_cast<hsize_t>(3)]; // dimensions of attribute
  adims[0] = static_cast<hsize_t>(3);     // datapoints in attibute dimension 0

  aspace = H5Screate_simple(arank, adims, nullptr); // dataspace for attribute
  vector = orientation.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE,
                                       aspace); // attribute vector
  vector.write(H5::PredType::NATIVE_DOUBLE,
               stdNormAxis.data()); // write angle axis to vector

  // units attribute
  strSize = strTypeOfSize(DEGREES);
  units = orientation.createAttribute(UNITS, strSize, SCALAR);
  units.write(strSize, DEGREES);

  // transformation-type attribute
  strSize = strTypeOfSize(ROTATION);
  transformationType =
      orientation.createAttribute(TRANSFORMATION_TYPE, strSize, SCALAR);
  transformationType.write(strSize, ROTATION);

  // dependency attribute
  strSize = strTypeOfSize(dependency);
  dependsOn = orientation.createAttribute(DEPENDS_ON, strSize, SCALAR);
  dependsOn.write(strSize, dependency);
}

/*
 * Function: NXInstrument
 * for NXentry parent (root group). Produces an NXinstrument group in the
 * parent group, and writes Nexus compliant datasets and metadata stored in
 * attributes to the new group.
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 * @return NXinstrument group, to be passed into children save methods.
 */
H5::Group NXInstrument(const H5::Group &parent,
                       const Geometry::ComponentInfo &compInfo) {

  std::string nameInCache = compInfo.name(compInfo.root());
  std::string instrName =
      nameInCache.empty() ? "unspecified_instrument" : nameInCache;
  H5::Group childGroup = parent.createGroup(instrName);

  writeStrDataset(childGroup, NAME, instrName);
  writeStrAttribute(childGroup, NX_CLASS, NX_INSTRUMENT);

  std::string defaultShortName = instrName.substr(0, 3);
  H5::DataSet name = childGroup.openDataSet(NAME);
  writeStrAttribute(name, SHORT_NAME, defaultShortName);
  return childGroup;
}

/*
 * Function: saveNXSample
 * For NXentry parent (root group). Produces an NXsample group in the parent
 * group, and writes the Nexus compliant datasets and metadata stored in
 * attributes to the new group.
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
void saveNXSample(const H5::Group &parentGroup,
                  const Geometry::ComponentInfo &compInfo) {

  std::string nameInCache = compInfo.name(compInfo.sample());
  std::string sampleName =
      nameInCache.empty() ? "unspecified_sample" : nameInCache;

  H5::Group childGroup = parentGroup.createGroup(sampleName);
  writeStrAttribute(childGroup, NX_CLASS, NX_SAMPLE);
  writeStrDataset(childGroup, NAME, sampleName);
}

/*
 * Function: saveNXSource
 * For NXentry (root group). Produces an NXsource group in the parent group,
 * and writes the Nexus compliant datasets and metadata stored in attributes
 * to the new group.
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
void saveNXSource(const H5::Group &parentGroup,
                  const Geometry::ComponentInfo &compInfo) {

  size_t index = compInfo.source();

  std::string nameInCache = compInfo.name(index);
  std::string sourceName =
      nameInCache.empty() ? "unspecified_source" : nameInCache;

  std::string dependency = NO_DEPENDENCY;

  Eigen::Vector3d position =
      Mantid::Kernel::toVector3d(compInfo.position(index));
  Eigen::Quaterniond rotation =
      Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

  bool locationIsOrigin = isApproxZero(position, PRECISION);
  bool orientationIsZero = isApproxZero(rotation, PRECISION);

  H5::Group childGroup = parentGroup.createGroup(sourceName);
  writeStrAttribute(childGroup, NX_CLASS, NX_SOURCE);

  // do not write NXtransformations if there is no translation or rotation
  if (!(locationIsOrigin && orientationIsZero)) {
    H5::Group transformations =
        simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

    // self, ".", is the default first NXsource dependency in the chain. first
    // check translation in NXsource is non-zero, and set dependency to
    // location if true and write location. Then check if orientation in
    // NXsource is non-zero, replace dependency with orientation if true. If
    // neither orientation nor location are non-zero, NXsource is self
    // dependent.
    if (!locationIsOrigin) {
      dependency = H5_OBJ_NAME(transformations) + "/" + LOCATION;
      writeLocation(transformations, position);
    }
    if (!orientationIsZero) {
      dependency = H5_OBJ_NAME(transformations) + "/" + ORIENTATION;

      // If location dataset is written to group also, then dependency for
      // orientation dataset containg the rotation transformation will be
      // location. Else dependency for orientation is self.
      std::string rotationDependency =
          locationIsOrigin ? NO_DEPENDENCY
                           : H5_OBJ_NAME(transformations) + "/" + LOCATION;
      writeOrientation(transformations, rotation, rotationDependency);
    }
  }

  writeStrDataset(childGroup, NAME, sourceName);
  writeStrDataset(childGroup, DEPENDS_ON, dependency);
}

/*
 * Function: saveNXMonitor
 * For NXinstrument parent (component info root). Produces an NXmonitor
 * groups from Component info, and saves it in the parent
 * group, along with the Nexus compliant datasets, and metadata stored in
 * attributes to the new group.
 *
 * @param parentGroup : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 * @param monitorID :  ID of the specific monitor.
 * @param index :  index of the specific monitor in the Instrument cache.
 */
void saveNXMonitor(const H5::Group &parentGroup,
                   const Geometry::ComponentInfo &compInfo, const int monitorId,
                   const size_t index) {

  // if the component is unnamed sets the name as unspecified with the
  // location of the component in the cache
  std::string nameInCache = compInfo.name(index);
  std::string monitorName = nameInCache.empty()
                                ? "unspecified_monitor_" + std::to_string(index)
                                : nameInCache;

  Eigen::Vector3d position =
      Mantid::Kernel::toVector3d(compInfo.position(index));
  Eigen::Quaterniond rotation =
      Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

  std::string dependency = NO_DEPENDENCY; // dependency initialiser

  bool locationIsOrigin = isApproxZero(position, PRECISION);
  bool orientationIsZero = isApproxZero(rotation, PRECISION);

  H5::Group childGroup = parentGroup.createGroup(monitorName);
  writeStrAttribute(childGroup, NX_CLASS, NX_MONITOR);

  // do not write NXtransformations if there is no translation or rotation
  if (!(locationIsOrigin && orientationIsZero)) {
    H5::Group transformations =
        simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

    // self, ".", is the default first NXmonitor dependency in the chain. first
    // check translation in NXmonitor is non-zero, and set dependency to
    // location if true and write location. Then check if orientation in
    // NXmonitor is non-zero, replace dependency with orientation if true. If
    // neither orientation nor location are non-zero, NXmonitor is self
    // dependent.
    if (!locationIsOrigin) {
      dependency = H5_OBJ_NAME(transformations) + "/" + LOCATION;
      writeLocation(transformations, position);
    }
    if (!orientationIsZero) {
      dependency = H5_OBJ_NAME(transformations) + "/" + ORIENTATION;

      // If location dataset is written to group also, then dependency for
      // orientation dataset containg the rotation transformation will be
      // location. Else dependency for orientation is self.
      std::string rotationDependency =
          locationIsOrigin ? NO_DEPENDENCY
                           : H5_OBJ_NAME(transformations) + "/" + LOCATION;
      writeOrientation(transformations, rotation, rotationDependency);
    }
  }

  H5::StrType dependencyStrType = strTypeOfSize(dependency);
  writeNXMonitorNumber(childGroup, monitorId);

  writeStrDataset(childGroup, BANK_NAME, monitorName);
  writeStrDataset(childGroup, DEPENDS_ON, dependency);
}

/*
 * Function: saveNXDetectors
 * For NXinstrument parent (component info root). Save method which produces a
 * set of NXdetctor groups from Component info detector banks, and saves it in
 * the parent group, along with the Nexus compliant datasets, and metadata
 * stored in attributes to the new group.
 *
 * @param parentGroup : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 * @param detIDs : global detector IDs, from which those specific to the
 * NXdetector will be extracted.
 */
void saveNXDetector(const H5::Group &parentGroup,
                    const Geometry::ComponentInfo &compInfo,
                    const std::vector<int> &detIds, const size_t index) {

  // if the component is unnamed sets the name as unspecified with the
  // location of the component in the cache
  std::string nameInCache = compInfo.name(index);
  std::string detectorName =
      nameInCache.empty() ? "unspecified_detector_at_" + std::to_string(index)
                          : nameInCache;

  Eigen::Vector3d position =
      Mantid::Kernel::toVector3d(compInfo.position(index));
  Eigen::Quaterniond rotation =
      Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

  std::string dependency = NO_DEPENDENCY; // dependency initialiser

  bool locationIsOrigin = isApproxZero(position, PRECISION);
  bool orientationIsZero = isApproxZero(rotation, PRECISION);

  H5::Group childGroup = parentGroup.createGroup(detectorName);
  writeStrAttribute(childGroup, NX_CLASS, NX_DETECTOR);

  // do not write NXtransformations if there is no translation or rotation
  if (!(locationIsOrigin && orientationIsZero)) {
    H5::Group transformations =
        simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

    // self, ".", is the default first NXdetector dependency in the chain. first
    // check translation in NXdetector is non-zero, and set dependency to
    // location if true and write location. Then check if orientation in
    // NXdetector is non-zero, replace dependency with orientation if true. If
    // neither orientation nor location are non-zero, NXdetector is self
    // dependent.
    if (!locationIsOrigin) {
      dependency = H5_OBJ_NAME(transformations) + "/" + LOCATION;
      writeLocation(transformations, position);
    }
    if (!orientationIsZero) {
      dependency = H5_OBJ_NAME(transformations) + "/" + ORIENTATION;

      // If location dataset is written to group also, then dependency for
      // orientation dataset containg the rotation transformation will be
      // location. Else dependency for orientation is self.
      std::string rotationDependency =
          locationIsOrigin ? NO_DEPENDENCY
                           : H5_OBJ_NAME(transformations) + "/" + LOCATION;
      writeOrientation(transformations, rotation, rotationDependency);
    }
  }

  H5::StrType dependencyStrType = strTypeOfSize(dependency);
  writePixelData(childGroup, compInfo, index);
  writeNXDetectorNumber(childGroup, compInfo, detIds, index);

  writeStrDataset(childGroup, BANK_NAME, detectorName);
  writeStrDataset(childGroup, DEPENDS_ON, dependency);
}

/*
 * Function: saveInstrument
 * calls the save methods to write components to file after exception checking.
 * Produces a Nexus format file containing the Instrument geometry and metadata.
 *
 * @param compInfo : componentInfo object.
 * @param fullPath : save destination as full path.
 * @param reporter : (optional) report to progressBase.
 */
void saveInstrument(
    const std::pair<std::unique_ptr<Geometry::ComponentInfo>,
                    std::unique_ptr<Geometry::DetectorInfo>> &instrPair,
    const std::string &fullPath, const std::string &rootPath,
    Kernel::ProgressBase *reporter) {

  const Geometry::ComponentInfo &compInfo = (*instrPair.first);
  const Geometry::DetectorInfo &detInfo = (*instrPair.second);

  // Exception handling.
  boost::filesystem::path tmp(fullPath);
  if (!boost::filesystem::is_directory(tmp.root_directory())) {
    throw std::invalid_argument(
        "The path provided for saving the file is invalid: " + fullPath + "\n");
  }

  // check the file extension matches any of the valid extensions defined in
  // nexus_geometry_extensions
  const auto ext = boost::filesystem::path(tmp).extension();
  bool isValidExt = std::any_of(
      nexus_geometry_extensions.begin(), nexus_geometry_extensions.end(),
      [&ext](const std::string &str) { return ext.generic_string() == str; });

  // throw if the file extension is invalid
  if (!isValidExt) {

    // string of valid extensions to output in exception
    std::string extensions;
    std::for_each(
        nexus_geometry_extensions.begin(), nexus_geometry_extensions.end(),
        [&extensions](const std::string &str) { extensions += " " + str; });
    throw std::invalid_argument("invalid extension for file: '" +
                                ext.generic_string() +
                                "'. Expected any of: " + extensions);
  }

  if (!compInfo.hasDetectorInfo()) {
    throw std::invalid_argument(
        "No detector info was found in the Instrument cache.\n");
  }
  if (!compInfo.hasSample()) {
    throw std::invalid_argument(
        "No sample was found in the Instrument cache.\n");
  }

  if (Mantid::Kernel::V3D{0, 0, 0} != compInfo.samplePosition()) {
    throw std::invalid_argument(
        "The sample posiiton is required to be at the origin.\n");
  }

  if (!compInfo.hasSource()) {
    throw std::invalid_argument("No source was found in the Instrument cache.");
  }

  // IDs of all detectors in Instrument cache
  const auto detIds = detInfo.detectorIDs();

  H5::H5File file(fullPath, H5F_ACC_TRUNC); // open file

  H5::Group rootGroup, instrument;

  // create and capture NXentry (file root)
  rootGroup = file.createGroup(rootPath);
  NexusGeometrySave::writeStrAttribute(rootGroup, NX_CLASS, NX_ENTRY);

  // save and capture NXinstrument (component root)
  instrument = NexusGeometrySave::NXInstrument(rootGroup, compInfo);

  // save NXsource
  NexusGeometrySave::saveNXSource(instrument, compInfo);

  // save NXsample
  NexusGeometrySave::saveNXSample(rootGroup, compInfo);

  // save NXdetectors
  for (size_t index = compInfo.root() - 1; index > detInfo.size(); --index) {
    if (Geometry::ComponentInfoBankHelpers::isSaveableBank(compInfo, index)) {
      if (reporter != nullptr)
        reporter->report();
      NexusGeometrySave::saveNXDetector(instrument, compInfo, detIds, index);
    }
  }

  // save NXmonitors
  for (size_t index = 0; index < detInfo.size(); ++index) {
    if (detInfo.isMonitor(index)) {
      if (reporter != nullptr)
        reporter->report();
      NexusGeometrySave::saveNXMonitor(instrument, compInfo, detIds[index],
                                       index);
    }
  }

  file.close(); // close file

} // saveInstrument
} // namespace NexusGeometrySave
} // namespace NexusGeometry
} // namespace Mantid
