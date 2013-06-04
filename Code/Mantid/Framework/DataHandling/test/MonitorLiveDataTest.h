#ifndef MANTID_DATAHANDLING_MONITORLIVEDATATEST_H_
#define MANTID_DATAHANDLING_MONITORLIVEDATATEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/MonitorLiveData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Strings.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class MonitorLiveDataTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MonitorLiveDataTest *createSuite() { return new MonitorLiveDataTest(); }
  static void destroySuite( MonitorLiveDataTest *suite ) { delete suite; }

  void setUp()
  {
    // Register algorithms
    FrameworkManager::Instance();
    AnalysisDataService::Instance().clear();
    ConfigService::Instance().setString("testdatalistener.reset_after", "0");
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter", "0");
    ConfigService::Instance().setString("testdatalistener.m_newStatus", "0");
  }


  void test_Init()
  {
    MonitorLiveData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  
  /** Create but don't start a MonitorLiveData thread */
  boost::shared_ptr<MonitorLiveData> makeAlgo(std::string output, std::string accumWS="",
      std::string AccumulationMethod="Replace",
      std::string EndRunBehavior="Restart", std::string UpdateEvery="1")
  {
    auto alg = boost::dynamic_pointer_cast<MonitorLiveData>(AlgorithmManager::Instance().create("MonitorLiveData", -1, false));
    alg->setPropertyValue("Instrument", "TestDataListener");
    alg->setPropertyValue("UpdateEvery", UpdateEvery);
    alg->setPropertyValue("AccumulationMethod", AccumulationMethod);
    alg->setPropertyValue("AccumulationWorkspace", accumWS);
    alg->setProperty("PreserveEvents", true);
    alg->setPropertyValue("EndRunBehavior", EndRunBehavior);
    alg->setPropertyValue("OutputWorkspace", output);
    return alg;
  }
  
  /** Create and run the algorithm asynchronously */
  void test_runAsync()
  {
    IAlgorithm_sptr alg = makeAlgo("fake1");
    Poco::ActiveResult<bool> res1 = alg->executeAsync();
    Poco::Thread::sleep(100); // give it some time to start

    // Abort the thread.
    alg->cancel();
    res1.wait(10000);
    //TS_ASSERT( AnalysisDataService::Instance().doesExist("fake1") );
  }

  /** Disallow if you detect another MonitorLiveData thread with the same */
  void test_DontAllowTwoAlgorithmsWithSameOutput()
  {
    IAlgorithm_sptr alg1 = makeAlgo("fake1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    while ( !alg1->isRunning() )
    {
      Poco::Thread::sleep(10); // give it some time to start
    }

    // This algorithm dies because another thread has the same output
    boost::shared_ptr<MonitorLiveData> alg2 = makeAlgo("fake1");
    TSM_ASSERT("validateInputs should complaing (return a non-empty map)", ! alg2->validateInputs().empty() );

    // Abort the thread.
    alg1->cancel();
    res1.wait(10000);
  }

  /** Disallow if you detect another MonitorLiveData thread with the AccumulationWorkspace name */
  void test_DontAllowTwoAlgorithmsWithSameAccumulationWorkspace()
  {
    IAlgorithm_sptr alg1 = makeAlgo("fake1", "accum1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    while ( !alg1->isRunning() )
    {
      Poco::Thread::sleep(10); // give it some time to start
    }

    // This algorithm dies because another thread has the same output
    boost::shared_ptr<MonitorLiveData> alg2 = makeAlgo("fake2", "accum1");
    TSM_ASSERT("validateInputs should complaing (return a non-empty map)", ! alg2->validateInputs().empty() );

    // Abort the thread.
    alg1->cancel();
    res1.wait(10000);
  }

  /** Disallow if you detect another MonitorLiveData thread with the same */
  void test_Allow_AnotherAlgo_IfTheOtherIsFinished()
  {
    // Start and stop one algorithm
    IAlgorithm_sptr alg1 = makeAlgo("fake1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    while ( !alg1->isRunning() )
    {
      Poco::Thread::sleep(10); // give it some time to start
    }
    alg1->cancel();
    res1.wait(10000);

    // This algorithm if OK because the other is not still running
    IAlgorithm_sptr alg2 = makeAlgo("fake1");
    TSM_ASSERT("validateInputs should give the all clear (an empty map)", alg2->validateInputs().empty() );
  }


  //--------------------------------------------------------------------------------------------
  /** Stop live data if EndRunBehavior="Stop" */
  void test_EndRunBehavior_Stop()
  {
    // Will reset after the 2nd call to extract data
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter", "3");
    ConfigService::Instance().setString("testdatalistener.m_newStatus", "4" /* ILiveListener::EndRun */);

    // Run this algorithm
    IAlgorithm_sptr alg1 = makeAlgo("fake1", "", "Add", "Stop", "0.1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    TS_ASSERT_THROWS_NOTHING( res1.wait(6000) );

    TSM_ASSERT("The algorithm should have exited by itself.", !alg1->isRunning() );
    TSM_ASSERT("The algorithm should have exited by itself.", alg1->isExecuted() );

    // Manually stop it so the test finishes, in case of failure
    if (alg1->isRunning())
    { alg1->cancel(); res1.wait(1000); return; }

    // The workspace stopped after 3 additions.
    EventWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake1");
    TS_ASSERT_EQUALS( ws->getNumberEvents(), 3*200);
  }



  //--------------------------------------------------------------------------------------------
/** Executes the given algorithm asynchronously, until you reach the given chunk number.
   * @return false if test failed*/
  bool runAlgoUntilChunk(boost::shared_ptr<MonitorLiveData> alg1, size_t stopAtChunk)
  {
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    Poco::Thread::sleep(50);
    while (alg1->m_chunkNumber < stopAtChunk)
      Poco::Thread::sleep(10);
    return true;
  }

  //--------------------------------------------------------------------------------------------
  /** Clear the accumulated data when a run ends if EndRunBehavior="Restart" */
  void test_EndRunBehavior_Restart()
  {
    // Will reset after the 2nd call to extract data
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter", "4");
    ConfigService::Instance().setString("testdatalistener.m_newStatus", "4" /* ILiveListener::EndRun */);

    boost::shared_ptr<MonitorLiveData> alg1 = makeAlgo("fake1", "", "Add", "Restart", "0.15");
    // Run this algorithm until that chunk #
    if (!runAlgoUntilChunk(alg1, 7)) return;

    // Cancel the algo before exiting test (avoids segfault)
    alg1->cancel();

    // The workspace was reset after 4 additions, and then got 3 more
    EventWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake1");
    TS_ASSERT_EQUALS( ws->getNumberEvents(), 3*200);

    Poco::Thread::sleep(500);
  }


  //--------------------------------------------------------------------------------------------
  /** Keep the old accumulated data when a run ends if EndRunBehavior="Rename" */
  void test_EndRunBehavior_Rename()
  {
    // Will reset after the 2nd call to extract data
    ConfigService::Instance().setString("testdatalistener.m_changeStatusAfter", "4");
    ConfigService::Instance().setString("testdatalistener.m_newStatus", "4" /* ILiveListener::EndRun */);

    boost::shared_ptr<MonitorLiveData> alg1 = makeAlgo("fake1", "", "Add", "Rename", "0.15");
    // Run this algorithm until that chunk #
    if (!runAlgoUntilChunk(alg1, 7)) return;

    // Cancel the algo before exiting test (avoids segfault)
    alg1->cancel();

    // The first workspace got cloned to a new name
    EventWorkspace_sptr ws1 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake1_0");
    TS_ASSERT_EQUALS( ws1->getNumberEvents(), 4*200);

    // And this is the current run
    EventWorkspace_sptr ws2 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake1");
    TS_ASSERT_EQUALS( ws2->getNumberEvents(), 3*200);

    Poco::Thread::sleep(500);
  }


};


#endif /* MANTID_DATAHANDLING_MONITORLIVEDATATEST_H_ */
