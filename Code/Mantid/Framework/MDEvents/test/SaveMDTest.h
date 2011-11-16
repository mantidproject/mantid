#ifndef MANTID_MDEVENTS_SAVEMDEWTEST_H_
#define MANTID_MDEVENTS_SAVEMDEWTEST_H_

#include "MantidAPI/BoxController.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/SaveMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using Mantid::Kernel::CPUTimer;


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
    do_test_exec(23, "SaveMDTest_filebacked.nxs", true);
  }

  void test_MakeFileBacked_then_UpdateFileBackEnd()
  {
    do_test_exec(23, "SaveMDTest_updating.nxs", true, true);
  }

  void test_save_then_Load_then_UpdateFileBackEnd()
  {
    do_test_exec(23, "SaveMDTest_reloaded.nxs");
    IAlgorithm_sptr alg = FrameworkManager::Instance().exec("LoadMD", 8,
        "Filename",  "SaveMDTest_reloaded.nxs",
        "OutputWorkspace", "SaveMDTest_ws",
        "FileBackEnd", "1",
        "Memory", "0");
    MDEventWorkspace1Lean::sptr ws = boost::dynamic_pointer_cast<MDEventWorkspace1Lean>(
        AnalysisDataService::Instance().retrieve("SaveMDTest_ws") );
    TS_ASSERT( ws );
    if (!ws) return;
    do_test_UpdateFileBackEnd(ws, "SaveMDTest_reloaded.nxs");
  }

  //===============================================================================================
  //===============================================================================================
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

    CPUTimer tim;

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeFileBacked", MakeFileBacked) );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    std::cout << tim << " to save " << ws->getBoxController()->getMaxId() << " boxes." << std::endl;

    std::string this_filename = alg.getProperty("Filename");
    TSM_ASSERT( "File was indeed created", Poco::File(this_filename).exists());

    if (MakeFileBacked)
    {
      TSM_ASSERT("Workspace was made file-backed", ws->isFileBacked() );
      TSM_ASSERT("File back-end no longer needs updating.", !ws->fileNeedsUpdating() );
      std::vector<IMDBox1Lean *> boxes;
      ws->getBox()->getBoxes(boxes, 1000, true);
      for (size_t i=0; i < boxes.size(); i++)
      {
        MDBox1Lean * box = dynamic_cast<MDBox1Lean *>(boxes[i]);
        TS_ASSERT( box );
        // All MDBoxes now say "on disk"
        TS_ASSERT( box->getOnDisk() );
      }
    }

    // Continue the test
    if (UpdateFileBackEnd)
      do_test_UpdateFileBackEnd(ws, filename);

  }
  
  //===============================================================================================
  //===============================================================================================
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
    ws->refreshCache();
    // Manually set the flag that the algo would set
    ws->setFileNeedsUpdating(true);

    TSM_ASSERT_EQUALS("Correctly added 100 events to original 230.",  ws->getNPoints(), 230+100);

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UpdateFileBackEnd", true) );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

//    std::string fullPath = ws->getBoxController()->getFilename();
//    ws->getBoxController()->closeFile();
//    ::NeXus::File * file = new ::NeXus::File(fullPath, NXACC_RDWR);
//    file->openGroup("MDEventWorkspace", "NXentry");
//    file->openGroup("event_data", "NXdata");
//    file->openData("event_data");

    ::NeXus::File * file = ws->getBoxController()->getFile();
    file->closeData();
    file->openData("event_data");
    std::cout << "My file is " << uint64_t(file) << std::endl;


    // Since there are 330 events, the file needs to be that big (or bigger).
    TS_ASSERT_LESS_THAN( 330, file->getInfo().dims[0]);

    TSM_ASSERT("File back-end no longer needs updating.", !ws->fileNeedsUpdating() );

  }





