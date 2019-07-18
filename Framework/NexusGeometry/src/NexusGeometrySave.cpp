// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include <H5Cpp.h>
#include <boost/filesystem/operations.hpp>
#include <memory> // unique_ptr
#include <string>

namespace Mantid {
namespace NexusGeometry {

namespace {

const std::string NX_CLASS = "NX_class";
const std::string NX_SAMPLE = "NXsample";
const std::string NX_DETECTOR = "NXdetector";
const std::string NX_OFF_GEOMETRY = "NXoff_geometry";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_CHAR = "NX_CHAR";
const std::string NX_SOURCE = "NXsource";
const std::string NX_TRANSFORMATION = "NXtransformation";

const std::string SHORT_NAME = "short_name";
const std::string TRANSFORMATIONS = "transformations";
const std::string TRANSLATION = "translation";
const std::string ROTATION = "rotation";
const std::string LOCAL_NAME = "local_name";
const std::string LOCATION = "location";
const std::string ORIENTATION = "orientation";
const std::string DEPENDS_ON = "depends_on";
const std::string X_PIXEL_OFFSET = "x_pixel_offset";
const std::string Y_PIXEL_OFFSET = "y_pixel_offset";
const std::string Z_PIXEL_OFFSET = "z_pixel_offset";
const std::string PIXEL_SHAPE = "pixel_shape";
const std::string SOURCE = "source";
const std::string SAMPLE = "sample";
const std::string DETECTOR_NUMBER = "detector_number";
const std::string TRANSFORMATION_TYPE = "transformation_type";

const std::string METRES = "m";
const std::string DEGREES = "degrees";
const std::string NAME = "name";
const std::string UNITS = "units";
const std::string SHAPE = "shape";
const std::string VECTOR = "vector";

const H5::DataSpace H5SCALAR(H5S_SCALAR);

namespace forwardCompatibility {

/*
 * Function "getObjName"
 * Ported from newer versions of the HDF API for forward compatibility.
 */

ssize_t getObjName(const H5::H5Object &obj, char *obj_name, size_t buf_size) {
  // H5Iget_name will get buf_size-1 chars of the name to null terminate it
  ssize_t name_size = H5Iget_name(obj.getId(), obj_name, buf_size);

  // If H5Iget_name returns a negative value, raise an exception
  if (name_size < 0) {
    throw H5::Exception("getObjName", "H5Iget_name failed");
  } else if (name_size == 0) {
    throw H5::Exception("getObjName",
                        "Object must have a name, but name length is 0");
  }
  // Return length of the name
  return (name_size);
}

std::string getObjName(const H5::H5Object &obj) {
  std::string obj_name(""); // object name to return

  // Preliminary call to get the size of the object name
  ssize_t name_size = H5Iget_name(obj.getId(), NULL, static_cast<size_t>(0));

  // If H5Iget_name failed, throw exception
  if (name_size < 0) {
    throw H5::Exception("getObjName", "H5Iget_name failed");
  } else if (name_size == 0) {
    throw H5::Exception("getObjName",
                        "Object must have a name, but name length is 0");
  }
  // Object's name exists, retrieve it
  else if (name_size > 0) {
    char *name_C = new char[name_size + 1]; // temporary C-string
    memset(name_C, 0, name_size + 1);       // clear buffer

    // Use overloaded function
    name_size = getObjName(obj, name_C, name_size + 1);

    // Convert the C object name to return
    obj_name = name_C;

    // Clean up resource
    delete[] name_C;
  }
  // Return object's name
  return (obj_name);
}

} // namespace forwardCompatibility

} // namespace

namespace NexusGeometrySave {

// TODO: add documenttation
inline Eigen::Translation3d toTranslation3d(const Eigen::Vector3d &vector) {
  Eigen::Translation3d translation(vector);
  return translation;
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
  H5::StrType stringType(1, (size_t)str.length());
  return stringType;
}

/*
 * Function: writeAttribute
 * writes a StrType HDF attribute and attribute value to a HDF group.
 *
 * @param grp : HDF group object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
inline void writeStrAttribute(H5::Group &grp, const std::string &attrName,
                              const std::string &attrVal,
                              H5::DataSpace dataSpace = H5SCALAR) {
  H5::StrType dataType = strTypeOfSize(attrVal);
  H5::Attribute attribute = grp.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

/*
 * Function: writeStrAttribute
 * Overload function which writes a StrType HDF attribute and attribute value to
 * a HDF dataset.
 *
 * @param dSet : HDF dataset object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
inline void writeStrAttribute(H5::DataSet &dSet, const std::string &attrName,
                              const std::string &attrVal,
                              H5::DataSpace dataSpace = H5SCALAR) {
  H5::StrType dataType = strTypeOfSize(attrVal);
  auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeXYZPixeloffset(H5::Group &grp,
                                const Geometry::ComponentInfo &compInfo,
                                const Geometry::DetectorInfo &detInfo,
                                size_t idx) {

  H5::DataSet xPixelOffset, yPixelOffset, zPixelOffset;
  auto childrenDetectors = compInfo.detectorsInSubtree(idx);

  std::vector<double> posx;
  std::vector<double> posy;
  std::vector<double> posz;

  posx.reserve(childrenDetectors.size());
  posy.reserve(childrenDetectors.size());
  posz.reserve(childrenDetectors.size());

  // absolute bank translation and rotation
  const Eigen::Vector3d absBankTransVec =
      Kernel::toVector3d(compInfo.position(idx));
  const Eigen::Quaterniond absBankRot =
      Kernel::toQuaterniond(compInfo.rotation(idx));

  for (const size_t &i : childrenDetectors) {

    Eigen::Vector3d absDetTransVec = Kernel::toVector3d(detInfo.position(i));
    Eigen::Quaterniond absDetRot = Kernel::toQuaterniond(detInfo.rotation(i));
    // relative rotation of detetctor
    Eigen::Quaterniond relDetRot = absDetRot * absBankRot.inverse();

    // detector offest
    Eigen::Vector3d offset =
        relDetRot.inverse() * (absDetTransVec - absBankTransVec);

    posx.push_back(offset[0]);
    posy.push_back(offset[1]);
    posz.push_back(offset[2]);
  }

  const hsize_t dimSize = (hsize_t)posx.size();

  int rank = 1;
  hsize_t dims[1];
  dims[0] = dimSize;

  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

  xPixelOffset =
      grp.createDataSet(X_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
  xPixelOffset.write(posx.data(), H5::PredType::NATIVE_DOUBLE, space);

  yPixelOffset =
      grp.createDataSet(Y_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
  yPixelOffset.write(posy.data(), H5::PredType::NATIVE_DOUBLE);

  zPixelOffset =
      grp.createDataSet(Z_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
  zPixelOffset.write(posz.data(), H5::PredType::NATIVE_DOUBLE);
}

/*
 * Function: writeDetectorNumber
 * For use with NXdetector group. Finds all detector pixels by index in the
 * component Info object and writes the detector numbers to a new dataset in the
 * group.
 *
 * @param grp : NXdetector group (HDF group)
 * @param compInfo : componentInfo object.
 */
inline void writeDetectorNumber(H5::Group &grp,
                                const Geometry::DetectorInfo &detInfo) {

  H5::DataSet detectorNumber;

  std::vector<int> detIDs = detInfo.detectorIDs();
  const hsize_t dimSize = (hsize_t)detIDs.size();

  int rank = 1;
  hsize_t dims[1];
  dims[0] = dimSize;

  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

  detectorNumber =
      grp.createDataSet(DETECTOR_NUMBER, H5::PredType::NATIVE_INT, space);
  detectorNumber.write(detIDs.data(), H5::PredType::NATIVE_INT, space);
}

/*
 * Function: writeLocation
 * For use with NXdetector group. Writes absolute position of detector bank to
 *dataset and metadata as attributes:
 *
 * =>	norm of position vector stored as value in NXtransformation dataset
 *		'location'.
 * =>	normalised position vector stored as array in attribute 'vector'.
 * =>	units of measurement stored as string value in attribute 'units'.
 * =>	type of transformation stored as string value in attribute
 *		'transformation_type'.
 * =>	dependency of transformation stored as string value in attribute
 *		'depends_on'.
 *
 * @param grp : NXdetector group : (HDF group)
 * @param compInfo : componentInfo object.
 */
inline void writeLocation(H5::Group &grp,
                          const Geometry::ComponentInfo &compInfo, size_t idx) {

  Eigen::Vector3d position;
  double norm;

  H5::DataSet location;
  H5::DataSpace dspace;
  H5::DataSpace aspace;

  H5::Attribute vector;
  H5::Attribute units;
  H5::Attribute transformationType;
  H5::Attribute dependsOn;
  H5::Attribute nxClass;

  H5::StrType strSize;

  int drank = 1;
  hsize_t ddims[(hsize_t)1];
  ddims[0] = 1;

  // dependency for location
  std::string dependency = "."; // self dependent

  dspace = H5Screate_simple(drank, ddims, NULL);
  location = grp.createDataSet(LOCATION, H5::PredType::NATIVE_DOUBLE, dspace);

  // write norm of absolute (normalised) position vector of detector to dataset
  // TODO: these should take the idices of detectors instead of hard-coded zero
  position = Kernel::toVector3d(compInfo.position(idx)); // of bank
  norm = position.norm();
  position.normalize(); // inline?
  location.write(&norm, H5::PredType::NATIVE_DOUBLE, dspace);

  // vector attribute
  strSize = strTypeOfSize(NX_TRANSFORMATION);
  nxClass = location.createAttribute(NX_CLASS, strSize, H5SCALAR);
  nxClass.write(strSize, NX_TRANSFORMATION);

  /*
   * Convert Eigen::Vector to std::vector buffer to write to attribute 'vector'.
   * stdNormAxis is the position vector as a std::vector<>. Normalised if norm
   * != 0.
   */
  auto asize = position.size();
  std::vector<double> stdNormPos;
  stdNormPos.resize(asize);
  Eigen::VectorXd::Map(&stdNormPos[0], asize) = position;

  int arank = 1;
  hsize_t adims[(hsize_t)3];
  adims[0] = 3;

  aspace = H5Screate_simple(arank, adims, NULL);
  vector =
      location.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE, aspace);
  vector.write(H5::PredType::NATIVE_DOUBLE, stdNormPos.data());

