#ifndef MANTID_MDEVENTS_SAVEMDEWTEST_H_
#define MANTID_MDEVENTS_SAVEMDEWTEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidMDAlgorithms/BinMD.h"
#include "MantidMDAlgorithms/SaveMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class SaveMDTester: public SaveMD
{
public:
  void saveExperimentInfos(::NeXus::File * const file, IMDEventWorkspace_const_sptr ws)
  {
    this->saveExperimentInfos(file, ws);
  }
};

/** Note: See the LoadMDTest class
 * for a more thorough test that does
 * a round-trip.
 */
class SaveMDTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    do_test_exec(23, "SaveMDTest.nxs");
  }

  void test_exec_noEvents()
  {
    do_test_exec(0, "SaveMDTest_noEvents.nxs");
  }

  void test_MakeFileBacked()
  {
    do_test_exec(23, "SaveMDTest.nxs", true);
  }

  void test_MakeFileBacked_then_UpdateFileBackEnd()
  {
    do_test_exec(23, "SaveMDTest_updating.nxs", true, true);
  }


  void do_test_exec(size_t numPerBox, std::string filename, bool MakeFileBacked = false, bool UpdateFileBackEnd = false)
  {
   
    // Make a 1D MDEventWorkspace
    MDEventWorkspace1Lean::sptr ws = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, numPerBox);
    // Make sure it is split
    ws->splitBox();

    AnalysisDataService::Instance().addOrReplace("SaveMDTest_ws", ws);

    ws->refreshCache();

    // There are this many boxes, so this is the max ID.
    TS_ASSERT_EQUALS( ws->getBoxController()->getMaxId(), 11);

    IMDEventWorkspace_sptr iws = ws;

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeFileBacked", MakeFileBacked) );

    // clean up possible rubbish from the previous runs
    std::string fullName = alg.getPropertyValue("Filename");
    if (fullName!="")
      if(Poco::File(fullName).exists()) Poco::File(fullName).remove();

    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    std::string this_filename = alg.getProperty("Filename");
    TSM_ASSERT( "File was indeed created", Poco::File(this_filename).exists());

    if (MakeFileBacked)
    {
      TSM_ASSERT("Workspace was made file-backed", ws->isFileBacked() );
      TSM_ASSERT("File back-end no longer needs updating.", !ws->fileNeedsUpdating() );
    }

    // Continue the test
    if (UpdateFileBackEnd)
      do_test_UpdateFileBackEnd(ws, filename);
    else
    {

      ws->clearFileBacked(false);
      if (Poco::File(this_filename).exists()) Poco::File(this_filename).remove();
    }

  }
  
  /// Add some data and update the back-end
  void do_test_UpdateFileBackEnd(MDEventWorkspace1Lean::sptr ws,  std::string filename)
  {
    size_t initial_numEvents = ws->getNPoints();
    TSM_ASSERT_EQUALS("Starting off with 230 events.", initial_numEvents, 230);

    // Add 100 events
    for (size_t i=0; i<100; i++)
    {
      MDLeanEvent<1> ev(1.0, 1.0);
      ev.setCenter(0, double(i) * 0.01 + 0.4);
      ws->addEvent(ev);
    }
    ws->splitAllIfNeeded(NULL);
    ws->refreshCache();
    // Manually set the flag that the algo would set
    ws->setFileNeedsUpdating(true);

    TSM_ASSERT_EQUALS("Correctly added 100 events to original 230.",  ws->getNPoints(), 230+100);

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UpdateFileBackEnd", true) );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    // Since there are 330 events, the file needs to be that big (or bigger).
    TS_ASSERT_LESS_THAN( 330, ws->getBoxController()->getFileIO()->getFileLength());

    TSM_ASSERT("File back-end no longer needs updating.", !ws->fileNeedsUpdating() );
    // Clean up file
    ws->clearFileBacked(false);
    std::string fullPath = alg.getPropertyValue("Filename");
    if (Poco::File(fullPath).exists()) Poco::File(fullPath).remove();
  }

  void test_saveExpInfo()
  {
    std::string filename("MultiExperSaveTest.nxs");
    // Make a 1D MDEventWorkspace
    MDEventWorkspace1Lean::sptr ws = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, 2);
    // Make sure it is split
    ws->splitBox();

    Mantid::Geometry::Goniometer gon;
    gon.pushAxis("Psi",0,1,0);
    // add experiment infos 
    for(int i=0;i<80;i++)
    {
      ExperimentInfo_sptr ei = boost::shared_ptr<ExperimentInfo>(new ExperimentInfo());
      ei->mutableRun().addProperty("Psi",double(i));
      ei->mutableRun().addProperty("Ei",400.);
      ei->mutableRun().setGoniometer(gon,true);
      ws->addExperimentInfo(ei);
    }

    AnalysisDataService::Instance().addOrReplace("SaveMDTest_ws", ws);

    ws->refreshCache();

    // There are this many boxes, so this is the max ID.
    TS_ASSERT_EQUALS( ws->getBoxController()->getMaxId(), 11);

    IMDEventWorkspace_sptr iws = ws;

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeFileBacked","0") );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );
    std::string this_filename = alg.getProperty("Filename");
    ws->clearFileBacked(false);
    if (Poco::File(this_filename).exists()) Poco::File(this_filename).remove();

  }

  void test_saveAffine()
  {
    std::string filename("MDAffineSaveTest.nxs");
    // Make a 4D MDEventWorkspace
    MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(10, 0.0, 10.0, 2);
    AnalysisDataService::Instance().addOrReplace("SaveMDTest_ws", ws);

    // Bin data to get affine matrix
    BinMD balg;
    balg.initialize();
    balg.setProperty("InputWorkspace", "SaveMDTest_ws");
    balg.setProperty("OutputWorkspace", "SaveMDTestHisto_ws");
    balg.setProperty("AlignedDim0", "Axis2,0,10,10");
    balg.setProperty("AlignedDim1", "Axis0,0,10,5");
    balg.setProperty("AlignedDim2", "Axis1,0,10,5");
    balg.setProperty("AlignedDim3", "Axis3,0,10,2");
    balg.execute();

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTestHisto_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeFileBacked","0") );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );
    std::string this_filename = alg.getProperty("Filename");
    ws->clearFileBacked(false);
    if (Poco::File(this_filename).exists())
    {
      Poco::File(this_filename).remove();
    }
  }

  /** Run SaveMD with the MDHistoWorkspace */
  void doTestHisto(MDHistoWorkspace_sptr ws)
  {
    std::string filename = "SaveMDTestHisto.nxs";

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    filename = alg.getPropertyValue("Filename");
    TSM_ASSERT( "File was indeed created", Poco::File(filename).exists());
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_histo2()
  {
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.5, 2, 10, 10.0, 3.5, "histo2", 4.5);
    doTestHisto(ws);
  }

};



class SaveMDTestPerformance : public CxxTest::TestSuite
{
public:
  MDEventWorkspace3Lean::sptr  ws;
  void setUp()
  {
    // Make a 1D MDEventWorkspace
    ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    ws->getBoxController()->setSplitInto(5);
    ws->getBoxController()->setSplitThreshold(2000);

    AnalysisDataService::Instance().addOrReplace("SaveMDTestPerformance_ws", ws);

    FrameworkManager::Instance().exec("FakeMDEventData", 4,
        "InputWorkspace", "SaveMDTestPerformance_ws", "UniformParams", "10000000");

    ws->refreshCache();
  }

  void test_exec_3D()
  {
    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTestPerformance_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "SaveMDTestPerformance.nxs") );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );
  }


};


#endif /* MANTID_MDEVENTS_SAVEMDEWTEST_H_ */

