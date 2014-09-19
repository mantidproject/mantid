#ifndef MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDSTEST_H_
#define MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiIndexKnownCompounds.h"

using Mantid::Poldi::PoldiIndexKnownCompounds;
using namespace Mantid::API;

class PoldiIndexKnownCompoundsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiIndexKnownCompoundsTest *createSuite() { return new PoldiIndexKnownCompoundsTest(); }
  static void destroySuite( PoldiIndexKnownCompoundsTest *suite ) { delete suite; }


  void test_Init()
  {
    PoldiIndexKnownCompounds alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("PoldiIndexKnownCompoundsTest_OutputWS");

    PoldiIndexKnownCompounds alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // TODO: Check the results

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDSTEST_H_ */
