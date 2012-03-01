#ifndef MANTID_DATAHANDLING_LOADLIVEDATATEST_H_
#define MANTID_DATAHANDLING_LOADLIVEDATATEST_H_

#include "MantidDataHandling/LoadLiveData.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

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
    AnalysisDataService::Instance().clear();
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
  EventWorkspace_sptr doExecEvent(std::string AccumulationMethod,
      std::string ProcessingAlgorithm = "",
      std::string ProcessingProperties = "",
      std::string PostProcessingAlgorithm = "",
      std::string PostProcessingProperties = ""
      )
  {
    LoadLiveData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Instrument", "TestDataListener") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("AccumulationMethod", AccumulationMethod) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ProcessingAlgorithm", ProcessingAlgorithm) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ProcessingProperties", ProcessingProperties) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PostProcessingAlgorithm", PostProcessingAlgorithm) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PostProcessingProperties", PostProcessingProperties) );
    if (!PostProcessingAlgorithm.empty())
      TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("AccumulationWorkspace", "fake_accum") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "fake") );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake") );
    TS_ASSERT(ws);
    return ws;
  }

  //--------------------------------------------------------------------------------------------
  void test_replace()
  {
    EventWorkspace_sptr ws1, ws2;

    ws1 = doExecEvent("Replace");
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws1->getNumberEvents(), 200);

    ws2 = doExecEvent("Replace");
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws2->getNumberEvents(), 200);
    TSM_ASSERT( "Workspace changed when replaced", ws1 != ws2 );
  }

  //--------------------------------------------------------------------------------------------
  void test_append()
  {
    EventWorkspace_sptr ws1, ws2;

    // First go creates the fake ws
    ws1 = doExecEvent("Append");
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);

    // Next one actually conjoins
    ws2 = doExecEvent("Append");
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 4);
  }

  //--------------------------------------------------------------------------------------------
  void test_add()
  {
    EventWorkspace_sptr ws1, ws2;

    // First go creates the fake ws
    ws1 = doExecEvent("Add");
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws1->getNumberEvents(), 200);

    // Next one adds events, keeps # of histos the same
    ws2 = doExecEvent("Add");
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws2->getNumberEvents(), 400);

    TSM_ASSERT( "Workspace being added stayed the same pointer", ws1 == ws2 );
  }
  

  //--------------------------------------------------------------------------------------------
  /** Simple processing of a chunk */
  void test_processChunk()
  {
    EventWorkspace_sptr ws;
    ws = doExecEvent("Replace", "Rebin", "Params=40e3, 1e3, 60e3");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    // Check that rebin was called
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
  }

  //--------------------------------------------------------------------------------------------
  /** Do PostProcessing */
  void test_PostProcessing()
  {
    // No chunk processing, but PostProcessing
    EventWorkspace_sptr ws = doExecEvent("Replace", "", "", "Rebin", "Params=40e3, 1e3, 60e3");
    EventWorkspace_sptr ws_accum = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake_accum");
    TS_ASSERT( ws )
    TS_ASSERT( ws_accum )

    // The accumulated workspace: it was NOT rebinned.
    TS_ASSERT_EQUALS(ws_accum->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws_accum->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws_accum->blocksize(), 1);

    // The post-processed workspace: Check that rebin was called
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
  }

  //--------------------------------------------------------------------------------------------
  /** Perform both chunk and post-processing*/
  void test_Chunk_and_PostProcessing()
  {
    // Process both times
    EventWorkspace_sptr ws = doExecEvent("Replace", "Rebin", "Params=20e3, 1e3, 60e3", "Rebin", "Params=40e3, 1e3, 60e3");
    EventWorkspace_sptr ws_accum = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("fake_accum");
    TS_ASSERT( ws )
    TS_ASSERT( ws_accum )

    // The accumulated workspace: it was rebinned starting at 20e3
    TS_ASSERT_EQUALS(ws_accum->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws_accum->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws_accum->blocksize(), 40);
    TS_ASSERT_DELTA(ws_accum->dataX(0)[0], 20e3, 1e-4);

    // The post-processed workspace was rebinned starting at 40e3
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT_DELTA(ws->dataX(0)[0], 40e3, 1e-4);
  }

};


#endif /* MANTID_DATAHANDLING_LOADLIVEDATATEST_H_ */
