// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/FileResource.h"
#include "MantidFrameworkTestHelpers/NexusFileReader.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"

#include "mockobjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace Mantid::NexusGeometry;

class NexusGeometrySaveTest : public CxxTest::TestSuite {
private:
  testing::NiceMock<MockLogger> m_mockLogger;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometrySaveTest *createSuite() { return new NexusGeometrySaveTest(); }
  static void destroySuite(NexusGeometrySaveTest *suite) { delete suite; }

  NexusGeometrySaveTest() {}

  /*
====================================================================

IO PRECONDITIONS TESTS

DESCRIPTION:

The following tests are written to document the behaviour of the SaveInstrument
method when a valid and invalid beamline Instrument is attempted to be saved
out from memory to file. Included also are tests that document the behaviour
when a valid (.nxs, .hdf5 ) or invalid output file extension is attempted to be
used.

====================================================================
*/

  void test_providing_invalid_path_throws() {

    FileResource fileResource("invalid_path_to_file_test_file.hdf5");
    const std::string badDestinationPath = "false_directory\\" + fileResource.fullPath();

    auto instrument = ComponentCreationHelper::createMinimalInstrument(V3D(0, 0, -10), V3D(0, 0, 0), V3D(0, 0, 10));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    TS_ASSERT_THROWS(
        NexusGeometrySave::saveInstrument(instr, badDestinationPath, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger),
        std::invalid_argument &);
  }