//
//
//  void test_nexus_only()
//  {
//    ::NeXus::File * file = new ::NeXus::File("nexus_error.nxs", NXACC_CREATE5);
//    file->makeGroup("entry", "NXentry", 1);
//
//    std::vector<double> data;
//    data.resize(150, 123);
//
//    std::vector<int64_t> dims(2,0);
//    dims[0] = NX_UNLIMITED;
//    dims[1] = 3;
//    std::vector<int64_t> chunk(2,0);
//    chunk[0] = 2000;
//    chunk[1] = 3;
//
//    file->makeCompData("array",::NeXus::FLOAT64, dims, ::NeXus::NONE, chunk, true);
//    file->putAttr("description", "signal, errorsquared, center (each dim.)");
//    TS_ASSERT_EQUALS( file->getInfo().dims[0], 1);
//
//    std::vector<int64_t> start(2,0);
//    std::vector<int64_t> size(2,0);
//    size[0] = 5;
//    size[1] = 3;
//
//    for (size_t i=0; i<10; i++)
//    {
//      data[0] = double(i);
//      start[0] = 5*i;
//      file->putSlab(data, start, size);
//      //  std::cout << "array size is now " << file->getInfo().dims[0] << std::endl;
//    }
//    file->closeData();
//    file->openData("array");
//    TS_ASSERT_EQUALS( file->getInfo().dims[0], 50);
//
//    file->close();
//
//    // ---- Re-opening the data file and increasing the array size -----
//    std::vector<double> data2;
//    data2.resize(150, 456);
//
//    file = new ::NeXus::File("nexus_error.nxs", NXACC_RDWR);
//    file->openGroup("entry", "NXentry");
//    file->openData("array");
//
//    start[0] = 50;
//    file->putSlab(data2, start, size);
//    file->openData("array");
//    int64_t newSize = file->getInfo().dims[0];
//    TS_ASSERT_EQUALS( newSize, 55 );
//  }
//
//
//  void test_expanding_dataset_error()
//  {
//    typedef MDLeanEvent<3> MDE;
//    std::string filename = "nexus_workspace_error.nxs";
//    ::NeXus::File * file;
//
//    std::vector<MDE> events;
//    for (size_t i=0; i<5; i++)
//      events.push_back( MDE( 1.23, 3.45 ) );
//    double sig;
//    double err;
//
//    file = new ::NeXus::File(filename, NXACC_CREATE5);
//    file->makeGroup("MDEventWorkspace", "NXentry", 1);
//    file->makeGroup("event_data", "NXdata", 1);
//
//    MDE::prepareNexusData(file, 2000);
//    uint64_t initialSize = MDE::openNexusData(file);
//    TS_ASSERT_EQUALS(initialSize, 1);
//    for (size_t i=0; i<9; i++)
//    {
//      MDE::saveVectorToNexusSlab( events, file, i*5, sig, err);
//      //std::cout << "array size is now " << file->getInfo().dims[0] << std::endl;
//    }
//    file->closeData();
//    MDE::openNexusData(file);
//    TS_ASSERT_EQUALS( file->getInfo().dims[0], 45);
//    MDE::saveVectorToNexusSlab( events, file, 45, sig, err);
//    MDE::openNexusData(file);
//    TSM_ASSERT_EQUALS( "You can add extra data upon first file creation.", file->getInfo().dims[0], 50);
////    MDE::closeNexusData(file);
//    file->close();
////    delete file;
//
//    file = new ::NeXus::File(filename, NXACC_RDWR);
//    file->openGroup("MDEventWorkspace", "NXentry");
//    file->openGroup("event_data", "NXdata");
//    MDE::openNexusData(file);
//    TS_ASSERT_EQUALS( file->getInfo().dims[0], 50);
//    for (size_t i=0; i<10; i++)
//      MDE::saveVectorToNexusSlab( events, file, 50+i*5, sig, err);
//
//    file->closeData();
//    MDE::openNexusData(file);
//    TSM_ASSERT_EQUALS( "You can add more data after reopening the file in RDWR mode", file->getInfo().dims[0], 100);
//  }
//
//
//  void test_expanding_dataset_2()
//  {
//    typedef MDLeanEvent<3> MDE;
//    std::string filename = "nexus_workspace_error2.nxs";
//    ::NeXus::File * file;
//
//    std::vector<MDE> events;
//    for (size_t i=0; i<5; i++)
//      events.push_back( MDE( 1.23, 3.45 ) );
//    double sig;
//    double err;
//
//    file = new ::NeXus::File(filename, NXACC_CREATE5);
//    file->makeGroup("MDEventWorkspace", "NXentry", 1);
//    file->makeGroup("event_data", "NXdata", 1);
//
//    MDE::prepareNexusData(file, 2000);
//    uint64_t initialSize = MDE::openNexusData(file);
//    TS_ASSERT_EQUALS(initialSize, 1);
//    for (size_t i=0; i<9; i++)
//    {
//      MDE::saveVectorToNexusSlab( events, file, i*5, sig, err);
//      //std::cout << "array size is now " << file->getInfo().dims[0] << std::endl;
//    }
//    MDE::saveVectorToNexusSlab( events, file, 45, sig, err);
//
//    // Done writing the event data.
//    MDE::closeNexusData(file);
//
//    std::vector<uint64_t> freeSpaceBlocks;
//    freeSpaceBlocks.resize(2, 0); // Needs a minimum size
//    std::vector<int64_t> free_dims(2,2); free_dims[0] = int64_t(freeSpaceBlocks.size()/2);
//    std::vector<int64_t> free_chunk(2,2); free_chunk[0] = 1000;
//    file->writeExtendibleData("free_space_blocks", freeSpaceBlocks, free_dims, free_chunk);
//    file->closeGroup();
//    file->makeGroup("box_structure", "NXdata",1);
//    file->close();
//
//    std::cout << "Done with the file, should have 50 events!" << std::endl;
//
//
//    file = new ::NeXus::File(filename, NXACC_RDWR);
//    file->openGroup("MDEventWorkspace", "NXentry");
//    file->openGroup("event_data", "NXdata");
//
//    BoxController_sptr bc(new BoxController(3));
//    TS_ASSERT_EQUALS( MDE::openNexusData(file), 50 );
//    bc->setFile(file, filename, 50 );
//
//    typedef MDBox<MDLeanEvent<3>,3> mdb;
//    mdb * mdbox = new mdb(bc);
//    // Fake a file-backed, 20-event one
//    mdbox->setFileIndex(10, 20);
//    mdbox->setOnDisk(true);
//    mdbox->setInMemory(false);
//    for(size_t i=0; i<30; i++)
//      mdbox->addEvent(MDE( 1.23, 3.45 ));
//    mdbox->save();
//
//    MDE::closeNexusData(file);
//    TS_ASSERT_EQUALS( MDE::openNexusData(file), 100 );
//
//    for(size_t i=0; i<10; i++)
//      mdbox->addEvent(MDE( 1.23, 3.45 ));
//    mdbox->save();
//
//    std::cout << "My file is " << uint64_t(file) << std::endl;
//
//    TS_ASSERT_EQUALS( MDE::openNexusData(file), 160 );
//  }

};



