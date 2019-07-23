// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_

#include "MantidKernel/ConfigService.h" // temporary, for unit test with parser.

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <H5Cpp.h>
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <gmock/gmock.h>
#include <memory>
#include <set>

#include <Poco/Glob.h> // temporary, for unit test with parser.

using namespace Mantid::NexusGeometry;

//---------------------------------------------------------------
namespace {

// for comparision of detector banks between saved instrument and reloaded
// instrument, as required for the unit test. Delete this with along with the
// temporary unit test to reload file.
typedef std::vector<size_t> Indices;
Indices banks(const Mantid::Geometry::ComponentInfo &compInfo) {

  Indices banks;
  for (size_t i = compInfo.root(); i > 0; --i) {
    if (compInfo.isDetector(i))
      break;

    if (compInfo.hasParent(i)) {

      size_t parent = compInfo.parent(i);
      auto parentType = compInfo.componentType(parent);

      if (compInfo.detectorsInSubtree(i).size() != 0) {

        if (parentType != Mantid::Beamline::ComponentType::Rectangular &&
            parentType != Mantid::Beamline::ComponentType::Structured &&
            parentType != Mantid::Beamline::ComponentType::Grid)
          banks.push_back(i);
      }
    }
  }
  return banks;
}

// for comparision of detector banks between saved instrument and reloaded
// instrument, as required for the unit test. Delete this with along with the
// temporary unit test to reload file.
std::vector<int> getDetIDs(const std::vector<size_t> detectorsInSubtree,
                           const std::vector<int> &detectorIDs) {

  std::vector<int> bankDetIDs;
  bankDetIDs.reserve(detectorsInSubtree.size());

  for (const size_t &index : detectorsInSubtree) {
    bankDetIDs.push_back(detectorIDs[index]);
  }

  return bankDetIDs;
}

const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t>(0);
const H5G_obj_t DATASET_TYPE = static_cast<H5G_obj_t>(1);

const std::string SHORT_NAME = "short_name";
const std::string NX_CLASS = "NX_class";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_SOURCE = "NXsource";
const std::string NX_SAMPLE = "NXsample";
const std::string SHAPE = "shape";

const std::string NX_TRANSFORMATION = "NXtransformation";
const std::string NX_CHAR = "NX_CHAR";

const std::string TRANSFORMATION_TYPE = "transformation_type";
const std::string ROTATION = "rotation";
const std::string TRANSLATION = "translation";
const std::string VECTOR = "vector";
const std::string LOCATION = "location";
const std::string ORIENTATION = "orientation";
const std::string SOURCE = "source";
const std::string SAMPLE = "sample";
const std::string NAME = "name";
const std::string DETECTOR_PREFIX = "detector_";
const std::string INSTRUMENT = "instrument";
const std::string X_PIXEL_OFFSET = "x_pixel_offset";
const std::string Y_PIXEL_OFFSET = "y_pixel_offset";
const std::string Z_PIXEL_OFFSET = "z_pixel_offset";

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(doReport, void(const std::string &));
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
};

// TODO, this is duplicated from NexusGeometryParserTest, should be made the
// same.
class MockLogger : public Mantid::NexusGeometry::Logger {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(warning, void(const std::string &));
  MOCK_METHOD1(error, void(const std::string &));
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

//  local Class used for validation of the structure of a nexus file as needed
//  for the unit tests.
class HDF5FileTestUtility {

public:
  HDF5FileTestUtility(const std::string &fullPath) {
    boost::filesystem::path tmp = fullPath;

    if (!boost::filesystem::exists(tmp)) {
      throw std::invalid_argument("no such file.\n");
    } else {
      m_file.openFile(fullPath, H5F_ACC_RDONLY);
    }
  }

  // check if nx transformations writtern to group properly.
  // void canGetTransformations(H5::H5File &file, std::string &groupName) {
  //}

  // moves down the index through groups starting at root, and if
  // child has expected CLASS_TYPE, and is in parent group with expected parent