  // units attribute
  strSize = strTypeOfSize(METRES);
  units = location.createAttribute(UNITS, strSize, H5SCALAR);
  units.write(strSize, METRES);

  // transformation-type attribute
  strSize = strTypeOfSize(TRANSLATION);
  transformationType =
      location.createAttribute(TRANSFORMATION_TYPE, strSize, H5SCALAR);
  transformationType.write(strSize, TRANSLATION);

  // dependency attribute
  strSize = strTypeOfSize(dependency);
  dependsOn = location.createAttribute(DEPENDS_ON, strSize, H5SCALAR);
  dependsOn.write(strSize, dependency);
}

/*
 * Function: writeOrientation
 * For use with NXdetector group. Writes the absolute rotation of detector bank
 *to dataset and metadata as attributes:
 *
 * =>	magnitude of angle of rotation stored as value in NXtransformation
 *dataset 'location'.
 * =>	axis of rotation as unit vector stored as array in attribute 'vector'.
 * =>	units of measurement stored as string value in attribute 'units'.
 * =>	type of transformation stored as string value in attribute
 *		'transformation_type'.
 * =>	dependency of transformation stored as string value in attribute
 *		'depends_on'.
 *
 * @param grp : NXdetector group : (HDF group)
 * @param compInfo : componentInfo object.
 */
inline void writeOrientation(H5::Group &grp,
                             const Geometry::ComponentInfo &compInfo,
                             size_t idx) {

  Eigen::Quaterniond rotation;
  double angle;

  H5::DataSet orientation;
  H5::DataSpace dspace;
  H5::DataSpace aspace;

  H5::Attribute vector;
  H5::Attribute units;
  H5::Attribute transformationType;
  H5::Attribute dependsOn;
  H5::Attribute nxClass;

  H5::StrType strSize;

  int rank = 1;
  hsize_t ddims[(hsize_t)1];
  ddims[0] = (hsize_t)1;

  // dependency for orientation
  std::string dependency = forwardCompatibility::getObjName(grp) + "/" +
                           LOCATION; // depends on location

  dspace = H5Screate_simple(rank, ddims, NULL);
  orientation =
      grp.createDataSet(ORIENTATION, H5::PredType::NATIVE_DOUBLE, dspace);

  // write absolute rotation in degrees of detector bank to dataset
  rotation = Kernel::toQuaterniond(compInfo.rotation(idx));
  angle = std::acos(rotation.w()) * (360.0 / M_PI);
  orientation.write(&angle, H5::PredType::NATIVE_DOUBLE, dspace);

  // NX_class attribute
  strSize = strTypeOfSize(NX_TRANSFORMATION);
  nxClass = orientation.createAttribute(NX_CLASS, strSize, H5SCALAR);
  nxClass.write(strSize, NX_TRANSFORMATION);

  // vector atrribute
  Eigen::Vector3d axisOfRotation = rotation.vec().normalized();

  /*
   * Convert Eigen::Vector to std::vector buffer to write to attribute 'vector'.
   * stdNormAxis is the Axis of rotation as a std::vector<>. Normalised if norm
   * != 0.
   */
  auto asize = axisOfRotation.size();
  std::vector<double> stdNormAxis;
  stdNormAxis.resize(asize);
  Eigen::VectorXd::Map(&stdNormAxis[0], asize) = axisOfRotation;

  int arank = 1;
  hsize_t adims[(hsize_t)3];
  adims[0] = 3;

  aspace = H5Screate_simple(arank, adims, NULL);
  vector =
      orientation.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE, aspace);
  vector.write(H5::PredType::NATIVE_DOUBLE, stdNormAxis.data());

