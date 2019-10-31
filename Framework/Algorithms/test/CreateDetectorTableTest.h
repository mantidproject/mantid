// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CREATEDETECTORTABLETEST_H_
#define CREATEDETECTORTABLETEST_H_

#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidAlgorithms/CreateDetectorTable.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CreateDetectorTableTest : public CxxTest::TestSuite {
public:
  static CreateDetectorTableTest *createSuite() {
    return new CreateDetectorTableTest();
  }

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
    TS_ASSERT_EQUALS(props.size(), 4);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());

    TS_ASSERT_EQUALS(props[1]->name(), "WorkspaceIndices");
    TS_ASSERT(props[1]->isDefault());

    TS_ASSERT_EQUALS(props[2]->name(), "IncludeData");
    TS_ASSERT(props[2]->isDefault());

    TS_ASSERT_EQUALS(props[3]->name(), "DetectorTableWorkspace");
    TS_ASSERT(props[3]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<TableWorkspace> *>(props[3]));
  }

  void test_Exec_Matrix_Workspace() {
    Workspace2D_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace",
                        boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Not setting an output workspace name should give the name:
    //[input workspace name] + "-Detectors"
    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 7);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Matrix_Workspace_With_Altered_Parameters() {
    Workspace2D_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    std::string outWSName{"Detector Table Test"};

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace",
                        boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WorkspaceIndices", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IncludeData", true));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorTableWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            outWSName));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 9);
    TS_ASSERT_EQUALS(ws->rowCount(), 1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_Exec_Peaks_Workspace() {
    PeaksWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createPeaksWorkspace(5, false);

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            inputWS->getName() + "-Detectors"));
    TS_ASSERT(ws);

    if (!ws) {
      return;
    }

    // Check the results
    TS_ASSERT_EQUALS(ws->columnCount(), 2)
    TS_ASSERT_EQUALS(ws->rowCount(), 5)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(ws->getName());
  }

  void test_Exec_Non_Peak_Table_Workspace_Throws_Exception() {
    ITableWorkspace_sptr inputWS = boost::make_shared<TableWorkspace>();

    CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))

    TS_ASSERT_THROWS(alg.executeAsChildAlg(), std::runtime_error);
  }
};

class CreateDetectorTablePerformance : public CxxTest::TestSuite {
public:
  MatrixWorkspace_sptr WS;
  CreateDetectorTable alg;

  void setUp() override {
    WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10000,
                                                                      1000);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove(WS->getName() + "-Detectors");
  }

  void testExec() {
    alg.initialize();
    alg.setProperty("InputWorkspace", WS);
    alg.execute();
  }
};

#endif // CREATEDETECTORTABLETEST_H_
