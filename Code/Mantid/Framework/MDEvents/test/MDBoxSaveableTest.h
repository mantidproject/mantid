#ifndef MDBOX_SAVEABLE_TEST_H
#define MDBOX_SAVEABLE_TEST_H

#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <Poco/File.h>
#include <nexus/NeXusFile.hpp>
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/BoxController.h"
#include "MantidMDEvents/CoordTransformDistance.h"
#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/BoxControllerNeXusIO.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/BoxControllerDummyIO.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

class MDBoxSaveableTest : public CxxTest::TestSuite
{
    BoxController_sptr sc;

    bool  DODEBUG;
    
    MDBoxSaveableTest()
    {
        sc = BoxController_sptr(new BoxController(3));
        DODEBUG = false;
    }



  /** Deletes the file created by do_saveNexus */
  static std::string do_deleteNexusFile(std::string barefilename = "MDBoxTest.nxs")
  {
    std::string filename(ConfigService::Instance().getString("defaultsave.directory") + barefilename);
    if (Poco::File(filename).exists())  
        Poco::File(filename).remove();
    return filename;
  }

public:
static MDBoxSaveableTest *createSuite() { return new MDBoxSaveableTest(); }
static void destroySuite(MDBoxSaveableTest * suite) { delete suite; }    


  //-----------------------------------------------------------------------------------------
  /** Create a test .NXS file with some data for a MDBox<3>.
   * 1000 events starting at position 500 of the file are made.
   * Each event is spread evenly around a 10x10x10 region from 0.5 to 9.5 in each direction
   * Then the file is open appropriately and returned.
   *
   * @param goofyWeights :: weights increasing from 0 to 999
   * @param barefilename :: file to save to (no path)
   * @param box :: MDBox3 that will get set to be file-backed
   * @return ptr to the NeXus file object
   * */
  void do_createNeXusBackedBox(MDBox<MDLeanEvent<3>,3> & box,BoxController_sptr bc,
      std::string barefilename = "MDBoxTest.nxs", bool goofyWeights = true)
  {
    // Create the NXS file
    std::string filename = do_createNexus(goofyWeights, barefilename);

  
    // Must get ready to load in the data
    auto loader  =boost::shared_ptr<API::IBoxControllerIO>(new BoxControllerNeXusIO(bc.get()));
    loader->setDataType(box.getCoordType(),box.getEventType());

    // Make BoxController file based
    bc->setFileBacked(loader,filename);
    // Handle the disk DiskBuffer values
    bc->getFileIO()->setWriteBufferSize(10000);


    // Make the box know where it is in the file
    box.setFileBacked(500,1000,true);
    // This would be set on loading. Only makes sense with GoofyWeights == false
    box.setSignal(1000.0);
    box.setErrorSquared(1000.0);


  }

 //-----------------------------------------------------------------------------------------
  /** Create a test .NXS file with some data for a MDBox<3>
   * 1000 events starting at position 500 of the file are made.
   *
   * @param goofyWeights :: weights increasing from 0 to 999
   * @param barefilename :: file to save to (no path)
   * @return filename with full path that was saved.
   * */
  std::string do_createNexus(bool goofyWeights = true, std::string barefilename = "MDBoxTest.nxs")
  {
    // Box with 1000 events evenly spread
    MDBox<MDLeanEvent<3>,3> b(sc.get());
    MDEventsTestHelper::feedMDBox(&b, 1, 10, 0.5, 1.0);
    TS_ASSERT_EQUALS( b.getNPoints(), 1000);
    if (goofyWeights)
    {
      // Give them goofy weights to be more interesting
      for (size_t i=0; i<1000; i++)
      {
        b.getEvents()[i].setSignal(float(i));
        b.getEvents()[i].setErrorSquared(float(i)+float(0.5));
      }
    }

    // Start a class which saves to NXS file
    auto Saver = new BoxControllerNeXusIO(sc.get());
    Saver->setDataType(b.getCoordType(),b.getEventType());

    std::string filename = do_deleteNexusFile(barefilename);

    Saver->openFile(filename,"w");

    // save events in the file, specifying direct position
    TS_ASSERT_THROWS_NOTHING(b.saveAt(Saver,500));

    delete Saver;
    return filename;
  }


 //-----------------------------------------------------------------------------------------
 //-----------------------------------------------------------------------------------------
 //-----------------------------------------------------------------------------------------
  /** test the methods related to the file back-end */
  void test_fileBackEnd_related()
  {
    auto bc = BoxController_sptr(new BoxController(2));
    // Box with 100 events
    MDBox<MDLeanEvent<2>,2> b(bc.get());
    MDEventsTestHelper::feedMDBox(&b, 1, 10, 0.5, 1.0);
    
    TS_ASSERT_EQUALS( b.getNPoints(), 100);
    b.refreshCache();
    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);

