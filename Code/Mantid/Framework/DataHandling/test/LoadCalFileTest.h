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
    TS_ASSERT(groupWS);  if (!groupWS) return;
    TS_ASSERT_EQUALS( int(groupWS->getValue(101001)), 2 );
    TS_ASSERT_EQUALS( int(groupWS->getValue(715079)), 7 );

    OffsetsWorkspace_sptr offsetsWS;
    TS_ASSERT_THROWS_NOTHING( offsetsWS = boost::dynamic_pointer_cast<OffsetsWorkspace>(AnalysisDataService::Instance().retrieve(outWSName+"_offsets")) );
    TS_ASSERT(offsetsWS); if (!offsetsWS) return;
    TS_ASSERT_DELTA( offsetsWS->getValue(101001), -0.0497075, 1e-7 );
    TS_ASSERT_DELTA( offsetsWS->getValue(714021), 0.0007437, 1e-7 );

    SpecialWorkspace2D_sptr maskWS;
    TS_ASSERT_THROWS_NOTHING( maskWS = boost::dynamic_pointer_cast<SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve(outWSName+"_mask")) );
    TS_ASSERT(maskWS); if (!maskWS) return;
    TS_ASSERT_EQUALS( int(maskWS->getValue(101001)), 1 );
    TS_ASSERT_EQUALS( int(maskWS->getValue(101003)), 0 );
    TS_ASSERT_EQUALS( int(maskWS->getValue(101008)), 0 );
    TS_ASSERT_EQUALS( int(maskWS->getValue(715079)), 1 );
    TS_ASSERT( !maskWS->getInstrument()->getDetector(101001)->isMasked() );
    TS_ASSERT( maskWS->getInstrument()->getDetector(101003)->isMasked() );
    TS_ASSERT( maskWS->getInstrument()->getDetector(101008)->isMasked() );
    TS_ASSERT( !maskWS->getInstrument()->getDetector(715079)->isMasked() );
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_DATAHANDLING_LOADCALFILETEST_H_ */

