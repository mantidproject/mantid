#ifndef MANTID_ALGORITHMS_CREATEPEAKSWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CREATEPEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CreatePeaksWorkspaceTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("CreatePeaksWorkspaceTest_OutputWS");
  
    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // Check the results
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 0);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_ALGORITHMS_CREATEPEAKSWORKSPACETEST_H_ */