    // Because it wasn't set, the # of points on disk is 0, so NPoints = data.size() + 0
    TS_ASSERT_EQUALS( b.getNPoints(), 100);
    TS_ASSERT_THROWS_NOTHING(b.setFileBacked(1234,100,true))

    // Now it returns the cached number of points + the number in the data
    TS_ASSERT_EQUALS( b.getNPoints(), 200);
    // Still returns the signal/error
    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);
  }
//
 
  //-----------------------------------------------------------------------------------------
  /** Can we load it back? */
  void test_loadDirectNexus()
  {
    // A box to load stuff from
    MDBox<MDLeanEvent<3>,3> c(sc.get());
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);

    std::string fileName = do_createNexus();
    // Start a class which loads NXS file
    auto loader = new BoxControllerNeXusIO(sc.get());
    loader->setDataType(c.getCoordType(),c.getEventType());
    loader->openFile(fileName,"r");

    c.loadAndAddFrom(loader,500,1000);

    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    //// still on disk
    //TS_ASSERT_EQUALS( c.getFileSize(), 1000);
    //TS_ASSERT_EQUALS( c.getDataMemorySize(), 1000);
    const std::vector<MDLeanEvent<3> > & events = c.getEvents();
    //TSM_ASSERT("Got events so data should be busy unill events are released ",c.isBusy());

    // Try a couple of events to see if they are correct
    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);

    delete loader;
    do_deleteNexusFile();
  }

  //-----------------------------------------------------------------------------------------
  /** What if the box has no events, does it crash? */
  void test_SetFileBacked_fileEvents()
  {
    // A box to load stuff from
    MDBox<MDLeanEvent<3>,3> c(sc.get());
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    auto loader =boost::shared_ptr<API::IBoxControllerIO>(new MantidTestHelpers::BoxControllerDummyIO(sc.get()));
    loader->setDataType(c.getCoordType(),c.getEventType());

    // Create and open the test dummy file with 1000 floats in it (have to recalulate to events differently)
    sc->setFileBacked(loader,"existingDummy");



    // Tell it we have 10 events on file located after first three events ;
    size_t base = 3;
    c.setFileBacked(base,10,true);

    TSM_ASSERT_EQUALS("No data in memory yet", c.getDataInMemorySize(), 0);
    TSM_ASSERT_EQUALS("there are some data on file", c.getNPoints(),10);

    const std::vector<MDLeanEvent<3> > & events = c.getEvents();
    TSM_ASSERT_EQUALS("No data in memory yet", c.getDataInMemorySize(), 10);

    TS_ASSERT_DELTA( events[0].getErrorSquared(), base*base, 1e-5);
    TS_ASSERT_DELTA( events[2].getSignal(), 2+base, 1e-5);
    TS_ASSERT_DELTA( events[9].getErrorSquared(), (base+9)*(base+9), 1e-5);
    

  }
/** Test splitting of a MDBox into a MDGridBox when the
   * original box is backed by a file. */
  void test_fileBackEnd_construction()
  {
// Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));
    bc->setSplitInto(5);


    // Make a box from 0-10 in 3D
    MDBox<MDLeanEvent<3>,3> * c = new MDBox<MDLeanEvent<3>,3>(bc.get(), 0);
    for (size_t d=0; d<3; d++) c->setExtents(d, 0, 10);

    TSM_ASSERT_EQUALS( "Box starts empty", c->getNPoints(), 0);

    // Create test NXS file and make box file-backed
    do_createNeXusBackedBox(*c,bc,"MDGridBoxTest.nxs");
    // Handle the disk MRU values
    bc->getFileIO()->setWriteBufferSize(10000);

    DiskBuffer * dbuf = bc->getFileIO();

    // Create and open the test NXS file
    TSM_ASSERT_EQUALS( "1000 events (on file)", c->getNPoints(), 1000);

    // At this point the MDBox is set to be on disk
    TSM_ASSERT_EQUALS( "No free blocks to start with", dbuf->getFreeSpaceMap().size(), 0);

    // Construct the grid box by splitting the MDBox
    MDGridBox<MDLeanEvent<3>,3> * gb = new MDGridBox<MDLeanEvent<3>,3>(c);
    TSM_ASSERT_EQUALS( "Grid box also has 1000 points", gb->getNPoints(), 1000);
    TSM_ASSERT_EQUALS( "Grid box has 125 children (5x5x5)", gb->getNumChildren(), 125);
    TSM_ASSERT_EQUALS( "The old spot in the file is now free", dbuf->getFreeSpaceMap().size(), 1);

    // Get a child
    MDBox<MDLeanEvent<3>,3> * b = dynamic_cast<MDBox<MDLeanEvent<3>,3> *>(gb->getChild(22));
    TSM_ASSERT_EQUALS( "Child has 8 events", b->getNPoints(), 8);
    TSM_ASSERT("The child is also saveabele",b->getISaveable()!=NULL);
    if(!b->getISaveable())return;

    TSM_ASSERT_EQUALS( "Child is NOT on disk", b->getISaveable()->wasSaved(), false);

    bc->getFileIO()->closeFile();      
    do_deleteNexusFile("MDGridBoxTest.nxs");
  }



