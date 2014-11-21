#ifndef MANTID_MDALGORITHMS_CALCULATECOVERAGEDGSTEST_H_
#define MANTID_MDALGORITHMS_CALCULATECOVERAGEDGSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/CalculateCoverageDGS.h"

using Mantid::MDAlgorithms::CalculateCoverageDGS;
using namespace Mantid::API;

class CalculateCoverageDGSTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateCoverageDGSTest *createSuite() { return new CalculateCoverageDGSTest(); }
  static void destroySuite( CalculateCoverageDGSTest *suite ) { delete suite; }


  void test_Init()
  {
    CalculateCoverageDGS alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("CalculateCoverageDGSTest_OutputWS");

    CalculateCoverageDGS alg;
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


#endif /* MANTID_MDALGORITHMS_CALCULATECOVERAGEDGSTEST_H_ */