class SaveMDTestPerformance : public CxxTest::TestSuite
{
public:
  MDEventWorkspace3Lean::sptr  ws;
  void setUp()
  {
    CPUTimer tim;

    // Make a 1D MDEventWorkspace
    ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    ws->getBoxController()->setSplitInto(5);
    ws->getBoxController()->setSplitThreshold(2000);

    AnalysisDataService::Instance().addOrReplace("SaveMDTestPerformance_ws", ws);

    FrameworkManager::Instance().exec("FakeMDEventData", 4,
        "InputWorkspace", "SaveMDTestPerformance_ws", "UniformParams", "10000000");

    std::cout << tim << " to fake the data." << std::endl;
    ws->refreshCache();
    std::cout << tim << " to refresh cache." << std::endl;

//    // There are this many boxes, so this is the max ID.
//    TS_ASSERT_EQUALS( ws->getBoxController()->getMaxId(), 11111);

  }

  void test_exec_3D()
  {
    CPUTimer tim;

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTestPerformance_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "SaveMDTestPerformance.nxs") );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    std::cout << tim << " to save " << ws->getBoxController()->getMaxId() << " boxes with " << double(ws->getNPoints())/1e6 << " million events." << std::endl;
  }


};


#endif /* MANTID_MDEVENTS_SAVEMDEWTEST_H_ */

