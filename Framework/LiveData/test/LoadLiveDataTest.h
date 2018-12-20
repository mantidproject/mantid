// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_LOADLIVEDATATEST_H_
#define MANTID_LIVEDATA_LOADLIVEDATATEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidLiveData/LoadLiveData.h"
#include "MantidTestHelpers/FacilityHelper.h"
#include "TestGroupDataListener.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid;
using namespace Mantid::LiveData;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class LoadLiveDataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadLiveDataTest *createSuite() { return new LoadLiveDataTest(); }
  static void destroySuite(LoadLiveDataTest *suite) { delete suite; }

  void setUp() override {
    FrameworkManager::Instance();
    AnalysisDataService::Instance().clear();
    ConfigService::Instance().setString("testdatalistener.reset_after", "0");
  }

  void test_Init() {
    LoadLiveData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //--------------------------------------------------------------------------------------------
  /** Run a test with a fake output, no processing
   *
   * @param AccumulationMethod :: parameter string
   * @return the created processed WS
   */
  template <typename TYPE>
  boost::shared_ptr<TYPE>
  doExec(std::string AccumulationMethod, std::string ProcessingAlgorithm = "",
         std::string ProcessingProperties = "",
         std::string PostProcessingAlgorithm = "",
         std::string PostProcessingProperties = "", bool PreserveEvents = true,
         ILiveListener_sptr listener = ILiveListener_sptr(),
         bool makeThrow = false) {
    FacilityHelper::ScopedFacilities loadTESTFacility(
        "unit_testing/UnitTestFacilities.xml", "TEST");

    LoadLiveData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Instrument", "TestDataListener"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("AccumulationMethod", AccumulationMethod));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ProcessingAlgorithm", ProcessingAlgorithm));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ProcessingProperties", ProcessingProperties));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PostProcessingAlgorithm",
                                                  PostProcessingAlgorithm));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PostProcessingProperties",
                                                  PostProcessingProperties));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PreserveEvents", PreserveEvents));
    if (!PostProcessingAlgorithm.empty())
      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue("AccumulationWorkspace", "fake_accum"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "fake"));
    if (listener)
      alg.setLiveListener(listener);

    if (!makeThrow) {
      TS_ASSERT_THROWS_NOTHING(alg.execute(););
    } else
      alg.exec();
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    boost::shared_ptr<TYPE> ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<TYPE>("fake"));
    TS_ASSERT(ws);

    if (auto matrixws = dynamic_cast<MatrixWorkspace *>(ws.get())) {
      TSM_ASSERT_LESS_THAN("Run number should be non-zero", 0,
                           matrixws->getRunNumber());
    }

    return ws;
  }

  //--------------------------------------------------------------------------------------------
  void test_replace() {
    EventWorkspace_sptr ws1, ws2;

    ws1 = doExec<EventWorkspace>("Replace");
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws1->getNumberEvents(), 200);

    ws2 = doExec<EventWorkspace>("Replace");
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws2->getNumberEvents(), 200);
    TSM_ASSERT("Workspace changed when replaced", ws1 != ws2);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 1);
  }

  //--------------------------------------------------------------------------------------------
  void test_replace_keeps_original_instrument() {
    auto ws1 = doExec<EventWorkspace>("Replace");
    auto &ws1CompInfo = ws1->mutableComponentInfo();
    // Put the sample somewhere else prior to the next replace
    const Kernel::V3D newSamplePosition =
        ws1CompInfo.position(ws1CompInfo.sample()) + V3D(1, 1, 1);
    ws1CompInfo.setPosition(ws1CompInfo.sample(), newSamplePosition);

    // Second Run of replace
    auto ws2 = doExec<EventWorkspace>("Replace");
    const auto &ws2CompInfo = ws2->componentInfo();
    // Check the sample is where I put it. i.e. Instrument should NOT be
    // overwritten.
    TSM_ASSERT_EQUALS("Instrument should NOT have been overwritten",
                      newSamplePosition,
                      ws2CompInfo.position(ws2CompInfo.sample()));
  }

  //--------------------------------------------------------------------------------------------
  void test_replace_workspace_with_group() {
    auto ws1 = doExec<EventWorkspace>("Replace");

    TS_ASSERT_THROWS_NOTHING(
        doExec<WorkspaceGroup>("Replace", "", "", "", "", false,
                               ILiveListener_sptr(new TestGroupDataListener)));
  }

  //--------------------------------------------------------------------------------------------
  void test_replace_group_with_workspace() {
    auto ws1 =
        doExec<WorkspaceGroup>("Replace", "", "", "", "", false,
                               ILiveListener_sptr(new TestGroupDataListener));

    TS_ASSERT_THROWS_NOTHING(doExec<EventWorkspace>("Replace"));
  }

  //--------------------------------------------------------------------------------------------
  void test_append() {
    EventWorkspace_sptr ws1, ws2;

    // First go creates the fake ws
    ws1 = doExec<EventWorkspace>("Append");
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);

    // Next one actually conjoins
    ws2 = doExec<EventWorkspace>("Append");
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 1);
  }

  //--------------------------------------------------------------------------------------------
  void test_add() {
    EventWorkspace_sptr ws1, ws2;

    // First go creates the fake ws
    ws1 = doExec<EventWorkspace>("Add");
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws1->getNumberEvents(), 200);

    // Next one adds events, keeps # of histos the same
    ws2 = doExec<EventWorkspace>("Add", "", "", "", "", true,
                                 ILiveListener_sptr());
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws2->getNumberEvents(), 400);

    TSM_ASSERT("Workspace being added stayed the same pointer", ws1 == ws2);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 1);

    // Test monitor workspace is present
    TS_ASSERT(ws2->monitorWorkspace());
  }

  //--------------------------------------------------------------------------------------------
  void test_add_DontPreserveEvents() {
    Workspace2D_sptr ws1, ws2;

    // First go creates the fake ws
    ws1 = doExec<Workspace2D>("Add", "Rebin", "Params=40e3, 1e3, 60e3", "", "",
                              false);
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);
    double total;
    total = 0;
    for (double yValue : ws1->readY(0))
      total += yValue;
    TS_ASSERT_DELTA(total, 100.0, 1e-4);

    // Next one adds the histograms together
    ws2 = doExec<Workspace2D>("Add", "Rebin", "Params=40e3, 1e3, 60e3", "", "",
                              false);
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 2);

    // The new total signal is 200.0
    total = 0;
    for (double yValue : ws1->readY(0))
      total += yValue;
    TS_ASSERT_DELTA(total, 200.0, 1e-4);

    TSM_ASSERT("Workspace being added stayed the same pointer", ws1 == ws2);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 1);

    TS_ASSERT(ws1->monitorWorkspace());
    TS_ASSERT_EQUALS(ws1->monitorWorkspace(), ws2->monitorWorkspace());
  }

  //--------------------------------------------------------------------------------------------
  /** Simple processing of a chunk */
  void test_ProcessChunk_DoPreserveEvents() {
    EventWorkspace_sptr ws;
    ws = doExec<EventWorkspace>("Replace", "", "", "Rebin",
                                "Params=40e3, 1e3, 60e3", true);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    // Check that rebin was called
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 2);
  }

  //--------------------------------------------------------------------------------------------
  /** DONT convert to workspace 2D when processing */
  void test_ProcessChunk_DontPreserveEvents() {
    Workspace2D_sptr ws;
    ws = doExec<Workspace2D>("Replace", "Rebin", "Params=40e3, 1e3, 60e3", "",
                             "", false);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    // Check that rebin was called
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 1);
    TS_ASSERT(ws->monitorWorkspace());
  }

  //--------------------------------------------------------------------------------------------
  /** Do PostProcessing */
  void test_PostProcessing() {
    // No chunk processing, but PostProcessing
    EventWorkspace_sptr ws = doExec<EventWorkspace>("Replace", "", "", "Rebin",
                                                    "Params=40e3, 1e3, 60e3");
    EventWorkspace_sptr ws_accum =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "fake_accum");
    TS_ASSERT(ws)
    TS_ASSERT(ws_accum)

    // The accumulated workspace: it was NOT rebinned.
    TS_ASSERT_EQUALS(ws_accum->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws_accum->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws_accum->blocksize(), 1);

    // The post-processed workspace: Check that rebin was called
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 2);
  }

  //--------------------------------------------------------------------------------------------
  /** Perform both chunk and post-processing*/
  void test_Chunk_and_PostProcessing() {
    // Process both times
    EventWorkspace_sptr ws =
        doExec<EventWorkspace>("Replace", "Rebin", "Params=20e3, 1e3, 60e3",
                               "Rebin", "Params=40e3, 1e3, 60e3");
    EventWorkspace_sptr ws_accum =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "fake_accum");
    TS_ASSERT(ws)
    TS_ASSERT(ws_accum)

    // Accumulated workspace: it was rebinned, but rebinning should be reset
    TS_ASSERT_EQUALS(ws_accum->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws_accum->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws_accum->blocksize(), 40);

    // The post-processed workspace was rebinned starting at 40e3
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().size(), 2);
  }

  //--------------------------------------------------------------------------------------------
  /** Do some processing that converts to a different type of workspace */
  void test_ProcessToMDWorkspace_and_Add() {
    IMDWorkspace_sptr ws;
    ws = doExec<IMDWorkspace>("Add", "ConvertToDiffractionMDWorkspace", "");
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getNumDims(), 3);
    TS_ASSERT_EQUALS(ws->getNPoints(), 200);

    // Does the adding work?
    ws = doExec<IMDWorkspace>("Add", "ConvertToDiffractionMDWorkspace", "");
    TS_ASSERT_EQUALS(ws->getNPoints(), 400);
  }

  //--------------------------------------------------------------------------------------------
  /** Handle WorkspaceGroups returned by the listener */
  void test_WorkspaceGroup_Replace_None_None() {
    WorkspaceGroup_sptr ws =
        doExec<WorkspaceGroup>("Replace", "", "", "", "", false,
                               ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(mws->blocksize(), 10);
    TS_ASSERT_EQUALS(mws->readX(1)[10], 10.0);
    TS_ASSERT_EQUALS(mws->readY(1)[5], 2.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     20.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  void test_WorkspaceGroup_Replace_Rebin_None() {
    WorkspaceGroup_sptr ws = doExec<WorkspaceGroup>(
        "Replace", "Rebin", "Params=0,2,8", "", "", false,
        ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(mws->blocksize(), 4);
    TS_ASSERT_EQUALS(mws->readX(1)[4], 8.0);
    TS_ASSERT_EQUALS(mws->readY(1)[3], 4.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     16.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  void test_WorkspaceGroup_Replace_None_Rebin() {
    WorkspaceGroup_sptr ws = doExec<WorkspaceGroup>(
        "Replace", "", "", "Rebin", "Params=0,2,8", false,
        ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(mws->blocksize(), 4);
    TS_ASSERT_EQUALS(mws->readX(1)[4], 8.0);
    TS_ASSERT_EQUALS(mws->readY(1)[3], 4.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     16.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  /** Handle WorkspaceGroups returned by the listener */
  void test_WorkspaceGroup_Add_None_None() {
    doExec<WorkspaceGroup>("Add", "", "", "", "", false,
                           ILiveListener_sptr(new TestGroupDataListener));
    WorkspaceGroup_sptr ws =
        doExec<WorkspaceGroup>("Add", "", "", "", "", false,
                               ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(mws->blocksize(), 10);
    TS_ASSERT_EQUALS(mws->readX(1)[10], 10.0);
    TS_ASSERT_EQUALS(mws->readY(1)[5], 4.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     40.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  void test_WorkspaceGroup_Add_Rebin_None() {
    doExec<WorkspaceGroup>("Add", "Rebin", "Params=0,2,8", "", "", false,
                           ILiveListener_sptr(new TestGroupDataListener));
    WorkspaceGroup_sptr ws =
        doExec<WorkspaceGroup>("Add", "Rebin", "Params=0,2,8", "", "", false,
                               ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(mws->blocksize(), 4);
    TS_ASSERT_EQUALS(mws->readX(1)[4], 8.0);
    TS_ASSERT_EQUALS(mws->readY(1)[3], 8.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     32.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  void test_WorkspaceGroup_Add_None_Rebin() {
    doExec<WorkspaceGroup>("Add", "", "", "Rebin", "Params=0,2,8", false,
                           ILiveListener_sptr(new TestGroupDataListener));
    WorkspaceGroup_sptr ws =
        doExec<WorkspaceGroup>("Add", "", "", "Rebin", "Params=0,2,8", false,
                               ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(mws->blocksize(), 4);
    TS_ASSERT_EQUALS(mws->readX(1)[4], 8.0);
    TS_ASSERT_EQUALS(mws->readY(1)[3], 8.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     32.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  /** Handle WorkspaceGroups returned by the listener */
  void test_WorkspaceGroup_Append_None_None() {
    doExec<WorkspaceGroup>("Append", "", "", "", "", false,
                           ILiveListener_sptr(new TestGroupDataListener));
    WorkspaceGroup_sptr ws =
        doExec<WorkspaceGroup>("Append", "", "", "", "", false,
                               ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(mws->blocksize(), 10);
    TS_ASSERT_EQUALS(mws->readX(1)[10], 10.0);
    TS_ASSERT_EQUALS(mws->readY(1)[5], 2.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     20.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  void test_WorkspaceGroup_Append_Rebin_None() {
    doExec<WorkspaceGroup>("Append", "Rebin", "Params=0,2,8", "", "", false,
                           ILiveListener_sptr(new TestGroupDataListener));
    WorkspaceGroup_sptr ws =
        doExec<WorkspaceGroup>("Append", "Rebin", "Params=0,2,8", "", "", false,
                               ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(mws->blocksize(), 4);
    TS_ASSERT_EQUALS(mws->readX(1)[4], 8.0);
    TS_ASSERT_EQUALS(mws->readY(1)[3], 4.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     16.0);
    AnalysisDataService::Instance().clear();
  }
  //--------------------------------------------------------------------------------------------
  void test_WorkspaceGroup_Append_None_Rebin() {
    doExec<WorkspaceGroup>("Append", "", "", "Rebin", "Params=0,2,8", false,
                           ILiveListener_sptr(new TestGroupDataListener));
    WorkspaceGroup_sptr ws =
        doExec<WorkspaceGroup>("Append", "", "", "Rebin", "Params=0,2,8", false,
                               ILiveListener_sptr(new TestGroupDataListener));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(ws->getName(), "fake");
    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("fake_2");
    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(mws->blocksize(), 4);
    TS_ASSERT_EQUALS(mws->readX(1)[4], 8.0);
    TS_ASSERT_EQUALS(mws->readY(1)[3], 4.0);
    TS_ASSERT_EQUALS(std::accumulate(mws->readY(1).begin(), mws->readY(1).end(),
                                     0.0, std::plus<double>()),
                     16.0);
    AnalysisDataService::Instance().clear();
  }
};

#endif /* MANTID_LIVEDATA_LOADLIVEDATATEST_H_ */
