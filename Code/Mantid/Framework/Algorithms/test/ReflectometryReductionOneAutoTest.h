#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryReductionOneAuto.h"

using Mantid::Algorithms::ReflectometryReductionOneAuto;
using namespace Mantid::API;

class ReflectometryReductionOneAutoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneAutoTest *createSuite() { return new ReflectometryReductionOneAutoTest(); }
  static void destroySuite( ReflectometryReductionOneAutoTest *suite ) { delete suite; }


  void test_Init()
  {
    ReflectometryReductionOneAuto alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("ReflectometryReductionOneAutoTest_OutputWS");
  
    ReflectometryReductionOneAuto alg;
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


#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_ */