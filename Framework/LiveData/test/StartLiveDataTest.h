// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/FacilityHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Timer.h"
#include "MantidLiveData/StartLiveData.h"
#include <cxxtest/TestSuite.h>

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>

using namespace Mantid;
using namespace Mantid::LiveData;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class StartLiveDataTest : public CxxTest::TestSuite {
private:
  FacilityHelper::ScopedFacilities loadTESTFacility;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StartLiveDataTest *createSuite() { return new StartLiveDataTest(); }
  static void destroySuite(StartLiveDataTest *suite) { delete suite; }

  StartLiveDataTest() : loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST") {
    FrameworkManager::Instance();
  }

  ~StartLiveDataTest() {
    AlgorithmManager::Instance().cancelAll();
    Poco::Thread::sleep(100);
  }

  void test_Init() {
    StartLiveData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //--------------------------------------------------------------------------------------------
  /** Run a test with a fake output, no processing
   *
   * @param AccumulationMethod :: parameter string
   * @return the created processed WS
   */
  std::tuple<std::shared_ptr<StartLiveData>, EventWorkspace_sptr>
  doExecEvent(const std::string &AccumulationMethod, double UpdateEvery, const std::string &ProcessingAlgorithm = "",
              const std::string &ProcessingProperties = "", const std::string &PostProcessingAlgorithm = "",
              const std::string &PostProcessingProperties = "") {
    auto alg = std::make_shared<StartLiveData>();
    TS_ASSERT_THROWS_NOTHING(alg->initialize())
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("FromNow", "1"));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("UpdateEvery", UpdateEvery));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Instrument", "TestDataListener"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", "fake"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AccumulationMethod", AccumulationMethod));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("ProcessingAlgorithm", ProcessingAlgorithm));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("ProcessingProperties", ProcessingProperties));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("PostProcessingAlgorithm", PostProcessingAlgorithm));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("PostProcessingProperties", PostProcessingProperties));
    if (!PostProcessingAlgorithm.empty())
      TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("AccumulationWorkspace", "fake_accum"));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("PreserveEvents", true));
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    // Retrieve the workspace from data service.
    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake"));
    TS_ASSERT(ws);
    return std::make_tuple(alg, ws);
  }

  //--------------------------------------------------------------------------------------------
  /** StartLiveData and run LoadLiveData only once (UpdateEvery=0)
   * This checks that the properties are copied to LoadLiveData */
  void test_startOnce() {
    const auto [alg, ws] = doExecEvent("Replace", 0, "", "", "Rebin", "Params=40e3, 1e3, 60e3");
    UNUSED_ARG(alg);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    // Check that rebin was called
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
  }

  //--------------------------------------------------------------------------------------------
  /** If the OutputWorkspace exists (from a previous run),
   * and you select "Add", it still REPLACES the input on the very first run.
   */
  void test_FirstCallReplacesTheOutputWorkspace() {
    // Make an existing output workspace "fake" that should be overwritten
    AnalysisDataService::Instance().addOrReplace("fake", WorkspaceCreationHelper::create2DWorkspace(23, 12));

    const auto [alg, ws] = doExecEvent("Add", 0);
    UNUSED_ARG(alg);

    // The "fake" workspace was replaced.
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
  }

  //--------------------------------------------------------------------------------------------
  /** Start and keep MonitorLiveData running */
  void test_start_andKeepRunning() {
    AlgorithmManager::Instance().cancelAll();
    AlgorithmManager::Instance().clear();
    const auto [alg, ws] = doExecEvent("Replace", 1);

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);

    // The MonitorLiveData algorithm is left running in the manager
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 1);
    // Get at it via the StartLiveData output property of the same name
    IAlgorithm_sptr monAlg = alg->getProperty("MonitorLiveData");
    TS_ASSERT_EQUALS(monAlg->name(), "MonitorLiveData");

    // Wait up to 2 seconds for the algorithm to report that it is running.
    Timer tim;
    while (!monAlg->isRunning()) {
      // Wait
      Poco::Thread::sleep(1);
      if (tim.elapsed(false) > 2.0) {
        // Fail if it never starts
        TS_ASSERT(0);
        break;
      }
    }
    // Cancel and wait for it to be cancelled
    monAlg->cancel();
    Poco::Thread::sleep(100);
  }
};
