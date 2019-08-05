// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <H5Cpp.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <gmock/gmock.h>
#include <iostream>
#include <memory>
#include <set>

using namespace Mantid::NexusGeometry;

//---------------------------------------------------------------
namespace {

const std::string DEFAULT_ROOT_PATH = "raw_data_1";
using FullH5Path = std::vector<std::string>;

// get path as string. Used for the dependency tests.
std::string toH5PathString(FullH5Path &path) {
  std::string pathString = "";
  for (const std::string &grp : path) {
    pathString += "/" + grp;
  }
  return pathString;
}

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

  /* safely open a HDF5 group path with additional helpful
   debug information to output where open fails) */
  H5::Group openfullH5Path(const FullH5Path &pathList) const {

    H5::Group child;
    H5::Group parent = m_file.openGroup(pathList[0]);

    for (size_t i = 1; i < pathList.size(); ++i) {
      child = parent.openGroup(pathList[i]);
      parent = child;
    }
    return child;
  }

  // moves down the index through groups starting at root, and if
  // child has expected CLASS_TYPE, and is in parent group with expected parent

  bool parentNXgroupHasChildNXgroup(const std::string &parentNX_CLASS_TYPE,
                                    const std::string &childNX_CLASS_TYPE) {

    H5::Group rootGroup = m_file.openGroup(DEFAULT_ROOT_PATH);

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
                               const FullH5Path &pathToGroup) {
    double value;
    int rank = 1;
    hsize_t dims[(hsize_t)1];
    dims[0] = (hsize_t)1;

    H5::DataSpace space = H5Screate_simple(rank, dims, NULL);

    // open dataset and read.
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(datasetName);
    dataset.read(&value, H5::PredType::NATIVE_DOUBLE, space);
    return value;
  }

  // HERE
  std::vector<double>
  readDoubleVectorFrom_d_Attribute(const std::string &attrName,
                                   const std::string &datasetName,
                                   const FullH5Path &pathToGroup) {

    // open dataset and read.
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataset = parentGroup.openDataSet(datasetName);

    H5::Attribute attribute = dataset.openAttribute(attrName);

    H5::DataType dataType = attribute.getDataType();
    H5::DataSpace dataSpace = attribute.getSpace();

    std::vector<double> value;
    value.resize(dataSpace.getSelectNpoints());

    attribute.read(dataType, value.data());

    return value;
  }

