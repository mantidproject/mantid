#ifndef MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUSTEST_H_
#define MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/AppendGeometryToSNSNexus.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class AppendGeometryToSNSNexusTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AppendGeometryToSNSNexusTest *createSuite() { return new AppendGeometryToSNSNexusTest(); }
  static void destroySuite( AppendGeometryToSNSNexusTest *suite ) { delete suite; }


  void test_Init()
  {
    AppendGeometryToSNSNexus alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
//    // Name of the output workspace.
//    std::string outWSName("AppendGeometryToSNSNexusTest_OutputWS");
  
    AppendGeometryToSNSNexus alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "CNCS_7860_event.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
//    Workspace_sptr ws;
//    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
//    TS_ASSERT(ws);
//    if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    //AnalysisDataService::Instance().remove(outWSName);
  }
  
  void xtest_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUSTEST_H_ */
