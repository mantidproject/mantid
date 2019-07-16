// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <gmock/gmock.h>
#include <memory>
#include <set>

#include <H5Cpp.h>

using namespace Mantid::NexusGeometry;

//---------------------------------------------------------------
namespace {

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

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(doReport, void(const std::string &));
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
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

  bool expectedValue(const std::string &dataSetValue,
                     const std::string &pathToGroup) const {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);

    try {
      H5::DataSet dataSet = parentGroup.openDataSet(m_dataSetName);
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

private:
  H5::H5File m_file;
  const std::string m_dataSetName =
      "name"; // the title for the dataset containing the instrument name
              // is 'local_name' in file.
};            // namespace

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

    const Quat bankRotation(45, V3D(0, 0, 1));
    const Quat detRotation(45, V3D(0, 0, 1));

    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
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

    TS_ASSERT_THROWS(saveInstrument(m_instrument, destinationFile),
                     std::invalid_argument &);
  }

  void test_progress_reporting() {

    MockProgressBase progressRep;
    EXPECT_CALL(progressRep, doReport(testing::_)).Times(1);

    ScopedFileHandle fileResource("progress_report_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    saveInstrument(m_instrument, destinationFile, &progressRep);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&progressRep));
  }

  void test_extension_validation() {

    ScopedFileHandle fileResource("invalid_extension_test_file.abc");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    TS_ASSERT_THROWS(saveInstrument(m_instrument, destinationFile),
                     std::invalid_argument &);
  }

  void test_root_group_is_nxentry_class() {

    ScopedFileHandle fileResource("check_nxentry_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    saveInstrument(m_instrument, destinationFile);

    HDF5FileTestUtility tester(destinationFile);
    std::string dataSetName = compInfo.name(compInfo.root());

    TS_ASSERT(tester.groupHasNxClass(NX_ENTRY, "/raw_data_1"));
  }

  void test_nxinstrument_class_group_exists_in_root_group() {

    ScopedFileHandle fileResource("check_nxinstrument_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    saveInstrument(m_instrument, destinationFile);
    HDF5FileTestUtility tester(destinationFile);
    std::string dataSetName = compInfo.name(compInfo.root());

    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_INSTRUMENT));
  }

  void test_NXinstrument_has_expected_name() {

    ScopedFileHandle fileResource("check_instrument_name_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    saveInstrument(m_instrument, destinationFile); // saves instrument
    HDF5FileTestUtility testUtility(destinationFile);

    TS_ASSERT(
        testUtility.groupExistsInPath("/raw_data_1/" + expectedInstrumentName));
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

    TS_ASSERT_THROWS(saveInstrument(instr, destinationFile),
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

    TS_ASSERT_THROWS(saveInstrument(instr, destinationFile),
                     std::invalid_argument &);
  }

  // throws if NXinstrument group does not have NXsource group
  void test_nxsource_class_exists_and_is_in_instrument_group() {

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    saveInstrument(m_instrument, destinationFile);
    HDF5FileTestUtility tester(destinationFile);
    TS_ASSERT(compInfo.hasSource());
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_INSTRUMENT, NX_SOURCE));
  }

  // throws if NXentry group does not have NXsample group
  void test_nxsample_class_exists_and_is_in_root_group() {

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    saveInstrument(m_instrument, destinationFile);
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

    TS_ASSERT_THROWS(saveInstrument(instr, destinationFile),
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
        ComponentCreationHelper::createTestInstrumentRectangular2(10, 50);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // saveinstrument
    saveInstrument(instr, destinationFile);
    auto &compInfo = (*instr.first);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;
    bool hasEither(true); // initialise with true

    HDF5FileTestUtility tester(destinationFile);

    for (size_t i = compInfo.root() - 1; i > 0; --i) {
      if (compInfo.isDetector(i))
        break;

      if (compInfo.hasParent(i)) {

        size_t parent = compInfo.parent(i);
        auto parentType = compInfo.componentType(parent);

        if (compInfo.detectorsInSubtree(i).size() != 0) {

          if (parentType != Mantid::Beamline::ComponentType::Rectangular &&
              parentType != Mantid::Beamline::ComponentType::Structured &&
              parentType != Mantid::Beamline::ComponentType::Grid) {

            auto pathToparent = "/raw_data_1/" + compInfo.name(compInfo.root());
            auto bankName = compInfo.name(i);
            auto fullPath = pathToparent + "/" + bankName;
            hasNXTransformation =
                tester.hasNXDataset(fullPath, NX_TRANSFORMATION);

            // assert all test banks have Nxtransformations
            TS_ASSERT(hasNXTransformation);

            hasTranslation =
                tester.hasDataset(fullPath, TRANSLATION, TRANSFORMATION_TYPE);

            hasRotation =
                tester.hasDataset(fullPath, ROTATION, TRANSFORMATION_TYPE);

            if (!(hasRotation || hasTranslation))
              hasEither = false;

            TS_ASSERT(hasEither);
          }
        }
      }
    }
  }

  void
  test_when_nx_source_group_has_nx_transformation_attribute_transformation_type_is_specified() {

    ScopedFileHandle fileResource(
        "check_nxsource_group_has_transformation_type_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // saveinstrument
    saveInstrument(m_instrument, destinationFile);
    auto &compInfo = (*m_instrument.first);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;
    bool hasEither(true); // default to true

    HDF5FileTestUtility tester(destinationFile);

    auto pathToparent = "/raw_data_1/" + compInfo.name(compInfo.root());
    auto sourceName = compInfo.name(compInfo.source());
    auto fullPath = pathToparent + "/" + sourceName;

    hasNXTransformation = tester.hasNXDataset(fullPath, NX_TRANSFORMATION);

    // assert the test source has Nxtransformation.
    TS_ASSERT(hasNXTransformation);

    hasTranslation =
        tester.hasDataset(fullPath, TRANSLATION, TRANSFORMATION_TYPE);

    hasRotation = tester.hasDataset(fullPath, ROTATION, TRANSFORMATION_TYPE);

    if (!(hasRotation || hasTranslation))
      hasEither = false;

    TS_ASSERT(hasEither);
  }

  void
  test_when_nx_sample_group_has_nx_transformation_attribute_transformation_type_is_specified() {

    ScopedFileHandle fileResource(
        "check_nxsample_group_has_transformation_type_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // saveinstrument
    saveInstrument(m_instrument, destinationFile);
    auto &compInfo = (*m_instrument.first);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;
    bool hasEither(true); // default to true

    HDF5FileTestUtility tester(destinationFile);

    auto pathToparent = "/raw_data_1/";
    auto sampleName = compInfo.name(compInfo.sample());
    auto fullPath = pathToparent + sampleName;

    hasNXTransformation = tester.hasNXDataset(fullPath, NX_TRANSFORMATION);

    // assert the test source has Nxtransformation.
    TS_ASSERT(hasNXTransformation);

    hasTranslation =
        tester.hasDataset(fullPath, TRANSLATION, TRANSFORMATION_TYPE);

    hasRotation = tester.hasDataset(fullPath, ROTATION, TRANSFORMATION_TYPE);

    if (!(hasRotation || hasTranslation))
      hasEither = false;

    TS_ASSERT(hasEither);
  }
};

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
