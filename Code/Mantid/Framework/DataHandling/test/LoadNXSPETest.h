#ifndef MANTID_DATAHANDLING_LOADNXSPETEST_H_
#define MANTID_DATAHANDLING_LOADNXSPETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadNXSPE.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadNXSPETest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadNXSPE alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void xtest_exec()
  {
    // Name of the output workspace.
    std::string outWSName("LoadNXSPETest_OutputWS");

    LoadNXSPE alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "/home/andrei/Mantid/Test/Data/CNCS_7850.nxspe") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

//    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "/home/andrei/Desktop/cncs.nxspe") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "/home/andrei/Desktop/reduction.py") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    // Retrieve the workspace from data service.
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

  }

};


#endif /* MANTID_DATAHANDLING_LOADNXSPETEST_H_ */