  void test_progress_reporting() {

    const int nbanks = 2;
    MockProgressBase progressRep;
    EXPECT_CALL(progressRep, doReport(testing::_)).Times(nbanks); // Progress report once for each bank

    FileResource fileResource("progress_report_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(nbanks /*number of banks*/,
                                                                                2 /*number of pixels per bank*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger, false /*strict*/,
                                      &progressRep);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&progressRep));
  }

  void test_false_file_extension_throws() {

    FileResource fileResource("invalid_extension_test_file.abc");
    std::string destinationFile = fileResource.fullPath();

    auto instrument = ComponentCreationHelper::createMinimalInstrument(V3D(0, 0, -10), V3D(0, 0, 0), V3D(0, 0, 10));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger),
                     std::invalid_argument &);
    // Same error but log rather than throw
    MockLogger logger;
    EXPECT_CALL(logger, error(testing::_)).Times(1);
    TS_ASSERT_THROWS(
        NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, logger, false /*append*/),
        const std::invalid_argument &);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&logger));
  }

  void test_instrument_without_sample_throws() {

    auto const &instrument = ComponentCreationHelper::createInstrumentWithOptionalComponents(true, false, true);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    FileResource fileResource("check_no_sample_throws_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    // instrument cache
    auto const &compInfo = (*instr.first);

    TS_ASSERT(compInfo.hasDetectorInfo()); // rule out throw by no detector info
    TS_ASSERT(compInfo.hasSource());       // rule out throw by no source
    TS_ASSERT(!compInfo.hasSample());      // verify component has no sample

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger),
                     std::invalid_argument &);

    // Same error but log rather than throw
    MockLogger logger;
    EXPECT_CALL(logger, error(testing::_)).Times(1);
    TS_ASSERT_THROWS(
        NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, logger, false /*append*/),
        std::invalid_argument &);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&logger));
  }

  void test_instrument_without_source_throws() {

    auto const &instrument = ComponentCreationHelper::createInstrumentWithOptionalComponents(false, true, true);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // instrument cache
    auto &compInfo = (*instr.first);

    FileResource fileResource("check_no_source_throws_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();

    TS_ASSERT(compInfo.hasDetectorInfo()); // rule out throw by no detector info
    TS_ASSERT(compInfo.hasSample());       // rule out throw by no sample
    TS_ASSERT(!compInfo.hasSource());      // verify component has no source

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger),
                     std::invalid_argument &);
    // Same error but log rather than throw
    MockLogger logger;
    EXPECT_CALL(logger, error(testing::_)).Times(1);
    TS_ASSERT_THROWS(
        NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, logger, false /*append*/),
        std::invalid_argument &);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&logger));
  }

  void test_sample_not_at_origin_throws() {

    auto instrument = ComponentCreationHelper::createMinimalInstrument(V3D(0, 0, -10), V3D(0, 0, 2), V3D(0, 0, 10));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    FileResource fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    TS_ASSERT_THROWS(NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger),
                     std::invalid_argument &);
    // Same error but log rather than throw
    MockLogger logger;
    EXPECT_CALL(logger, error(testing::_)).Times(1);
    TS_ASSERT_THROWS(
        NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, logger, false /*append*/),
        std::invalid_argument &);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&logger));
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
    FileResource fileResource("check_nxentry_group_test_file.nxs");
    std::string destinationFile = fileResource.fullPath();

    // test instrument
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/, V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check output file
    NexusFileReader tester(destinationFile);

    // assert the group at the root H5 path is NXentry
    TS_ASSERT(tester.groupHasNxClass(NX_ENTRY, DEFAULT_ROOT_ENTRY_NAME));
  }

  void test_nxinstrument_group_exists_in_root_group() {
    // this test checks that inside of the NXentry root group, the instrument
    // data is saved to a group of NXclass NXinstrument

    // RAII file resource for test file destination
    FileResource fileResource("check_nxinstrument_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with some geometry
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/, V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveinstrument taking test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check the output file
    NexusFileReader tester(destinationFile);

    // assert that inside a group with attribute NXentry, which as per the
    // previous test we know to be the root group, there exists a group of
    // NXclass NXinstrument.
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_INSTRUMENT));
  }

  void test_NXInstrument_name_is_aways_instrument() {
    // TODO: Deprecate and "clean up" (i.e. re-integrate) the
    //  `SaveNexusESS`, `NexusGeometrySave` and `NexusGeometryParser`
    //   code sections.
    // What is described by the next comment is just one example of many, where
    //   an implementation seemed to just "stop in the middle".

    // THIS TEST DOES NOT VERIFY WHAT ITS NAME ACTUALLY SUGGESTS:
    //   in order to be backwards compatible, for the "legacy" instrument format;
    //   YES, the NXInstrument group does need to be named "instrument".
    // However, what this test actually verifies is that the NXinstrument group is assigned the same name
    //   as the name of the `ComponentInfo` root.

    // Since the NXinstrument group actually has a separate "name" attribute, almost certainly,
    // for these codes, it could just be given the group name "instrument".
    // However, fixing this was beyond the scope of the current changes.

    // RAII file resource for test file destination
    FileResource fileResource("check_instrument_name_test_file.nxs");
    auto destinationFile = fileResource.fullPath();

    // test instrument
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/, V3D(0, 0, 10) /*bank position*/);

    // set name of instrument
    instrument->setName("test_instrument_name");
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument passing the test instrument as parameter.
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME,
                                      m_mockLogger); // saves instrument

    // test utility to check the output file.
    NexusFileReader testUtility(destinationFile);

    const auto &compInfo = *instr.first;

    // full H5 path to the NXinstrument group
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, compInfo.name(compInfo.root())};

    // assert no exception thrown on open of instrument group in file with
    // manually set name.
    TS_ASSERT_THROWS_NOTHING(testUtility.openfullH5Path(path));

    // assert group is indeed NXinstrument.
    TS_ASSERT(testUtility.hasNXAttributeInGroup(NX_INSTRUMENT, path));

    // assert the dataset containing the instrument name has been correctly
    // stored also.
    TS_ASSERT(testUtility.dataSetHasStrValue(NAME, compInfo.name(compInfo.root()), path));
  }

  void test_NXclass_without_name_is_assigned_unique_default_name_for_each_group() {
    // this test will try to save and unnamed instrument with multiple unnamed
    // detector banks, to verify that the unique group names which
    // saveInstrument provides for each NXclass do not throw a H5 error due to
    // duplication of group names. If any group in the same tree path share the
    // same name, HDF5 will throw a group exception. In this test, we expect no
    // such exception to throw.

    // RAII file resource for test file destination.
    FileResource fileResource("default_group_names_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // unnamed ("") instrument with multiple unnamed detector banks ("")
    auto instrument = ComponentCreationHelper::createTestUnnamedRectangular2(2 /*number of banks*/, 2);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    TS_ASSERT_THROWS_NOTHING(
        NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger));
  }

  void test_nxsource_group_exists_and_is_in_nxinstrument_group() {
    // this test checks that inside of the NXinstrument group, the source
    // data is saved to a group of NXclass NXsource

    // RAII file resource for test file destination.
    FileResource fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/, V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check output file
    NexusFileReader tester(destinationFile);

    // assert that inside a group with attribute NXinstrument, which as per the
    // previous test we know to be the instrument group, there exists a group of
    // NXclass NXsource.
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_INSTRUMENT, NX_SOURCE));
  }

  void test_nxsample_group_exists_and_is_in_nxentry_group() {

    FileResource fileResource("check_nxsource_group_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, -10) /*source position*/, V3D(0, 0, 0) /*sample position*/, V3D(0, 0, 10) /*bank position*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // instrument cache
    auto &compInfo = (*instr.first);

    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);
    NexusFileReader tester(destinationFile);

    TS_ASSERT(compInfo.hasSample());
    TS_ASSERT(tester.parentNXgroupHasChildNXgroup(NX_ENTRY, NX_SAMPLE));
  }

  void test_correct_number_of_detectors_saved() {

    FileResource fileResource("check_num_of_banks_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    int banksInInstrument = 3;

    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(banksInInstrument /*number of banks*/,
                                                                                4 /*pixels (arbitrary)*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);
    NexusFileReader tester(destinationFile);
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "basic_rect" /*instrument name*/};

    int numOfNXDetectors = tester.countNXgroup(path, NX_DETECTOR);

    TS_ASSERT_EQUALS(numOfNXDetectors, banksInInstrument);
  }

  /*
====================================================================

NEXUS TRANSFOMATIONS TESTS

DESCRIPTION:

The following tests document that saveInstrument will find and write detectors
and other Instrument components to file in Nexus format, and where there exists
transformations in ComponentInfo and DetectorInfo, SaveInstrument will generate
'NXtransformations' groups to contain the corresponding component
rotations/translations, and pixel offsets in any 'NXdetector' found in the
Instrument cache.

====================================================================
*/

  void test_rotation_of_NXdetector_written_to_file_is_same_as_in_component_info() {

    /*
   test scenario: pass into saveInstrument an instrument with manually set
   non-zero rotation in a detector bank. Expectation: test utilty will search
   file for orientaion dataset, read the magnitude of the angle, and the axis
   vector. The output quaternion from file will be compared to the input
   quaternion manually set. Asserts that they are approximately equal,
   indicating that saveinstrument has correctly written the orientation data.
   */

    // RAII file resource for test file destination
    FileResource fileResource("check_rotation_written_to_nxdetector_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare rotation for instrument
    const Quat bankRotation(15, V3D(0, 1, 0));
    const Quat detRotation(30, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument = ComponentCreationHelper::createSimpleInstrumentWithRotation(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0), Mantid::Kernel::V3D(0, 0, 10), bankRotation,
        detRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // instance of test utility to check saved file
    NexusFileReader tester(destinationFile);

    // full path to group to be opened in test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument-with-detector-rotations" /*instrument name*/,
                       "detector-stage" /*bank name*/, TRANSFORMATIONS};

    // get angle magnitude in dataset
    double angleInFile = tester.readDoubleFromDataset(ORIENTATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(attributeName, ORIENTATION, path);
    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of bankRotation for assertation
    Eigen::Quaterniond bankRotationCopy = Mantid::Kernel::toQuaterniond(bankRotation);

    // bank rotation in file as eigen Quaternion for assertation
    Eigen::Quaterniond rotationInFile = Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

    TS_ASSERT(rotationInFile.isApprox(bankRotationCopy));
  }

  void test_rotation_of_NXmonitor_written_to_file_is_same_as_in_component_info() {

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
    FileResource fileResource("check_rotation_written_to_nx_monitor_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare rotation for instrument
    const V3D monitorPosition(0, 1, 0);
    const Quat monitorRotation(30, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument = ComponentCreationHelper::createMinimalInstrumentWithMonitor(monitorPosition, monitorRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // instance of test utility to check saved file
    NexusFileReader tester(destinationFile);

    // full path to group to be opened in test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument-with-monitor", "test-monitor", TRANSFORMATIONS};

    // get angle magnitude in dataset
    double angleInFile = tester.readDoubleFromDataset(ORIENTATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(attributeName, ORIENTATION, path);
    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of monitorRotation for assertation
    Eigen::Quaterniond monitorRotationCopy = Mantid::Kernel::toQuaterniond(monitorRotation);

    // bank rotation in file as eigen Quaternion for assertation
    Eigen::Quaterniond rotationInFile = Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

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
    FileResource fileResource("check_location_written_to_nxsource_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare location for instrument
    const V3D sourceLocation(0, 0, 10);

    // create test instrument and get cache
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        sourceLocation, Mantid::Kernel::V3D(0, 0, 0), Mantid::Kernel::V3D(0, 0, 10), Quat(90, V3D(0, 1, 0)));
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // instance of test utility to check saved file
    NexusFileReader tester(destinationFile);

    // full path to group to be opened in test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument" /*instrument name*/, "source" /*source name*/,
                       TRANSFORMATIONS};

    // get magnitude of vector in dataset
    double normInFile = tester.readDoubleFromDataset(LOCATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> data = tester.readDoubleVectorFrom_d_Attribute(attributeName, LOCATION, path);
    Eigen::Vector3d unitVecInFile = {data[0], data[1], data[2]};

    // Eigen copy of sourceRotation for assertation
    Eigen::Vector3d sourceLocationCopy = Mantid::Kernel::toVector3d(sourceLocation);

    auto positionInFile = normInFile * unitVecInFile;

    TS_ASSERT(positionInFile.isApprox(sourceLocationCopy));
  }

  void test_rotation_of_nx_source_written_to_file_is_same_as_in_component_info() {

    /*
    test scenario: pass into saveInstrument an instrument with manually set
    non-zero rotation in the source. Expectation: test utilty will search file
    for orientaion dataset, read the magnitude of the angle, and the axis
    vector. The output quaternion from file will be compared to the input
    quaternion manually set. Asserts that they are approximately equal,
    indicating that saveinstrument has correctly written the orientation data.
    */

    // RAII file resource for test file destination
    FileResource fileResource("check_rotation_written_to_nxsource_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare rotation for instrument
    const Quat sourceRotation(90, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0), Mantid::Kernel::V3D(0, 0, 10), sourceRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // instance of test utility to check saved file
    NexusFileReader tester(destinationFile);

    // full path to group to be opened in test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument" /*instrument name*/, "source" /*source name*/,
                       TRANSFORMATIONS};

    // get angle magnitude in dataset
    double angleInFile = tester.readDoubleFromDataset(ORIENTATION, path);

    // get axis or rotation
    std::string attributeName = "vector";
    std::vector<double> axisInFile = tester.readDoubleVectorFrom_d_Attribute(attributeName, ORIENTATION, path);
    V3D axisVectorInFile = {axisInFile[0], axisInFile[1], axisInFile[2]};

    // Eigen copy of sourceRotation for assertation
    Eigen::Quaterniond sourceRotationCopy = Mantid::Kernel::toQuaterniond(sourceRotation);

    // source rotation in file as eigen Quaternion for assertation
    Eigen::Quaterniond rotationInFile = Mantid::Kernel::toQuaterniond(Quat(angleInFile, axisVectorInFile));

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
    FileResource fileResource("origin_nx_source_location_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare geometry for instrument
    const V3D detectorLocation(0, 0, 10);
    const V3D sourceLocation(0, 0, 0); // set to zero for test
    const Quat sourceRotation(90, V3D(0, 1, 0));

    // create test instrument and get cache
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
        sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call saveInstrument
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // instance of test utility to check saved file
    NexusFileReader tester(destinationFile);

    // full path to group to be opened in test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument" /*instrument name*/, "source" /*source name*/,
                       TRANSFORMATIONS};

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
    FileResource fileResource("zero_nx_detector_rotation_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with zero source rotation
    auto instrument = ComponentCreationHelper::createSimpleInstrumentWithRotation(
        sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation, bankRotation, someRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // full path to access NXtransformations group with test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument-with-detector-rotations" /*instrument name*/,
                       "detector-stage" /*bank name*/, TRANSFORMATIONS};

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check output file
    NexusFileReader tester(destinationFile);

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
    FileResource fileResource("zero_nx_monitor_rotation_file_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    V3D someLocation(0.0, 0.0, -5.0); // arbitrary monitor location

    // test instrument with zero monitor rotation
    auto instrument = ComponentCreationHelper::createMinimalInstrumentWithMonitor(
        someLocation, Quat(0, V3D(0, 1, 0)) /*monitor rotation of zero*/);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // full path to group to be opened in test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument-with-monitor", "test-monitor", TRANSFORMATIONS};

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check output file
    NexusFileReader tester(destinationFile);

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
    FileResource inFileResource("zero_nx_source_rotation_file_test.hdf5");
    std::string destinationFile = inFileResource.fullPath();

    // test instrument with zero rotation
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
        sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // full path to group to be opened in test utility
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument", "source", TRANSFORMATIONS};

    // call saveinstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check output file
    NexusFileReader tester(destinationFile);

    // assert dataset 'orientation' doesnt exist
    bool hasRotation = tester.hasDataset(ORIENTATION, path);
    TS_ASSERT(!hasRotation);
  }

  void test_xyz_pixel_offset_in_file_is_relative_position_from_bank_without_bank_transformations() {

    // this test will check that the pixel offsets are stored as their positions
    // relative to the parent bank, ignoring any transformations

    /*
    test scenario: instrument with manually set pixel offset passed into
    saveInstrument. Inspection: xyz pixel offset written in file matches the
    manually set offset.
    */

    // create RAII file resource for testing
    FileResource fileResource("check_pixel_offset_format_test_file.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // prepare geometry for instrument
    const Quat relativeBankRotation(45.0, V3D(0.0, 1.0, 0.0));
    const Quat relativeDetRotation(45.0, V3D(0.0, 1.0, 0.0));
    const V3D absBankposition(0, 0, 10);
    const V3D relativeDetposition(2.0, -2.0, 0.0); // i.e. pixel offset

    // create test instrument with one bank consisting of one detector (pixel)
    auto instrument =
        ComponentCreationHelper::createSimpleInstrumentWithRotation(Mantid::Kernel::V3D(0, 0, -10), // source position
                                                                    Mantid::Kernel::V3D(0, 0, 0),   // sample position
                                                                    absBankposition,                // bank position
                                                                    relativeBankRotation,           // bank rotation
                                                                    relativeDetRotation,  // detector (pixel) rotation
                                                                    relativeDetposition); // detector (pixel) position
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // call save insrument passing the test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // instance of test utility to check saved file
    NexusFileReader tester(destinationFile);
    FullNXPath path = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument-with-detector-rotations" /*instrument name*/,
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
    TS_ASSERT(offsetInFile.isApprox(Mantid::Kernel::toVector3d(relativeDetposition)));
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
    FileResource fileResource("no_location_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with location of source at zero
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        sourceLocation, Mantid::Kernel::V3D(0, 0, 0) /*sample position at zero*/, detectorLocation, sourceRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    FullNXPath transformationsPath = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument" /*instrument name*/,
                                      "source" /*source name*/, TRANSFORMATIONS};

    FullNXPath sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument with test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check output file
    NexusFileReader tester(destinationFile);

    // assert what there is no 'location' dataset in NXtransformations, but
    // there is the dataset 'orientation', confirming that saveInstrument has
    // skipped writing a zero translation.
    bool hasLocation = tester.hasDataset(LOCATION, transformationsPath);
    bool hasOrientation = tester.hasDataset(ORIENTATION, transformationsPath);
    TS_ASSERT(hasOrientation); // assert orientation dataset exists.
    TS_ASSERT(!hasLocation);   // assert location dataset doesn't exist.

    // assert that the NXsource depends on dataset 'orientation' in the
    // transformationsPath, since the dataset exists.
    bool sourceDependencyIsOrientation =
        tester.dataSetHasStrValue(DEPENDS_ON, toNXPathString(transformationsPath) + "/" + ORIENTATION, sourcePath);
    TS_ASSERT(sourceDependencyIsOrientation);

    // assert that the orientation depends on itself, since not translation is
    // present
    bool orientationDependencyIsSelf =
        tester.hasAttributeInDataSet(ORIENTATION, DEPENDS_ON, NO_DEPENDENCY, transformationsPath);
    TS_ASSERT(orientationDependencyIsSelf);
  }

  void
  test_when_orientation_is_not_written_and_location_exists_dependency_is_location_path_and_location_is_self_dependent() {

    // USING SOURCE FOR DEMONSTRATION.

    /*
    test scenario: saveInstrument called with zero rotation, and some
    non-zero translation in source. Expected behaviour is: (dataset)
   'depends_on' has value "/absolute/path/to/location", and (dataset)
   'location' has dAttribute (AKA attribute of dataset) 'depends_on' with
   value "."
    */

    // Geometry for test instrument
    const V3D detectorLocation(0.0, 0.0, 10.0);         // arbitrary
    const V3D sourceLocation(0.0, 0.0, -10.0);          // arbitrary
    const Quat sourceRotation(0.0, V3D(0.0, 1.0, 0.0)); // set to zero

    // create RAII file resource for testing
    FileResource fileResource("no_orientation_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with rotation of source of zero
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        sourceLocation, Mantid::Kernel::V3D(0.0, 0.0, 0.0) /*samle position*/, detectorLocation, sourceRotation);
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    FullNXPath transformationsPath = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument" /*instrument name*/,
                                      "source" /*source name*/, TRANSFORMATIONS};

    FullNXPath sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument with test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility for checking file
    NexusFileReader tester(destinationFile);

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
        DEPENDS_ON /*dataset name*/, toNXPathString(transformationsPath) + "/" + LOCATION /*dataset value*/,
        sourcePath /*where the dataset lives*/);
    TS_ASSERT(sourceDependencyIsLocation);

    // assert that the location depends on itself.
    bool locationDependencyIsSelf = tester.hasAttributeInDataSet(
        LOCATION /*dataset name*/, DEPENDS_ON /*dAttribute name*/, NO_DEPENDENCY /*attribute value*/,
        transformationsPath /*where the dataset lives*/);
    TS_ASSERT(locationDependencyIsSelf);
  }

  void test_when_both_orientation_and_Location_are_written_dependency_chain_is_orientation_location_self_dependent() {

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
    FileResource fileResource("both_transformations_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with non zero rotation and translation
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
        sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // path to NXtransformations subgoup in NXsource
    FullNXPath transformationsPath = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument" /*instrument name*/,
                                      "source" /*source name*/, TRANSFORMATIONS};

    // path to NXsource group
    FullNXPath sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility for checking output file
    NexusFileReader tester(destinationFile);

    // assert both location and orientation exists
    bool hasLocation = tester.hasDataset(LOCATION, transformationsPath);
    bool hasOrientation = tester.hasDataset(ORIENTATION, transformationsPath);
    TS_ASSERT(hasOrientation); // assert orientation dataset exists.
    TS_ASSERT(hasLocation);    // assert location dataset exists.

    bool sourceDependencyIsOrientation = tester.dataSetHasStrValue(
        DEPENDS_ON /*dataset name*/, toNXPathString(transformationsPath) + "/" + ORIENTATION /*value in dataset*/,
        sourcePath /*where the dataset lives*/);
    TS_ASSERT(sourceDependencyIsOrientation);
    auto x = toNXPathString(transformationsPath) + "/" + LOCATION /*dAttribute value*/;
    bool orientationDependencyIsLocation = tester.hasAttributeInDataSet(
        ORIENTATION /*dataset name*/, DEPENDS_ON /*dAttribute name*/,
        toNXPathString(transformationsPath) + "/" + LOCATION /*dAttribute value*/, transformationsPath
        /*where the dataset lives*/);
    TS_ASSERT(orientationDependencyIsLocation);

    bool locationDependencyIsSelf = tester.hasAttributeInDataSet(
        LOCATION /*dataset name*/, DEPENDS_ON /*dAttribute name*/, NO_DEPENDENCY /*dAttribute value*/,
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
    FileResource fileResource("neither_transformations_dependency_test.hdf5");
    std::string destinationFile = fileResource.fullPath();

    // test instrument with zero translation and rotation
    auto instrument = ComponentCreationHelper::createInstrumentWithSourceRotation(
        sourceLocation, Mantid::Kernel::V3D(0, 0, 0), detectorLocation,
        sourceRotation); // source rotation
    auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    // path to NXtransformations subgoup in NXsource
    FullNXPath transformationsPath = {DEFAULT_ROOT_ENTRY_NAME, "test-instrument" /*instrument name*/,
                                      "source" /*source name*/, TRANSFORMATIONS};

    // path to NXsource group
    FullNXPath sourcePath = transformationsPath;
    sourcePath.pop_back(); // source path is one level abve transformationsPath

    // call saveInstrument passing test instrument as parameter
    NexusGeometrySave::saveInstrument(instr, destinationFile, DEFAULT_ROOT_ENTRY_NAME, m_mockLogger);

    // test utility to check output file
    NexusFileReader tester(destinationFile);

    // assert source is self dependent
    bool sourceDependencyIsSelf = tester.dataSetHasStrValue(DEPENDS_ON, NO_DEPENDENCY, sourcePath);
    TS_ASSERT(sourceDependencyIsSelf);

    // assert the group NXtransformations doesn't exist in the file
    TS_ASSERT_THROWS(tester.openfullH5Path(transformationsPath), H5::GroupIException &);
  }
};
