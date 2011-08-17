#ifndef MANTID_MDEVENTS_MERGEMDEWTEST_H_
#define MANTID_MDEVENTS_MERGEMDEWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MergeMDEW.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class MergeMDEWTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    MergeMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Create a bunch of input files
    std::vector<std::string> filenames;
    std::vector<MDEventWorkspace3Lean::sptr> inWorkspaces;
    for (size_t i=0; i<3; i++)
    {
      std::ostringstream mess;
      mess << "MergeMDEWTestInput" << i;
      MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeFileBackedMDEW(mess.str(), true);
      inWorkspaces.push_back(ws);
      filenames.push_back(ws->getBoxController()->getFilename());
    }

    // Name of the output workspace.
    std::string outWSName("MergeMDEWTest_OutputWS");
  
    MergeMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Filenames", filenames) );
    //TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputFilename", "MergeMDEWTestOutput.nxs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    TS_ASSERT_EQUALS( ws->getNPoints(), 30000);
    IMDBox3Lean * box = ws->getBox();
    TS_ASSERT_EQUALS( box->getNumChildren(), 64);
    // Every sub-box has some events since it was uniformly distributed before
    for (size_t i=0; i<box->getNumChildren(); i++)
      TS_ASSERT_LESS_THAN( 300, box->getChild(i)->getNPoints());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }



};


#endif /* MANTID_MDEVENTS_MERGEMDEWTEST_H_ */

