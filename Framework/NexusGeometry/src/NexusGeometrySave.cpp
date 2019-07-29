// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
 * NexusGeometrySave
 *
 * Save Beamline NXInstrument from Memory to disk
 *
 *@author Takudzwa Makoni, RAL (UKRI), ISIS
 *@date 22/07/2019
 */

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include <H5Cpp.h>
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <cmath>
#include <memory> // unique_ptr
#include <string>

namespace Mantid {
namespace NexusGeometry {

namespace {

const std::string NX_CLASS = "NX_class";
const std::string NX_SAMPLE = "NXsample";
const std::string NX_DETECTOR = "NXdetector";
const std::string NX_MONITOR = "NXmonitor";
const std::string NX_OFF_GEOMETRY = "NXoff_geometry";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_CHAR = "NX_CHAR";
const std::string NX_SOURCE = "NXsource";
const std::string NX_TRANSFORMATIONS = "NXtransformations";

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

// these strings belong to DataSets which are duplicates of each other. written
// to NXmonitor group to handle the naming inconsistency. probably temporary.
const std::string DETECTOR_NUMBER = "detector_number";
const std::string DETECTOR_ID = "detector_id";

const std::string TRANSFORMATION_TYPE = "transformation_type";

const std::string METRES = "m";
const std::string DEGREES = "degrees";
const std::string NAME = "name";
const std::string UNITS = "units";
const std::string SHAPE = "shape";
const std::string VECTOR = "vector";

const double PRECISION = 1e-9;
const double PI = M_PI;

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

/*
 * Function toStdVector (Overloaded). Store data in Mantid::Kernel::V3D vector
 * into std::vector<double> vector. Used by saveInstrument to write array-type
 * datasets to file.
 *
 * @param data : Mantid::Kernel::V3D vector containing data values
 * @return std::vector<double> vector containing data values in
 * Mantid::Kernel::V3D format.
 */
std::vector<double> toStdVector(const V3D &data) {

  std::vector<double> stdVector;
  stdVector.resize(3);

  for (int i = 0; i < 3; ++i) {
    stdVector[i] = data[i];
  }
  return stdVector;
}

/*
 * Function toStdVector (Overloaded). Store data in Mantid::Kernel::Quat
 * vector into std::vector<double> vector. Used by saveInstrument to write
 * array-type datasets to file.
 *
 * @param data :  Mantid::Kernel::Quat quaternion containing data values
 * @return std::vector<double> vector containing data values in Quat format
 */
std::vector<double> toStdVector(const Quat &data) {

  std::vector<double> stdVector;
  stdVector.resize(4);

  for (int i = 0; i < 4; ++i) {
    stdVector[i] = data[i];
  }
  return stdVector;
}

/*
 * Function toStdVector (Overloaded). Store data in Eigen::Vector3d vector into
 * std::vector<double> vector. Used by saveInstrument to write array-type
 * datasets to file.
 *
 * @param data : Eigen::Vector3d vector containing data values
 * @return std::vector<double> vector containing data values in Eigen::Vector3d
 * format
 */
std::vector<double> toStdVector(const Eigen::Vector3d &data) {

  std::vector<double> stdVector;
  stdVector.resize(3);

  for (int i = 0; i < 3; ++i) {
    stdVector[i] = data[i];
  }
  return stdVector;
}

/*
 * Function toStdVector (Overloaded). Store data in Eigen::Quaterniond vector
 * into std::vector<double> vector. Used by saveInstrument to write array-type
 * datasets to file.
 *
 * @param data : Eigen::Quaterniond quaternion containing data values
 * @return std::vector<double> vector containing data values in
 * Eigen::Quaterniond format
 */
std::vector<double> toStdVector(const Eigen::Quaterniond &data) {

  std::vector<double> stdVector;
  stdVector.resize(4);

  // using Eigen Quaterniond storage format (w is stored as last element)
  stdVector[3] = data.w();
  for (int i = 0; i < 3; ++i) {
    stdVector[i] = data.vec()[i];
  }
  return stdVector;
}

/*
 * Function: isApproxZero. returns true if all values in a std vector container
 * evaluate to zero with a given level of precision. Used by SaveInstrument
 * methods to determine whether or not to write a dataset to file. If data is
 * Quaternion, returns true if quaternion is equivalent to zero rotation
 *
 * @param data : std::vector<T> data
 * @param precision : double precision specifier
 * @param isQuaternion : bool quaternion flag. if true, evaluates is rotation is
 * zero
 *
 * @return true if all elements are approx zero, else false.
 */
template <typename T>
bool isApproxZero(const std::vector<T> &data, const double &precision,
                  const bool isQuaternion = false) {

  // if data is a quaternion return true if the associated rotation about an
  // axis is approximately zero
  if (isQuaternion) {
    // Kernel Quat case
    if (abs(data[0] - 1.0) < precision && data[1] < precision &&
        data[2] < precision && data[3] < precision)
      return true;
    // Eigen Quaterniond case
    if (data[0] < precision && data[1] < precision && data[2] < precision &&
        abs(data[3] - 1.0) < precision)
      return true;
  }
  return std::all_of(data.begin(), data.end(), [&precision](const T &element) {
    return abs(element) < precision;
  });
}

/*
 * Function: nxDetectorIndices. finds any banks in component info and returns
 * all indexes found.
 *
 * @param compInfo : Mantid::Geometry::ComponentInfo object.
 * @return std::vector<size_t> container with all indices in compInfo found to
 * be a detector bank
 */
std::vector<size_t> nxDetectorIndices(const Geometry::ComponentInfo &compInfo) {

  std::vector<size_t> banksInComponent;
  for (size_t index = compInfo.root() - 1; index > 0; --index) {
    if (Geometry::ComponentInfoBankHelpers::isAnyBank(compInfo, index)) {
      banksInComponent.push_back(index);
    }
  }
  return banksInComponent;
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
 * Function: writeStrDataset
 * writes a StrType HDF dataset and dataset value to a HDF group.
 *
 * @param grp : HDF group object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
inline void writeStrDataset(H5::Group &grp, const std::string &dSetName,
                            const std::string &dSetVal,
                            H5::DataSpace dataSpace = H5SCALAR) {
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

// function to create a simple sub-group that has a nexus class attribute,
// inside a parent group.
inline H5::Group simpleNXSubGroup(H5::Group &parent, const std::string &name,
                                  const std::string &nexusAttribute) {
  H5::Group subGroup = parent.createGroup(name);
  writeStrAttribute(subGroup, NX_CLASS, nexusAttribute);
  return subGroup;
}

/*
 * Function: writeXYZPixeloffset. Writes the x X
 */
inline void writeXYZPixeloffset(H5::Group &grp,
                                const Geometry::ComponentInfo &compInfo,
                                size_t idx) {

  H5::DataSet xPixelOffset, yPixelOffset, zPixelOffset;
  auto childrenDetectors = compInfo.detectorsInSubtree(idx);

  std::vector<double> posx;
  std::vector<double> posy;
  std::vector<double> posz;

  posx.reserve(childrenDetectors.size());
  posy.reserve(childrenDetectors.size());
  posz.reserve(childrenDetectors.size());

  for (const size_t &i : childrenDetectors) {

    auto offset = Geometry::ComponentInfoBankHelpers::offsetFromAncestor(
        compInfo, idx, i);

    posx.push_back(offset[0]);
    posy.push_back(offset[1]);
    posz.push_back(offset[2]);
  }

  bool xIsZero = isApproxZero(posx, PRECISION);
  bool yIsZero = isApproxZero(posy, PRECISION);
  bool zIsZero = isApproxZero(posz, PRECISION);

  auto bankName = compInfo.name(idx);
  const auto nDetectorsInBank = static_cast<hsize_t>(posx.size());

  int rank = 1;
  hsize_t dims[static_cast<hsize_t>(1)];
  dims[0] = nDetectorsInBank;

  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

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
 * @idx : size_t index of bank in compInfo.
 */
void writeNXDetectorNumber(H5::Group &grp,
                           const Geometry::ComponentInfo &compInfo,
                           const std::vector<int> &detectorIDs,
                           const size_t &idx) {

  H5::DataSet detectorNumber;

  std::vector<int> bankDetIDs; // IDs of detectors beloning to bank
  std::vector<size_t> bankDetectors = compInfo.detectorsInSubtree(idx);
  bankDetIDs.reserve(bankDetectors.size());

  for (size_t &index : bankDetectors) {
    bankDetIDs.push_back(detectorIDs[index]);
  }

  const auto nDetectorsInBank = static_cast<hsize_t>(bankDetIDs.size());

  int rank = 1;
  hsize_t dims[static_cast<hsize_t>(1)];
  dims[0] = nDetectorsInBank;

  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

  detectorNumber =
      grp.createDataSet(DETECTOR_NUMBER, H5::PredType::NATIVE_INT, space);
  detectorNumber.write(bankDetIDs.data(), H5::PredType::NATIVE_INT, space);
}

/*
 * Function: writeNXDetectorNumber
 * For use with NXmonitor group. write 'detector_id's of an NXmonitor, which is
 * a specific type of NXdetector, to its group.
 *
 * @param grp : NXmonitor group (HDF group)
 * @param compInfo : componentInfo object.
 * @param monitorIDs : std::vector<int> container of all monitorIDs to be stored
 * into dataset 'detector_id' (or 'detector_number'. naming convention
 * inconsistency?).
 * @idx : size_t index of monitor in compInfo.
 */
void writeNXMonitorNumber(H5::Group &grp,
                          const Geometry::ComponentInfo &compInfo,
                          const std::vector<int> &monitorIDs,
                          const size_t &idx) {

  // these DataSets are duplicates of each other. written to the NXmonitor group
  // to handle the naming inconsistency. probably temporary.
  H5::DataSet detectorNumber, detector_id;

  std::vector<int> bankDetIDs;
  std::vector<size_t> bankDetectors = compInfo.detectorsInSubtree(idx);
  bankDetIDs.reserve(bankDetectors.size());

  for (size_t &index : bankDetectors) {
    bankDetIDs.push_back(monitorIDs[index]);
  }

  const auto nDetectorsInBank = static_cast<hsize_t>(bankDetIDs.size());

  int rank = 1;
  hsize_t dims[static_cast<hsize_t>(1)];
  dims[0] = nDetectorsInBank;

  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

  // these DataSets are duplicates of each other. written to the group to
  // handle the naming inconsistency. probably temporary.
  detectorNumber =
      grp.createDataSet(DETECTOR_NUMBER, H5::PredType::NATIVE_INT, space);
  detectorNumber.write(bankDetIDs.data(), H5::PredType::NATIVE_INT, space);

  detector_id = grp.createDataSet(DETECTOR_ID, H5::PredType::NATIVE_INT, space);
  detector_id.write(bankDetIDs.data(), H5::PredType::NATIVE_INT, space);
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

  H5::StrType strSize;

  int drank = 1;
  hsize_t ddims[static_cast<hsize_t>(1)];
  ddims[0] = static_cast<hsize_t>(1);

  // dependency for location
  std::string dependency = "."; // self dependent

  dspace = H5Screate_simple(drank, ddims, NULL);
  location = grp.createDataSet(LOCATION, H5::PredType::NATIVE_DOUBLE, dspace);
  position = Kernel::toVector3d(compInfo.position(idx)); // of bank

  // get translation from position by inverting rotation.

  // capture the norm before inline normalisation.
  norm = position.norm();
  position.normalize();

  location.write(&norm, H5::PredType::NATIVE_DOUBLE, dspace);

  // normalised if norm is not approximately zero.

  std::vector<double> stdNormPos = toStdVector(position);

  int arank = 1;
  hsize_t adims[static_cast<hsize_t>(3)];
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
 * @param idx : size_t index of bank in component Info.
 * @param noTranslation : bool flag specifying if the save method calling
 * writeOrientation also wrote a translation. If true, then no translation was
 * written to file, and the dependency for the orientation dataset is itself.
 *
 * Compliant to the Mantid Instrument Definition file, if a translation exists,
 * it precedes a rotation.
 * https://docs.mantidproject.org/nightly/concepts/InstrumentDefinitionFile.html
 */
inline void writeOrientation(H5::Group &grp,
                             const Geometry::ComponentInfo &compInfo,
                             size_t idx, bool noTranslation) {

  Eigen::Quaterniond rotation;
  double angle;

  H5::DataSet orientation;
  H5::DataSpace dspace;
  H5::DataSpace aspace;

  H5::Attribute vector;
  H5::Attribute units;
  H5::Attribute transformationType;
  H5::Attribute dependsOn;

  H5::StrType strSize;

  int rank = 1;
  hsize_t ddims[static_cast<hsize_t>(1)];
  ddims[0] = static_cast<hsize_t>(1);

  // dependency for orientation defaults to self-dependent. If Location dataset
  // exists, the orientation will depend on it instead.
  std::string dependency = ".";
  if (!noTranslation)
    dependency = forwardCompatibility::getObjName(grp) + "/" + LOCATION;
  dspace = H5Screate_simple(rank, ddims, NULL);
  orientation =
      grp.createDataSet(ORIENTATION, H5::PredType::NATIVE_DOUBLE, dspace);

  // write absolute rotation in degrees of detector bank to dataset
  rotation = Kernel::toQuaterniond(compInfo.rotation(idx));
  angle = std::acos(rotation.w()) * (360.0 / PI);
  orientation.write(&angle, H5::PredType::NATIVE_DOUBLE, dspace);

  // normalised if norm is not approximately zero.
  Eigen::Vector3d axisOfRotation = rotation.vec().normalized();

  std::vector<double> stdNormAxis = toStdVector(axisOfRotation);

  int arank = 1;
  hsize_t adims[static_cast<hsize_t>(3)];
  adims[0] = static_cast<hsize_t>(3);

  aspace = H5Screate_simple(arank, adims, NULL);
  vector =
      orientation.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE, aspace);
  vector.write(H5::PredType::NATIVE_DOUBLE, stdNormAxis.data());

  // units attribute
  strSize = strTypeOfSize(DEGREES);
  units = orientation.createAttribute(UNITS, strSize, H5SCALAR);
  units.write(strSize, DEGREES);

  // transformation-type attribute
  strSize = strTypeOfSize(ROTATION);
  transformationType =
      orientation.createAttribute(TRANSFORMATION_TYPE, strSize, H5SCALAR);
  transformationType.write(strSize, ROTATION);

  // dependency attribute
  strSize = strTypeOfSize(dependency);
  dependsOn = orientation.createAttribute(DEPENDS_ON, strSize, H5SCALAR);
  dependsOn.write(strSize, dependency);
}

/*
 * Function: NXInstrument
 * for NXentry parent (root group). Produces an NXinstrument group in the parent
 * group, and writes Nexus compliant datasets and metadata stored in attributes
 * to the new group.
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
H5::Group NXInstrument(const H5::Group &parent,
                       const Geometry::ComponentInfo &compInfo) {

  std::string instrumentName = compInfo.name(compInfo.root());
  H5::Group childGroup = parent.createGroup(instrumentName);

  writeStrDataset(childGroup, NAME, instrumentName);
  writeStrAttribute(childGroup, NX_CLASS, NX_INSTRUMENT);

  std::string defaultShortName = instrumentName.substr(0, 3);
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
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
void saveNXSample(const H5::Group &parentGroup,
                  const Geometry::ComponentInfo &compInfo) {

  H5::Group childGroup;

  std::string sampleName = compInfo.name(compInfo.sample());
  childGroup = parentGroup.createGroup(sampleName);

  writeStrAttribute(childGroup, NX_CLASS, NX_SAMPLE);
  writeStrDataset(childGroup, NAME, sampleName);
}

/*
 * Function: saveNXSource
 * For NXentry (root group). Produces an NXsource group in the parent group, and
 * writes the Nexus compliant datasets and metadata stored in attributes to the
 * new group.
 *
 *
 * @param parent : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 */
void saveNXSource(const H5::Group &parentGroup,
                  const Geometry::ComponentInfo &compInfo) {

  size_t index = compInfo.source();

  Eigen::Vector3d position =
      Mantid::Kernel::toVector3d(compInfo.position(index));
  Eigen::Quaterniond rotation =
      Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

  std::string dependency = ".";
  std::string sourceName = compInfo.name(compInfo.source());

  bool locationIsOrigin = isApproxZero(toStdVector(position), PRECISION);
  bool orientationIsZero = isApproxZero(toStdVector(rotation), PRECISION, true);

  H5::Group childGroup = parentGroup.createGroup(sourceName);
  writeStrAttribute(childGroup, NX_CLASS, NX_SOURCE);

  H5::Group transformations =
      simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

  // Orientation is the default first dependency in the chain. first check
  // translation in component is non-zero, and set dependency to location
  // if true and write location. Then check if orientation in component is
  // non-zero, replace dependency with orientation if true. If neither
  // orientation nor location are non-zero, component is self dependent.
  if (!locationIsOrigin) {
    dependency =
        forwardCompatibility::getObjName(transformations) + "/" + LOCATION;
    writeLocation(transformations, compInfo, index);
  }
  if (!orientationIsZero) {
    dependency =
        forwardCompatibility::getObjName(transformations) + "/" + ORIENTATION;
    writeOrientation(transformations, compInfo, index, locationIsOrigin);
  }

  writeStrDataset(childGroup, NAME, sourceName);
  writeStrDataset(childGroup, DEPENDS_ON, dependency);
}

/*
 * Function: saveNXMonitors
 * For NXinstrument parent (component info root). Produces a set of NXmonitor
 * groups from Component info, and saves it in the parent
 * group, along with the Nexus compliant datasets, and metadata stored in
 * attributes to the new group.
 *
 * @param parentGroup : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 * @param detInfo : DetectorInfo object.
 */
void saveNXMonitors(const H5::Group &parentGroup,
                    const Geometry::ComponentInfo &compInfo,
                    const Geometry::DetectorInfo &detInfo) {

  auto detIds = detInfo.detectorIDs();

  for (const int &ID : detIds) {
    auto index = detInfo.indexOf(ID);
    if (detInfo.isMonitor(index)) {

      Eigen::Vector3d position =
          Mantid::Kernel::toVector3d(compInfo.position(index));
      Eigen::Quaterniond rotation =
          Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

      std::string dependency = ".";            // dependency initialiser
      std::string name = compInfo.name(index); // group name

      bool locationIsOrigin = isApproxZero(toStdVector(position), PRECISION);
      bool orientationIsZero =
          isApproxZero(toStdVector(rotation), PRECISION, true);

      H5::Group childGroup = parentGroup.createGroup(name);
      writeStrAttribute(childGroup, NX_CLASS, NX_MONITOR);

      H5::Group transformations =
          simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

      // Orientation is the default first dependency in the chain. first check
      // translation in component is non-zero, and set dependency to location
      // if true and write location. Then check if orientation in component is
      // non-zero, replace dependency with orientation if true. If neither
      // orientation nor location are non-zero, component is self dependent.
      if (!locationIsOrigin) {
        dependency =
            forwardCompatibility::getObjName(transformations) + "/" + LOCATION;
        writeLocation(transformations, compInfo, index);
      }
      if (!orientationIsZero) {
        dependency = forwardCompatibility::getObjName(transformations) + "/" +
                     ORIENTATION;
        writeOrientation(transformations, compInfo, index, locationIsOrigin);
      }

      H5::StrType dependencyStrType = strTypeOfSize(dependency);
      writeNXMonitorNumber(childGroup, compInfo, detIds, index);

      writeStrDataset(childGroup, LOCAL_NAME, name);
      writeStrDataset(childGroup, DEPENDS_ON, dependency);
    }
  }
}

/*
 * Function: saveNXDetectors
 * For NXinstrument parent (component info root). Produces a set of NXdetctor
 * groups from Component info detector banks, and saves it in the parent
 * group, along with the Nexus compliant datasets, and metadata stored in
 * attributes to the new group.
 *
 * @param parentGroup : parent group in which to write the NXinstrument group.
 * @param compInfo : componentInfo object.
 * @param detInfo : DetectorInfo object.
 */
void saveNXDetector(const H5::Group &parentGroup,
                    const Geometry::ComponentInfo &compInfo,
                    const size_t &bankIdx,
                    const Geometry::DetectorInfo &detInfo) {

  const auto detIds = detInfo.detectorIDs();

  Eigen::Vector3d position =
      Mantid::Kernel::toVector3d(compInfo.position(bankIdx));
  Eigen::Quaterniond rotation =
      Mantid::Kernel::toQuaterniond(compInfo.rotation(bankIdx));

  std::string dependency = ".";              // dependency initialiser
  std::string name = compInfo.name(bankIdx); // group name

  bool locationIsOrigin = isApproxZero(toStdVector(position), PRECISION);
  bool orientationIsZero = isApproxZero(toStdVector(rotation), PRECISION, true);

  H5::Group childGroup = parentGroup.createGroup(name);
  writeStrAttribute(childGroup, NX_CLASS, NX_DETECTOR);

  H5::Group transformations =
      simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

  // Orientation is the default first dependency in the chain. first check
  // translation in component is non-zero, and set dependency to location
  // if true and write location. Then check if orientation in component is
  // non-zero, replace dependency with orientation if true. If neither
  // orientation nor location are non-zero, component is self dependent.
  if (!locationIsOrigin) {
    dependency =
        forwardCompatibility::getObjName(transformations) + "/" + LOCATION;
    writeLocation(transformations, compInfo, bankIdx);
  }
  if (!orientationIsZero) {
    dependency =
        forwardCompatibility::getObjName(transformations) + "/" + ORIENTATION;
    writeOrientation(transformations, compInfo, bankIdx, locationIsOrigin);
  }

  H5::StrType dependencyStrType = strTypeOfSize(dependency);
  writeXYZPixeloffset(childGroup, compInfo, bankIdx);
  writeNXDetectorNumber(childGroup, compInfo, detIds, bankIdx);

  writeStrDataset(childGroup, LOCAL_NAME, name);
  writeStrDataset(childGroup, DEPENDS_ON, dependency);
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

  std::vector<size_t> nxDetectors = nxDetectorIndices(compInfo);

  H5::Group rootGroup, instrument;

  // create NXentry (file root)
  rootGroup = file.createGroup(rootPath);
  NexusGeometrySave::writeStrAttribute(rootGroup, NX_CLASS, NX_ENTRY);

  // save and capture NXinstrument (component root)
  instrument = NexusGeometrySave::NXInstrument(rootGroup, compInfo);

  // save NXdetectors
  for (const size_t &index : nxDetectors) {
    NexusGeometrySave::saveNXDetector(instrument, compInfo, index, detInfo);
  }

  // save NXmonitors
  NexusGeometrySave::saveNXMonitors(instrument, compInfo, detInfo);

  // save NXsource
  NexusGeometrySave::saveNXSource(instrument, compInfo);

  // save NXsample
  NexusGeometrySave::saveNXSample(rootGroup, compInfo);

  file.close(); // close file

} // saveInstrument
} // namespace NexusGeometrySave
} // namespace NexusGeometry
} // namespace Mantid
