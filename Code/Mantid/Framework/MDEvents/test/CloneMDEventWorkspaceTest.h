#ifndef MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_

#include "LoadMDEWTest.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/CloneMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class CloneMDEventWorkspaceTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CloneMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec_InMemory()
  {
    do_test(false);
  }

  void test_exec_FileBacked()
  {
    do_test(true);
  }

  void test_exec_FileBacked_withFilename()
  {
    do_test(true, "CloneMDEventWorkspaceTest_ws_custom_cloned_name.nxs");
  }


  void do_test(bool fileBacked, std::string Filename = "")
  {
    // Name of the output workspace.
    std::string outWSName("CloneMDEventWorkspaceTest_OutputWS");

    // Make a fake file-backed (or not) MDEW
    MDEventWorkspace3Lean::sptr ws1 = MDEventsTestHelper::makeFileBackedMDEW("CloneMDEventWorkspaceTest_ws", fileBacked);
  
    CloneMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "CloneMDEventWorkspaceTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", Filename) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws2;
    TS_ASSERT_THROWS_NOTHING( ws2 = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws2); if (!ws2) return;
    
    // Compare the two workspaces
    LoadMDEWTest::do_compare_MDEW(ws1, ws2);
    
    // Check that the custom file name file exists
    if (fileBacked && !Filename.empty())
    {
      std::string realFile = alg.getPropertyValue("Filename");
      TS_ASSERT( Poco::File( realFile ).exists() );
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("CloneMDEventWorkspaceTest_ws");
    AnalysisDataService::Instance().remove(outWSName);
  }
  

};


#endif /* MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_ */

