// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidAlgorithms/CreateDetectorTable.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Detector.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class CreateDetectorTableTest : public CxxTest::TestSuite {
public:
  static CreateDetectorTableTest *createSuite() { return new CreateDetectorTableTest(); }

  static void destroySuite(CreateDetectorTableTest *suite) { delete suite; }

  void test_Name() {
    CreateDetectorTable alg;
    TS_ASSERT_EQUALS(alg.name(), "CreateDetectorTable");
  }

  void test_Version() {
    Mantid::Algorithms::CreateDetectorTable alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_Init() {
    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    const auto &props = alg.getProperties();
    TS_ASSERT_EQUALS(props.size(), 5);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());

    TS_ASSERT_EQUALS(props[1]->name(), "WorkspaceIndices");
    TS_ASSERT(props[1]->isDefault());

    TS_ASSERT_EQUALS(props[2]->name(), "IncludeData");
    TS_ASSERT(props[2]->isDefault());

    TS_ASSERT_EQUALS(props[3]->name(), "IncludeDetectorPosition");
    TS_ASSERT(props[3]->isDefault());

    TS_ASSERT_EQUALS(props[4]->name(), "DetectorTableWorkspace");
    TS_ASSERT(props[4]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<TableWorkspace> *>(props[4]));
  }

  void test_Exec_Matrix_Workspace() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 11);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_with_no_valid_spectra() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    auto &spec = inputWS->getSpectrum(0);
    spec.clearDetectorIDs(); // clear the detectors to test the exception catching

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 11);
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), -1); // Spectrum No should be -1

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_With_Altered_Parameters() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    std::string outWSName{"Detector Table Test"};

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WorkspaceIndices", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludeData", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Check that a missing efixed value throws a runtime error
    const auto &spectrumInfo = inputWS->spectrumInfo();
    std::shared_ptr<const IDetector> detector(&spectrumInfo.detector(0), Mantid::NoDeleting());
    TS_ASSERT_THROWS(inputWS->getEFixed(detector), const std::runtime_error &);
    // Check that an invalid efixed value throws an invalid argument error
    auto &run = inputWS->mutableRun();
    run.addProperty("deltaE-mode", std::string("Direct"), true);
    run.addProperty("Ei", std::string("23423f42"));

    TS_ASSERT_THROWS(inputWS->getEFixed(detector), const std::invalid_argument &);

    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(outWSName));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 13);
    TS_ASSERT_EQUALS(ws->rowCount(), 1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_Exec_Peaks_Workspace() {
    PeaksWorkspace_sptr inputWS = WorkspaceCreationHelper::createPeaksWorkspace(5, false);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 2);
    TS_ASSERT_EQUALS(ws->rowCount(), 5);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Non_Peak_Table_Workspace_Throws_Exception() {
    ITableWorkspace_sptr inputWS = std::make_shared<TableWorkspace>();

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));

    TS_ASSERT_THROWS(alg.executeAsChildAlg(), const std::runtime_error &);
  }

  void test_Exec_Matrix_Workspace_with_Include_DetPos() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludeDetectorPosition", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 12); // extra column compared to test_Exec_Matrix_Workspace
    TS_ASSERT_EQUALS(ws->rowCount(), 2);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_with_no_valid_spectra_include_DetPos() {
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    auto &spec = inputWS->getSpectrum(0);
    spec.clearDetectorIDs(); // clear the detectors to test the exception catching

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludeDetectorPosition", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 12);
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->cell<int>(0, 1), -1);                  // Spectrum No should be -1
    TS_ASSERT_EQUALS(ws->cell<V3D>(0, 11), V3D(0.0, 0.0, 0.0)); // Detector Position should be (0.0, 0.0, 0.0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }
};

class CreateDetectorTablePerformance : public CxxTest::TestSuite {
public:
  MatrixWorkspace_sptr WS;
  CreateDetectorTable alg;

  void setUp() override { WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10000, 1000); }

  void tearDown() override { AnalysisDataService::Instance().remove(WS->getName() + "-Detectors"); }

  void testExec() {
    alg.initialize();
    alg.setProperty("InputWorkspace", WS);
    alg.execute();
  }
};
