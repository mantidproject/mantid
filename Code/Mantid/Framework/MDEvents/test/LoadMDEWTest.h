#ifndef MANTID_MDEVENTS_LOADMDEWTEST_H_
#define MANTID_MDEVENTS_LOADMDEWTEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/LoadMDEW.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidKernel/CPUTimer.h"

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class LoadMDEWTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void xtest_exec()
  {
    // Name of the output workspace.
    std::string outWSName("LoadMDEWTest_OutputWS");
  
    CPUTimer tim;

    LoadMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "SaveMDEWTest.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    std::cout << tim << " to load the MDEW." << std::endl;
    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  

};


#endif /* MANTID_MDEVENTS_LOADMDEWTEST_H_ */

