// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/SaveNexusGeometry.h"
#include "MantidFrameworkTestHelpers/FileResource.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/ProgressBase.h"

#include <H5Cpp.h>
#include <cxxtest/TestSuite.h>

using Mantid::DataHandling::SaveNexusGeometry;

class SaveNexusGeometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNexusGeometryTest *createSuite() { return new SaveNexusGeometryTest(); }
  static void destroySuite(SaveNexusGeometryTest *suite) { delete suite; }

  void test_Init() {
    SaveNexusGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    FileResource fileResource("algorithm_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();
    // Create test input
    Mantid::API::IEventWorkspace_sptr inputWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument2(1, 5);

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS", inputWS));
    SaveNexusGeometry alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "testWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("EntryName", "algorithm_test_data"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void test_execution_successful_when_no_h5_root_provided_and_default_root_is_used() {

    FileResource fileResource("algorithm_no_h5_root_file.hdf5");
    auto destinationFile = fileResource.fullPath();
    // Create test input if necessary
    Mantid::API::IEventWorkspace_sptr inputWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument2(1, 5);

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS", inputWS));
    SaveNexusGeometry alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "testWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void test_invalid_workspace_throws() {

    /*
    test that runtime error is thrown when a workspace without an Instrument is passed
    into the Input workspace property.
    */

    FileResource fileResource("algorithm_no_instrument_in_workspace_provided_test_file.hdf5");
    auto destinationFile = fileResource.fullPath();
    // Create test input if necessary

    WorkspaceCreationHelper::EPPTableRow row(1, 1, 1, WorkspaceCreationHelper::EPPTableRow::FitStatus::SUCCESS);
    std::vector<WorkspaceCreationHelper::EPPTableRow> rows{row};
    Mantid::API::ITableWorkspace_sptr inputWS = WorkspaceCreationHelper::createEPPTableWorkspace(rows);

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS", inputWS));
    SaveNexusGeometry alg;
    // Don't put output in ADS by default

    alg.setChild(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", "testWS"), std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName", destinationFile));
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void test_valid_fileName_with_invalid_extension_propagates_throw() {
    /*
    test saveInstrument propagates invalid_argument in executuion of algorithm
    when invalid file extension is passed into fileName property.
    */

    FileResource fileResource("algorithm_invalid_extension_provided_test_file.txt");
    auto destinationFile = fileResource.fullPath();
    // Create test workspace
    Mantid::API::IEventWorkspace_sptr inputWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument2(5, 5);

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().addOrReplace("testWS", inputWS));
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

    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().remove("testWS"));
  }

  void test_eight_pack() {

    FileResource fileResource("eight_pack.hdf5");
    auto destinationFile = fileResource.fullPath();

    Mantid::DataHandling::LoadEmptyInstrument alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("Filename", "BILBY_Definition.xml");
    alg.setPropertyValue("OutputWorkspace", "ws");
    alg.execute();
    Mantid::API::MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");

    Mantid::DataHandling::SaveNexusGeometry saver;
    saver.setChild(true);
    saver.setRethrows(true);
    saver.initialize();
    saver.setProperty("Filename", destinationFile);
    saver.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS_NOTHING(saver.execute());
    TS_ASSERT(saver.isExecuted());
  }

  void test_duplicate_named_components_in_instrument_throws() {

    /*
    instrument Definition HET_Definition_old.xml contains at least two monitors
    both named "monitor". Expected behaviour is that nexus geometry save will
    not allow naming of two groups with the same name in the same parent; Throws
    exception.
    */

    FileResource fileResource("duplicate_names_test.hdf5");
    auto destinationFile = fileResource.fullPath();

    Mantid::DataHandling::LoadEmptyInstrument loader;
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Filename", "HET_Definition_old.xml");
    loader.setPropertyValue("OutputWorkspace", "dummy");
    loader.execute();
    Mantid::API::MatrixWorkspace_sptr ws = loader.getProperty("OutputWorkspace");

    SaveNexusGeometry saver;
    saver.setChild(true);
    saver.setRethrows(true);
    saver.initialize();
    saver.setProperty("Filename", destinationFile);
    saver.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS(saver.execute(), std::invalid_argument &);
    TS_ASSERT(!saver.isExecuted());
  }
};
