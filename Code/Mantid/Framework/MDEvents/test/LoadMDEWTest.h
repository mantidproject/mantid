#ifndef MANTID_MDEVENTS_LOADMDEWTEST_H_
#define MANTID_MDEVENTS_LOADMDEWTEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/LoadMDEW.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "SaveMDEWTest.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
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
  

  /// Compare two box controllers and assert each part of them.
  void compareBoxControllers(BoxController & a, BoxController & b)
  {
    TS_ASSERT_EQUALS( a.getNDims(), b.getNDims());
    TS_ASSERT_EQUALS( a.getMaxDepth(), b.getMaxDepth());
    TS_ASSERT_EQUALS( a.getMaxId(), b.getMaxId());
    TS_ASSERT_EQUALS( a.getSplitThreshold(), b.getSplitThreshold());
    TS_ASSERT_EQUALS( a.getNumMDBoxes(), b.getNumMDBoxes());
    TS_ASSERT_EQUALS( a.getNumSplit(), b.getNumSplit());
    TS_ASSERT_EQUALS( a.getMaxNumMDBoxes(), b.getMaxNumMDBoxes());
    for (size_t d=0; d< a.getNDims(); d++)
    {
      TS_ASSERT_EQUALS( a.getSplitInto(d), b.getSplitInto(d));
    }
  }

  /** Compare two MDEventWorkspaces
   *
   * @param ws :: workspace to check
   * @param ws1 :: reference workspace
   */
  template<typename MDE, size_t nd>
  void do_compare_MDEW(boost::shared_ptr<MDEventWorkspace<MDE,nd> > ws,
      boost::shared_ptr<MDEventWorkspace<MDE,nd> > ws1)
  {
    TS_ASSERT(ws->getBox());

    // Compare the initial to the final workspace
    TS_ASSERT_EQUALS(ws->getBox()->getNumChildren(), ws1->getBox()->getNumChildren());
    TS_ASSERT_EQUALS(ws->getNPoints(), ws1->getNPoints());

    TS_ASSERT_EQUALS(ws->getBoxController()->getMaxId(), ws1->getBoxController()->getMaxId());
    // Compare all the details of the box controllers
    compareBoxControllers(*ws->getBoxController(), *ws1->getBoxController());
    
    // Compare every box
    std::vector<IMDBox<MDE,nd>*> boxes;
    std::vector<IMDBox<MDE,nd>*> boxes1;

    ws->getBox()->getBoxes(boxes, 1000, false);
    ws1->getBox()->getBoxes(boxes1, 1000, false);

    TS_ASSERT_EQUALS( boxes.size(), boxes1.size());
    if (boxes.size() != boxes1.size()) return;

    for (size_t j=0; j<boxes.size(); j++)
    {
      IMDBox<MDE,nd>* box = boxes[j];
      IMDBox<MDE,nd>* box1 = boxes1[j];

      TS_ASSERT_EQUALS( box->getId(), box1->getId() );
      TS_ASSERT_EQUALS( box->getNumChildren(), box1->getNumChildren() );
      for (size_t i=0; i<box->getNumChildren(); i++)
      {
        TS_ASSERT_EQUALS( box->getChild(i)->getId(), box1->getChild(i)->getId() );
      }
      TS_ASSERT_DELTA( box->getSignal(), box1->getSignal(), 1e-3);
      TS_ASSERT_DELTA( box->getErrorSquared(), box1->getErrorSquared(), 1e-3);
      for (size_t d=0; d<nd; d++)
      {
        TS_ASSERT_DELTA( box->getExtents(d).min, box1->getExtents(d).min, 1e-5);
        TS_ASSERT_DELTA( box->getExtents(d).max, box1->getExtents(d).max, 1e-5);
      }
      TS_ASSERT_DELTA( box->getVolume(), box1->getVolume(), 1e-3);
      TS_ASSERT_EQUALS( box->getNPoints(), box1->getNPoints() );
      TS_ASSERT( box->getBoxController() );
      TS_ASSERT_EQUALS( box->getBoxController(), ws->getBoxController() );

      // Are both MDBoxes (with events)
      MDBox<MDE,nd>* mdbox = dynamic_cast<MDBox<MDE,nd>*>(box);
      MDBox<MDE,nd>* mdbox1 = dynamic_cast<MDBox<MDE,nd>*>(box1);
      if (mdbox)
      {
        TS_ASSERT( mdbox1 );
        std::vector<MDE > & events = mdbox->getEvents();
        std::vector<MDE > & events1 = mdbox1->getEvents();
        TS_ASSERT_EQUALS( events.size(), events1.size() );
        if (events.size() == events1.size() && events.size() > 2)
        {
          // Check first and last event
          for (size_t i=0; i<events.size(); i+=events.size()-1)
          {
            for (size_t d=0; d<nd; d++)
            {
              TS_ASSERT_DELTA( events[i].getCenter(d), events1[i].getCenter(d), 1e-4);
            }
            TS_ASSERT_DELTA( events[i].getSignal(), events1[i].getSignal(), 1e-4);
            TS_ASSERT_DELTA( events[i].getErrorSquared(), events1[i].getErrorSquared(), 1e-4);
          }
        }
      }
    }

  }



  template <size_t nd>
  void do_test_exec(bool FileBackEnd)
  {
    //------ Start by creating the file ----------------------------------------------
    // Make a 1D MDEventWorkspace
    boost::shared_ptr<MDEventWorkspace<MDEvent<nd>,nd> > ws1 = MDEventsTestHelper::makeMDEW<nd>(10, 0.0, 10.0, 0);
    ws1->getBoxController()->setSplitThreshold(100);
    // Put in ADS so we can use fake data
    AnalysisDataService::Instance().addOrReplace("LoadMDEWTest_ws", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws1));
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 6,
        "InputWorkspace", "LoadMDEWTest_ws", "UniformParams", "10000", "RandomizeSignal", "1");