//---------------------------------------------------------------------------------------------------------------
// TESTS BELOW ARE NOT UNIT TESTS ANY MORE AS THE UNIT FUNCTIONALITY IS TESTED ELSEWHERE
// THEY STILL LEFT HERE AS SIMPLIFIED SYSTEM TESTS 
//---------------------------------------------------------------------------------------------------------------
//
  //-----------------------------------------------------------------------------------------
  /** If a MDBox is file-backed, test that
   * you can add events to it without having to load the data from disk.
   */
  void test_fileBackEnd_addEvent()
  {
    // Create a box with a controller for the back-end
    // Create on stack to ensure it's cleaned up properly. Its lifetime is sure to exceed those of the things that use it.
    BoxController bc(3);

    // Create and open the test NXS file
    MDBox<MDLeanEvent<3>,3> c(&bc, 0);
    auto loader = boost::shared_ptr<API::IBoxControllerIO>(new MantidTestHelpers::BoxControllerDummyIO(&bc));
    loader->setDataType(c.getCoordType(),c.getEventType());
    loader->setWriteBufferSize(10000);

    // Create and open the test dummy file with 1000 floats in it 
    bc.setFileBacked(loader,"existingDummy");
    c.setFileBacked(0,1000,true);



    TSM_ASSERT_EQUALS("Nothing in memory", c.getDataInMemorySize(), 0);
    TSM_ASSERT_EQUALS("Nothing in memory", c.getTotalDataSize(), 1000);
    TSM_ASSERT_EQUALS("1000 events on file", c.getNPoints(), 1000);
    TSM_ASSERT_DELTA("Incorrect cached signal", c.getSignal(),0, 1.e-6);
    TSM_ASSERT("Data is not flagged as modified", !c.isDataAdded());


    // Add an event to it
    MDLeanEvent<3> ev(1.2, 3.4);
    ev.setCenter(0, 1.5);
    ev.setCenter(1, 2.5);
    ev.setCenter(2, 3.5);
    c.addEvent(ev);

    TSM_ASSERT_EQUALS("But now 1001 events total because they are in two places.", c.getNPoints(), 1001);
    TSM_ASSERT_EQUALS("But only one in memory", c.getDataInMemorySize(), 1);
    TSM_ASSERT_EQUALS("The object size -- number of points in it", c.getTotalDataSize(), 1001);
    TSM_ASSERT_DELTA("At this point the cached signal is still incorrect - this is normal", c.getSignal(), 0, 1e-3);

    // Get the const vector of events AFTER adding events
    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
    TSM_ASSERT_EQUALS("The data is ALL in memory right now.", c.getDataInMemorySize(),1001);
    TSM_ASSERT_EQUALS("The resulting event vector has concatenated both", events.size(), 1001);
    TSM_ASSERT_DELTA("The first event is the one that was manually added.", events[0].getSignal(), 1.2, 1e-4);
    c.releaseEvents();

    // Flush the cache to write out the modified data
    loader->flushCache();
    TSM_ASSERT_EQUALS("Now there is nothing in memory", c.getDataInMemorySize(), 0);
    TSM_ASSERT_EQUALS("There is 1001 ppoint in total", c.getTotalDataSize(), 1001);
    TSM_ASSERT_EQUALS("And the block must have been moved since it grew", c.getISaveable()->getFilePosition(), 1000);
    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1001);
    //TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1001.2, 1e-3);
    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1000.*(1000.-1)/2.+1.2, 1e-3);

    TSM_ASSERT_EQUALS("The size of the file's field matches the last available point", loader->getFileLength(), 2001);

    {
    // Now getEvents in a const way then call addEvent()
    const std::vector<MDLeanEvent<3> > & events2 = c.getConstEvents();
    TSM_ASSERT("Data is not flagged as modified because it was accessed as const", !c.getISaveable()->isDataChanged());
    (void) events2;
    c.addEvent(ev);

    TSM_ASSERT("Data is still not flagged as modified because it was accessed as const", !c.getISaveable()->isDataChanged());
    TSM_ASSERT_EQUALS("Still 1001 events on file", c.getISaveable()->getFileSize(), 1001);
    TSM_ASSERT_EQUALS("And  1002 events in memory ", c.getTotalDataSize(), 1002);
    TSM_ASSERT_EQUALS("But the number of points had grown.", c.getNPoints(), 1002);
    c.releaseEvents();
    loader->flushCache();
    TSM_ASSERT("Data is not flagged as modified because it was written out to disk.", !c.getISaveable()->isDataChanged());
    TSM_ASSERT_EQUALS("Now there are 1002 events on file", c.getISaveable()->getFileSize(), 1002);
    TSM_ASSERT_EQUALS("And the block must have been moved back as the file length was 2001", c.getISaveable()->getFilePosition(), 0);
    TSM_ASSERT_EQUALS("And the data is no longer in memory.", c.getDataInMemorySize(),0);
    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1002);
    //TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1002.4, 1e-3);
     TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1000.*(1000.-1)/2.+2.4, 1e-3);
    }

    {
    // Now getEvents in a non-const way then call addEvent()
    std::vector<MDLeanEvent<3> > & events3 = c.getEvents();
    (void) events3;
    c.addEvent(ev);
    TSM_ASSERT_EQUALS("Still 1002 events on file", c.getISaveable()->getFileSize(), 1002);
    TSM_ASSERT_EQUALS("And 1003 events in memory", c.getTotalDataSize(), 1003);
    TSM_ASSERT_EQUALS("But the number of points had grown.", c.getNPoints(), 1003);
    c.releaseEvents();
    loader->flushCache();
    TSM_ASSERT_EQUALS("Nothing in memory", c.getDataInMemorySize(), 0);
    TSM_ASSERT_EQUALS("1003 events in total", c.getTotalDataSize(), 1003);
    TSM_ASSERT_EQUALS("1003 events on file", c.getISaveable()->getFileSize(), 1003);
    TSM_ASSERT_EQUALS("File now the size of 2001 and the was written over ", c.getISaveable()->getFilePosition(), 0);
    TSM_ASSERT_EQUALS("Now there is nothing in memory", c.getDataInMemorySize(), 0);
    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1003);
    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(),  1000.*(1000.-1)/2.+3.6, 1e-3);

    // 
    const std::vector<MDLeanEvent<3> > & events4 = c.getEvents();
    TSM_ASSERT_DELTA("The data were writtem over, two new events are at the beginning and old are sitting at the end", events4[2].getSignal(),  1, 1e-6);
    c.releaseEvents();
    loader->flushCache(); // nothing was written but the data are not in memory any more
    TSM_ASSERT_EQUALS("Now there is nothing in memory", c.getDataInMemorySize(), 0);
    }

    {
    // changes have been saved
    std::vector<MDLeanEvent<3> > & events4 = c.getEvents();
    TSM_ASSERT("Data  flagged as modified", c.getISaveable()->isDataChanged());
    TSM_ASSERT_DELTA("This was on file",events4[234].getSignal(),233.,1.e-6);
    events4[234].setSignal(1.);
    c.releaseEvents();
    loader->flushCache();
    TSM_ASSERT_EQUALS("Nothing in memory", c.getDataInMemorySize(), 0);
    TSM_ASSERT_EQUALS("All gone ",events4.size(),0);
    TSM_ASSERT_EQUALS("1003 events on the file", c.getISaveable()->getFileSize(), 1003);
    TSM_ASSERT_EQUALS("The file position have not changed ", c.getISaveable()->getFilePosition(), 0);
    TSM_ASSERT_EQUALS("Now there is nothing in memory", c.getDataInMemorySize(), 0);
    const std::vector<MDLeanEvent<3> > & events5 = c.getConstEvents();
    TSM_ASSERT_DELTA("The changes have been stored ",events5[234].getSignal(),1.,1.e-6);
    }

 // changes have been lost
    {
    const std::vector<MDLeanEvent<3> > & events6 = c.getConstEvents();
    TSM_ASSERT("Data  flagged as unmodifiable ", !c.getISaveable()->isDataChanged());
    TSM_ASSERT_DELTA("This was on file",events6[234].getSignal(),1.,1.e-6);
    // now do nasty thing and modify the signal
    std::vector<MDLeanEvent<3> > & events6m = const_cast<std::vector<MDLeanEvent<3> > &>(events6);
    events6m[234].setSignal(0.);
    c.releaseEvents();
    loader->flushCache();
    // changes lost; checks that constEvents are not saved back on HDD 
    const std::vector<MDLeanEvent<3> > & events7 = c.getConstEvents();
    TSM_ASSERT("Data  flagged as unmodifiable ", !c.getISaveable()->isDataChanged());
    TSM_ASSERT_DELTA("This was on file",events7[234].getSignal(),1.,1.e-6);
    }
    // changes forced: save of data to HDD is controlled by isDataChanged parameter
    {
    const std::vector<MDLeanEvent<3> > & events6 = c.getConstEvents();
    TSM_ASSERT("Data  flagged as unmodifiable ", !c.getISaveable()->isDataChanged());
    c.getISaveable()->setDataChanged();
    TSM_ASSERT("Data  flagged as modifiable ", c.getISaveable()->isDataChanged());
    TSM_ASSERT_DELTA("This was on file",events6[234].getSignal(),1.,1.e-6);
    // now do nasty thing and modify the signal
    std::vector<MDLeanEvent<3> > & events6m = const_cast<std::vector<MDLeanEvent<3> > &>(events6);
    events6m[234].setSignal(0.);

    c.releaseEvents();
    loader->flushCache();
    // changes now saved 
    const std::vector<MDLeanEvent<3> > & events7 = c.getConstEvents();
    TSM_ASSERT("Data  flagged as unmodifiable ", !c.getISaveable()->isDataChanged());
    TSM_ASSERT_DELTA("This was on file",events7[234].getSignal(),0.,1.e-6);
    }

  }




  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and xest accessing data */
  void test_fileBackEnd()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    MDBox<MDLeanEvent<3>,3> c(bc.get(), 0);
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);

    // Create test NXS file and make box file-backed
    do_createNeXusBackedBox(c,bc);

    DiskBuffer * dbuf = bc->getFileIO();
    // It is empty now
    TS_ASSERT_EQUALS(dbuf->getWriteBufferUsed(), 0);

    // Set the stuff that is handled outside the box itself
    c.setSignal(1234.5); // fake value loaded from disk
    c.setErrorSquared(456.78);

    // Now it gives the cached value
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);
    TSM_ASSERT("Data is not flagged as busy", !c.getISaveable()->isBusy());
    TSM_ASSERT("System expects that data were saved ",c.getISaveable()->wasSaved());

    // This should actually load the events from the file
    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
    TSM_ASSERT("Data accessed and flagged as modified", c.getISaveable()->isBusy());
    // Try a couple of events to see if they are correct
    TS_ASSERT_EQUALS( events.size(), 1000);
    if (events.size() != 1000) return;
    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);

    // The box's data is busy
    TS_ASSERT( c.getISaveable()->isBusy() );
    // Done with the data.
    c.releaseEvents();
    TS_ASSERT( !c.getISaveable()->isBusy() );
    // Something in the to-write buffer
    TS_ASSERT_EQUALS( dbuf->getWriteBufferUsed(), 1000);

     // Now this actually does it
    c.refreshCache();
    // The real values are back
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TS_ASSERT_DELTA( c.getSignal(), 499500.0, 1e-2);
    TS_ASSERT_DELTA( c.getErrorSquared(), 500000.0, 1e-2);

     // This should NOT call the write method since we had const access. Hard to test though!
    dbuf->flushCache();
    TS_ASSERT_EQUALS( dbuf->getWriteBufferUsed(), 0);

    // this operation should not happends in real lifa as it destroys file-backed stuff but here we do it just to allow the following operation to work
    bc->getFileIO()->closeFile();
    do_deleteNexusFile();
  }

  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data
   * in a non-const way, and writing it back out*/
  void test_fileBackEnd_nonConst_access()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    MDBox<MDLeanEvent<3>,3> c(bc.get(), 0);
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);

    // Create test NXS file and make box file-backed
    do_createNeXusBackedBox(c,bc);

    DiskBuffer * dbuf = bc->getFileIO();
    // It is empty now
    TS_ASSERT_EQUALS(dbuf->getWriteBufferUsed(), 0);


    // The # of points (from the file, not in memory)
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TSM_ASSERT("Data is not flagged as modified", !c.getISaveable()->isDataChanged());

    // Non-const access to the events.
    std::vector<MDLeanEvent<3> > & events = c.getEvents();
    TSM_ASSERT("Data is flagged as modified", c.getISaveable()->isDataChanged());
    TS_ASSERT_EQUALS( events.size(), 1000);
    if (events.size() != 1000) return;
    TS_ASSERT_DELTA( events[123].getSignal(), 123.0, 1e-5);

    // Modify the event
    events[123].setSignal(456.0);

    // Done with the events
    c.releaseEvents();

    // Flushing the cache will write out the events.
    dbuf->flushCache();

    // Now let's pretend we re-load that data into another box. It makes the box file backed but the location appears undefined
    MDBox<MDLeanEvent<3>,3> c2(c,bc.get());
    TSM_ASSERT_EQUALS("the data should not be in memory", c2.getDataInMemorySize(),0); 
    c2.setFileBacked(500,1000,true);
    TSM_ASSERT_EQUALS("the data should not be in memory", c2.getDataInMemorySize(),0); 

    // Is that event modified?
    std::vector<MDLeanEvent<3> > & events2 = c2.getEvents();
    TS_ASSERT_EQUALS( events2.size(), 1000);
    if (events2.size() != 1000) return;
    TS_ASSERT_DELTA( events2[123].getSignal(), 456.0, 1e-5);

 // this operation should not happends in real lifa as it destroys file-backed stuff but here we do it just to allow the following operation to work
    bc->getFileIO()->closeFile();

    do_deleteNexusFile();
  }


  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and xest accessing data
   * where the number of events in the box is reduced or increased. */
  void test_fileBackEnd_nonConst_EventListChangesSize()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    MDBox<MDLeanEvent<3>,3> c(bc.get(), 0);
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);

    // Create test NXS file and make box file-backed
    do_createNeXusBackedBox(c,bc);

    DiskBuffer * dbuf = bc->getFileIO();
    // It is empty now
    TS_ASSERT_EQUALS(dbuf->getWriteBufferUsed(), 0);


    // The # of points (from the file, not in memory)
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TSM_ASSERT("Data is not flagged as modified", !c.getISaveable()->isDataChanged());

    // Non-const access to the events.
    std::vector<MDLeanEvent<3> > & events = c.getEvents();
    TSM_ASSERT("Data is flagged as modified", c.getISaveable()->isDataChanged());
    TS_ASSERT_EQUALS( events.size(), 1000);
    if (events.size() != 1000) return;
    TS_ASSERT_DELTA( events[123].getSignal(), 123.0, 1e-5);

    // Modify an event
    events[123].setSignal(456.0);
    // Also change the size of the event list
    events.resize(600);
    events[599].setSignal(995.);

    // Done with the events
    c.releaseEvents();

    // Flushing the cache will write out the events.
    dbuf->flushCache();

    // The size on disk should have been changed (but not the position since that was the only free spot)
    TS_ASSERT_EQUALS( c.getISaveable()->getFilePosition(), 500);
    TS_ASSERT_EQUALS( c.getISaveable()->getTotalDataSize(), 600);
    TS_ASSERT_EQUALS( c.getDataInMemorySize(), 0);
    TS_ASSERT_EQUALS( c.getNPoints(), 600);


    // Now let's pretend we re-load that data into another box
    MDBox<MDLeanEvent<3>,3> c2(c,bc.get());
    c2.setFileBacked(500,600,true);
    
    // Is that event modified?
    std::vector<MDLeanEvent<3> > & events2 = c2.getEvents();
    TS_ASSERT_EQUALS( events2.size(), 600);
    if (events2.size() != 600) return;
    TS_ASSERT_DELTA( events2[123].getSignal(), 456.0, 1e-5);

    // Now we GROW the event list
    events2.resize(1500);
    events2[1499].setSignal(789.0);
    // and disentangle new events from old events as DB would think that they are on the same place and would write it accordingly
    c2.setFileBacked(1100,1500,false);
    // And we finish and write it out
    c2.releaseEvents();
    dbuf->flushCache();
    // The new event list should have ended up at the end of the file
    TS_ASSERT_EQUALS( c2.getISaveable()->getFilePosition(), 1500);
    TS_ASSERT_EQUALS( c2.getDataInMemorySize(), 0);
    TS_ASSERT_EQUALS( c2.getTotalDataSize(), 1500);
    // The file has now grown.
    TS_ASSERT_EQUALS( dbuf->getFileLength(), 3000);

    // and c-box data are there
    const std::vector<MDLeanEvent<3> > & events0a = c.getEvents();

    TS_ASSERT_DELTA(events0a[599].getSignal(),995.,1.e-6);
    // no writing shuld happen, just discarded from memory
    c.releaseEvents();
    dbuf->flushCache();
    TS_ASSERT_EQUALS( dbuf->getFileLength(), 3000);
    TS_ASSERT_EQUALS( c.getISaveable()->getFilePosition(), 500);
    TS_ASSERT_EQUALS( c.getISaveable()->getTotalDataSize(), 600);
    TS_ASSERT_EQUALS( c.getDataInMemorySize(), 0);


    // Now let's pretend we re-load that data into a 3rd box from c2 file location
    MDBox<MDLeanEvent<3>,3> c3(c,bc.get());
    c3.setFileBacked(c2.getISaveable()->getFilePosition(),1500,true);

    // Is that event modified?
    const std::vector<MDLeanEvent<3> > & events3 = c3.getEvents();
    TS_ASSERT_EQUALS( events3.size(), 1500);
    TS_ASSERT_DELTA( events3[1499].getSignal(), 789.0, 1e-5);
    c3.releaseEvents();

 // this operation should not happends in real lifa as it destroys file-backed stuff but here we do it just to allow the following operation to work
    bc->getFileIO()->closeFile();
    do_deleteNexusFile();
  }




  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data
   * by binning and stuff */
  void do_test_fileBackEnd_binningOperations(bool parallel)
  {
// Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    MDBox<MDLeanEvent<3>,3> c(bc.get(), 0);
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);

    // Create test NXS file and make box file-backed
    do_createNeXusBackedBox(c,bc,"MDBoxBinningxest.nxs",false);

    DiskBuffer * dbuf = bc->getFileIO();
    // It is empty now
    TS_ASSERT_EQUALS(dbuf->getWriteBufferUsed(), 0);

    PARALLEL_FOR_IF(parallel)
    for (int i=0; i<20; i++)
    {
      //std::cout << "Bin try " << i << "\n";
      // Try a bin, 2x2x2 so 8 events should be in there
      MDBin<MDLeanEvent<3>,3> bin;
      for (size_t d=0; d<3; d++)
      {
        bin.m_min[d] = 2.0;
        bin.m_max[d] = 4.0;
        bin.m_signal = 0;
      }
      c.centerpointBin(bin, NULL);
      TS_ASSERT_DELTA( bin.m_signal, 8.0, 1e-4);
      TS_ASSERT_DELTA( bin.m_errorSquared, 8.0, 1e-4);
    }

    PARALLEL_FOR_IF(parallel)
    for (int i=0; i<20; i++)
    {
      //std::cout << "Sphere try " << i << "\n";
      // Integrate a sphere in the middle
      bool dimensionsUsed[3] = {true,true,true};
      coord_t center[3] = {5,5,5};
      CoordTransformDistance sphere(3, center, dimensionsUsed);

      signal_t signal = 0;
      signal_t error = 0;
      c.integrateSphere(sphere, 1.0, signal, error);
      TS_ASSERT_DELTA( signal, 8.0, 1e-4);
      TS_ASSERT_DELTA( error, 8.0, 1e-4);
    }

    bc->getFileIO()->closeFile();    
    do_deleteNexusFile("MDBoxBinningxest.nxs");
    //suppress unused variable when built without openmp 
    UNUSED_ARG(parallel) 
 }

  void test_fileBackEnd_binningOperations()
  {
    do_test_fileBackEnd_binningOperations(false);
  }

  // TODO : does not work multithreaded and have never been workging. -- to fix 
  void xxest_fileBackEnd_binningOperations_inParallel()
  {
    do_test_fileBackEnd_binningOperations(true);
  }


  //------------------------------------------------------------------------------------------------
  /** This test splits a large number of events,
   * for a workspace that is backed by a file (and thus tries to stay below
   * a certain amount of memory used).
   */
  void test_splitAllIfNeeded_fileBacked()
  {
    typedef MDLeanEvent<2> MDE;

   
    // Create the grid box and make it file-backed.
    MDBoxBase<MDE,2> * b = MDEventsTestHelper::makeMDGridBox<2>();
    // box controlled is owned by the workspace, so here we make shared pointer from the box pointer as it is owned by this function
    BoxController_sptr spBc = boost::shared_ptr<BoxController >(b->getBoxController());


    auto fbc =boost::shared_ptr<API::IBoxControllerIO>(new MDEvents::BoxControllerNeXusIO(spBc.get()));
    spBc->setSplitThreshold(100);
    spBc->setMaxDepth(4);
    spBc->setFileBacked(fbc,"MDGridBoxTest.nxs");

    spBc->getFileIO()->setWriteBufferSize(1000);

    DiskBuffer * dbuf = fbc.get();


    size_t num_repeat = 10;
    if (DODEBUG) num_repeat = 40;
    Timer tim;
    if (DODEBUG) std::cout << "Adding " << num_repeat*10000 << " events...\n";
    MDEventsTestHelper::feedMDBox<2>(b, num_repeat, 100, 0.05f, 0.1f);
    if (DODEBUG) std::cout << "Adding events done in " << tim.elapsed() << "!\n";

    // Split those boxes in parallel.
    ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);
    b->splitAllIfNeeded(ts);
    tp.joinAll();

    if (DODEBUG) std::cout << "Splitting events done in " << tim.elapsed() << " sec.\n";

    // Get all the MDBoxes created
    std::vector<API::IMDNode *> boxes;
    b->getBoxes(boxes, 1000, true);
    TS_ASSERT_EQUALS(boxes.size(), 10000);
    size_t numOnDisk = 0;
    uint64_t eventsOnDisk = 0;
    uint64_t maxFilePos = 0;
    for (size_t i=0; i<boxes.size(); i++)
    {
      API::IMDNode * box = boxes[i];
      TS_ASSERT_EQUALS( box->getNPoints(), num_repeat );
      auto  mdbox = dynamic_cast<MDBox<MDE,2> *>(box);
      TS_ASSERT( mdbox);

      auto pIO = mdbox->getISaveable();
      TS_ASSERT(pIO!=NULL);
      if(!pIO)continue;

      if ( pIO->wasSaved() ) 
      {
           numOnDisk++;
           eventsOnDisk += pIO->getFileSize();
           // Track the last point used in the file
           uint64_t fileEnd = pIO->getFilePosition() + pIO->getFileSize();
           if (fileEnd > maxFilePos) maxFilePos = fileEnd;
      //std::cout << mdbox->getFilePosition() << " file pos " << i << std::endl;
      }
    }
    TSM_ASSERT_EQUALS("disk buffer correctly knows the last point  in the file used",dbuf->getFileLength(),maxFilePos);
    TSM_ASSERT_EQUALS("disk buffer correctly knows the number of events",10000*num_repeat,eventsOnDisk+dbuf->getWriteBufferUsed());
    dbuf->flushCache();
    TSM_ASSERT_EQUALS("All new boxes were set to be cached to disk.", dbuf->getFileLength(), 10000*num_repeat);
    TSM_ASSERT_EQUALS("Nothing left in memory.", dbuf->getWriteBufferUsed(), 0);
    uint64_t minimumSaved = 10000*(num_repeat-2);
    TSM_ASSERT_LESS_THAN("Length of the file makes sense", minimumSaved, dbuf->getFileLength());
    TSM_ASSERT_LESS_THAN("Most of the boxes' events were cached to disk (some remain in memory because of the MRU cache)", minimumSaved, eventsOnDisk);
    TSM_ASSERT_LESS_THAN("And the events were properly saved sequentially in the files.", minimumSaved, maxFilePos);
    std::cout << dbuf->getMemoryStr() << std::endl;
    

    const std::string filename = fbc->getFileName();
    fbc->closeFile();
    if (Poco::File(filename).exists()) Poco::File(filename).remove();
  }


  //-----------------------------------------------------------------------------------------
  