  // units attribute
  strSize = strTypeOfSize(DEGREES);
  units = orientation.createAttribute(UNITS, strSize, H5SCALAR);
  units.write(strSize, DEGREES);

  // transformation-type attribute
  strSize = strTypeOfSize(TRANSLATION);
  transformationType =
      orientation.createAttribute(TRANSFORMATION_TYPE, strSize, H5SCALAR);
  transformationType.write(strSize, ROTATION);

  // dependency attribute
  strSize = strTypeOfSize(dependency);
  dependsOn = orientation.createAttribute(DEPENDS_ON, strSize, H5SCALAR);
  dependsOn.write(strSize, dependency);
}

/*
 * Function: instrument
 * for NXentry parent (root group). Produces an NXinstrument group in the parent
 * group, and writes Nexus compliant datasets and metadata stored in attributes
 * to the new group.
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
H5::Group instrument(const H5::Group &parent,
                     const Geometry::ComponentInfo &compInfo) {

  std::string instrumentNameStr = compInfo.name(compInfo.root());
  H5::Group group = parent.createGroup(instrumentNameStr);
  writeStrAttribute(group, NX_CLASS, NX_INSTRUMENT);
  H5::StrType nameStrType = strTypeOfSize(instrumentNameStr);
  H5::DataSet name = group.createDataSet("name", nameStrType, H5SCALAR);
  writeStrAttribute(name, SHORT_NAME,
                    instrumentNameStr); // placeholder

  name.write(instrumentNameStr, nameStrType, H5SCALAR);

  return group;
}

/*
 * Function: saveSample
 * For NXentry parent (root group). Produces an NXsample group in the parent
 * group, and writes the Nexus compliant datasets and metadata stored in
 * attributes to the new group.
 *
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
void saveSample(const H5::Group &parentGroup,
                const Geometry::ComponentInfo &compInfo) {

  H5::Group childGroup;

  size_t idx = compInfo.sample();

  std::string sampleName = compInfo.name(compInfo.sample());
  childGroup = parentGroup.createGroup(sampleName);

  writeLocation(childGroup, compInfo, idx);
  writeOrientation(childGroup, compInfo, idx);
  writeStrAttribute(childGroup, NX_CLASS, NX_SAMPLE);
}

/*
 * Function: saveSource
 * For NXentry (root group). Produces an NXsource group in the parent group, and
 * writes the Nexus compliant datasets and metadata stored in attributes to the
 * new group.
 *
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
void saveSource(const H5::Group &parentGroup,
                const Geometry::ComponentInfo &compInfo) {

  H5::Group childGroup;
  size_t idx = compInfo.source();

  std::string sourceName = compInfo.name(compInfo.source());
  childGroup = parentGroup.createGroup(sourceName);
  writeStrAttribute(childGroup, NX_CLASS, NX_SOURCE);

  writeLocation(childGroup, compInfo, idx);
  writeOrientation(childGroup, compInfo, idx);
}

/*
 * Function: saveDetectors
 * For NXinstrument parent (root group). Produces a set of NXdetctor groups in
 * the parent group, and writes the Nexus compliant datasets and metadata stored
 * in attributes to the new group. The NXdetector group created will be
 * orientation dependent. returns vector of NXdetectors
 *
 * @param parentGroup : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 * @
 */
void saveDetectors(const H5::Group &parentGroup,
                   const Geometry::ComponentInfo &compInfo,
                   const Geometry::DetectorInfo &detInfo) {

  for (size_t i = compInfo.root() - 1; i > 0; --i) {
    if (compInfo.isDetector(i))
      break;

    if (compInfo.hasParent(i)) {

      size_t parent = compInfo.parent(i);
      auto parentType = compInfo.componentType(parent);

      if (compInfo.detectorsInSubtree(i).size() != 0)
        if (parentType != Beamline::ComponentType::Rectangular &&
            parentType != Beamline::ComponentType::Structured &&
            parentType != Beamline::ComponentType::Grid) {

          std::string name = compInfo.name(i);

          H5::Group childGroup = parentGroup.createGroup(name);
          writeStrAttribute(childGroup, NX_CLASS, NX_DETECTOR);

          std::string localNameStr = name; // optional: possible removal
          H5::StrType localNameStrType = strTypeOfSize(localNameStr);

          std::string dependency =
              forwardCompatibility::getObjName(childGroup) + "/" + ORIENTATION;
          H5::StrType dependencyStrType = strTypeOfSize(dependency);

          writeDetectorNumber(childGroup, detInfo);
          writeLocation(childGroup, compInfo, i);
          writeOrientation(childGroup, compInfo, i);
          writeXYZPixeloffset(childGroup, compInfo, detInfo, i);

          H5::DataSet localName =
              childGroup.createDataSet(LOCAL_NAME, localNameStrType, H5SCALAR);
          localName.write(localNameStr, localNameStrType, H5SCALAR);
          H5::DataSet dependsOn =
              childGroup.createDataSet(DEPENDS_ON, dependencyStrType, H5SCALAR);
          dependsOn.write(dependency, dependencyStrType, H5SCALAR);
        }
    }
  }
}

} // namespace NexusGeometrySave

