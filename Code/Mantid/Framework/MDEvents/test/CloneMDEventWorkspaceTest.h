#ifndef MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_

#include "LoadMDEWTest.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/CloneMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

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
  
  void xtest_exec_InMemory()
  {
    do_test(false);
  }

  void xtest_exec_FileBacked()
  {
    do_test(true);
  }


  void do_test(bool fileBacked)
  {
    // Name of the output workspace.
    std::string outWSName("CloneMDEventWorkspaceTest_OutputWS");

    // ---------- Make a file-backed MDEventWorkspace -----------------------
    MDEventWorkspace3Lean::sptr ws1 = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    ws1->getBoxController()->setSplitThreshold(100);
    AnalysisDataService::Instance().addOrReplace("CloneMDEventWorkspaceTest_ws", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws1));
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 6,
        "InputWorkspace", "CloneMDEventWorkspaceTest_ws", "UniformParams", "10000", "RandomizeSignal", "1");
    if (fileBacked)
    {
      AlgorithmHelper::runAlgorithm("SaveMDEW", 4,
          "InputWorkspace", "CloneMDEventWorkspaceTest_ws", "Filename", "CloneMDEventWorkspaceTest_ws.nxs");
      AlgorithmHelper::runAlgorithm("LoadMDEW", 8,
          "OutputWorkspace", "CloneMDEventWorkspaceTest_ws", "Filename", "CloneMDEventWorkspaceTest_ws.nxs",
          "FileBackEnd", "1", "Memory", "0");
    }

  
    CloneMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "CloneMDEventWorkspaceTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws2;
    TS_ASSERT_THROWS_NOTHING( ws2 = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws2); if (!ws2) return;
    
    // Compare the two workspaces
    LoadMDEWTest::do_compare_MDEW(ws1, ws2);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("CloneMDEventWorkspaceTest_ws");
    AnalysisDataService::Instance().remove(outWSName);
  }
  

};


#endif /* MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_ */