  // HERE
  bool hasDatasetWithNXAttribute(const std::string &pathToGroup,
                                 const std::string &nx_attributeVal) {

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

  // HERE
  bool hasDatasetWithAttribute(const std::string &pathToGroup,
                               const std::string &attributeVal,
                               const std::string &attrName) {

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

  bool hasDataset(const std::string dsetName, const FullH5Path &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    auto numOfChildren = parentGroup.getNumObjs();
    for (hsize_t i = 0; i < numOfChildren; i++) {
      if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
        std::string dataSetName = parentGroup.getObjnameByIdx(i);
        if (dsetName == dataSetName) {
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

  // HERE
  bool dataSetHasStrValue(
      const std::string &dataSetName, const std::string &dataSetValue,
      const FullH5Path &pathToGroup /*where the dataset lives*/) const {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    try {
      H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
      std::string dataSetVal;
      auto type = dataSet.getDataType();
      dataSet.read(dataSetVal, type);
      dataSetVal.resize(type.getSize());
      return dataSetVal == dataSetValue;
    } catch (H5::DataSetIException &) {
      return false;
    }
  }

  // check if dataset or group has name-specific attribute
  bool hasAttributeInGroup(const std::string &attrName,
                           const std::string &attrVal,
                           const FullH5Path &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(attrName);
    std::string attributeValue;
    auto type = attribute.getDataType();
    attribute.read(type, attributeValue);
    attributeValue.resize(type.getSize());
    return attributeValue == attrVal;
  }

  bool hasNXAttributeInGroup(const std::string &attrVal,
                             const FullH5Path &pathToGroup) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasAttributeInDataSet(
      const std::string dataSetName, const std::string &attrName,
      const std::string &attrVal,
      const FullH5Path &pathToGroup /*where the dataset lives*/) {

    H5::Attribute attribute;
    H5::Group parentGroup = openfullH5Path(pathToGroup);
    H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);
    attribute = dataSet.openAttribute(attrName);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasNXAttributeInDataSet(const std::string dataSetName,
                               const std::string &attrVal,
                               const FullH5Path &pathToGroup) {
    H5::Attribute attribute;
    H5::Group parentGroup = openfullH5Path(pathToGroup);
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

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometrySaveTest *createSuite() {
    return new NexusGeometrySaveTest();
  }
  static void destroySuite(NexusGeometrySaveTest *suite) { delete suite; }

  NexusGeometrySaveTest() {}

  /*
====================================================================

IO PRECONDITIONS TESTS

DESCRIPTION:

The following tests are written to document the behaviour of the SaveInstrument
method when a valid and invalid beamline Instrument are attempted to be saved
out from memory to file. Included also are tests that document the behaviour
when a valid (.nxs, .hdf5 ) or invalid output file extension is attempted to
used.

====================================================================
*/

  void test_providing_invalid_path_throws() {

    ScopedFileHandle fileResource("invalid_path_to_file_test_file.hdf5");
    const std::string badDestinationPath =
        "false_directory\\" + fileResource.fullPath();

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10), V3D(0, 0, 0), V3D(0, 0, 10));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(
                         instr, badDestinationPath, DEFAULT_ROOT_PATH),
                     std::invalid_argument &);
  }

  void test_progress_reporting() {

    MockProgressBase progressRep;
    EXPECT_CALL(progressRep, doReport(testing::_)).Times(2);

    ScopedFileHandle fileResource("progress_report_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto instrument =
        ComponentCreationHelper::createTestInstrumentRectangular2(2, 2);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_PATH,
                                      &progressRep);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&progressRep));
  }

  void test_false_file_extension_throws() {

    ScopedFileHandle fileResource("invalid_extension_test_file.abc");
    std::string destinationFile = fileResource.fullPath();

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10), V3D(0, 0, 0), V3D(0, 0, 10));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile,
                                                       DEFAULT_ROOT_PATH),
                     std::invalid_argument &);
  }

  void test_instrument_without_sample_throws() {

    auto const &instrument =
        ComponentCreationHelper::createInstrumentWithOptionalComponents(
            true, false, true);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    ScopedFileHandle fileResource("check_no_sample_throws_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    // instrument cache
    auto const &compInfo = (*instr.first);

    TS_ASSERT(compInfo.hasDetectorInfo()); // rule out throw by no detector info
    TS_ASSERT(compInfo.hasSource());       // rule out throw by no source
    TS_ASSERT(!compInfo.hasSample());      // verify component has no sample

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile,
                                                       DEFAULT_ROOT_PATH),
                     std::invalid_argument &);
  }

  void test_instrument_without_source_throws() {

    auto const &instrument =
        ComponentCreationHelper::createInstrumentWithOptionalComponents(
            false, true, true);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // instrument cache
    auto &compInfo = (*instr.first);

    ScopedFileHandle fileResource("check_no_source_throws_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    TS_ASSERT(compInfo.hasDetectorInfo()); // rule out throw by no detector info
    TS_ASSERT(compInfo.hasSample());       // rule out throw by no sample
    TS_ASSERT(!compInfo.hasSource());      // verify component has no source

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile,
                                                       DEFAULT_ROOT_PATH),
                     std::invalid_argument &);
  }

  void test_sample_not_at_origin_throws() {

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10), V3D(0, 0, 2), V3D(0, 0, 10));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile,
                                                       DEFAULT_ROOT_PATH),
                     std::invalid_argument &);
  }

  /*
 ====================================================================

 NEXUS FILE FORMAT TESTS

 DESCRIPTION:

 The following tests document that the file format produced by saveInstrument is
 compliant to the present Nexus standard as of the date corresponding to the
 latest version of this document.

 ====================================================================
 */

  void test_root_group_is_nxentry_class() {
    // this test checks that the root group of the output file in saveInstrument
    // has NXclass attribute of NXentry. as required by the Nexus file format.

    // RAII file resource for test file destination
    ScopedFileHandle fileResource("check_nxentry_group_test_file.nxs");
    std::string destinationFile = fileResource.fullPath();

    // test instrument
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/,
        V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check output file
    HDF5FileTestUtility tester(destinationFile);

    // assert the group at the root H5 path is NXentry
    TS_ASSERT(tester.groupHasNxClass(NX_ENTRY, DEFAULT_ROOT_PATH));
  }

  void test_nxinstrument_group_exists_in_root_group() {
    // this test checks that inside of the NXentry root group, the instrument
    // data is saved to a group of NXclass NXinstrument

    // RAII file resource for test file destination
    ScopedFileHandle fileResource("check_nxinstrument_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with some geometry
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/,
        V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveinstrument taking test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check the output file
    HDF5FileTestUtility tester(destinationFile);

    // assert that inside a group with attribute NXentry, which as per the
    // previous test we know to be the root group, there exists a group of
    // NXclass NXinstrument.
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_INSTRUMENT));
  }

  void test_NXclass_with_name_has_same_group_name_and_is_stored_in_dataset() {
    // this test checks that when a name for some component in the instrument
    // cache has been provided, saveInstrument will save the relevant group uder
    // that name. this test is done for the done for the NXinstrument group. the
    // name of the instrument will be manually set, then the test utility will
    // try to open a group with that same same name, if such a group does not
    // exist, a H5 group error is thrown. no such exception is expected to be
    // thrown.

    // RAII file resource for test file destination
    ScopedFileHandle fileResource("check_instrument_name_test_file.nxs");
    auto destinationFile = fileResource.fullPath();

    // test instrument
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/,
        V3D(0, 0, 10) /*bank position*/);

    // set name of instrument
    instrument->setName("test_instrument_name");
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument passing the test instrument as parameter.
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH); // saves instrument

    // test utility to check the output file.
    HDF5FileTestUtility testUtility(destinationFile);

    // full H5 path to the NXinstrument group
    FullH5Path path = {DEFAULT_ROOT_PATH, "test_instrument_name"};

    // assert no exception thrown on open of instrument group in file with
    // manually set name.
    TS_ASSERT_THROWS_NOTHING(testUtility.openfullH5Path(path));

    // assert group is indeed NXinstrument.
    TS_ASSERT(testUtility.hasNXAttributeInGroup(NX_INSTRUMENT, path));

    // assert the dataset containing the instrument name has been correctly
    // stored also.
    TS_ASSERT(
        testUtility.dataSetHasStrValue(NAME, "test_instrument_name", path));
  }

  void
  test_NXclass_without_name_is_assigned_unique_default_name_for_each_group() {
    // this test will try to save and unnameed instrument with multiple unnamed
    // detector banks, to vefiry that the unique group names which
    // saveInstrument provides for each NXclass do not throw a H5 error due to
    // duplication of group names. If any group in the same tree path share the
    // same name, HDF5 will throw a group exception. In this test, we expect no
    // such exception to throw.

    // RAII file resource for test file destination.
    ScopedFileHandle fileResource("default_group_names_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // unnamed ("") instrument with multiple unnamed detector banks ("")
    auto instrument = ComponentCreationHelper::createTestUnnamedRectangular2(
        2 /*number of banks*/, 2 /*number of pixels*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    TS_ASSERT_THROWS_NOTHING(NexusGeometrySave::saveInstrument(
        instr, destinationFile, DEFAULT_ROOT_PATH));
  }

  void test_nxsource_group_exists_and_is_in_nxinstrument_group() {
    // this test checks that inside of the NXinstrument group, the the source
    // data is saved to a group of NXclass NXsource

    // RAII file resource for test file destination.
    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/,
        V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check output file
    HDF5FileTestUtility tester(destinationFile);

    // assert that inside a group with attribute NXinstrument, which as per the
    // previous test we know to be the instrument group, there exists a group of
    // NXclass NXsource.
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_INSTRUMENT, NX_SOURCE));
  }

  void test_nxsample_group_exists_and_is_in_nxentry_group() {

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/,
        V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    auto const &compInfo = (*instr.first);

    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);
    HDF5FileTestUtility tester(destinationFile);

    TS_ASSERT(compInfo.hasSample());
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_SAMPLE));
  }

  /*
====================================================================

NEXUS TRANSFOMATIONS TESTS

DESCRIPTION:

The following tests document that saveInstrument will find and write detectors
and other Instrument components to file in Nexus format, and where there exists
transformations in ComponentInfo and DetectorInfo, SaveInstrument will generate
'NXtransformations' groups to contain the corresponding component
rotations/translations, and pixel offsets in any 'NXdetector' or 'NXmonitor'
found in the Instrument cache.

====================================================================
*/

  void
  test_rotation_of_NXdetector_written_to_file_is_same_as_in_component_info() {

    /*
   test scenario: pass into saveInstrument an instrument with manually set
   non-zero rotation in a detector bank. Expectation: test utilty will search
   file for orientaion dataset, read the magnitude of the angle, and the axis
   vector. The output quaternion from file will be compared to the input
   quaternion manually set. Asserts that they are approximately equal,
   indicating that saveinstrument has correctly written the orientation data.
   */

    // RAII file resource for test file destination
    ScopedFileHandle fileResource(
        "check_rotation_written_to_nxdetector_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare rotation for instrument
    const Quat bankRotation(15, V3D(0, 1, 0));
    const Quat detRotation(30, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
            Mantid::Kernel::V3D(0, 0, 10), bankRotation, detRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    FullH5Path path = {
        DEFAULT_ROOT_PATH,
        "test-instrument-with-detector-rotations" /*instrument name*/,
        "detector-stage" /*bank name*/, TRANSFORMATIONS};

    // get angle magnitude in dataset
    double angleInFile = tester.readDoubleFromDataset(ORIENTATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(
        attributeName, ORIENTATION, path);
    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of bankRotation for assertation
    Eigen::Quaterniond bankRotationCopy =
        Mantid::Kernel::toQuaterniond(bankRotation);

    // bank rotation in file as eigen Quaternion for assertation
    Eigen::Quaterniond rotationInFile =
        Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

    TS_ASSERT(rotationInFile.isApprox(bankRotationCopy));
  }

  void
  test_rotation_of_NXmonitor_written_to_file_is_same_as_in_component_info() {

    /*
        test scenario: pass into saveInstrument an instrument with manually set
        non-zero rotation in a monitor. Expectation: test utilty will search
       file for orientaion dataset, read the magnitude of the angle, and the
       axis vector. The output quaternion from file will be compared to the
       input quaternion manually set. Asserts that they are approximately equal,
        indicating that saveinstrument has correctly written the orientation
       data.
        */

    // RAII file resource for test file destination
    ScopedFileHandle fileResource(
        "check_rotation_written_to_nx_monitor_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare rotation for instrument
    const V3D monitorPosition(0, 1, 0);
    const Quat monitorRotation(30, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createMinimalInstrumentWithMonitor(
            monitorPosition, monitorRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    FullH5Path path = {DEFAULT_ROOT_PATH, "test-instrument-with-monitor",
                       "test-monitor", TRANSFORMATIONS};

    // get angle magnitude in dataset
    double angleInFile = tester.readDoubleFromDataset(ORIENTATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(
        attributeName, ORIENTATION, path);
    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of monitorRotation for assertation
    Eigen::Quaterniond monitorRotationCopy =
        Mantid::Kernel::toQuaterniond(monitorRotation);

    // bank rotation in file as eigen Quaternion for assertation
    Eigen::Quaterniond rotationInFile =
        Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

    TS_ASSERT(rotationInFile.isApprox(monitorRotationCopy));
  }

  void test_location_written_to_file_is_same_as_in_component_info() {

    /*
    test scenario: pass into saveInstrument an instrument with manually set
    non-zero translation in the source. Expectation: test utilty will search
    file for location dataset, read the norm of the vector, and the unit vector.
    The output vector from file will be compared to the input vector manually
    set. Asserts that they are approximately equal, indicating that
    saveinstrument has correctly written the location data.
    */

    // RAII file resource for test file destination
    ScopedFileHandle fileResource(
        "check_location_written_to_nxsource_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare location for instrument
    const V3D sourceLocation(0, 0, 10);

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0),
            Mantid::Kernel::V3D(0, 0, 10), Quat(90, V3D(0, 1, 0)));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    FullH5Path path = {DEFAULT_ROOT_PATH, "test-instrument" /*instrument name*/,
                       "source" /*source name*/, TRANSFORMATIONS};

    // get magnitude of vector in dataset
    double normInFile = tester.readDoubleFromDataset(LOCATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> data =
        tester.readDoubleVectorFrom_d_Attribute(attributeName, LOCATION, path);
    Eigen::Vector3d unitVecInFile = {data[0], data[1], data[2]};

    // Eigen copy of sourceRotation for assertation
    Eigen::Vector3d sourceLocationCopy =
        Mantid::Kernel::toVector3d(sourceLocation);

    auto positionInFile = normInFile * unitVecInFile;

    TS_ASSERT(positionInFile.isApprox(sourceLocationCopy));
  }

  void
  test_rotation_of_nx_source_written_to_file_is_same_as_in_component_info() {

    /*
    test scenario: pass into saveInstrument an instrument with manually set
    non-zero rotation in the source. Expectation: test utilty will search file
    for orientaion dataset, read the magnitude of the angle, and the axis
    vector. The output quaternion from file will be compared to the input
    quaternion manually set. Asserts that they are approximately equal,
    indicating that saveinstrument has correctly written the orientation data.
    */

    // RAII file resource for test file destination
    ScopedFileHandle fileResource(
        "check_rotation_written_to_nxsource_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare rotation for instrument
    const Quat sourceRotation(90, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
            Mantid::Kernel::V3D(0, 0, 10), sourceRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    FullH5Path path = {DEFAULT_ROOT_PATH, "test-instrument" /*instrument name*/,
                       "source" /*source name*/, TRANSFORMATIONS};

    // get angle magnitude in dataset
    double angleInFile = tester.readDoubleFromDataset(ORIENTATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(
        attributeName, ORIENTATION, path);
    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of sourceRotation for assertation
    Eigen::Quaterniond sourceRotationCopy =
        Mantid::Kernel::toQuaterniond(sourceRotation);

    // source rotation in file as eigen Quaternion for assertation
    Eigen::Quaterniond rotationInFile =
        Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

    TS_ASSERT(rotationInFile.isApprox(sourceRotationCopy));
  }


  void test_an_nx_class_location_is_not_written_when_component_position_is_at_origin() { 

    /*
    test scenario: pass into saveInstrument an instrument with zero source
    translation. Inspection: test utilty will search file for location
    dataset and should return false, indicating that saveInstrument
    identified the transformation as effectively zero, and did not write the
    transformation to file
    */

    // RAII file resource for test file destination
    ScopedFileHandle fileResource("origin_nx_source_location_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare geometry for instrument
    const V3D detectorLocation(0, 0, 10);
    const V3D sourceLocation(0, 0, 0); // set to zero for test
    const Quat sourceRotation(90, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
            sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    FullH5Path path = {DEFAULT_ROOT_PATH, "test-instrument" /*instrument name*/,
                       "source" /*source name*/, TRANSFORMATIONS};

    // assertations
    bool hasLocation = tester.hasDataset(LOCATION, path);
    TS_ASSERT(!hasLocation);
  }

  void test_nx_detector_rotation_not_written_when_is_zero() {

    /*
   test scenario: pass into saveInstrument an instrument with zero detector bank
   rotation. Inspection: test utilty will search file for orientation
   dataset and should return false, indicating that saveInstrument
   identified the transformation as effectively zero, and did not write the
   transformation to file
   */

    const V3D detectorLocation(0, 0, 10); // arbitrary non-zero
    const V3D sourceLocation(0, 0, -10);  // arbitrary

    const Quat someRotation(30, V3D(1, 0, 0)); // arbitrary
    const Quat bankRotation(0, V3D(0, 0, 1));  // set (angle) to zero

    // RAII file resource for test file destination
    ScopedFileHandle fileResource("zero_nx_detector_rotation_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with zero source rotation
    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
            bankRotation, someRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // full path to access NXtransformations group with test utility
    FullH5Path path = {
        DEFAULT_ROOT_PATH,
        "test-instrument-with-detector-rotations" /*instrument name*/,
        "detector-stage" /*bank name*/, TRANSFORMATIONS};

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check output file
    HDF5FileTestUtility tester(destinationFile);

    // assert rotation not written to file
    bool hasRotation = tester.hasDataset(ORIENTATION, path);
    TS_ASSERT(!hasRotation);
  }

  void test_nx_monitor_rotation_not_written_when_is_zero() {

    /*
    test scenario: pass into saveInstrument an instrument with zero monitor
    rotation. Inspection: test utilty will search file for orientation dataset
    and should return false, indicating that saveInstrument identified the
    transformation as effectively zero, and did not write the transformation to
    file
    */

    // RAII file resource for test file destination
    ScopedFileHandle fileResource("zero_nx_monitor_rotation_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    V3D someLocation(0.0, 0.0, -5.0); // arbitrary monitor location

    // test instrument with zero monitor rotation
    auto instrument =
        ComponentCreationHelper::createMinimalInstrumentWithMonitor(
            someLocation, Quat(0, V3D(0, 1, 0)) /*monitor rotation of zero*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // full path to group to be opened in test utility
    FullH5Path path = {DEFAULT_ROOT_PATH, "test-instrument-with-monitor",
                       "test-monitor", TRANSFORMATIONS};

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check output file
    HDF5FileTestUtility tester(destinationFile);

    // assert that no dataset named 'orientation' exists in output file
    bool hasRotation = tester.hasDataset(ORIENTATION, path);
    TS_ASSERT(!hasRotation);
  }

  void test_source_rotation_not_written_when_is_zero() {

    /*
    test scenario: pass into saveInstrument an instrument with zero source
    rotation. Inspection: test utilty will search file for orientation dataset
    and should return false, indicating that saveInstrument identified the
    transformation as effectively zero, and did not write the transformation to
    file
    */

    // geometry for test instrument
    const V3D detectorLocation(0, 0, 10);
    const V3D sourceLocation(-10, 0, 0);
    const Quat sourceRotation(0, V3D(0, 0, 1)); // set (angle) to zero

    // RAII file resource for test file destination
    ScopedFileHandle inFileResource("zero_nx_source_rotation_file_test.hdf5");
    std::string destinationFile = inFileResource.fullPath();

    // test instrument with zero rotation
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
            sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // full path to group to be opened in test utility
    FullH5Path path = {DEFAULT_ROOT_PATH, "test-instrument", "source",
                       TRANSFORMATIONS};

    // call saveinstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check output file
    HDF5FileTestUtility tester(destinationFile);

    // assert dataset 'orientation' doesnt exist
    bool hasRotation = tester.hasDataset(ORIENTATION, path);
    TS_ASSERT(!hasRotation);
  }

  void
  test_xyz_pixel_offset_in_file_is_relative_position_from_bank_without_bank_transformations() {

    // this test will check that the pixel offsets are stored as their positions
    // relative to the parent bank, ignoring any transformations

    /*
    test scenario: instrument with manually set pixel offset passed into
    saveInstrument. Inspection: xyz pixel offset written in file matches the
    manually set offset.
    */

    // create RAII file resource for testing
    ScopedFileHandle fileResource("check_pixel_offset_format_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare geometry for instrument
    const Quat relativeBankRotation(45.0, V3D(0.0, 1.0, 0.0));
    const Quat relativeDetRotation(45.0, V3D(0.0, 1.0, 0.0));
    const V3D absBankposition(0, 0, 10);
    const V3D relativeDetposition(2.0, -2.0, 0.0); // i.e. pixel offset

    // create test instrument with one bank consisting of one detector (pixel)
    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            Mantid::Kernel::V3D(0, 0, -10), // source position
            Mantid::Kernel::V3D(0, 0, 0),   // sample position
            absBankposition,                // bank position
            relativeBankRotation,           // bank rotation
            relativeDetRotation,            // detector (pixel) rotation
            relativeDetposition);           // detector (pixel) position
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call save insrument passing the test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);
    FullH5Path path = {
        DEFAULT_ROOT_PATH,
        "test-instrument-with-detector-rotations" /*instrument name*/,
        "detector-stage" /*bank name*/};

    // initalise to zero for case when an offset is not written to file,
    // thus its values are zero

    // read the xyz offset of the pixel from the output file
    double pixelOffsetX = tester.readDoubleFromDataset(X_PIXEL_OFFSET, path);
    double pixelOffsetY = tester.readDoubleFromDataset(Y_PIXEL_OFFSET, path);

    // implicitly assert that z offset is zero, and not written to file, as
    // demonstrated in eairlier tests, where the same method is apled for the
    // pixel offsets.
    TS_ASSERT(!tester.hasDataset(Z_PIXEL_OFFSET, path));

    // store offset in this bank to Eigen vector for testing
    Eigen::Vector3d offsetInFile(pixelOffsetX, pixelOffsetY, 0);

    // assert the offset in the file is approximately the same as that specified
    // manually. thus the offset written by saveInstrument has removed the
    // transformations of the bank
    TS_ASSERT(
        offsetInFile.isApprox(Mantid::Kernel::toVector3d(relativeDetposition)));
  }

  /*
  ====================================================================

  DEPENDENCY CHAIN TESTS

  DESCRIPTION:
  The following tests document that saveInstrument will write the
  NXtransformations dependencies as specified in the Mantid Instrument
  Definition file, which says that if a translation and rotation exists, the
  translation precedes the rotation, so that the NXclass depends on dataset
  'orientation', which depends on dataset 'location'. If only one
  NXtransformation exists, the NXclass group will depend on it. Finally, if no
  NXtransformations are present, the NXclass group will be self dependent.

  ====================================================================
  */

  void
  test_when_location_is_not_written_and_orientation_exists_dependency_is_orientation_path_and_orientation_is_self_dependent() {

    // USING SOURCE FOR DEMONSTRATION.

    /*
        test scenario: saveInstrument called with zero translation, and some
    non-zero rotation in source. Expected behaviour is: (dataset) 'depends_on'
    has value /absoulute/path/to/orientation, and (dataset) 'orientation' has
    dAttribute (AKA attribute of dataset) 'depends_on' with value "."
    */

    // geometry for test instrument
    const V3D detectorLocation(0, 0, 10);        // arbitrary
    const Quat sourceRotation(90, V3D(0, 1, 0)); // arbitrary
    const V3D sourceLocation(0, 0, 0);           // set to zero

    // create RAII file resource for testing
    ScopedFileHandle fileResource("no_location_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with location of source at zero
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation,
            Mantid::Kernel::V3D(0, 0, 0) /*sample position at zero*/,
            detectorLocation, sourceRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    FullH5Path transformationsPath = {
        DEFAULT_ROOT_PATH, "test-instrument" /*instrument name*/,
        "source" /*source name*/, TRANSFORMATIONS};

    FullH5Path sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument with test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check output file
    HDF5FileTestUtility tester(destinationFile);

    // assert what there is no 'location' dataset in NXtransformations, but
    // there is the dataset 'orientation', confirming that saveInstrument has
    // skipped writing a zero translation.
    bool hasLocation = tester.hasDataset(LOCATION, transformationsPath);
    bool hasOrientation = tester.hasDataset(ORIENTATION, transformationsPath);
    TS_ASSERT(hasOrientation); // assert orientation dataset exists.
    TS_ASSERT(!hasLocation);   // assert location dataset doesn't exist.

    // assert that the NXsource depends on dataset 'orientation' in the
    // transformationsPath, since the dataset exists.
    bool sourceDependencyIsOrientation = tester.dataSetHasStrValue(
        DEPENDS_ON, toH5PathString(transformationsPath) + "/" + ORIENTATION,
        sourcePath);
    TS_ASSERT(sourceDependencyIsOrientation);

    // assert that the orientation depends on itself, since not translation is
    // present
    bool orientationDependencyIsSelf = tester.hasAttributeInDataSet(
        ORIENTATION, DEPENDS_ON, NO_DEPENDENCY, transformationsPath);
    TS_ASSERT(orientationDependencyIsSelf);
  }

  void
  test_when_orientation_is_not_written_and_location_exists_dependency_is_location_path_and_location_is_self_dependent() {

    // USING SOURCE FOR DEMONSTRATION.

    /*
    test scenario: saveInstrument called with zero rotation, and some
    non-zero translation in source. Expected behaviour is: (dataset)
   'depends_on' has value "/absoulute/path/to/location", and (dataset)
   'location' has dAttribute (AKA attribute of dataset) 'depends_on' with
   value "."
    */

    // Geometry for test instrument
    const V3D detectorLocation(0.0, 0.0, 10.0);         // arbitrary
    const V3D sourceLocation(0.0, 0.0, -10.0);          // arbitrary
    const Quat sourceRotation(0.0, V3D(0.0, 1.0, 0.0)); // set to zero

    // create RAII file resource for testing
    ScopedFileHandle fileResource("no_orientation_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with rotation of source of zero
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation,
            Mantid::Kernel::V3D(0.0, 0.0, 0.0) /*samle position*/,
            detectorLocation, sourceRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    FullH5Path transformationsPath = {
        DEFAULT_ROOT_PATH, "test-instrument" /*instrument name*/,
        "source" /*source name*/, TRANSFORMATIONS};

    FullH5Path sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument with test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility for checking file
    HDF5FileTestUtility tester(destinationFile);

    // assert what there is no 'orientation' dataset in NXtransformations, but
    // there is the dataset 'location', confirming that saveInstrument has
    // skipped writing a zero reotation.
    bool hasLocation = tester.hasDataset(LOCATION, transformationsPath);
    bool hasOrientation = tester.hasDataset(ORIENTATION, transformationsPath);
    TS_ASSERT(!hasOrientation); // assert orientation dataset doesn't exist.
    TS_ASSERT(hasLocation);     // assert location dataset exists.

    // assert that the NXsource depends on dataset 'location' in the
    // transformationsPath, since the dataset exists.
    bool sourceDependencyIsLocation = tester.dataSetHasStrValue(
        DEPENDS_ON /*dataset name*/,
        toH5PathString(transformationsPath) + "/" + LOCATION /*dataset value*/,
        sourcePath /*where the dataset lives*/);
    TS_ASSERT(sourceDependencyIsLocation);

    // assert that the location depends on itself.
    bool locationDependencyIsSelf = tester.hasAttributeInDataSet(
        LOCATION /*dataset name*/, DEPENDS_ON /*dAttribute name*/,
        NO_DEPENDENCY /*attribute value*/,
        transformationsPath /*where the dataset lives*/);
    TS_ASSERT(locationDependencyIsSelf);
  }

  void
  test_when_both_orientation_and_Location_are_written_dependency_chain_is_orientation_location_self_dependent() {

    // USING SOURCE FOR DEMONSTRATION.

    /*
        test scenario: saveInstrument called with non-zero rotation, and some
        non-zero translation in source. Expected behaviour is: (dataset)
       'depends_on' has value "/absoulute/path/to/orientation", (dataset)
       'orientation' has dAttribute (AKA attribute of dataset) 'depends_on' with
       value "/absoulute/path/to/location", and (dataset) 'location' has
       dAttribute 'depends_on' with value "."
         */

    // Geometry for test instrument
    const V3D detectorLocation(0, 0, 10);        // arbitrary
    const V3D sourceLocation(0, 0, -10);         // arbitrary non-origin
    const Quat sourceRotation(45, V3D(0, 1, 0)); // arbitrary non-zero

    // create RAII file resource for testing
    ScopedFileHandle fileResource("both_transformations_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with non zero rotation and translation
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
            sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // path to NXtransformations subgoup in NXsource
    FullH5Path transformationsPath = {
        DEFAULT_ROOT_PATH, "test-instrument" /*instrument name*/,
        "source" /*source name*/, TRANSFORMATIONS};

    // path to NXsource group
    FullH5Path sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility for checking output file
    HDF5FileTestUtility tester(destinationFile);

    // assert both location and orientation exists
    bool hasLocation = tester.hasDataset(LOCATION, transformationsPath);
    bool hasOrientation = tester.hasDataset(ORIENTATION, transformationsPath);
    TS_ASSERT(hasOrientation); // assert orientation dataset exists.
    TS_ASSERT(hasLocation);    // assert location dataset exists.

    bool sourceDependencyIsLocation =
        tester.dataSetHasStrValue(DEPENDS_ON /*dataset name*/,
                                  toH5PathString(transformationsPath) + "/" +
                                      ORIENTATION /*value in dataset*/,
                                  sourcePath /*where the dataset lives*/);
    TS_ASSERT(sourceDependencyIsLocation);

    bool orientationDependencyIsLocation = tester.hasAttributeInDataSet(
        ORIENTATION /*dataset name*/, DEPENDS_ON /*dAttribute name*/,
        toH5PathString(transformationsPath) + "/" +
            LOCATION /*attribute value*/,
        transformationsPath
        /*where the dataset lives*/);
    TS_ASSERT(orientationDependencyIsLocation);

    bool locationDependencyIsSelf = tester.hasAttributeInDataSet(
        LOCATION /*dataset name*/, DEPENDS_ON /*dAttribute name*/,
        NO_DEPENDENCY /*dAttribute value*/,
        transformationsPath /*where the dataset lives*/);
    TS_ASSERT(locationDependencyIsSelf);
  }

  void
  test_when_neither_orientation_nor_Location_are_written_dependency_is_self_and_nx_transformations_group_is_not_written() {

    // USING SOURCE FOR DEMONSTRATION.

    /*
     test scenario: saveInstrument called with zero rotation, and
     zero translation in source. Expected behaviour is: (dataset)
     'depends_on' has value "."
    */

    const V3D detectorLocation(0, 0, 10);       // arbitrary
    const V3D sourceLocation(0, 0, 0);          // set to zero
    const Quat sourceRotation(0, V3D(0, 1, 0)); // set to zero

    // create RAII file resource for testing
    ScopedFileHandle fileResource(
        "neither_transformations_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with zero translation and rotation
    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
            sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // path to NXtransformations subgoup in NXsource
    FullH5Path transformationsPath = {
        DEFAULT_ROOT_PATH, "test-instrument" /*instrument name*/,
        "source" /*source name*/, TRANSFORMATIONS};

    // path to NXsource group
    FullH5Path sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // test utility to check output file
    HDF5FileTestUtility tester(destinationFile);

    // assert source is self dependent
    bool sourceDependencyIsSelf =
        tester.dataSetHasStrValue(DEPENDS_ON, NO_DEPENDENCY, sourcePath);
    TS_ASSERT(sourceDependencyIsSelf);

    // assert the group NXtransformations doesnt exist in file
    TS_ASSERT_THROWS(tester.openfullH5Path(transformationsPath),
                     H5::GroupIException &)
  }
};

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