Eigen::Affine3d toEigenTransform(const V3D vector, const Quat quaternion) {

  Eigen::Vector3d eigenVector = Mantid::Kernel::toVector3d(vector);

  Eigen::Quaterniond eigenVersor = Mantid::Kernel::toQuaterniond(quaternion);

  Eigen::Translation3d translation(eigenVector);

  Eigen::Affine3d compoundTransform = eigenVersor * translation;

  return compoundTransform;
}

/*
 * Function: Saveinstrument
 * writes the instrument from memory to disk in Nexus format.
 *
 * @param compInfo : componentInfo object.
 * @param fullPath : save destination as full path.
 * @param reporter : report to progressBase.
 */
void saveInstrument(
    const std::pair<std::unique_ptr<Geometry::ComponentInfo>,
                    std::unique_ptr<Geometry::DetectorInfo>> &instrPair,
    const std::string &fullPath, Kernel::ProgressBase *reporter) {

  const Geometry::ComponentInfo &compInfo = (*instrPair.first);
  const Geometry::DetectorInfo &detInfo = (*instrPair.second);

  /// Exception handling.
  boost::filesystem::path tmp(fullPath);
  if (!boost::filesystem::is_directory(tmp.root_directory())) {
    throw std::invalid_argument(
        "The path provided for saving the file is invalid: " + fullPath + "\n");
  }
  const auto ext = boost::filesystem::path(tmp).extension();
  if ((ext != ".nxs") && (ext != ".hdf5")) {
    throw std::invalid_argument("invalid extension for file: " +
                                ext.generic_string());
  }
  if (reporter != nullptr) {
    reporter->report();
  }
  if (!compInfo.hasDetectorInfo()) {
    throw std::invalid_argument("The component has no detector info.\n");
  }
  if (!compInfo.hasSample()) {

    throw std::invalid_argument("The component has no sample.\n");
  }

  if (Mantid::Kernel::V3D{0, 0, 0} != compInfo.samplePosition()) {
    throw std::invalid_argument("The sample is not at the origin.\n");
  }

  if (!compInfo.hasSource()) {
    throw std::invalid_argument("The component has no source.");
  }

  // open file
  H5::H5File file(fullPath, H5F_ACC_TRUNC); // open file

  H5::Group rootfile, instrument;

  // create NXentry (file root)
  rootfile = file.createGroup("/raw_data_1");
  NexusGeometrySave::writeStrAttribute(rootfile, NX_CLASS, NX_ENTRY);

  // NXinstrument (component root)
  instrument = NexusGeometrySave::instrument(rootfile, compInfo);

  // NXdetectors
  NexusGeometrySave::saveDetectors(instrument, compInfo, detInfo);

  // NXsource
  NexusGeometrySave::saveSource(instrument, compInfo);

  // NXsample
  NexusGeometrySave::saveSample(rootfile, compInfo);

  file.close(); // close file

} // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
