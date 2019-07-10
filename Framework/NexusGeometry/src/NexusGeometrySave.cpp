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

inline void
writeStrAttributeToDataSetHelper(H5::DataSet &dSet, const std::string &attrName,
                                 const std::string &attrVal,
                                 H5::DataSpace dataSpace = H5SCALAR) {
  H5::StrType dataType = strTypeOfSize(attrVal);
  auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
  attribute.write(dataType, attrVal);
}

inline void writeStrValueToDataSetHelper(H5::DataSet &dSet,
                                         std::string dSetValue) {

  dSet.write(dSetValue, dSet.getDataType(), H5SCALAR);
}

inline void writeDetectorNumberHelper(H5::Group &grp,
                                      const Geometry::ComponentInfo &compInfo) {

  std::vector<int> detectorIndices;
  for (hsize_t i = compInfo.root(); i > 0; --i) {
    if (compInfo.isDetector(i - 1)) {
      detectorIndices.push_back(i - 1);
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
  H5::DataSet detectorNumber =
      grp.createDataSet(DETECTOR_NUMBER, H5::PredType::NATIVE_INT, space);
  detectorNumber.write(data, H5::PredType::NATIVE_INT, space);
  free(data);
}

inline void writeLocationHelper(H5::Group &grp,
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

  /*single dimension and rank for entry of norm value of position vector. fixed
   to 1 dimensions (rank 1)*/
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

  double *data = (double *)malloc(rank * sizeof(double));
  data[0] = norm;

  location.write(data, H5::PredType::NATIVE_DOUBLE, space);
  free(data);
}

inline void writeOrientationHelper(H5::Group &grp,
                                   const Geometry::ComponentInfo &compInfo) {

  int rank = 1;
  hsize_t dims[(hsize_t)1]; // number of dimensions
  dims[0] = 1;              // number of entries in first dimension

  double *data = (double *)malloc(rank * sizeof(double));
  H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

  double val = 14.44444; // placeholder
  data[0] = val;

  H5::DataSet dset =
      grp.createDataSet(ORIENTATION, H5::PredType::NATIVE_DOUBLE, space);
  dset.write(data, H5::PredType::NATIVE_DOUBLE, space);
  free(data);
}

// inline void writePixelOffsetsHelper() {} TODO

/*
==============================================================================================================

    classes for NGS

==============================================================================================================
*/

class NGSInstrument {

public:
  NGSInstrument(const H5::Group &parent,
                const Geometry::ComponentInfo &compInfo) {

    std::string instrumentNameStr = compInfo.name(compInfo.root());

    m_group = parent.createGroup(instrumentNameStr);
    writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_INSTRUMENT);

    H5::StrType nameStrSize = strTypeOfSize(instrumentNameStr);
    H5::DataSet name = m_group.createDataSet("name", nameStrSize, H5SCALAR);
    writeStrAttributeToDataSetHelper(name, SHORT_NAME,
                                     instrumentNameStr); // placeholder

    writeStrValueToDataSetHelper(name, instrumentNameStr);
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

    std::string sampleName = compInfo.name(compInfo.sample());

    m_group = parent.createGroup(sampleName);
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

    m_group = parent.createGroup(name);
    writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_DETECTOR);

    // depencency
    std::string dependencyStr = m_group.getObjName() + "/" + LOCATION;
    std::string localNameStr = "INST"; // placeholder

    H5::StrType dependencyStrSize = strTypeOfSize(dependencyStr);
    H5::StrType localNameStrSize = strTypeOfSize(localNameStr);

    writeDetectorNumberHelper(m_group, compInfo);
    writeLocationHelper(m_group, compInfo);
    writeOrientationHelper(m_group, compInfo);
    // writePixelOffsetsHelper();

    // string type datasets.
    H5::DataSet localName =
        m_group.createDataSet(LOCAL_NAME, localNameStrSize, H5SCALAR);
    H5::DataSet dependency =
        m_group.createDataSet(DEPENDS_ON, dependencyStrSize, H5SCALAR);

    // write string type dataset values
    writeStrValueToDataSetHelper(localName, localNameStr); // placeholder
    writeStrValueToDataSetHelper(dependency, dependencyStr);
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

class NGSPixelShape {

public:
  NGSPixelShape(const H5::Group &parent,
                const Geometry::ComponentInfo &compInfo) {

    m_group = parent.createGroup(PIXEL_SHAPE);
    writeStrAttributeToGroupHelper(m_group, NX_CLASS, NX_OFF_GEOMETRY);
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

 write component to tree structure.

==============================================================================================================
*/

  H5::H5File file(fullPath, H5F_ACC_TRUNC);

  // create root group @NXentry
  H5::Group rootGroup = file.createGroup("/raw_data_1");
  writeStrAttributeToGroupHelper(rootGroup, NX_CLASS, NX_ENTRY);

  NGSInstrument instr(rootGroup, compInfo);
  NGSDetector detector1("detector_0", instr.group(), compInfo);
  NGSSource source(instr.group(), compInfo);
  NGSSample sample(rootGroup, compInfo);
  NGSPixelShape pixelShape(detector1.group(), compInfo);

  file.close();

} // saveInstrument

} // namespace NexusGeometry
} // namespace Mantid