  bool parentNXgroupHasChildNXgroup(const std::string &parentNX_CLASS_TYPE,
                                    const std::string &childNX_CLASS_TYPE) {

    H5::Group rootGroup =
        m_file.openGroup("raw_data_1"); // as defined in saveInstrument.

    // if specified parent NX class type is NX entry, check the top level of
    // file structure only. (dont take extra step to look for parent group)
    if (parentNX_CLASS_TYPE == NX_ENTRY) {

      for (hsize_t i = 0; i < rootGroup.getNumObjs(); ++i) {
        if (rootGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
          std::string childPath = rootGroup.getObjnameByIdx(i);
          // Open the sub group
          H5::Group childGroup = rootGroup.openGroup(childPath);
          // Test attribute at current index for NX_class
          H5::Attribute attribute = childGroup.openAttribute(NX_CLASS);
          std::string attrVal;
          attribute.read(attribute.getDataType(), attrVal);
          if (attrVal == childNX_CLASS_TYPE) {
            return true;
          }
        }
      }
    }

    // Iterate over children of root group, and determine if a group
    for (hsize_t i = 0; i < rootGroup.getNumObjs(); ++i) {
      if (rootGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
        std::string childPath = rootGroup.getObjnameByIdx(i);
        // Open the sub group
        H5::Group childGroup = rootGroup.openGroup(childPath);
        // check current child group going down from root has the specified
        // NX_CLASS parent group
        H5::Attribute parentAttribute = childGroup.openAttribute(NX_CLASS);
        std::string parentAttrVal;
        parentAttribute.read(parentAttribute.getDataType(), parentAttrVal);
        if (parentAttrVal == parentNX_CLASS_TYPE) {
          for (hsize_t i = 0; i < childGroup.getNumObjs(); ++i) {
            if (childGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
              std::string grandchildPath = childGroup.getObjnameByIdx(i);
              // Open the sub group
              H5::Group grandchildGroup = childGroup.openGroup(grandchildPath);
              // check NX class
              H5::Attribute grandchildAttribute =
                  grandchildGroup.openAttribute(NX_CLASS);
              std::string grandchildAttrVal;
              grandchildAttribute.read(grandchildAttribute.getDataType(),
                                       grandchildAttrVal);
              if (childNX_CLASS_TYPE == grandchildAttrVal) {
                return true;
              }
            }
          }
        }
      }
    }

    return false;
  } // namespace

  double readDoubleFromDataset(const std::string &datasetName,
                               const std::string &pathToGroup) {
    double value;
    int rank = 1;
    hsize_t dims[(hsize_t)1];
    dims[0] = (hsize_t)1;

    H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

    // open dataset and read.
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(datasetName);
    dataset.read(&value, H5::PredType::NATIVE_DOUBLE, space);
    return value;
  }

  // read attribute of dataset
  std::vector<double>
  readDoubleVectorFrom_d_Attribute(const std::string &attrName,
                                   const std::string &datasetName,
                                   const std::string &pathToGroup) {

    // open dataset and read.
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(datasetName);

    H5::Attribute attribute = dataset.openAttribute(attrName);

    H5::DataType dataType = attribute.getDataType();
    H5::DataSpace dataSpace = attribute.getSpace();

    std::vector<double> value;
    value.resize(dataSpace.getSelectNpoints());

    attribute.read(dataType, value.data());

    return value;
  }

