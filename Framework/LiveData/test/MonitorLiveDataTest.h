#ifndef MANTID_LIVEDATA_MONITORLIVEDATATEST_H_
#define MANTID_LIVEDATA_MONITORLIVEDATATEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidLiveData/MonitorLiveData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Strings.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/FacilityHelper.h"

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>

using namespace Mantid;
using namespace Mantid::LiveData;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class MonitorLiveDataTest : public CxxTest::TestSuite {
private:
  FacilityHelper::ScopedFacilities loadTESTFacility;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MonitorLiveDataTest *createSuite() {
    return new MonitorLiveDataTest();
  }
  static void destroySuite(MonitorLiveDataTest *suite) { delete suite; }

  MonitorLiveDataTest()
      : loadTESTFacility("IDFs_for_UNIT_TESTING/UnitTestFacilities.xml",
                         "TEST") {}

  void setUp() {
    // Register algorithms
    FrameworkManager::Instance();
    AnalysisDataService::Instance().clear();
    ConfigService::Instance().setString("testdatalistener.reset_after", "0");
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter",
                                        "0");
    ConfigService::Instance().setString("testdatalistener.m_newStatus", "0");
  }

  void test_Init() {
    MonitorLiveData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /** Create but don't start a MonitorLiveData thread */
  boost::shared_ptr<MonitorLiveData>
  makeAlgo(std::string output, std::string accumWS = "",
           std::string AccumulationMethod = "Replace",
           std::string RunTransitionBehavior = "Restart",
           std::string UpdateEvery = "1") {
    auto alg = boost::dynamic_pointer_cast<MonitorLiveData>(
        AlgorithmManager::Instance().create("MonitorLiveData", -1, false));
    alg->setPropertyValue("Instrument", "TestDataListener");
    alg->setPropertyValue("UpdateEvery", UpdateEvery);
    alg->setPropertyValue("AccumulationMethod", AccumulationMethod);
    alg->setPropertyValue("AccumulationWorkspace", accumWS);
    alg->setProperty("PreserveEvents", true);
    alg->setPropertyValue("RunTransitionBehavior", RunTransitionBehavior);
    alg->setPropertyValue("OutputWorkspace", output);
    return alg;
  }

  /** Disallow if you detect another MonitorLiveData thread with the same */
  void test_DontAllowTwoAlgorithmsWithSameOutput() {
    IAlgorithm_sptr alg1 = makeAlgo("fake1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    while (!alg1->isRunning()) {
      Poco::Thread::sleep(10); // give it some time to start
    }

    // This algorithm dies because another thread has the same output
    boost::shared_ptr<MonitorLiveData> alg2 = makeAlgo("fake1");
    TSM_ASSERT("validateInputs should complaing (return a non-empty map)",
               !alg2->validateInputs().empty());

    // Abort the thread.
    alg1->cancel();
    res1.wait(10000);
  }

  /** Disallow if you detect another MonitorLiveData thread with the
   * AccumulationWorkspace name */
  void test_DontAllowTwoAlgorithmsWithSameAccumulationWorkspace() {
    IAlgorithm_sptr alg1 = makeAlgo("fake1", "accum1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    while (!alg1->isRunning()) {
      Poco::Thread::sleep(10); // give it some time to start
    }

    // This algorithm dies because another thread has the same output
    boost::shared_ptr<MonitorLiveData> alg2 = makeAlgo("fake2", "accum1");
    TSM_ASSERT("validateInputs should complaing (return a non-empty map)",
               !alg2->validateInputs().empty());

    // Abort the thread.
    alg1->cancel();
    res1.wait(10000);
  }

  /** Disallow if you detect another MonitorLiveData thread with the same */
  void test_Allow_AnotherAlgo_IfTheOtherIsFinished() {
    // Start and stop one algorithm
    IAlgorithm_sptr alg1 = makeAlgo("fake1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    while (!alg1->isRunning()) {
      Poco::Thread::sleep(10); // give it some time to start
    }
    alg1->cancel();
    res1.wait(10000);

    // This algorithm if OK because the other is not still running
    IAlgorithm_sptr alg2 = makeAlgo("fake1");
    TSM_ASSERT("validateInputs should give the all clear (an empty map)",
               alg2->validateInputs().empty());
  }

  //--------------------------------------------------------------------------------------------
  /** Stop live data if RunTransitionBehavior="Stop" */
  void test_RunTransitionBehavior_Stop() {
    // Will reset after the 2nd call to extract data
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter",
                                        "3");
    ConfigService::Instance().setString("testdatalistener.m_newStatus",
                                        "4" /* ILiveListener::EndRun */);

    // Run this algorithm
    IAlgorithm_sptr alg1 = makeAlgo("fake1", "", "Add", "Stop", "0.1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    TS_ASSERT_THROWS_NOTHING(res1.wait(6000));

    TSM_ASSERT("The algorithm should have exited by itself.",
               !alg1->isRunning());
    TSM_ASSERT("The algorithm should have exited by itself.",
               alg1->isExecuted());

    // Manually stop it so the test finishes, in case of failure
    if (alg1->isRunning()) {
      alg1->cancel();
      res1.wait(1000);
      return;
    }

    // The workspace stopped after 3 additions.
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake1");
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 3 * 200);
  }

  //--------------------------------------------------------------------------------------------
  /** Executes the given algorithm asynchronously, until you reach the given
   * chunk number.
     * @return false if test failed*/
  bool runAlgoUntilChunk(boost::shared_ptr<MonitorLiveData> alg1,
                         size_t stopAtChunk) {
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    Poco::Thread::sleep(50);
    while (alg1->m_chunkNumber < stopAtChunk)
      Poco::Thread::sleep(10);
    return true;
  }

  //--------------------------------------------------------------------------------------------
  /** Clear the accumulated data when a run ends if
   * RunTransitionBehavior="Restart" */
  void test_RunTransitionBehavior_Restart() {
    // Will reset after the 2nd call to extract data
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter",
                                        "4");
    ConfigService::Instance().setString("testdatalistener.m_newStatus",
                                        "4" /* ILiveListener::EndRun */);

    boost::shared_ptr<MonitorLiveData> alg1 =
        makeAlgo("fake1", "", "Add", "Restart", "0.05");
    // Run this algorithm until that chunk #
    if (!runAlgoUntilChunk(alg1, 5))
      return;

    // Cancel the algo before exiting test (avoids segfault)
    alg1->cancel();

    // The workspace was reset after 4 additions, and then got 3 more
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake1");
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);

    Kernel::Timer timer;
    while (alg1->isRunning() && timer.elapsed_no_reset() < 0.5) {
    }
  }

  //--------------------------------------------------------------------------------------------
  /** Keep the old accumulated data when a run ends if
   * RunTransitionBehavior="Rename" */
  void test_RunTransitionBehavior_Rename() {
    // Will reset after the 2nd call to extract data
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter",
                                        "4");
    ConfigService::Instance().setString("testdatalistener.m_newStatus",
                                        "4" /* ILiveListener::EndRun */);

    boost::shared_ptr<MonitorLiveData> alg1 =
        makeAlgo("fake2", "", "Add", "Rename", "0.05");
    // Run this algorithm until that chunk #
    if (!runAlgoUntilChunk(alg1, 5))
      return;

    // Cancel the algo before exiting test (avoids segfault)
    alg1->cancel();

    // The first workspace got cloned to a new name (the suffix is set in the
    // TestDataListener)
    EventWorkspace_const_sptr ws1 =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake2_999");
    TS_ASSERT_EQUALS(ws1->getNumberEvents(), 4 * 200);
    // Make sure the monitor workspace is present and correct
    TS_ASSERT(ws1->monitorWorkspace());
    TS_ASSERT_EQUALS(ws1->monitorWorkspace()->readY(0)[0], 4);

    // And this is the current run
    EventWorkspace_sptr ws2 =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake2");
    TS_ASSERT_EQUALS(ws2->getNumberEvents(), 200);
    // Make sure the monitor workspace is present and correct
    TS_ASSERT(ws2->monitorWorkspace());
    TS_ASSERT_EQUALS(ws2->monitorWorkspace()->readY(0)[0], 1);

    Kernel::Timer timer;
    while (alg1->isRunning() && timer.elapsed_no_reset() < 0.5) {
    }
  }
};

#endif /* MANTID_LIVEDATA_MONITORLIVEDATATEST_H_ */
