#ifndef MANTID_DATAHANDLING_MONITORLIVEDATATEST_H_
#define MANTID_DATAHANDLING_MONITORLIVEDATATEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/MonitorLiveData.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class MonitorLiveDataTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MonitorLiveDataTest *createSuite() { return new MonitorLiveDataTest(); }
  static void destroySuite( MonitorLiveDataTest *suite ) { delete suite; }


  void test_Init()
  {
    MonitorLiveData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  
  /** Create but don't start a MonitorLiveData thread */
  IAlgorithm_sptr makeAlgo(std::string output, std::string accum="")
  {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("MonitorLiveData");
    alg->setPropertyValue("Instrument", "TestDataListener");
    alg->setPropertyValue("UpdateEvery", "1");
    alg->setPropertyValue("AccumulationMethod", "Replace");
    alg->setPropertyValue("AccumulationWorkspace", accum);
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
    Poco::Thread::sleep(100); // give it some time to start

    // This algorithm dies because another thread has the same output
    IAlgorithm_sptr alg2 = makeAlgo("fake1");
    TS_ASSERT_THROWS_ANYTHING( alg2->execute(); );
    TS_ASSERT( !alg2->isExecuted() );

    // Abort the thread.
    alg1->cancel();
    res1.wait(10000);
  }

  /** Disallow if you detect another MonitorLiveData thread with the AccumulationWorkspace name */
  void test_DontAllowTwoAlgorithmsWithSameAccumulationWorkspace()
  {
    IAlgorithm_sptr alg1 = makeAlgo("fake1", "accum1");
    Poco::ActiveResult<bool> res1 = alg1->executeAsync();
    Poco::Thread::sleep(100); // give it some time to start

    // This algorithm dies because another thread has the same output
    IAlgorithm_sptr alg2 = makeAlgo("fake2", "accum1");
    TS_ASSERT_THROWS_ANYTHING( alg2->execute() );
    TS_ASSERT( !alg2->isExecuted() );

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
    Poco::Thread::sleep(100); // give it some time to start
    alg1->cancel();
    res1.wait(10000);

    // This algorithm if OK because the other is not still running
    IAlgorithm_sptr alg2 = makeAlgo("fake1");
    Poco::ActiveResult<bool> res2 = alg2->executeAsync();
    Poco::Thread::sleep(100); // give it some time to start
    TS_ASSERT( alg2->isRunning() );
    alg2->cancel();
    res2.wait(10000);
  }


};


#endif /* MANTID_DATAHANDLING_MONITORLIVEDATATEST_H_ */
