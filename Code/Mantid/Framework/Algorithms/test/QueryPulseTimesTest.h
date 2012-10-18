#ifndef MANTID_ALGORITHMS_QUERYPULSETIMESTEST_H_
#define MANTID_ALGORITHMS_QUERYPULSETIMESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/QueryPulseTimes.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::Algorithms::QueryPulseTimes;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class QueryPulseTimesTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryPulseTimesTest *createSuite() { return new QueryPulseTimesTest(); }
  static void destroySuite( QueryPulseTimesTest *suite ) { delete suite; }


  void test_Init()
  {
    QueryPulseTimes alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  /*
  Test that the input workspace must be an event workspace, other types of matrix workspace will not do.
  */
  void test_input_workspace2D_throws()
  {
    using Mantid::DataObjects::Workspace2D;
    Workspace_sptr workspace2D = boost::make_shared<Workspace2D>();

    QueryPulseTimes alg;
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", workspace2D), std::invalid_argument);
  }

  

  


  //
  //void test_exec()
  //{
  //  using Mantid::API::AnalysisDataService;

  //  // Name of the output workspace.
  //  std::string outWSName("QueryPulseTimesTest_OutputWS");
  //
  //  QueryPulseTimes alg;
  //  TS_ASSERT_THROWS_NOTHING( alg.initialize() )
  //  TS_ASSERT( alg.isInitialized() )
  //  TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
  //  TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
  //  TS_ASSERT_THROWS_NOTHING( alg.execute(); );
  //  TS_ASSERT( alg.isExecuted() );
  //  
  //  // Retrieve the workspace from data service. TODO: Change to your desired type
  //  Workspace_sptr ws;
  //  
  //  TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
  //  TS_ASSERT(ws);
  //  if (!ws) return;
  //  
  //  // TODO: Check the results
  //  
  //  // Remove workspace from the data service.
  //  AnalysisDataService::Instance().remove(outWSName);
  //}
  //
  //void test_Something()
  //{
  //  TSM_ASSERT( "You forgot to write a test!", 0);
  //}


};


#endif /* MANTID_ALGORITHMS_QUERYPULSETIMESTEST_H_ */