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
const std::string NX_TRANSFORMATIONS = "NXtransformations";

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
const std::string DETECTOR_NUMBER = "depends_on";

// metadata
const std::string METRES = "m";
const std::string NAME = "name";
const std::string UNITS = "UNITS";
const std::string SHAPE = "shape";

// NEXUS COMPLIANT ATTRIBUTE VALUES

//
const H5::StrType H5VARIABLE(0, H5T_VARIABLE); // this may be inefficient
const H5::DataSpace H5SCALAR(H5S_SCALAR);

} // namespace

/*
==============================================================================================================

    Helper functions

==============================================================================================================
*/

inline void writeStrAttributeToGroupHelper(H5::Group &grp,
                                           const std::string &attrName,
                                           const std::string &attrVal,
                                           H5::StrType dataType = H5VARIABLE,
                                           H5::DataSpace dataSpace = H5SCALAR) {
  H5::Attribute attribute = grp.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeStrAttributeToDataSetHelper(
    H5::DataSet &dSet, const std::string &attrName, const std::string &attrVal,
    H5::StrType dataType = H5VARIABLE, H5::DataSpace dataSpace = H5SCALAR) {
  auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeStrValueToDataSetHelper(H5::DataSet &dSet,
                                         std::string dSetValue) {

  dSet.write(dSetValue, dSet.getDataType(), H5SCALAR);
}

inline void writeDetectorNumberDataSetToGroupHelper(
    H5::Group &grp, const Geometry::ComponentInfo &compInfo) {

  std::vector<int> detectorIndices;
  for (int i = compInfo.root(); i >= 0; --i) {
    if (compInfo.isDetector(i)) {
      detectorIndices.push_back(i);
    }
  }

  const hsize_t dsz = detectorIndices.size();

  int rank = 1;
  hsize_t dims[1];
  dims[0] = dsz;

  int *data = (int *)malloc(dsz * sizeof(int));
  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);
  for (int i = 0; i < dsz; i++) {
    data[i] = i + 1; // placeholder
  }
  H5::DataSet dset =
      grp.createDataSet("detector_number", H5::PredType::NATIVE_INT, space);
  dset.write(data, H5::PredType::NATIVE_INT, space);
  free(data);
}

inline void
writeLocationToDataSetHelper(H5::Group &grp,
                             const Geometry::ComponentInfo &compInfo) {
  /*
  int rank = 1;
  hsize_t dims[1];
  dims[0] = 1;

  int *data = (int *)malloc(rank * sizeof(int));
  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

  hsize_t val = 13;
  data[0] = val;

  H5::DataSet dset =
      grp.createDataSet("detector_number", H5::PredType::NATIVE_INT, space);
  dset.write(data, H5::PredType::NATIVE_INT, space);
  free(data);
  */
}

/*
==============================================================================================================

    classes for NGS

==============================================================================================================
*/

class NGSInstrument {

public:
  NGSInstrument(const H5::Group &parent,
                const Geometry::ComponentInfo &compInfo) {

    m_group = parent.createGroup(compInfo.name(compInfo.root()));
    writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_INSTRUMENT);

    H5::DataSet name = m_group.createDataSet("name", H5VARIABLE, H5SCALAR);
    writeStrAttributeToDataSetHelper(name, SHORT_NAME,
                                     compInfo.name(compInfo.root()));

    writeStrValueToDataSetHelper(name, compInfo.name(compInfo.root()).c_str());
  }

  void addChild(H5::Group &child) { m_childrenGroups.push_back(child); }
  H5::Group group() const { return m_group; }

private:
  const std::string m_name;
  const std::string m_dependency;
  H5::Group m_group;
  std::vector<H5::Group> m_childrenGroups;
};

class NGSSample {

public:
  NGSSample(const H5::Group &parent, const Geometry::ComponentInfo &compInfo) {

    m_group = parent.createGroup(compInfo.name(compInfo.sample()));
    writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_SAMPLE);
  }

  void addChild(H5::Group &child) { m_childrenGroups.push_back(child); }
  H5::Group group() const { return m_group; }

private:
  H5::Group m_group;
  std::vector<H5::Group> m_childrenGroups;
};

class NGSDetector {

public:
  NGSDetector(const std::string &name, const H5::Group &parent,
              const Geometry::ComponentInfo &compInfo) {

    m_group = parent.createGroup("detector_0");
    writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_DETECTOR);

    writeDetectorNumberDataSetToGroupHelper(m_group, compInfo);
    // writeLocationToDataSetHelper(m_group, compInfo);

    H5::DataSet localName =
        m_group.createDataSet(LOCAL_NAME, H5VARIABLE, H5SCALAR);
    H5::DataSet orientation =
        m_group.createDataSet(ORIENTATION, H5VARIABLE, H5SCALAR);
    H5::DataSet xPixelOffset =
        m_group.createDataSet(X_PIXEL_OFFSET, H5VARIABLE, H5SCALAR);
    H5::DataSet yPixelOffset =
        m_group.createDataSet(Y_PIXEL_OFFSET, H5VARIABLE, H5SCALAR);
    H5::DataSet dependency =
        m_group.createDataSet(DEPENDS_ON, H5VARIABLE, H5SCALAR);

    // dataset values
    writeStrValueToDataSetHelper(localName, "INST");
    writeStrValueToDataSetHelper(dependency,
                                 m_group.getObjName() + "/" + LOCATION);

    // dataset attributes
  }

  void addChild(H5::Group &child) { m_childrenGroups.push_back(child); }
  H5::Group group() const { return m_group; }

private:
  const std::string m_name;
  const std::string m_dependency;
  H5::Group m_group;
  std::vector<H5::Group> m_childrenGroups;
};

class NGSSource {

public:
  NGSSource(const H5::Group &parent, const Geometry::ComponentInfo &compInfo) {

    m_group = parent.createGroup(compInfo.name(compInfo.source()));
    writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_SOURCE);
  }

  void addChild(H5::Group &child) { m_childrenGroups.push_back(child); }
  H5::Group group() const { return m_group; }

private:
  H5::Group m_group;
  std::vector<H5::Group> m_childrenGroups;
};

/*
==============================================================================================================

      Beginning of saveInstrument

==============================================================================================================
*/

void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const std::string &fullPath,
                    Kernel::ProgressBase *reporter) {

  boost::filesystem::path tmp(fullPath);

  // check the directory for the file is valid.
  if (!boost::filesystem::is_directory(tmp.root_directory())) {
    throw std::invalid_argument(
        "The path provided for saving the file is invalid: " + fullPath + "\n");
  }

  // check the file itself has valid extensions.
  const auto ext = boost::filesystem::path(tmp).extension();
  if ((ext != ".nxs") && (ext != ".hdf5")) {

    throw std::invalid_argument("invalid extension for file: " +
                                ext.generic_string());
  }

  // does reporting if optional reporter exists.
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

 Parse tree structure in component

==============================================================================================================
*/

  H5::H5File file(fullPath, H5F_ACC_TRUNC);

  // create root group @NXentry
  H5::Group rootGroup = file.createGroup("/raw_data_1");
  writeStrAttributeToGroupHelper(rootGroup, NX_CLASS, NX_ENTRY);

  NGSInstrument instr(rootGroup, compInfo);
  NGSDetector detector1("detector_1", instr.group(), compInfo);
  NGSSource source(instr.group(), compInfo);
  NGSSample sample(rootGroup, compInfo);

  file.close();

} // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
