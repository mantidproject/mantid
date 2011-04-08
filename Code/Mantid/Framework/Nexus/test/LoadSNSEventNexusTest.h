#ifndef LOADSNSEVENTNEXUSTEST_H_
#define LOADSNSEVENTNEXUSTEST_H_

#include "MantidNexus/LoadSNSEventNexus.h"
#include <cxxtest/TestSuite.h>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;


/* NOTE: More thorough tests are in LoadEventNexusTest */
class LoadSNSEventNexusTest : public CxxTest::TestSuite
{
public:

  void test_fileCheck()
  {
    LoadSNSEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs"); // Only doing this to resolve the path to the file
    TS_ASSERT_EQUALS(ld.fileCheck(ld.getPropertyValue("Filename")), 0);

    //Try an ISIS nexus file
    ld.setPropertyValue("Filename","LOQ49886.nxs");
    TS_ASSERT_EQUALS(ld.fileCheck(ld.getPropertyValue("Filename")), 0);
  }


  void test_Simple()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "cncs";
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace",outws_name);
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.execute();
    TS_ASSERT( ld.isExecuted() );
    DataObjects::EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(
        WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name)); )
    //Valid WS and it is an EventWorkspace
    TS_ASSERT( WS );
    if (!WS) return;
    //Pixels have to be padded
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
    TS_ASSERT_LESS_THAN( 0, WS->getNumberEvents() );
  }

};


#endif /*LOADSNSEVENTNEXUSTEST_H_*/



