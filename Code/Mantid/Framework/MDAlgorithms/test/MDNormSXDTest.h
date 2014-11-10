#ifndef MANTID_MDALGORITHMS_MDNORMSXDTEST_H_
#define MANTID_MDALGORITHMS_MDNORMSXDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/MDNormSXD.h"

using Mantid::MDAlgorithms::MDNormSXD;
using namespace Mantid::API;

class MDNormSXDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDNormSXDTest *createSuite() { return new MDNormSXDTest(); }
  static void destroySuite( MDNormSXDTest *suite ) { delete suite; }


  void test_Init()
  {
    MDNormSXD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  //No test for now. Should be part of ticket #9105
  void test_exec()
  {
    // Name of the output workspace.
    /*std::string outWSName("MDNormSXDTest_OutputWS");

    MDNormSXD alg;
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
    AnalysisDataService::Instance().remove(outWSName);*/
  }
  

};


#endif /* MANTID_MDALGORITHMS_MDNORMSXDTEST_H_ */
