#ifndef MANTID_MDEVENTS_LOADMDEWTEST_H_
#define MANTID_MDEVENTS_LOADMDEWTEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/LoadMDEW.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "SaveMDEWTest.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class LoadMDEWTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    LoadMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    //------ Start by creating the file ----------------------------------------------
    // Make a 1D MDEventWorkspace
    MDEventWorkspace1::sptr ws1 = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, 23);
    // Recurse split so that it has lots more boxes
    MDEventsTestHelper::recurseSplit<1>( dynamic_cast<MDGridBox<MDEvent<1>,1>*>(ws1->getBox()), 0, 4);
    // Add more points
    MDEventsTestHelper::feedMDBox(ws1->getBox(), 1, 9e3, 1e-3);
    ws1->refreshCache();

    // Save it
    SaveMDEW saver;
    TS_ASSERT_THROWS_NOTHING( saver.initialize() )
    TS_ASSERT( saver.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("InputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws1) ) );
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("Filename", "LoadMDEWTest.nxs") );
    TS_ASSERT_THROWS_NOTHING( saver.execute(); );
    TS_ASSERT( saver.isExecuted() );

    // Retrieve the full path
    std::string filename = saver.getPropertyValue("Filename");



    //------ Now the loading -------------------------------------
    // Name of the output workspace.
    std::string outWSName("LoadMDEWTest_OutputWS");
  
    CPUTimer tim;

    LoadMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    std::cout << tim << " to do the entire MDEW loading." << std::endl;

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING( iws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(iws);
    if (!iws) return;

    MDEventWorkspace1::sptr ws = boost::dynamic_pointer_cast<MDEventWorkspace1>(iws);
    TS_ASSERT(ws->getBox());

    // Compare the initial to the final workspace
    TS_ASSERT_EQUALS(ws->getBox()->getNumChildren(), ws1->getBox()->getNumChildren());
    TS_ASSERT_EQUALS(ws->getNPoints(), ws1->getNPoints());
    TS_ASSERT_EQUALS(ws->getBoxController()->getMaxId(), ws1->getBoxController()->getMaxId());
    
    // Compare every box
    std::vector<IMDBox<MDEvent<1>,1>*> boxes;
    std::vector<IMDBox<MDEvent<1>,1>*> boxes1;

    ws->getBox()->getBoxes(boxes, 1000, false);
    ws1->getBox()->getBoxes(boxes1, 1000, false);

    TS_ASSERT_EQUALS( boxes.size(), 111111);
    TS_ASSERT_EQUALS( boxes.size(), boxes1.size());
    if (boxes.size() != boxes1.size()) return;

    for (size_t i=0; i<boxes.size(); i++)
    {
      IMDBox<MDEvent<1>,1>* box = boxes[i];
      IMDBox<MDEvent<1>,1>* box1 = boxes1[i];
      TS_ASSERT_EQUALS( box->getId(), box1->getId() );
      TS_ASSERT_DELTA( box->getExtents(0).min, box1->getExtents(0).min, 1e-5);
      TS_ASSERT_DELTA( box->getExtents(0).max, box1->getExtents(0).max, 1e-5);
      TS_ASSERT_EQUALS( box->getNPoints(), box1->getNPoints() );
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  

};


#endif /* MANTID_MDEVENTS_LOADMDEWTEST_H_ */


