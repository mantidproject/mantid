#ifndef MANTID_DATAHANDLING_LOADLIVEDATATEST_H_
#define MANTID_DATAHANDLING_LOADLIVEDATATEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadLiveData.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class LoadLiveDataTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadLiveDataTest *createSuite() { return new LoadLiveDataTest(); }
  static void destroySuite( LoadLiveDataTest *suite ) { delete suite; }

  void setUp()
  {
    FrameworkManager::Instance();
    if (AnalysisDataService::Instance().doesExist("fake"))
      AnalysisDataService::Instance().remove("fake");
  }

  void test_Init()
  {
    LoadLiveData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  //--------------------------------------------------------------------------------------------
  /** Run a test with a fake output, no processing
   *
   * @param AccumulationMethod :: parameter string
   * @return the created processed WS
   */
  EventWorkspace_sptr doExecEvent(std::string AccumulationMethod)
  {
    LoadLiveData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Instrument", "FakeEventDataListener") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "fake") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("AccumulationMethod", AccumulationMethod) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake") );
    TS_ASSERT(ws);
    return ws;

  }

  //--------------------------------------------------------------------------------------------
  void test_conjoin()
  {
    EventWorkspace_sptr ws;

    // First go creates the fake ws
    ws = doExecEvent("Conjoin");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);

    // Next one actually conjoins
    ws = doExecEvent("Conjoin");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 4);
  }
  

};


#endif /* MANTID_DATAHANDLING_LOADLIVEDATATEST_H_ */
