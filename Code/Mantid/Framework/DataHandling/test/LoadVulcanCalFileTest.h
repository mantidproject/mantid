#ifndef MANTID_DATAHANDLING_LOCALVULCANCALFILETEST_H_
#define MANTID_DATAHANDLING_LOCALVULCANCALFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadVulcanCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class LoadVulcanCalFileTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadVulcanCalFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("LoadVulcanCalFileTest");
    std::string offsetfilename = "pid_offset_vulcan_new.dat";
  
    LoadVulcanCalFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OffsetFilename", offsetfilename) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Grouping", "6Modules"));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("BankIDs", "21,22,23,26,27,28") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("EffectiveDIFCs",
                                                   "16372.601900,16376.951300,16372.096300,16336.622200,16340.822400,16338.777300") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Effective2Thetas", "90.091000,90.122000,90.089000,89.837000,89.867000,89.852000") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    GroupingWorkspace_sptr groupWS;
    TS_ASSERT_THROWS_NOTHING( groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(outWSName+"_group") );
    TS_ASSERT(groupWS);  if (!groupWS) return;

    TS_ASSERT_EQUALS(groupWS->getNumberHistograms(), 7392);

    TS_ASSERT_EQUALS( int(groupWS->readY(0)[0]), 1 );
    TS_ASSERT_EQUALS( int(groupWS->readY(7391)[0]), 6 );

    //Check if filename is saved
    TS_ASSERT_EQUALS(alg.getPropertyValue("OffsetFilename"), groupWS->run().getProperty("Filename")->value());

    OffsetsWorkspace_sptr offsetsWS;
    TS_ASSERT_THROWS_NOTHING( offsetsWS = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outWSName+"_offsets") );
    TS_ASSERT(offsetsWS); if (!offsetsWS) return;

    TS_ASSERT_DELTA( offsetsWS->getValue(26250), -0.000472175, 1e-7 );
    TS_ASSERT_DELTA( offsetsWS->readY(7391)[0], 6.39813e-05, 1e-7 );
    //Check if filename is saved
    TS_ASSERT_EQUALS(alg.getPropertyValue("OffsetFilename"),offsetsWS->run().getProperty("Filename")->value());

    // Masking
    SpecialWorkspace2D_sptr maskWS;
    TS_ASSERT_THROWS_NOTHING( maskWS = AnalysisDataService::Instance().retrieveWS<SpecialWorkspace2D>(outWSName+"_mask") );
    TS_ASSERT(maskWS); if (!maskWS) return;

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName+"_group");
    AnalysisDataService::Instance().remove(outWSName+"_offsets");
    AnalysisDataService::Instance().remove(outWSName+"_mask");
    AnalysisDataService::Instance().remove(outWSName+"_TOF_offsets");

    return;
  }


  void test_exec2BanksBadPixel()
  {
    // Name of the output workspace.
    std::string outWSName("LoadVulcanCalFileTest");
    std::string offsetfilename = "pid_offset_vulcan_new.dat";
    std::string badpixelfilename = "bad_pids_vulcan_new_6867_7323.dat";

    LoadVulcanCalFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OffsetFilename", offsetfilename) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Grouping", "2Banks"));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("BadPixelFilename", badpixelfilename ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("BankIDs", "21,22,23,26,27,28") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("EffectiveDIFCs",
                                                   "16376.951300,16376.951300,16376.951300, 16340.822400,16340.822400,16340.822400") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Effective2Thetas",
                                                   "90.122000,90.122000,90.122000, 89.867000,89.867000,89.867000") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );


    // Retrieve the workspace from data service. TODO: Change to your desired type
    GroupingWorkspace_sptr groupWS;
    TS_ASSERT_THROWS_NOTHING( groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(outWSName+"_group") );
    TS_ASSERT(groupWS);  if (!groupWS) return;

    TS_ASSERT_EQUALS( int(groupWS->getValue(26410)), 1 );
    TS_ASSERT_EQUALS( int(groupWS->getValue(34298)), 2 );

    // Masking
    SpecialWorkspace2D_sptr maskWS;
    TS_ASSERT_THROWS_NOTHING( maskWS = AnalysisDataService::Instance().retrieveWS<SpecialWorkspace2D>(outWSName+"_mask") );
    TS_ASSERT(maskWS); if (!maskWS) return;

    size_t nummasked = 0;
    for (size_t i = 0; i < maskWS->getNumberHistograms(); ++i)
    {
      if (maskWS->readY(i)[0] > 0.5)
      {
        ++nummasked;
        TS_ASSERT(maskWS->getDetector(i)->isMasked());
      }
    }

    TS_ASSERT_EQUALS(nummasked, 6);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName+"_group");
    AnalysisDataService::Instance().remove(outWSName+"_offsets");
    AnalysisDataService::Instance().remove(outWSName+"_mask");
    AnalysisDataService::Instance().remove(outWSName+"_TOF_offsets");

    return;
  }

};


#endif /* MANTID_DATAHANDLING_LOCALVULCANCALFILETEST_H_ */

