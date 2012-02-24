#ifndef MANTID_DATAHANDLING_STARTLIVEDATATEST_H_
#define MANTID_DATAHANDLING_STARTLIVEDATATEST_H_

#include "MantidDataHandling/StartLiveData.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidKernel/SingletonHolder.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class StartLiveDataTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StartLiveDataTest *createSuite() { return new StartLiveDataTest(); }
  static void destroySuite( StartLiveDataTest *suite ) { delete suite; }


  void test_Init()
  {
    StartLiveData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  //--------------------------------------------------------------------------------------------
  /** Run a test with a fake output, no processing
   *
   * @param AccumulationMethod :: parameter string
   * @return the created processed WS
   */
  EventWorkspace_sptr doExecEvent(std::string AccumulationMethod,
      double UpdateEvery,
      std::string ProcessingAlgorithm = "",
      std::string ProcessingProperties = "")
  {
    StartLiveData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("FromNow", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UpdateEvery", UpdateEvery) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Instrument", "TestDataListener") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "fake") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("AccumulationMethod", AccumulationMethod) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ProcessingAlgorithm", ProcessingAlgorithm) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ProcessingProperties", ProcessingProperties) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake") );
    TS_ASSERT(ws);
    return ws;
  }


  //--------------------------------------------------------------------------------------------
  /** StartLiveData and run LoadLiveData only once.
   * This checks that the properties are copied to LoadLiveData */
  void test_start_with_processChunk()
  {
    // Declare all algorithms, e.g. Rebin
    FrameworkManager::Instance();
    EventWorkspace_sptr ws;
    ws = doExecEvent("Replace", 0, "Rebin", "Params=40e3, 1e3, 60e3");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    // Check that rebin was called
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
  }

};


#endif /* MANTID_DATAHANDLING_STARTLIVEDATATEST_H_ */