//    AlgorithmHelper::runAlgorithm("FakeMDEventData", 6,
//        "InputWorkspace", "LoadMDEWTest_ws", "PeakParams", "30000, 5.0, 0.01", "RandomizeSignal", "1");

    std::ostringstream fileStream;
    fileStream << "LoadMDEWTest" << nd << ".nxs";

    // Save it
    SaveMDEW saver;
    TS_ASSERT_THROWS_NOTHING( saver.initialize() )
    TS_ASSERT( saver.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("InputWorkspace", "LoadMDEWTest_ws" ) );
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("Filename", fileStream.str()) );
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
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FileBackEnd", FileBackEnd) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    std::cout << tim << " to do the entire MDEW loading." << std::endl;

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING( iws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(iws);
    if (!iws) return;

    boost::shared_ptr<MDEventWorkspace<MDEvent<nd>,nd> > ws = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<nd>,nd> >(iws);

    // Perform the full comparison
    do_compare_MDEW(ws, ws1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }


  /// Load directly to memory
  void test_exec_1D()
  {
    do_test_exec<1>(false);
  }

  /// Run the loading but keep the events on file and load on demand
  void test_exec_1D_with_file_backEnd()
  {
    do_test_exec<1>(true);
  }

  /// Load directly to memory
  void test_exec_3D()
  {
    do_test_exec<3>(false);
  }

  /// Run the loading but keep the events on file and load on demand
  void test_exec_3D_with_file_backEnd()
  {
    do_test_exec<3>(true);
  }
  

  void xtest_exec_from_file()
  {
    AlgorithmHelper::runAlgorithm("LoadMDEW", 4,
        "Filename", "pg3.nxs",
        "OutputWorkspace", "DontCare");
  }

};


#endif /* MANTID_MDEVENTS_LOADMDEWTEST_H_ */