  // check dataset exists in path with attribute value of NX_class.
  bool hasNXDataset(const std::string pathToGroup,
                    const std::string nx_attributeVal) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    auto numOfChildren = parentGroup.getNumObjs();
    for (hsize_t i = 0; i < numOfChildren; i++) {
      if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
        std::string dSetName = parentGroup.getObjnameByIdx(i);
        H5::DataSet dSet = parentGroup.openDataSet(dSetName);
        if (dSet.attrExists(NX_CLASS)) {
          H5::Attribute attribute = dSet.openAttribute(NX_CLASS);
          std::string attributeValue;
          attribute.read(attribute.getDataType(), attributeValue);
          if (attributeValue == nx_attributeVal)
            return true;
        }
      }
    }
    return false;
  }

  // check dataset exists in path with attribute value of attribute name.
  bool hasDataset(const std::string pathToGroup, const std::string attributeVal,
                  const std::string attrName) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    auto numOfChildren = parentGroup.getNumObjs();
    for (hsize_t i = 0; i < numOfChildren; i++) {
      if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
        std::string dSetName = parentGroup.getObjnameByIdx(i);
        H5::DataSet dSet = parentGroup.openDataSet(dSetName);
        if (dSet.attrExists(NX_CLASS)) {
          H5::Attribute attribute = dSet.openAttribute(attrName);
          std::string attributeValue;
          attribute.read(attribute.getDataType(), attributeValue);
          if (attributeValue == attributeVal)
            return true;
        }
      }
    }
    return false;
  }

  bool groupHasNxClass(const std::string &attrVal,
                       const std::string &pathToGroup) const {

    H5::Attribute attribute;
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool groupExistsInPath(std::string fullPath) {

    try {
      H5::Group parentGroup = m_file.openGroup(fullPath);
    } catch (H5::Exception &) {

      return false;
    }

    return true;
  }

  bool dataSetHasStrValue(const std::string &dataSetName,
                          const std::string &dataSetValue,
                          const std::string &pathToGroup) const {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);

    try {
      H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
      std::string dataSetVal;
      dataSet.read(dataSetVal, dataSet.getDataType());
      return dataSetVal == dataSetValue;
    } catch (H5::DataSetIException &) {
      return false;
    }
  }

  // check if dataset or group has name-specific attribute
  bool hasAttributeInGroup(const std::string &pathToGroup,
                           const std::string &attrName,
                           const std::string &attrVal) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(attrName);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasNXAttributeInGroup(const std::string &pathToGroup,
                             const std::string &attrVal) {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasAttributeInDataSet(const std::string &pathToGroup,
                             const std::string dataSetName,
                             const std::string &attrName,
                             const std::string &attrVal) {

    H5::Attribute attribute;
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
    attribute = dataSet.openAttribute(attrName);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasNXAttributeInDataSet(const std::string &pathToGroup,
                               const std::string dataSetName,
                               const std::string &attrVal) {

    H5::Attribute attribute;
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
    attribute = dataSet.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

private:
  H5::H5File m_file;

}; // HDF5FileTestUtility

// RAII: Gives a clean file destination and removes the file when
// handle is out of scope. Must be stack allocated.
class ScopedFileHandle {

public:
  ScopedFileHandle(const std::string &fileName) {

    const auto temp_dir = boost::filesystem::temp_directory_path();
    auto temp_full_path = temp_dir;
    temp_full_path /= fileName;

    // Check proposed location and throw std::invalid argument if file does
    // not exist. otherwise set m_full_path to location.

    if (boost::filesystem::is_directory(temp_dir)) {
      m_full_path = temp_full_path;

    } else {
      throw std::invalid_argument("failed to load temp directory: " +
                                  temp_dir.generic_string());
    }
  }

  std::string fullPath() const { return m_full_path.generic_string(); }

  ~ScopedFileHandle() {

    // file is removed at end of file handle's lifetime
    if (boost::filesystem::is_regular_file(m_full_path)) {
      boost::filesystem::remove(m_full_path);
    }
  }

private:
  boost::filesystem::path m_full_path; // full path to file

  // prevent heap allocation for ScopedFileHandle
protected:
  static void *operator new(std::size_t); // prevent heap allocation of scalar.
  static void *operator new[](std::size_t); // prevent heap allocation of array.
};

} // namespace

//---------------------------------------------------------------------

class NexusGeometrySaveTest : public CxxTest::TestSuite {

private:
  std::pair<std::unique_ptr<Mantid::Geometry::ComponentInfo>,
            std::unique_ptr<Mantid::Geometry::DetectorInfo>>
      m_instrument;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometrySaveTest *createSuite() {
    return new NexusGeometrySaveTest();
  }
  static void destroySuite(NexusGeometrySaveTest *suite) { delete suite; }

  NexusGeometrySaveTest() {

    const Quat bankRotation(15, V3D(0, 1, 0));
    const Quat detRotation(15, V3D(0, 1, 0));

    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            Mantid::Kernel::V3D(0, 0, -7), Mantid::Kernel::V3D(0, 0, 0),
            Mantid::Kernel::V3D(0, 0, 10), bankRotation, detRotation);
    instrument->setName("example-detector-bank");
    m_instrument =
        Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
  }

  void test_providing_invalid_path_throws() {

    ScopedFileHandle fileResource("invalid_path_to_file_test_file.hdf5");
    std::string destinationFile = "false_directory\\" + fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    TS_ASSERT_THROWS(
        NexusGeometrySave::saveInstrument(m_instrument, destinationFile),
        std::invalid_argument &);
  }

  void test_progress_reporting() {

    MockProgressBase progressRep;
    EXPECT_CALL(progressRep, doReport(testing::_)).Times(1);

    ScopedFileHandle fileResource("progress_report_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile,
                                      &progressRep);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&progressRep));
  }

  void test_extension_validation() {

    ScopedFileHandle fileResource("invalid_extension_test_file.abc");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    TS_ASSERT_THROWS(
        NexusGeometrySave::saveInstrument(m_instrument, destinationFile),
        std::invalid_argument &);
  }

  void test_root_group_is_nxentry_class() {

    ScopedFileHandle fileResource("check_nxentry_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    NexusGeometrySave::saveInstrument(m_instrument, destinationFile);

    HDF5FileTestUtility tester(destinationFile);
    std::string dataSetName = compInfo.name(compInfo.root());

    TS_ASSERT(tester.groupHasNxClass(NX_ENTRY, "/raw_data_1"));
  }

  void test_nxinstrument_class_group_exists_in_root_group() {

    ScopedFileHandle fileResource("check_nxinstrument_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile);
    HDF5FileTestUtility tester(destinationFile);
    std::string dataSetName = compInfo.name(compInfo.root());

    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_INSTRUMENT));
  }

  void test_NXinstrument_has_expected_name() {

    ScopedFileHandle fileResource("check_instrument_name_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);

    // name of instrument
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    NexusGeometrySave::saveInstrument(m_instrument,
                                      destinationFile); // saves instrument
    HDF5FileTestUtility testUtility(destinationFile);

    std::string pathToGroup = "/raw_data_1/" + INSTRUMENT;

    TS_ASSERT(
        testUtility.groupExistsInPath(pathToGroup)); // assert group exists

    TS_ASSERT(testUtility.hasNXAttributeInGroup(
        pathToGroup, NX_INSTRUMENT)); // assert group is NXinstrument

    TS_ASSERT(testUtility.dataSetHasStrValue(
        NAME, expectedInstrumentName,
        pathToGroup)) // assert name stored in 'name'
  }

  void test_instrument_without_sample_throws() {

    auto const &instrument =
        ComponentCreationHelper::createInstrumentWithOptionalComponents(
            true, false, true);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    auto &compInfo = (*instr.first);

    ScopedFileHandle fileResource("check_no_sample_throws_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    TS_ASSERT(compInfo.hasDetectorInfo()); // rule out throw by no detector info
    TS_ASSERT(compInfo.hasSource());       // rule out throw by no source
    TS_ASSERT(!compInfo.hasSample());      // verify component has no sample

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile),
                     std::invalid_argument &);
  }

  void test_instrument_without_source_throws() {

    auto const &instrument =
        ComponentCreationHelper::createInstrumentWithOptionalComponents(
            false, true, true);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    auto &compInfo = (*instr.first);

    ScopedFileHandle fileResource("check_no_source_throws_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    TS_ASSERT(compInfo.hasDetectorInfo()); // rule out throw by no detector info
    TS_ASSERT(compInfo.hasSample());       // rule out throw by no sample
    TS_ASSERT(!compInfo.hasSource());      // verify component has no source

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile),
                     std::invalid_argument &);
  }

  // throws if NXinstrument group does not have NXsource group
  void test_nxsource_class_exists_and_is_in_instrument_group() {

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile);
    HDF5FileTestUtility tester(destinationFile);
    TS_ASSERT(compInfo.hasSource());
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_INSTRUMENT, NX_SOURCE));
  }

  // throws if NXentry group does not have NXsample group
  void test_nxsample_class_exists_and_is_in_nxentry_group() {

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile);
    HDF5FileTestUtility tester(destinationFile);

    TS_ASSERT(compInfo.hasSample());
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_SAMPLE));
  }

  void test_sample_not_at_origin_throws() {

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10), V3D(0, 0, 2), V3D(0, 0, 10));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile),
                     std::invalid_argument &);
  }

  // test that if NXtransformation exists in NXdetector, either orientation or
  // rotation also exists in NXdetector.
  void
  test_when_nx_detector_groups_have_nx_transformation_attribute_transformation_type_is_specified_for_all() {

    ScopedFileHandle fileResource(
        "check_nxdetector_groups_have_transformation_types_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // instrument with 10 banks
    auto instrument =
        ComponentCreationHelper::createTestInstrumentRectangular2(10, 10);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // saveinstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile);
    auto &compInfo = (*instr.first);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;

    HDF5FileTestUtility tester(destinationFile);

    int detCounter = 1;
    for (size_t i = compInfo.root() - 1; i > 0; --i) {

      if (Mantid::Geometry::ComponentInfoBankHelpers::isAnyBank(compInfo, i)) {

        auto pathToparent = "/raw_data_1/" + INSTRUMENT;
        auto bankGroupName = DETECTOR_PREFIX + std::to_string(detCounter);
        auto fullPath = pathToparent + "/" + bankGroupName;
        hasNXTransformation = tester.hasNXDataset(fullPath, NX_TRANSFORMATION);

        // assert all test banks have Nxtransformations
        TS_ASSERT(hasNXTransformation);

        hasTranslation =
            tester.hasDataset(fullPath, TRANSLATION, TRANSFORMATION_TYPE);

        hasRotation =
            tester.hasDataset(fullPath, ROTATION, TRANSFORMATION_TYPE);

        TS_ASSERT((hasRotation || hasTranslation));
        ++detCounter;
      }
    }
  }

  void
  test_when_nx_sample_group_has_nx_transformation_attribute_transformation_type_is_specified() {

    ScopedFileHandle fileResource(
        "check_nxsample_group_has_transformation_type_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // saveinstrument
    NexusGeometrySave::saveInstrument(m_instrument, destinationFile);
    auto &compInfo = (*m_instrument.first);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;

    HDF5FileTestUtility tester(destinationFile);

    auto pathToparent = "/raw_data_1/";
    auto fullPathToGroup = pathToparent + SAMPLE;

    hasNXTransformation =
        tester.hasNXDataset(fullPathToGroup, NX_TRANSFORMATION);

    // assert the test sample has Nxtransformation.
    TS_ASSERT(hasNXTransformation);

    hasTranslation =
        tester.hasDataset(fullPathToGroup, TRANSLATION, TRANSFORMATION_TYPE);

    hasRotation =
        tester.hasDataset(fullPathToGroup, ROTATION, TRANSFORMATION_TYPE);

    TS_ASSERT((hasRotation || hasTranslation));
  }

  void
  test_when_nx_source_group_has_nx_transformation_attribute_transformation_type_is_specified() {

    ScopedFileHandle fileResource(
        "check_nxsource_group_has_transformation_type_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // saveinstrument
    NexusGeometrySave::saveInstrument(m_instrument, destinationFile);
    auto &compInfo = (*m_instrument.first);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;

    HDF5FileTestUtility tester(destinationFile);

    auto pathToparent = "/raw_data_1/" + INSTRUMENT;
    auto sourceName = compInfo.name(compInfo.source());
    auto fullPath = pathToparent + "/" + sourceName;

    hasNXTransformation = tester.hasNXDataset(fullPath, NX_TRANSFORMATION);

    // assert the test source has Nxtransformation.
    TS_ASSERT(hasNXTransformation);

    hasTranslation =
        tester.hasDataset(fullPath, TRANSLATION, TRANSFORMATION_TYPE);

    hasRotation = tester.hasDataset(fullPath, ROTATION, TRANSFORMATION_TYPE);

    TS_ASSERT((hasRotation || hasTranslation));
  }

  void
  test_rotation_of_nx_detector_and_bank_accounted_for_in_xyz_pixel_offset_when_rotation_is_present() {

    ScopedFileHandle fileResource("check_pixel_offset_format_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    const Quat relativeBankRotation(45.0, V3D(0.0, 1.0, 0.0));
    const Quat relativeDetRotation(45.0, V3D(0.0, 1.0, 0.0));
    const V3D absBankposition(0, 0, 10);
    const V3D detOffset(2.0, -2.0, 0.0);

    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
            absBankposition,      // bank position
            relativeBankRotation, // bank rotation
            relativeDetRotation,
            detOffset); // detector rotation, detector offset
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // saveinstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile);
    auto &compInfo = (*instr.first);

    HDF5FileTestUtility tester(destinationFile);

    int detCounter = 1; // suffixes of NXdetectors start at 1
    for (size_t i = compInfo.root() - 1; i > 0; --i) {

      if (Mantid::Geometry::ComponentInfoBankHelpers::isAnyBank(compInfo, i)) {

        auto pathToparent = "/raw_data_1/" + INSTRUMENT;
        auto bankGroupName = DETECTOR_PREFIX + std::to_string(detCounter);
        auto fullPathToGroup = pathToparent + "/" + bankGroupName;

        // get the xyz offset of the pixels, and use trig to verify that its
        // position reflects det rotation relative to bank.
        double detOffsetX =
            tester.readDoubleFromDataset(X_PIXEL_OFFSET, fullPathToGroup);
        double detOffsetY =
            tester.readDoubleFromDataset(Y_PIXEL_OFFSET, fullPathToGroup);
        double detOffsetZ =
            tester.readDoubleFromDataset(Z_PIXEL_OFFSET, fullPathToGroup);

        Eigen::Vector3d offsetInFile(detOffsetX, detOffsetY, detOffsetZ);

        Eigen::Vector3d expectedOffset = Mantid::Kernel::toVector3d(detOffset);

        TS_ASSERT(offsetInFile.isApprox(expectedOffset));
        ++detCounter;
      }
    }
  }

  void
  test_rotation_of_sample_written_to_file_in_nx_format_when_rotation_is_present() {

    const Quat sampleRotation(30, V3D(1, 0, 0));
    const Quat sourceRotation(90, V3D(0, 1, 0));

    auto instrument =
        ComponentCreationHelper::createInstrumentWithSampleAndSourceRotation(
            Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
            Mantid::Kernel::V3D(0, 0, 10),
            sampleRotation,  // sample rotation
            sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    auto &compInfo = (*instr.first);

    ScopedFileHandle fileResource(
        "check_rotation_written_to_nxsample_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    NexusGeometrySave::saveInstrument(instr, destinationFile);
    HDF5FileTestUtility tester(destinationFile);

    auto pathToparent = "/raw_data_1/";
    auto fullPathToGroup = pathToparent + SAMPLE;

    std::string dataSetName = "orientation";
    std::string attributeName = "vector";

    double angleInFile =
        tester.readDoubleFromDataset(dataSetName, fullPathToGroup);
    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(
        attributeName, dataSetName, fullPathToGroup);

    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of sampleRotation
    Eigen::Quaterniond sampleRotationCopy =
        Mantid::Kernel::toQuaterniond(sampleRotation);

    // sample rotation in file as eigen Quaternion
    Eigen::Quaterniond rotationInFile =
        Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

    TS_ASSERT(rotationInFile.isApprox(sampleRotationCopy));
  }

  void
  test_rotation_of_source_written_to_file_in_nexus_format_when_rotation_is_present() {

    const Quat sampleRotation(30, V3D(1, 0, 0));
    const Quat sourceRotation(90, V3D(0, 1, 0));

    auto instrument =
        ComponentCreationHelper::createInstrumentWithSampleAndSourceRotation(
            Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
            Mantid::Kernel::V3D(0, 0, 10),
            sampleRotation,  // sample rotation
            sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    auto &compInfo = (*instr.first);

    ScopedFileHandle fileResource(
        "check_rotation_written_to_nxsource_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    NexusGeometrySave::saveInstrument(instr, destinationFile);
    HDF5FileTestUtility tester(destinationFile);

    auto pathToparent = "/raw_data_1/" + INSTRUMENT;
    auto fullPathToGroup = pathToparent + "/" + SOURCE;

    std::string dataSetName = "orientation";
    std::string attributeName = "vector";

    double angleInFile =
        tester.readDoubleFromDataset(dataSetName, fullPathToGroup);

    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(
        attributeName, dataSetName, fullPathToGroup);

    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of sampleRotation
    Eigen::Quaterniond sourceRotationCopy =
        Mantid::Kernel::toQuaterniond(sourceRotation);

    // sample rotation in file as eigen Quaternion
    Eigen::Quaterniond rotationInFile =
        Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

    TS_ASSERT(rotationInFile.isApprox(sourceRotationCopy));
  }

  void test_reload_into_parser_produces_identical_instrument_temp() {

    ScopedFileHandle inFileResource("reload_into_parser_file_test.hdf5");
    std::string inDestinationFile = inFileResource.fullPath();

    auto instrument =
        ComponentCreationHelper::createTestInstrumentRectangular2(10, 10);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    auto &compInfo = (*instr.first);
    auto &detInfo = (*instr.second);

    NexusGeometrySave::saveInstrument(instr, inDestinationFile);

    // reload to Nexus Geometry Parser
    std::unique_ptr<Logger> logger = std::make_unique<MockLogger>();
    auto reloadedInstrument = NexusGeometryParser::createInstrument(
        inDestinationFile, std::move(logger));

    auto instr2 =
        Mantid::Geometry::InstrumentVisitor::makeWrappers(*reloadedInstrument);
    auto &compInfo2 = (*instr2.first);
    auto &detInfo2 = (*instr2.second);

    // sources
    std::string inSourceName = compInfo.name(compInfo.source());
    std::string outSourceName = compInfo2.name(compInfo2.source());

    Eigen::Vector3d inSourcePos =
        Mantid::Kernel::toVector3d(compInfo.sourcePosition());
    Eigen::Vector3d outSourcePos =
        Mantid::Kernel::toVector3d(compInfo2.sourcePosition());

    // samples
    std::string inSampleName = compInfo.name(compInfo.sample());
    std::string outSampleName = compInfo2.name(compInfo2.sample());

    Eigen::Vector3d inSamplePos =
        Mantid::Kernel::toVector3d(compInfo.samplePosition());
    Eigen::Vector3d outSamplePos =
        Mantid::Kernel::toVector3d(compInfo2.samplePosition());

    // instruments
    std::string inInstrumentName = compInfo.name(compInfo.root());
    std::string outInstrumentName = compInfo2.name(compInfo2.root());

    // detector banks
    Indices inBanks = banks(compInfo);
    Indices outBanks = banks(compInfo2);

    size_t inNumOfBanks = inBanks.size();
    size_t outNumOfBanks = outBanks.size();

    std::vector<Eigen::Vector3d> inBankPositions;
    inBankPositions.reserve(inNumOfBanks);

    std::vector<Eigen::Vector3d> outBankPositions;
    inBankPositions.reserve(outNumOfBanks);

    const auto detIDs1 = detInfo.detectorIDs();
    const auto detIDs2 = detInfo2.detectorIDs();

    // assertations for banks/detectors

    for (const size_t &idx1 : inBanks) {

      std::string bankName1 = compInfo.name(idx1);

      for (const size_t &idx2 : outBanks) {
        std::string bankName2 = compInfo2.name(idx2);
        if (bankName1 == bankName2) {

          Eigen::Vector3d bankPos1 =
              Mantid::Kernel::toVector3d(compInfo.position(idx1));
          Eigen::Vector3d bankPos2 =
              Mantid::Kernel::toVector3d(compInfo2.position(idx2));
          Eigen::Quaterniond bankRot1 =
              Mantid::Kernel::toQuaterniond(compInfo.rotation(idx1));
          Eigen::Quaterniond bankRot2 =
              Mantid::Kernel::toQuaterniond(compInfo2.rotation(idx2));

          // indexes of in-file and out-file detectors
          auto inDetectors = compInfo.detectorsInSubtree(idx1);
          auto outDetectors = compInfo2.detectorsInSubtree(idx2);
          TS_ASSERT(inDetectors ==
                    outDetectors); // assert det indexes equal (and implicitly
                                   // that the number of detectors are equal)

          // IDs of in-file and out-file detectors
          auto inDetIDs = getDetIDs(inDetectors, detIDs1);
          auto outDetIDs = getDetIDs(outDetectors, detIDs2);
          // assert all detector IDs equal
          TS_ASSERT(inDetIDs == outDetIDs);

          for (const size_t &index1 : inDetectors) {

            for (const size_t &index2 : outDetectors) {

              if (index1 == index2) {

                Eigen::Vector3d detPos1 = Mantid::Kernel::toVector3d(
                    compInfo.relativePosition(index1));
                Eigen::Vector3d detPos2 = Mantid::Kernel::toVector3d(
                    compInfo2.relativePosition(index2));
                Eigen::Quaterniond detRot1 = Mantid::Kernel::toQuaterniond(
                    compInfo.relativeRotation(index1));
                Eigen::Quaterniond detRot2 = Mantid::Kernel::toQuaterniond(
                    compInfo2.relativeRotation(index2));

                TS_ASSERT(
                    detPos1.isApprox(detPos2)); // assert det positions equal
                TS_ASSERT(
                    detRot1.isApprox(detRot2)); // assert det rotations equal
              }
            }
          }

          TS_ASSERT(bankPos1.isApprox(bankPos2)); // assert bank positions equal
          TS_ASSERT(bankRot1.isApprox(bankRot2)); // assert bank rotations equal
        }
      }
    }

    // assert names equal
    TS_ASSERT(inSampleName == outSampleName);
    TS_ASSERT(inSourceName == outSourceName);
    TS_ASSERT(inInstrumentName == outInstrumentName);

    // assert equal number of detector banks
    TS_ASSERT(inNumOfBanks == outNumOfBanks);

    // assert positions equal
    TS_ASSERT(inSamplePos.isApprox(outSamplePos));
    TS_ASSERT(inSourcePos.isApprox(outSourcePos));
  }

  void test_resave_of_instrument_has_identical_files_temp() {

    ScopedFileHandle inFileResource("resave_from_parser_file_test.hdf5");
    std::string destinationFile = inFileResource.fullPath();

    std::string nexusFilename = "unit_testing/SMALLFAKE_example_geometry.hdf5";
    const auto fullpath = Mantid::Kernel::ConfigService::Instance().getFullPath(
        nexusFilename, true, Poco::Glob::GLOB_DEFAULT);

    std::unique_ptr<Logger> logger = std::make_unique<MockLogger>();
    auto reloadedInstrument =
        NexusGeometryParser::createInstrument(fullpath, std::move(logger));

    auto instr =
        Mantid::Geometry::InstrumentVisitor::makeWrappers(*reloadedInstrument);
    auto &compInfo = (*instr.first);
    auto &detInfo = (*instr.second);

    NexusGeometrySave::saveInstrument(instr, destinationFile);
    // TODO HDF5FileTestUtility assertations on both files.
  }
};

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
