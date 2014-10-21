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
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("LoadNXSPETest_OutputWS");

    LoadNXSPE alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "NXSPEData.nxspe") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    AnalysisDataService::Instance().remove(outWSName);

  }

  void test_identifier_confidence()
  {
    const int high_confidence = LoadNXSPE::identiferConfidence("NXSPE");
    const int good_confidence = LoadNXSPE::identiferConfidence("NXSP");
    const int no_confidence = LoadNXSPE::identiferConfidence("NXS");

    TS_ASSERT(high_confidence > good_confidence);
    TS_ASSERT(good_confidence > no_confidence);
  }

};


#endif /* MANTID_DATAHANDLING_LOADNXSPETEST_H_ */

