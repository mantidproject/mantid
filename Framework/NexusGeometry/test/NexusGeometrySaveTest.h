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

#include <H5Cpp.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <gmock/gmock.h>
#include <iostream> // TODO: FOR DEBUGGING. DELETE LATER
#include <memory>
#include <set>

// TODO: FOR DEBUGGING. DELETE LATER
#define DEBUG_PRINT(x) std::cout << "\nDEBUG: " << x << std::endl
// TODO: FOR DEBUGGING. DELETE LATER
#define WAIT std::cin.get()

using namespace Mantid::NexusGeometry;

//---------------------------------------------------------------
namespace {

// for convenience in unit testing while using the HDF5FileTestUtility
typedef std::vector<std::string> fullH5Path;

const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t>(0);
const H5G_obj_t DATASET_TYPE = static_cast<H5G_obj_t>(1);

const std::string SHORT_NAME = "short_name";
const std::string NX_CLASS = "NX_class";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_SOURCE = "NXsource";
const std::string NX_SAMPLE = "NXsample";
const std::string SHAPE = "shape";

const std::string NX_TRANSFORMATIONS = "NXtransformations";
const std::string NX_CHAR = "NX_CHAR";

const std::string TRANSFORMATION_TYPE = "transformation_type";
const std::string ROTATION = "rotation";
const std::string TRANSLATION = "translation";
const std::string TRANSFORMATIONS = "transformations";
const std::string VECTOR = "vector";
const std::string LOCATION = "location";
const std::string DEPENDS_ON = "depends_on";
const std::string ORIENTATION = "orientation";
const std::string UNITS = "units";
const std::string METRES = "m";
const std::string NAME = "name";
const std::string X_PIXEL_OFFSET = "x_pixel_offset";
const std::string Y_PIXEL_OFFSET = "y_pixel_offset";
const std::string Z_PIXEL_OFFSET = "z_pixel_offset";
const std::string SELF_DEPENDENT = ".";

const std::string DEFAULT_ROOT_PATH = "raw_data_1";

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

  H5::Group openfullH5Path(const fullH5Path &pathList) {
    H5::Group parent = m_file.openGroup(pathList[0]);
    H5::Group child;
    for (int i = 1; i < pathList.size(); ++i) {
      child = parent.openGroup(pathList[i]);
      parent = child;
    }
    return child;
  }

