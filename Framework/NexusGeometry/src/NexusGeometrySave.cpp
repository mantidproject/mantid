// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
==============================================================================================================
    saves the instrument to file.
==============================================================================================================
*/

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include <H5Cpp.h>
#include <boost/filesystem/operations.hpp>
#include <stdlib.h>
#include <string>
#include <vector>

namespace Mantid {
namespace NexusGeometry {

namespace {

// NEXUS NAMES
const std::string NX_CLASS = "NX_class";
const std::string NX_SAMPLE = "NXsample";
const std::string NX_DETECTOR = "NXdetector";
const std::string NX_OFF_GEOMETRY = "NXoff_geometry";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_CHAR = "NX_CHAR";
const std::string NX_SOURCE = "NXsource";
const std::string NX_TRANSFORMATION = "NXtransformation";

// group/attribute/dataset names
const std::string SHORT_NAME = "short_name";
const std::string TRANSFORMATIONS = "transformations";
const std::string LOCAL_NAME = "local_name";
const std::string LOCATION = "location";
const std::string ORIENTATION = "orientation";
const std::string DEPENDS_ON = "depends_on";
const std::string X_PIXEL_OFFSET = "x_pixel_offset";
const std::string Y_PIXEL_OFFSET = "y_pixel_offset";
const std::string PIXEL_SHAPE = "pixel_shape";
const std::string SOURCE = "source";
const std::string SAMPLE = "sample";
const std::string DETECTOR_NUMBER = "detector_number";

// metadata
const std::string METRES = "m";
const std::string NAME = "name";
const std::string UNITS = "units";
const std::string SHAPE = "shape";

// NEXUS COMPLIANT ATTRIBUTE VALUES

//

const H5::DataSpace H5SCALAR(H5S_SCALAR);

} // namespace

namespace NexusGeometrySave {

/*
==============================================================================================================
    Helper functions
==============================================================================================================
*/

inline H5::StrType strTypeOfSize(const std::string &str) {
  H5::StrType stringType(1, (size_t)str.length());
  return stringType;
}

inline void writeStrAttributeToGroupHelper(H5::Group &grp,
                                           const std::string &attrName,
                                           const std::string &attrVal,
                                           H5::DataSpace dataSpace = H5SCALAR) {
  H5::StrType dataType = strTypeOfSize(attrVal);
  H5::Attribute attribute = grp.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeStrAttribute(H5::DataSet &dSet, const std::string &attrName,
                              const std::string &attrVal,
                              H5::DataSpace dataSpace = H5SCALAR) {
  H5::StrType dataType = strTypeOfSize(attrVal);
  auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeStrValue(H5::DataSet &dSet, std::string dSetValue) {

  dSet.write(dSetValue, dSet.getDataType(), H5SCALAR);
}

inline void writeDetectorNumber(H5::Group &grp,
                                const Geometry::ComponentInfo &compInfo) {

  std::vector<int> detectorIndices;
  hsize_t ullOne = (hsize_t)1;
  hsize_t ullZero = (hsize_t)0;
  for (hsize_t i = compInfo.root(); i > ullZero; --i) {
    if (compInfo.isDetector(i - ullOne)) {
      detectorIndices.push_back(i - ullOne);
    }
  }

  const double dsz = (double)detectorIndices.size();

  int rank = 1;
  hsize_t dims[1];
  dims[0] = dsz;

  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

  std::vector<double> data;
  for (double i = 0.0; i < dsz; i++) {
    data.push_back(i + 1.0); // placeholder
  }
  H5::DataSet detectorNumber =
      grp.createDataSet(DETECTOR_NUMBER, H5::PredType::NATIVE_INT, space);
  detectorNumber.write(&data, H5::PredType::NATIVE_INT, space);
}

inline void writeLocation(H5::Group &grp,
                          const Geometry::ComponentInfo &compInfo) {

  Eigen::Vector3d position, normalisedPosition;
  double norm;

  H5::DataSet location;
  H5::DataSpace space;

  H5::Attribute vector;
  H5::Attribute units;
  H5::Attribute transformatioType;
  H5::Attribute dependsOn;
  H5::Attribute nxClass;

  int rank = 1;
  hsize_t dims[(hsize_t)1];
  dims[0] = 1;

  space = H5Screate_simple(rank, dims, NULL);
  location = grp.createDataSet(LOCATION, H5::PredType::NATIVE_DOUBLE, space);

  // write attributes
  H5::StrType strSize = strTypeOfSize(NX_TRANSFORMATION);
  nxClass = location.createAttribute(NX_CLASS, strSize, H5SCALAR);
  nxClass.write(strSize, NX_TRANSFORMATION);

  // write norm value to dataset
  position = Kernel::toVector3d(compInfo.position(compInfo.root()));
  normalisedPosition = position.normalized();
  norm = position.norm();

  std::vector<double> data = {norm};
  location.write(&data, H5::PredType::NATIVE_DOUBLE, space);
}

inline void writeOrientation(H5::Group &grp,
                             const Geometry::ComponentInfo &compInfo) {

  Eigen::Quaterniond rotation;
  double norm;

  H5::DataSet orientation;
  H5::DataSpace space;

  H5::Attribute vector;
  H5::Attribute units;
  H5::Attribute transformatioType;
  H5::Attribute dependsOn;
  H5::Attribute nxClass;

  int rank = 1;
  hsize_t dims[(hsize_t)1];
  dims[0] = 1;

  space = H5Screate_simple(rank, dims, NULL);
  orientation =
      grp.createDataSet(ORIENTATION, H5::PredType::NATIVE_DOUBLE, space);

  // write attributes
  H5::StrType strSize = strTypeOfSize(NX_TRANSFORMATION);
  nxClass = orientation.createAttribute(NX_CLASS, strSize, H5SCALAR);
  nxClass.write(strSize, NX_TRANSFORMATION);

  // write norm value to dataset
  rotation = Kernel::toQuaterniond(compInfo.rotation(compInfo.root()));
  norm = rotation.norm();

  std::vector<double> data = {norm};
  orientation.write(&data, H5::PredType::NATIVE_DOUBLE, space);
}

/*
==============================================================================================================
    Functions for NexusGeometrySave
==============================================================================================================
*/

H5::Group instrument(const H5::Group &parent,
                     const Geometry::ComponentInfo &compInfo) {

  std::string instrumentNameStr = compInfo.name(compInfo.root());

  H5::Group group = parent.createGroup(instrumentNameStr);
  writeStrAttributeToGroupHelper(group, NX_CLASS, NX_INSTRUMENT);

  H5::StrType nameStrSize = strTypeOfSize(instrumentNameStr);
  H5::DataSet name = group.createDataSet("name", nameStrSize, H5SCALAR);
  writeStrAttribute(name, SHORT_NAME,
                    instrumentNameStr); // placeholder
  writeStrValue(name, instrumentNameStr);

  return group;
}

H5::Group sample(const H5::Group &parent,
                 const Geometry::ComponentInfo &compInfo) {

  H5::Group m_group;

  std::string sampleName = compInfo.name(compInfo.sample());

  m_group = parent.createGroup(sampleName);
  writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_SAMPLE);

  return m_group;
}

H5::Group detector(const std::string &name, const H5::Group &parent,
                   const Geometry::ComponentInfo &compInfo) {

  H5::Group m_group;

  m_group = parent.createGroup(name);
  writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_DETECTOR);

  // depencency
  std::string dependencyStr = m_group.getObjName() + "/" + LOCATION;
  std::string localNameStr = "INST"; // placeholder

  H5::StrType dependencyStrSize = strTypeOfSize(dependencyStr);
  H5::StrType localNameStrSize = strTypeOfSize(localNameStr);

  writeDetectorNumber(m_group, compInfo);
  writeLocation(m_group, compInfo);
  writeOrientation(m_group, compInfo);
  // writePixelOffsetsHelper();

  // string type datasets.
  H5::DataSet localName =
      m_group.createDataSet(LOCAL_NAME, localNameStrSize, H5SCALAR);
  H5::DataSet dependency =
      m_group.createDataSet(DEPENDS_ON, dependencyStrSize, H5SCALAR);

  // write string type dataset values
  writeStrValue(localName, localNameStr); // placeholder
  writeStrValue(dependency, dependencyStr);

  return m_group;
}

/*
 * create source group in parent, write Nexus class datasets to the group, and
 * return the source group. Nexus Class Type : NXsource
 */
H5::Group source(const H5::Group &parent,
                 const Geometry::ComponentInfo &compInfo) {

  H5::Group m_group;
  m_group = parent.createGroup(compInfo.name(compInfo.source()));
  writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_SOURCE);
  return m_group;
}

} // namespace NexusGeometrySave

/*
==============================================================================================================
      Beginning of saveInstrument
==============================================================================================================
*/

void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const std::string &fullPath,
                    Kernel::ProgressBase *reporter) {

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

  /*
 ==============================================================================================================
  write component to tree structure.
 ==============================================================================================================
 */

  H5::H5File file(fullPath, H5F_ACC_TRUNC);
  H5::Group root = file.createGroup("/raw_data_1");
  NexusGeometrySave::writeStrAttributeToGroupHelper(root, NX_CLASS, NX_ENTRY);
  H5::Group instr = NexusGeometrySave::instrument(root, compInfo);
  H5::Group detector =
      NexusGeometrySave::detector("detector_0", instr, compInfo);
  H5::Group source = NexusGeometrySave::source(instr, compInfo);
  H5::Group sample = NexusGeometrySave::sample(root, compInfo);

} // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
