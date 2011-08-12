#ifndef MANTID_MDEVENTS_PLUSMDEWTEST_H_
#define MANTID_MDEVENTS_PLUSMDEWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/PlusMDEW.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class PlusMDEWTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    PlusMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void xtest_exec()
  {
    // Name of the output workspace.
    std::string outWSName("PlusMDEWTest_OutputWS");
  
    PlusMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  

};


#endif /* MANTID_MDEVENTS_PLUSMDEWTEST_H_ */

