#ifndef MANTID_DATAHANDLING_LOADCALFILETEST_H_
#define MANTID_DATAHANDLING_LOADCALFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class LoadCalFileTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadCalFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("LoadCalFileTest");
  
    LoadCalFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InstrumentName", "GEM") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeGroupingWorkspace", true) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeOffsetsWorkspace", true) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeMaskingWorkspace", true) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CalFilename", "offsets_2006_cycle064.cal") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WorkspaceName", outWSName ) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    GroupingWorkspace_sptr groupWS;
    TS_ASSERT_THROWS_NOTHING( groupWS = boost::dynamic_pointer_cast<GroupingWorkspace>(AnalysisDataService::Instance().retrieve(outWSName+"_group")) );
    TS_ASSERT(groupWS);
    if (!groupWS) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_DATAHANDLING_LOADCALFILETEST_H_ */

