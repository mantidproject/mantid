#ifndef MANTID_MDEVENTS_LOADMDEWTEST_H_
#define MANTID_MDEVENTS_LOADMDEWTEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/LoadMDEW.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
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
  static void compareBoxControllers(BoxController & a, BoxController & b)
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
  static void do_compare_MDEW(boost::shared_ptr<MDEventWorkspace<MDE,nd> > ws,
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

      //std::cout << "ID: " << box->getId() << std::endl;
      TS_ASSERT_EQUALS( box->getId(), box1->getId() );
      TS_ASSERT_EQUALS( box->getDepth(), box1->getDepth() );
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

      // Are both MDGridBoxes ?
      MDGridBox<MDE,nd>* gridbox = dynamic_cast<MDGridBox<MDE,nd>*>(box);
      MDGridBox<MDE,nd>* gridbox1 = dynamic_cast<MDGridBox<MDE,nd>*>(box1);
      if (gridbox)
      {
        for (size_t d=0; d<nd; d++)
        {
          TS_ASSERT_DELTA( gridbox->getBoxSize(d), gridbox1->getBoxSize(d), 1e-4);
        }
      }

      // Are both MDBoxes (with events)
      MDBox<MDE,nd>* mdbox = dynamic_cast<MDBox<MDE,nd>*>(box);
      MDBox<MDE,nd>* mdbox1 = dynamic_cast<MDBox<MDE,nd>*>(box1);
      if (mdbox)
      {
        TS_ASSERT( mdbox1 );
        const std::vector<MDE > & events = mdbox->getConstEvents();
        const std::vector<MDE > & events1 = mdbox1->getConstEvents();
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
        mdbox->releaseEvents();
        mdbox1->releaseEvents();
      }
    }

  }



  template <size_t nd>
  void do_test_exec(bool FileBackEnd, bool deleteWorkspace=true, bool metadataonly=false)
  {
    //------ Start by creating the file ----------------------------------------------
    // Make a 1D MDEventWorkspace
    boost::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>,nd> > ws1 = MDEventsTestHelper::makeMDEW<nd>(10, 0.0, 10.0, 0);
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
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Memory", 0) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MetadataOnly", metadataonly));
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    std::cout << tim << " to do the entire MDEW loading." << std::endl;

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING( iws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(iws);
    if (!iws) return;

    boost::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>,nd> > ws = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<nd>,nd> >(iws);

    // Perform the full comparison
    do_compare_MDEW(ws, ws1);

    // Remove workspace from the data service.
    if (deleteWorkspace)
    {
      ws->getBoxController()->closeFile();
      AnalysisDataService::Instance().remove(outWSName);
    }
  }


  /** Follow up test that saves AGAIN to update a file back end */
  template <size_t nd>
  void do_test_UpdateFileBackEnd()
  {
    std::string outWSName("LoadMDEWTest_OutputWS");
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING( iws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(iws); if (!iws) return;
    boost::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>,nd> > ws2 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<nd>,nd> >(iws);

    // Modify that by adding some boxes
    MDGridBox<MDLeanEvent<nd>,nd> * box = dynamic_cast<MDGridBox<MDLeanEvent<nd>,nd>*>(ws2->getBox());
    // Now there are 2002 boxes
    box->splitContents(12);
    ws2->refreshCache();

    // There are some new boxes that are not cached to disk at this point.
    // Save it again.
    SaveMDEW saver;
    TS_ASSERT_THROWS_NOTHING( saver.initialize() )
    TS_ASSERT( saver.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("InputWorkspace", outWSName ) );
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("Filename", "") );
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("UpdateFileBackEnd", "1") );
    TS_ASSERT_THROWS_NOTHING( saver.execute(); );
    TS_ASSERT( saver.isExecuted() );

    // The file should have been modified but that's tricky to check directly.
    std::string filename = ws2->getBoxController()->getFilename();

    // Now we re-re-load it!
    LoadMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FileBackEnd", false) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "reloaded_again") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( iws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve("reloaded_again")) );
    boost::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>,nd> > ws3 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<nd>,nd> >(iws);
    TS_ASSERT(ws3); if (!ws3) return;
    ws3->refreshCache();

    // Perform the full comparison of the second and 3rd loaded workspaces
    do_compare_MDEW(ws2, ws3);

    ws2->getBoxController()->closeFile();
    AnalysisDataService::Instance().remove(outWSName);

  }

  void testMetaDataOnly()
  {
    //------ Start by creating the file ----------------------------------------------
    // Make a 1D MDEventWorkspace
    boost::shared_ptr<MDEventWorkspace<MDLeanEvent<2>,2> > ws1 = MDEventsTestHelper::makeMDEW<2>(10, 0.0, 10.0, 0);
    ws1->getBoxController()->setSplitThreshold(100);
    // Put in ADS so we can use fake data
    AnalysisDataService::Instance().addOrReplace("LoadMDEWTest_ws", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws1));
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 6,
        "InputWorkspace", "LoadMDEWTest_ws", "UniformParams", "10000", "RandomizeSignal", "1");

    std::ostringstream fileStream;
    fileStream << "LoadMDEWTest" << 2 << ".nxs";

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
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FileBackEnd", false) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Memory", 0) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MetadataOnly", true));
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    std::cout << tim << " to do the entire MDEW loading without Events." << std::endl;

    boost::shared_ptr<MDEventWorkspace<MDLeanEvent<2>,2> > ws = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<2>,2> >(AnalysisDataService::Instance().retrieve(outWSName));

    TSM_ASSERT_EQUALS("Should have no events!", 0, ws->getNPoints());
    TSM_ASSERT_EQUALS("Wrong number of dimensions", 2, ws->getNumDims());

    ws->getBoxController()->closeFile();
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
  void test_exec_3D_with_FileBackEnd()
  {
    do_test_exec<3>(true);
  }
  

  /** Use the file back end,
   * then change it and save to update the file at the back end.
   */
  void test_exec_3D_with_FileBackEnd_then_update_SaveMDEW()
  {
    do_test_exec<3>(true, false);
    do_test_UpdateFileBackEnd<3>();
  }


};


#endif /* MANTID_MDEVENTS_LOADMDEWTEST_H_ */