  // check if nx transformations writtern to group properly.
  // void canGetTransformations(H5::H5File &file, std::string &groupName) {
  //}

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
                               const fullH5Path &pathToGroup) {
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
                                   const fullH5Path &pathToGroup) {

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

    // TODO: REMOVE LATER
    groupExistsInPath(pathToGroup);

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
    // TODO: REMOVE LATER
    groupExistsInPath(pathToGroup);

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

  bool hasDataset(const fullH5Path &pathToGroup, const std::string dsetName) {

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

    // TODO: REMOVE LATER
    groupExistsInPath(pathToGroup);

    H5::Attribute attribute;
    H5::Group parentGroup = m_file.openGroup(pathToGroup);
    attribute = parentGroup.openAttribute(NX_CLASS);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool groupExistsInPath(const std::string &fullPath) const {

    try {
      H5::Group parentGroup = m_file.openGroup(fullPath);
    } catch (H5::Exception &) {
      DEBUG_PRINT("group doesnt exist...");
      DEBUG_PRINT(fullPath);
      WAIT;
      exit(1);
    }
    return true;
  }

  // HERE
  bool dataSetHasStrValue(const std::string &dataSetName,
                          const std::string &dataSetValue,
                          const std::string &pathToGroup) const {

    // TODO: REMOVE LATER
    groupExistsInPath(pathToGroup);

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
  bool hasAttributeInGroup(const fullH5Path &pathToGroup,
                           const std::string &attrName,
                           const std::string &attrVal) {

    H5::Group parentGroup = openfullH5Path(pathToGroup);

    H5::Attribute attribute = parentGroup.openAttribute(attrName);
    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  bool hasNXAttributeInGroup(const std::string &pathToGroup,
                             const std::string &attrVal) {

    // TODO: REMOVE LATER
    groupExistsInPath(pathToGroup);

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

    // TODO: REMOVE LATER
    groupExistsInPath(pathToGroup);

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

    // TODO: REMOVE LATER
    groupExistsInPath(pathToGroup);

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

  /*
====================================================================

IO PRECONDITIONS TESTS

DESCRIPTION:

The following tests are written to document the behaviour of the SaveInstrument
method when a valid and invalid beamline Instrument are attempted to be saved
out from memory to file. Included also are tests that document the behaviour
when a valid (.nxs, .hdf5 ) or invalid output file extension is attempted to
used.

LIST IN DESCENDING ORDER:

* test providing invalid path throws
* test progress reporting
* test false file extension throws
* test instrument without sample throws
* test instrument without source throws

====================================================================
*/

  void test_providing_invalid_path_throws() {

    ScopedFileHandle fileResource("invalid_path_to_file_test_file.hdf5");
    std::string destinationFile = "false_directory\\" + fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(
                         m_instrument, destinationFile, DEFAULT_ROOT_PATH),
                     std::invalid_argument &);
  }

  void test_progress_reporting() {

    MockProgressBase progressRep;
    EXPECT_CALL(progressRep, doReport(testing::_)).Times(1);

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

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(
                         m_instrument, destinationFile, DEFAULT_ROOT_PATH),
                     std::invalid_argument &);
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

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile,
                                                       DEFAULT_ROOT_PATH),
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

 LIST IN DESCENDING ORDER:

 * test root group is NXentry class
 * test NXinstrument group exists in root group
 * test NXinstrument has expected name
 * test NXsource group exists and is in NXinstrument group
 * test NXsample group exists and is in NXentry group

 ====================================================================
 */

  void test_root_group_is_nxentry_class() {

    ScopedFileHandle fileResource("check_nxentry_group_test_file.nxs");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    NexusGeometrySave::saveInstrument(m_instrument, destinationFile,
                                      DEFAULT_ROOT_PATH);

    HDF5FileTestUtility tester(destinationFile);
    std::string dataSetName = compInfo.name(compInfo.root());

    TS_ASSERT(tester.groupHasNxClass(NX_ENTRY, "/raw_data_1"));
  }

  void test_nxinstrument_group_exists_in_root_group() {

    ScopedFileHandle fileResource("check_nxinstrument_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile,
                                      DEFAULT_ROOT_PATH);
    HDF5FileTestUtility tester(destinationFile);
    std::string dataSetName = compInfo.name(compInfo.root());

    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_INSTRUMENT));
  }

  void test_NXinstrument_has_expected_name() {

    ScopedFileHandle fileResource("check_instrument_name_test_file.nxs");
    auto destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);

    // name of instrument
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile,
                                      DEFAULT_ROOT_PATH); // saves instrument
    HDF5FileTestUtility testUtility(destinationFile);

    std::string pathToGroup = "/raw_data_1/" + expectedInstrumentName;

    TS_ASSERT(
        testUtility.groupExistsInPath(pathToGroup)); // assert group exists

    TS_ASSERT(testUtility.hasNXAttributeInGroup(
        pathToGroup, NX_INSTRUMENT)); // assert group is NXinstrument

    TS_ASSERT(testUtility.dataSetHasStrValue(
        NAME, expectedInstrumentName,
        pathToGroup)) // assert name stored in 'name'
  }

  void test_nxsource_group_exists_and_is_in_nxinstrument_group() {

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile,
                                      DEFAULT_ROOT_PATH);
    HDF5FileTestUtility tester(destinationFile);
    TS_ASSERT(compInfo.hasSource());
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_INSTRUMENT, NX_SOURCE));
  }

  void test_nxsample_group_exists_and_is_in_nxentry_group() {

    ScopedFileHandle fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto const &compInfo = (*m_instrument.first);

    NexusGeometrySave::saveInstrument(m_instrument, destinationFile,
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

LIST(S) IN DESCENDING ORDER:

* test when NXdetector groups have NXtransformations, 'transformation_type' is
specified for all
* test when NXmonitor groups have NXtransformations, 'transformation_type' is
specified for all
*  test when NXsource group has NXtransformations, 'transformation_type' is
specified
* test x/y/z pixel offset in file is relative position from bank without
rotation

* test rotation of source written to file is same as in component info.

LOCATION TESTS:
* test NXmonitor location not written when is at origin
* test NXdetector location not written when is at origin
* test NXsource location not written when is at origin

ROTATION TESTS
* test NXdetector rotation not written when is zero
* test NXmonitor rotation not written when is zero
* test NXsource rotation not written when is zero

====================================================================
*/

  void
  test_when_nx_detector_groups_have_nx_transformations_transformation_type_is_specified_for_all() {
    /*

    ScopedFileHandle fileResource(
        "check_nxdetector_groups_have_transformation_types_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto instrument =
        ComponentCreationHelper::createTestInstrumentRectangular2(2, 2);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // saveinstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);
    auto &compInfo = (*instr.first);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;

    HDF5FileTestUtility tester(destinationFile);

    for (size_t i = compInfo.root() - 1; i > 0; --i) {

      if (Mantid::Geometry::ComponentInfoBankHelpers::isAnyBank(compInfo, i)) {

        auto fullPathToGroup = "/raw_data_1/" + compInfo.name(compInfo.root()) +
                               "/" + compInfo.name(i) + "/" + TRANSFORMATIONS;
        hasNXTransformation = tester.hasAttributeInGroup(
            fullPathToGroup, NX_CLASS, NX_TRANSFORMATIONS);

        // TODO: having such a group may be optional.
        TS_ASSERT(hasNXTransformation);

        hasTranslation = tester.hasDataset(fullPathToGroup, LOCATION);

        hasRotation = tester.hasDataset(fullPathToGroup, ORIENTATION);

        TS_ASSERT((hasRotation || hasTranslation));
      }
    }
        */
  }

  void
  test_when_nx_monitor_groups_have_nx_transformations_transformation_type_is_specified_for_all() {
    /*
    ScopedFileHandle fileResource(
        "check_nxmonitor_groups_have_transformation_types_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    const Quat someRotation(45.0, V3D(0.0, 1.0, 0.0));

    auto instrument =
        ComponentCreationHelper::createMinimalInstrumentWithMonitor(
            V3D(0, 0, 0), someRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // saveinstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);
    auto &compInfo = (*instr.first);
    auto &detInfo = (*instr.second);

    bool hasNXTransformation;
    bool hasRotation;
    bool hasTranslation;

    HDF5FileTestUtility tester(destinationFile);

    auto detIds = detInfo.detectorIDs();

    for (const int &ID : detIds) {
      auto index = detInfo.indexOf(ID);
      if (detInfo.isMonitor(index)) {

        auto fullPathToGroup = "/raw_data_1/" + compInfo.name(compInfo.root()) +
                               "/" + compInfo.name(index) + "/" +
                               TRANSFORMATIONS;
        hasNXTransformation = tester.hasAttributeInGroup(
            fullPathToGroup, NX_CLASS, NX_TRANSFORMATIONS);

        // TODO: having such a group may be optional.
        TS_ASSERT(hasNXTransformation);

        hasTranslation = tester.hasDataset(fullPathToGroup, LOCATION);

        hasRotation = tester.hasDataset(fullPathToGroup, ORIENTATION);

        TS_ASSERT((hasRotation || hasTranslation));
      }
    }
        */
  }

  void
  test_when_nx_source_group_has_nx_transformations_transformation_type_is_specified() {

    // LAST REACTIVATED TEST

    // create RAII file resource for testing
    ScopedFileHandle fileResource(
        "check_nxsource_group_has_transformation_type_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // call saveInstrument
    NexusGeometrySave::saveInstrument(m_instrument, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // get Instrument cache from default unit test instrument
    auto &compInfo = (*m_instrument.first);

    // get component names to access path to H5 group
    auto instrName = compInfo.name(compInfo.root());
    auto sourceName = compInfo.name(compInfo.source());

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    fullH5Path path = {DEFAULT_ROOT_PATH, instrName, sourceName,
                       TRANSFORMATIONS};

    // assertations
    bool hasNXTransformation =
        tester.hasAttributeInGroup(path, NX_CLASS, NX_TRANSFORMATIONS);
    bool hasTranslation = tester.hasDataset(path, LOCATION);
    bool hasRotation = tester.hasDataset(path, ORIENTATION);

    // TODO: having such a group may be optional.
    TS_ASSERT(hasNXTransformation);
    TS_ASSERT((hasRotation || hasTranslation));
  }

  void
  test_xyz_pixel_offset_in_file_is_relative_position_from_bank_without_rotation() {

    // create RAII file resource for testing
    ScopedFileHandle fileResource("check_pixel_offset_format_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare geometry for instrument
    const Quat relativeBankRotation(45.0, V3D(0.0, 1.0, 0.0));
    const Quat relativeDetRotation(45.0, V3D(0.0, 1.0, 0.0));
    const V3D absBankposition(0, 0, 10);
    const V3D detPosition(2.0, -2.0, 0.0);

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
            absBankposition,      // bank position
            relativeBankRotation, // bank rotation
            relativeDetRotation,  // detector rotation
            detPosition);         // detector position
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    auto &compInfo = (*instr.first);

    // saveinstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // get instrument name to access H5 group in test utility
    auto instrName = compInfo.name(compInfo.root());

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    for (size_t idx = compInfo.root() - 1; idx > 0; --idx) {

      if (Mantid::Geometry::ComponentInfoBankHelpers::isAnyBank(compInfo,
                                                                idx)) {
        auto childrenDetectors = compInfo.detectorsInSubtree(idx);

        // get specific bank group name to access H5 group in test utility
        auto bankGroupName = compInfo.name(idx);

        fullH5Path path = {DEFAULT_ROOT_PATH, instrName, bankGroupName};

        for (const size_t &i : childrenDetectors) {

          // initalise to zero for case when an offset is not written to file,
          // thus its values are zero
          double pixelOffsetX = 0.0;
          double pixelOffsetY = 0.0;
          double pixelOffsetZ = 0.0;

          // get the xyz offset of the pixels, and verify that its
          // position reflects removal of rotation transformation relative to
          // bank.
          if (tester.hasDataset(path, X_PIXEL_OFFSET)) {
            pixelOffsetX = tester.readDoubleFromDataset(X_PIXEL_OFFSET, path);
          }

          if (tester.hasDataset(path, Y_PIXEL_OFFSET)) {
            pixelOffsetY = tester.readDoubleFromDataset(Y_PIXEL_OFFSET, path);
          }
          if (tester.hasDataset(path, Z_PIXEL_OFFSET)) {
            pixelOffsetZ = tester.readDoubleFromDataset(Z_PIXEL_OFFSET, path);
          }

          // store offset in this bank to Eigen vector for testing
          Eigen::Vector3d offsetInFile(pixelOffsetX, pixelOffsetY,
                                       pixelOffsetZ);

          // store offset in this bank to Eigen vector for testing
          Eigen::Vector3d expectedOffset =
              Mantid::Geometry::ComponentInfoBankHelpers::offsetFromAncestor(
                  compInfo, idx, i);

          // assert the offsets are equal
          TS_ASSERT(offsetInFile.isApprox(expectedOffset));
        }
      }
    }
  }

  void test_rotation_of_source_written_to_file_is_same_as_in_component_info() {

    // create RAII file resource for testing
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
    auto &compInfo = (*instr.first);

    // get component names to access path to H5 group
    auto instrName = compInfo.name(compInfo.root());
    auto sourceName = compInfo.name(compInfo.source());

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    fullH5Path path = {DEFAULT_ROOT_PATH, instrName, sourceName,
                       TRANSFORMATIONS};

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

  void test_nx_detector_location_not_written_when_is_at_origin() {

    // create RAII file resource for testing
    ScopedFileHandle fileResource("origin_nx_detector_location_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare geometry for instrument
    const V3D bankLocation(0, 0, 0); // set to origin for test
    const V3D sourceLocation(0, 0, -10);
    const Quat someRotation(30, V3D(1, 0, 0));

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0), bankLocation,
            someRotation, someRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    auto &compInfo = (*instr.first);

    // get component names to access path to H5 group
    std::string bankName = "detector-stage";
    auto instrName = compInfo.name(compInfo.root());

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    fullH5Path path = {DEFAULT_ROOT_PATH, instrName, bankName, TRANSFORMATIONS};

    // assertations
    bool hasLocation = tester.hasDataset(path, LOCATION);
    TS_ASSERT(!hasLocation);
  }

  void test_nx_monitor_location_not_written_when_is_at_origin() {

    // create RAII file resource for testing
    ScopedFileHandle fileResource("origin_nx_monitor_location_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare geometry for instrument
    Quat someRotation(45, V3D(0, 1, 0));
    V3D monitorPosition(0, 0, 0); // set to zero for test

    // create test instrument and get cache
    auto instrument =
        ComponentCreationHelper::createMinimalInstrumentWithMonitor(
            monitorPosition, someRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    auto &compInfo = (*instr.first);
    auto &detInfo = (*instr.second);

    // get component names to access path to H5 group
    std::string monitorName = "test-monitor";
    auto instrName = compInfo.name(compInfo.root());

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    fullH5Path path = {DEFAULT_ROOT_PATH, instrName, monitorName,
                       TRANSFORMATIONS};

    // assertations
    bool hasLocation = tester.hasDataset(path, LOCATION);
    TS_ASSERT(detInfo.isMonitor(1)); // assert NXmonitor is at this index
    TS_ASSERT(!hasLocation);
  }

  void test_nx_source_location_not_written_when_is_at_origin() {

    // create RAII file resource for testing
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
    auto &compInfo = (*instr.first);

    // get component names to access path to H5 group
    auto instrName = compInfo.name(compInfo.root());
    auto sourceName = compInfo.name(compInfo.source());

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    // instance of test utility to check saved file
    HDF5FileTestUtility tester(destinationFile);

    // full path to group to be opened in test utility
    fullH5Path path = {DEFAULT_ROOT_PATH, instrName, sourceName,
                       TRANSFORMATIONS};

    // assertations
    bool hasLocation = tester.hasDataset(path, LOCATION);
    TS_ASSERT(!hasLocation);
  }

  void test_nx_detector_rotation_not_written_when_is_zero() {

    /*
const V3D detectorLocation(0, 0, 10);
const V3D sourceLocation(0, 0, -10);

const Quat someRotation(30, V3D(1, 0, 0));
const Quat bankRotation(0, V3D(0, 0, 1)); // set (angle) to zero

ScopedFileHandle fileResource("zero_nx_detector_rotation_file_test.hdf5");
std::string destinationFile = fileResource.fullPath();

auto instrument =
  ComponentCreationHelper::createSimpleInstrumentWithRotation(
      sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
      bankRotation, someRotation);
auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

auto &compInfo = (*instr.first);

std::string bankName = "detector-stage";
auto pathToparent = "/raw_data_1/" + compInfo.name(compInfo.root());
auto fullPathToGroup =
  pathToparent + "/" + bankName + "/" + TRANSFORMATIONS;

NexusGeometrySave::saveInstrument(instr, destinationFile,
                                DEFAULT_ROOT_PATH);

HDF5FileTestUtility tester(destinationFile);

bool hasRotation = tester.hasDataset(fullPathToGroup, ORIENTATION);

TS_ASSERT(!hasRotation);
  */
  }

  void test_nx_monitor_rotation_not_written_when_is_zero() {
    /*
    ScopedFileHandle fileResource("zero_nx_monitor_rotation_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    V3D someLocation(0.0, 0.0, -5.0);

    auto instrument =
        ComponentCreationHelper::createMinimalInstrumentWithMonitor(
            someLocation, Quat(0, V3D(0, 1, 0)));

    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    auto &compInfo = (*instr.first);
    auto &detInfo = (*instr.second);

    // NXmonitor is at following index
    TS_ASSERT(detInfo.isMonitor(1));

    std::string monitorName = "test-monitor";
    auto pathToparent = "/raw_data_1/" + compInfo.name(compInfo.root());
    auto fullPathToGroup =
        pathToparent + "/" + monitorName + "/" + TRANSFORMATIONS;

    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    HDF5FileTestUtility tester(destinationFile);

    bool hasRotation = tester.hasDataset(fullPathToGroup, ORIENTATION);

    TS_ASSERT(!hasRotation);
        */
  }

  void test_source_rotation_not_written_when_is_zero() {
    /*
      const V3D detectorLocation(0, 0, 10);
      const V3D sourceLocation(-10, 0, 0);

      const Quat sourceRotation(0, V3D(0, 0, 1)); // set (angle) to zero

      ScopedFileHandle inFileResource("zero_nx_source_rotation_file_test.hdf5");
      std::string destinationFile = inFileResource.fullPath();

      auto instrument =
          ComponentCreationHelper::createInstrumentWithSourceRotation(
              sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
              sourceRotation); // source rotation
      auto instr =
      Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

      auto &compInfo = (*instr.first);

      auto pathToparent = "/raw_data_1/" + compInfo.name(compInfo.root());
      auto fullPathToGroup = pathToparent + "/" +
                             compInfo.name(compInfo.source()) + "/" +
                             TRANSFORMATIONS;

      NexusGeometrySave::saveInstrument(instr, destinationFile,
                                        DEFAULT_ROOT_PATH);

      HDF5FileTestUtility tester(destinationFile);

      bool hasRotation = tester.hasDataset(fullPathToGroup, ORIENTATION);

      TS_ASSERT(!hasRotation);
          */
  }
  /*
  ====================================================================

  DEPENDENCY CHAIN TESTS

  DESCRIPTION:

  LIST IN DESCENDING ORDER:

  * test when location is not written and orientation exists, source dependency
  is 'orientation' path and orientation is self dependent.
  * test when orientation is not written and location exists, source dependency
  is 'location' path and location is self dependent.
  * test when both orientation and location are written in source, dependency
  chain is: source => orientation => location => self dependent.

  ====================================================================
  */

  void
  test_when_location_is_not_written_and_orientation_exists_source_dependency_is_orientation_path_and_orientation_is_self_dependent() {
    /*
    const V3D detectorLocation(0, 0, 10);
    const V3D sourceLocation(0, 0, 0); // set to zero

    const Quat sourceRotation(90, V3D(0, 1, 0));

    ScopedFileHandle fileResource("no_location_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto instrument =
        ComponentCreationHelper::createInstrumentWithSourceRotation(
            sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
            sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    auto &compInfo = (*instr.first);

    auto pathToparent =
        DEFAULT_ROOT_PATH + "/" + compInfo.name(compInfo.root());
    auto sourceGroup = pathToparent + "/" + compInfo.name(compInfo.source());
    auto transformationsGroup = pathToparent + "/" +
                                compInfo.name(compInfo.source()) + "/" +
                                TRANSFORMATIONS;

    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                      DEFAULT_ROOT_PATH);

    HDF5FileTestUtility tester(destinationFile);

    bool hasLocation = tester.hasDataset(transformationsGroup, LOCATION);
    bool hasOrientation = tester.hasDataset(transformationsGroup, ORIENTATION);

    TS_ASSERT(hasOrientation); // assert orientation dataset exists.
    TS_ASSERT(!hasLocation);   // assert location dataset doesn't exist.

    bool sourceDependencyIsOrientation = tester.dataSetHasStrValue(
        DEPENDS_ON, transformationsGroup + "/" + ORIENTATION, sourceGroup);
    bool orientationDependencyIsSelf = tester.hasAttributeInDataSet(
        transformationsGroup, ORIENTATION, DEPENDS_ON, SELF_DEPENDENT);

    TS_ASSERT(sourceDependencyIsOrientation);
    TS_ASSERT(orientationDependencyIsSelf);
        */
  }

  void
  test_when_orientation_is_not_written_and_location_exists_source_dependency_is_location_path_and_location_is_self_dependent() {
    /*
const V3D detectorLocation(0, 0, 10);
const V3D sourceLocation(0, 0, -10);

const Quat sourceRotation(0, V3D(0, 1, 0)); // set to zero

ScopedFileHandle fileResource("no_orientation_dependency_test.hdf5");
std::string destinationFile = fileResource.fullPath();

auto instrument =
  ComponentCreationHelper::createInstrumentWithSourceRotation(
      sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
      sourceRotation); // source rotation
auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

auto &compInfo = (*instr.first);

auto pathToparent =
  DEFAULT_ROOT_PATH + "/" + compInfo.name(compInfo.root());
auto sourceGroup = pathToparent + "/" + compInfo.name(compInfo.source());
auto transformationsGroup = pathToparent + "/" +
                          compInfo.name(compInfo.source()) + "/" +
                          TRANSFORMATIONS;

NexusGeometrySave::saveInstrument(instr, destinationFile,
                                DEFAULT_ROOT_PATH);

HDF5FileTestUtility tester(destinationFile);

bool hasLocation = tester.hasDataset(transformationsGroup, LOCATION);
bool hasOrientation = tester.hasDataset(transformationsGroup, ORIENTATION);

TS_ASSERT(!hasOrientation); // assert orientation dataset doesn't exist.
TS_ASSERT(hasLocation);     // assert location dataset exists.

bool sourceDependencyIsLocation = tester.dataSetHasStrValue(
  DEPENDS_ON, transformationsGroup + "/" + LOCATION, sourceGroup);
bool locationDependencyIsSelf = tester.hasAttributeInDataSet(
  transformationsGroup, LOCATION, DEPENDS_ON, SELF_DEPENDENT);

TS_ASSERT(sourceDependencyIsLocation);
TS_ASSERT(locationDependencyIsSelf);
*/
  }

  void
  test_when_both_orientation_and_Location_are_written_in_source_dependency_chain_is_source_orientation_location_self_dependent() {
    /*
    const V3D detectorLocation(0, 0, 10);
    const V3D sourceLocation(0, 0, -10);

    const Quat sourceRotation(0, V3D(0, 1, 0)); // set to zero

    ScopedFileHandle fileResource("both_transformations_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto instrument =
      ComponentCreationHelper::createInstrumentWithSourceRotation(
          sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
          sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    auto &compInfo = (*instr.first);

    auto pathToparent =
      DEFAULT_ROOT_PATH + "/" + compInfo.name(compInfo.root());
    auto sourceGroup = pathToparent + "/" + compInfo.name(compInfo.source());
    auto transformationsGroup = pathToparent + "/" +
                              compInfo.name(compInfo.source()) + "/" +
                              TRANSFORMATIONS;

    NexusGeometrySave::saveInstrument(instr, destinationFile,
                                    DEFAULT_ROOT_PATH);

    HDF5FileTestUtility tester(destinationFile);

    bool hasLocation = tester.hasDataset(transformationsGroup, LOCATION);
    bool hasOrientation = tester.hasDataset(transformationsGroup, ORIENTATION);

    TS_ASSERT(!hasOrientation); // assert orientation dataset doesn't exist.
    TS_ASSERT(hasLocation);     // assert location dataset exists.

    bool sourceDependencyIsLocation = tester.dataSetHasStrValue(
      DEPENDS_ON, transformationsGroup + "/" + LOCATION, sourceGroup);
    bool locationDependencyIsSelf = tester.hasAttributeInDataSet(
      transformationsGroup, LOCATION, DEPENDS_ON, SELF_DEPENDENT);

    TS_ASSERT(sourceDependencyIsLocation);
    TS_ASSERT(locationDependencyIsSelf);
      */
  }
};

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
