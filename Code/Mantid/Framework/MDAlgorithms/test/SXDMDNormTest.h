#ifndef MANTID_MDALGORITHMS_SXDMDNORMTEST_H_
#define MANTID_MDALGORITHMS_SXDMDNORMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/SXDMDNorm.h"

using Mantid::MDAlgorithms::SXDMDNorm;

class SXDMDNormTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SXDMDNormTest *createSuite() { return new SXDMDNormTest(); }
  static void destroySuite( SXDMDNormTest *suite ) { delete suite; }


  void test_Init()
  {
    SXDMDNorm alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
/*
    // Name of the output workspace.
    std::string outWSName("SXDMDNormTest_OutputWS");
  
    SXDMDNorm alg;
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
*/
  }
  



};


#endif /* MANTID_MDALGORITHMS_SXDMDNORMTEST_H_ */