// 
// THIS MODE IS NOT SUPPORTED IN THIS FORM aNY MORE
//  //-----------------------------------------------------------------------------------------
//  /** Set up the file back end and test accessing data.
//   * This time, use no DiskBuffer so that reading/loading is done within the object itself */
//  void xxest_fileBackEnd_noMRU()
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//
//    // Handle the disk DiskBuffer values
//    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 0);
//    // DiskBuffer won't be used
////    TS_ASSERT( !bc->useWriteBuffer());
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//    // It is empty now
//    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
//
//    // Create and open the test NXS file
//    MDBox<MDLeanEvent<3>,3> c(bc, 0);
//    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);
//    ::NeXus::File * file = do_saveAndOpenNexus(c);
//
//    // Set the stuff that is handled outside the box itself
//    c.setSignal(1234.5); // fake value loaded from disk
//    c.setErrorSquared(456.78);
//
//    // Now it gives the cached value
//    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
//    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
//    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);
//    TSM_ASSERT("Data is not flagged as modified", !c.isDataChanged());
//
//    // This should actually load the events from the file
//    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
//    TSM_ASSERT("Data is STILL not flagged as modified", !c.isDataChanged());
//    // Try a couple of events to see if they are correct
//    TS_ASSERT_EQUALS( events.size(), 1000);
//    if (events.size() != 1000) return;
//    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
//    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
//    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);
//
//  //  TSM_ASSERT_EQUALS( "DiskBuffer has nothing still - it wasn't used",  dbuf.getWriteBufferUsed(), 0);
//    TSM_ASSERT_EQUALS( "DiskBuffer has this object inside",  dbuf.getWriteBufferUsed(), 1000);
//    TSM_ASSERT("Data is busy", c.isBusy() );
//    TSM_ASSERT("Data is in memory", c.getInMemory() );
//    // Done with the data.
//    c.releaseEvents();
//    TSM_ASSERT("Data is no longer busy", !c.isBusy() );
//    TSM_ASSERT("Data stillin memory", c.getInMemory() );
//    dbuf.flushCache();
//    TSM_ASSERT("Data is not in memory", !c.getInMemory() );
//    TSM_ASSERT_EQUALS( "DiskBuffer has nothing still - it wasn't used",  dbuf.getWriteBufferUsed(), 0);
//
//    file->close();
//    do_deleteNexusFile();
//  }
//


};
#endif
