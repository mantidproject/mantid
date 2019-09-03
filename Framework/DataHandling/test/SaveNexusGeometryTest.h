// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENEXUSGEOMETRYTEST_H_
#define MANTID_DATAHANDLING_SAVENEXUSGEOMETRYTEST_H_

#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/SaveNexusGeometry.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidTestHelpers/FileResource.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <H5Cpp.h>
#include <Poco/Glob.h>
#include <boost/filesystem.hpp>
#include <cxxtest/TestSuite.h>

using Mantid::DataHandling::SaveNexusGeometry;

class SaveNexusGeometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNexusGeometryTest *createSuite() {
    return new SaveNexusGeometryTest();
  }
  static void destroySuite(SaveNexusGeometryTest *suite) { delete suite; }

  void test_Init() {
    SaveNexusGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    ScopedFileHandle fileResource("algorithm_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();
    // Create test input if necessary
    Mantid::API::IEventWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument2(1, 5);

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS",
                                                                  inputWS));
    SaveNexusGeometry alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "testWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("H5Path", "algorithm_test_data"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void
  test_execution_succesful_when_no_h5_root_provided_and_default_root_is_used() {

    ScopedFileHandle fileResource("algorithm_no_h5_root_file.hdf5");
    auto destinationFile = fileResource.fullPath();
    // Create test input if necessary
    Mantid::API::IEventWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument2(1, 5);

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS",
                                                                  inputWS));
    SaveNexusGeometry alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "testWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void test_invalid_workspace_throws() {

    /*
    test runtime error is thrown when a workspae without an Instrument is passed
    into the Input workspace property.
    */

    ScopedFileHandle fileResource(
        "algorithm_no_instrument_in_workspace_provided_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();
    // Create test input if necessary

    WorkspaceCreationHelper::EPPTableRow row(
        1, 1, 1, WorkspaceCreationHelper::EPPTableRow::FitStatus::SUCCESS);
    std::vector<WorkspaceCreationHelper::EPPTableRow> rows{row};
    Mantid::API::ITableWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createEPPTableWorkspace(rows);

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS",
                                                                  inputWS));
    SaveNexusGeometry alg;
    // Don't put output in ADS by default

    alg.setChild(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", "testWS"),
                     std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void test_valid_fileName_with_invalid_extension_propagates_throw() {
    /*
    test saveInstrument propagates invalid_argument in executuion of algorithm
    when invalid file extension is passed into fileName property.
    */

    ScopedFileHandle fileResource(
        "algorithm_invalid_extension_provided_test_file.txt");
    auto destinationFile = fileResource.fullPath();
    // Create test workspace
    Mantid::API::IEventWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument2(5, 5);

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS",
                                                                  inputWS));
    SaveNexusGeometry alg;
    // Don't put output in ADS by default

    alg.setChild(false);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "testWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
    TS_ASSERT(!alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void test_characterise_eight_pack_bug_fix() {

    // Bilby contains eight-packs, though other instrument features could still
    // be problematic
    Mantid::DataHandling::LoadEmptyInstrument alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("Filename", "BILBY_Definition.xml");
    alg.setPropertyValue("OutputWorkspace", "ws");
    alg.execute();
    Mantid::API::MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");

    // problem actually in
    // Mantid::NexusGeometry::NexusGeometrySave::saveInstrument /
    // ComponentInfoBankHelpers::isSaveableBank but demonstrated here via save
    // algorithm.

    Mantid::DataHandling::SaveNexusGeometry saver;
    saver.setChild(true);
    saver.setRethrows(true);
    saver.initialize();
    saver.setProperty("Filename", "output.nxs");
    saver.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS_NOTHING(saver.execute());
    TS_ASSERT(saver.isExecuted());
  }
};

#endif /* MANTID_DATAHANDLING_SAVENEXUSGEOMETRYTEST_H_ */
