#ifndef MDBOXTEST_H
#define MDBOXTEST_H

#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <Poco/File.h>
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/BoxController.h"
#include "MantidMDEvents/CoordTransformDistance.h"
#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

class MDBoxTest :    public CxxTest::TestSuite
{

public:
  void test_default_constructor()
  {
    MDBox<MDLeanEvent<3>,3> b3;
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);
    TS_ASSERT_EQUALS( b3.getDepth(), 0);
  }

  void test_constructor()
  {
    BoxController_sptr sc( new BoxController(3));
    MDBox<MDLeanEvent<3>,3> b3(sc, 2);
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getBoxController(), sc);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);
    TS_ASSERT_EQUALS( b3.getDepth(), 2);
    TS_ASSERT_EQUALS( b3.getNumMDBoxes(), 1);
  }

  void test_constructorWithExtents()
  {
    BoxController_sptr sc( new BoxController(1));
    std::vector<MDDimensionExtents> extents(1);
    extents[0].min=123;
    extents[0].max=234;
    MDBox<MDLeanEvent<1>,1> box(sc, 2, extents);
    TS_ASSERT_EQUALS( box.getNumDims(), 1);
    TS_ASSERT_EQUALS( box.getBoxController(), sc);
    TS_ASSERT_EQUALS( box.getNPoints(), 0);
    TS_ASSERT_EQUALS( box.getDepth(), 2);
    TS_ASSERT_EQUALS( box.getNumMDBoxes(), 1);
    TS_ASSERT_DELTA( box.getExtents(0).min, 123, 1e-5);
    TS_ASSERT_DELTA( box.getExtents(0).max, 234, 1e-5);
  }

  void test_copy_constructor()
  {
    BoxController_sptr sc( new BoxController(1));
    std::vector<MDDimensionExtents> extents(1);
    extents[0].min=123;
    extents[0].max=234;
    MDBox<MDLeanEvent<1>,1> box1(sc, 2, extents);
    MDLeanEvent<1> ev(1.23, 2.34);
    for (size_t i=0; i<15; i++)
    {
      ev.setCenter(0, static_cast<coord_t>(i));
      box1.addEvent(ev);
    }
    // Do the copy
    MDBox<MDLeanEvent<1>,1> box2(box1);
    // Compare
    std::vector<MDLeanEvent<1> > events = box2.getEvents();
    TS_ASSERT_EQUALS( box2.getNumDims(), 1);
    TS_ASSERT_EQUALS( box2.getBoxController(), sc);
    TS_ASSERT_EQUALS( box2.getNPoints(), 15);
    TS_ASSERT_EQUALS( events.size(), 15);
    TS_ASSERT_DELTA( events[7].getCenter(0), 7.0, 1e-4);
    TS_ASSERT_EQUALS( box2.getDepth(), 2);
    TS_ASSERT_EQUALS( box2.getNumMDBoxes(), 1);
    TS_ASSERT_DELTA( box2.getExtents(0).min, 123, 1e-5);
    TS_ASSERT_DELTA( box2.getExtents(0).max, 234, 1e-5);
  }


  /** Adding events tracks the total signal */
  void test_addEvent()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getNPoints(), 1)
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    b.refreshCache();
#endif
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*1, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*1, 1e-5);
    // Weight of 1.0 per event.
    TS_ASSERT_EQUALS( b.getTotalWeight(), 1.0);

  }
  /** Adding events in unsafe way also works */
  void test_addEventUnsafe()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEventUnsafe(ev);
    TS_ASSERT_EQUALS( b.getNPoints(), 1)
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    b.refreshCache();
#endif
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*1, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*1, 1e-5);
  }


  /** Add a vector of events */
  void test_addEvents()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(1.2, 3.4);
    std::vector< MDLeanEvent<2> > vec;
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    vec.push_back(ev);
    vec.push_back(ev);
    vec.push_back(ev);
    b.addEvents(vec);
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    b.refreshCache();
#endif
    TS_ASSERT_EQUALS( b.getNPoints(), 3)
    TS_ASSERT_DELTA( b.getEvents()[2].getSignal(), 1.2, 1e-5)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*3, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*3, 1e-5);
  }

  /** Add a vector of events and give start/end spots*/
  void test_addEvents_with_start_stop()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(1.2, 3.4);
    std::vector< MDLeanEvent<2> > vec;
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    for (size_t i=0; i<10; i++)
      vec.push_back(ev);

    b.addEventsPart(vec, 5, 8);
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    b.refreshCache();
#endif
    TS_ASSERT_EQUALS( b.getNPoints(), 3)
    TS_ASSERT_DELTA( b.getEvents()[2].getSignal(), 1.2, 1e-5)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*3, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*3, 1e-5);
  }


  /** Try to add a large number of events in parallel
   * to the same MDBox, to make sure it is thread-safe.
   */
  void test_addEvent_inParallel()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);

    int num = 500000;
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < num; i++)
    {
      b.addEvent(ev);
    }
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    b.refreshCache();
#endif

    TS_ASSERT_EQUALS( b.getNPoints(), num)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*num, 1e-5*num);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*num, 1e-5*num);
  }

  void test_calculateDimensionStats()
  {
    MDDimensionStats stats[2];
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);
    ev.setCenter(0, 4.0);
    ev.setCenter(1, 5.0);
    b.addEvent(ev);
    TS_ASSERT_THROWS_NOTHING( b.calculateDimensionStats(stats); )
    TS_ASSERT_DELTA( stats[0].getMean(), 3.0, 1e-3);
    TS_ASSERT_DELTA( stats[1].getMean(), 4.0, 1e-3);
    TS_ASSERT_DELTA( stats[0].getApproxVariance(), 0.5, 1e-3);
    TS_ASSERT_DELTA( stats[1].getApproxVariance(), 0.5, 1e-3);
  }

  void test_transformDimensions()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);
    ev.setCenter(0, 4.0);
    ev.setCenter(1, 5.0);
    b.addEvent(ev);

    std::vector<double> scaling(2, 3.0);
    std::vector<double> offset(2, 1.0);
    b.transformDimensions(scaling, offset);
    const std::vector<MDLeanEvent<2> > events = b.getConstEvents();
    TS_ASSERT_DELTA( events[0].getCenter(0), 7.0, 1e-3);
    TS_ASSERT_DELTA( events[0].getCenter(1), 10.0, 1e-3);
    TS_ASSERT_DELTA( events[1].getCenter(0), 13.0, 1e-3);
    TS_ASSERT_DELTA( events[1].getCenter(1), 16.0, 1e-3);
    b.releaseEvents();
  }

  void test_clear()
  {
    BoxController_sptr bc( new BoxController(2));
    MDBox<MDLeanEvent<2>,2> b(bc);
    MDLeanEvent<2> ev(1.2, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    b.refreshCache();
#endif
    TS_ASSERT_EQUALS( b.getNPoints(), 2)
    TS_ASSERT_DELTA( b.getSignal(), 2.4, 1e-5)
    b.clear();
    TS_ASSERT_EQUALS( b.getNPoints(), 0)
    TS_ASSERT_DELTA( b.getSignal(), 0.0, 1e-5)
    TS_ASSERT_DELTA( b.getErrorSquared(), 0.0, 1e-5)
  }

  void test_getEvents()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getEvents().size(), 3);
    TS_ASSERT_EQUALS( b.getEvents()[2].getSignal(), 4.0);
  }

  void test_getEventsCopy()
  {
    MDBox<MDLeanEvent<2>,2> b;
    MDLeanEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    std::vector<MDLeanEvent<2> > * events;
    events = b.getEventsCopy();
    TS_ASSERT_EQUALS( events->size(), 3);
    TS_ASSERT_EQUALS( (*events)[2].getSignal(), 4.0);
  }

  void test_sptr()
  {
    typedef MDBox<MDLeanEvent<3>,3> mdbox3;
    TS_ASSERT_THROWS_NOTHING( mdbox3::sptr a( new mdbox3()); )
  }

  void test_bad_splitter()
  {
    BoxController_sptr sc( new BoxController(4));
    sc->setSplitThreshold(10);
    typedef MDBox<MDLeanEvent<3>,3> MACROS_ARE_DUMB; //...since they get confused by commas
    TS_ASSERT_THROWS( MACROS_ARE_DUMB b3(sc), std::invalid_argument);
  }


  void test_splitter()
  {
    BoxController_sptr sc( new BoxController(3));
    sc->setSplitThreshold(10);
    MDBox<MDLeanEvent<3>,3> b3(sc);
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);

    MDLeanEvent<3> ev(1.2, 3.4);
    std::vector< MDLeanEvent<3> > vec;
    for(size_t i=0; i < 12; i++) vec.push_back(ev);
    b3.addEvents( vec );

    TS_ASSERT_EQUALS( b3.getBoxController(), sc);
  }


  void test_centerpointBin()
  {
    MDBox<MDLeanEvent<2>,2> box;
    for (double x=0.5; x < 10.0; x += 1.0)
      for (double y=0.5; y < 10.0; y += 1.0)
      {
        MDLeanEvent<2> ev(1.0, 1.5);
        ev.setCenter(0, static_cast<coord_t>(x));
        ev.setCenter(1, static_cast<coord_t>(y));
        box.addEvent(ev);
      }
    TS_ASSERT_EQUALS(box.getNPoints(), 100);
    // First, a bin object that holds everything
    MDBin<MDLeanEvent<2>,2> bin;
    // Perform the centerpoint binning
    box.centerpointBin(bin, NULL);
    // 100 events = 100 weight.
    TS_ASSERT_DELTA( bin.m_signal, 100.0, 1e-4);
    TS_ASSERT_DELTA( bin.m_errorSquared, 150.0, 1e-4);

    // Next, a more restrictive bin. a 2.0 x 2.0 square, with 4 events
    bin.m_signal = 0;
    bin.m_errorSquared = 0;
    bin.m_min[0] = 4.0;
    bin.m_max[0] = 6.0;
    bin.m_min[1] = 1.0;
    bin.m_max[1] = 3.0;
    box.centerpointBin(bin, NULL);
    TS_ASSERT_DELTA( bin.m_signal, 4.0, 1e-4);
    TS_ASSERT_DELTA( bin.m_errorSquared, 6.0, 1e-4);
  }


  /** For test_integrateSphere,
   *
   * @param box
   * @param radius :: radius to integrate
   * @param numExpected :: how many events should be in there
   */
  void dotest_integrateSphere(MDBox<MDLeanEvent<3>,3> & box, coord_t x, coord_t y, coord_t z, const coord_t radius, double numExpected)
  {
    // The sphere transformation
    bool dimensionsUsed[3] = {true,true,true};
    coord_t center[3] = {x,y,z};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    signal_t signal = 0;
    signal_t errorSquared = 0;
    box.integrateSphere(sphere, radius*radius, signal, errorSquared);
    TS_ASSERT_DELTA( signal, 1.0*numExpected, 1e-5);
    TS_ASSERT_DELTA( errorSquared, 1.5*numExpected, 1e-5);
  }

  void test_integrateSphere()
  {
    // One event at each integer coordinate value between 1 and 9
    MDBox<MDLeanEvent<3>,3> box;
    for (double x=1.0; x < 10.0; x += 1.0)
      for (double y=1.0; y < 10.0; y += 1.0)
        for (double z=1.0; z < 10.0; z += 1.0)
        {
          MDLeanEvent<3> ev(1.0, 1.5);
          ev.setCenter(0, x);
          ev.setCenter(1, y);
          ev.setCenter(2, z);
          box.addEvent(ev);
        }

    TS_ASSERT_EQUALS( box.getNPoints(), 9*9*9);

    dotest_integrateSphere(box, 5.0,5.0,5.0,  0.5,   1.0);
    dotest_integrateSphere(box, 0.5,0.5,0.5,  0.5,   0.0);
    dotest_integrateSphere(box, 5.0,5.0,5.0,  1.1f,   7.0);
    dotest_integrateSphere(box, 5.0,5.0,5.0,  10., 9*9*9);
  }

  //-----------------------------------------------------------------------------------------
  /** refreshCache() tracks the centroid */
  void test_refreshCentroid()
  {
    MDBox<MDLeanEvent<2>,2> b;

    MDLeanEvent<2> ev(2.0, 2.0);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);

    MDLeanEvent<2> ev2(4.0, 4.0);
    ev2.setCenter(0, 4.0);
    ev2.setCenter(1, 4.0);
    b.addEvent(ev2);

    // Must call the signal cache first.
    b.refreshCache();
    b.refreshCentroid();
#ifdef MDBOX_TRACK_CENTROID
    // This should be the weighted centroid
    TS_ASSERT_DELTA( b.getCentroid(0), 3.333, 0.001);
    TS_ASSERT_DELTA( b.getCentroid(1), 3.666, 0.001);
#endif
  }


  /** Centroid of an empty MDBox is 0.0 */
  void test_refreshCache_withCentroid_emptyMDBox()
  {
    MDBox<MDLeanEvent<2>,2> b;
    b.refreshCache();
    b.refreshCentroid();
#ifdef MDBOX_TRACK_CENTROID
    TS_ASSERT_DELTA( b.getCentroid(0), 0.000, 0.001);
    TS_ASSERT_DELTA( b.getCentroid(1), 0.000, 0.001);
#endif
  }


  //-----------------------------------------------------------------------------------------
  void test_centroidSphere()
  {
    MDBox<MDLeanEvent<2>,2> b;

    MDLeanEvent<2> ev(2.0, 2.0);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);

    MDLeanEvent<2> ev2(4.0, 4.0);
    ev2.setCenter(0, 4.0);
    ev2.setCenter(1, 4.0);
    b.addEvent(ev2);


    // The sphere transformation
    bool dimensionsUsed[2] = {true,true};
    coord_t center[2] = {0,0};
    CoordTransformDistance sphere(2, center, dimensionsUsed);

    // Set up the data for the centroid
    coord_t centroid[2] = {0,0};
    signal_t signal = 0.0;
    b.centroidSphere(sphere, 400., centroid, signal);
    for (size_t d=0; d<2; d++)
      centroid[d] /= static_cast<coord_t>(signal);

    // This should be the weighted centroid
    TS_ASSERT_DELTA( signal, 6.000, 0.001);
    TS_ASSERT_DELTA( centroid[0], 3.333, 0.001);
    TS_ASSERT_DELTA( centroid[1], 3.666, 0.001);

    // --- Reset and reduce the radius ------
    signal = 0;
    for (size_t d=0; d<2; d++)
      centroid[d] = 0.0;
    b.centroidSphere(sphere, 16., centroid, signal);
    for (size_t d=0; d<2; d++)
      centroid[d] /= static_cast<coord_t>(signal);
    // Only one event was contained
    TS_ASSERT_DELTA( signal, 2.000, 0.001);
    TS_ASSERT_DELTA( centroid[0], 2.000, 0.001);
    TS_ASSERT_DELTA( centroid[1], 3.000, 0.001);
  }


  //-----------------------------------------------------------------------------------------
  /** Test the methods related to the file back-end */
  void test_fileBackEnd_related()
  {
    // Box with 100 events
    MDBox<MDLeanEvent<2>,2> b;
    MDEventsTestHelper::feedMDBox(&b, 1, 10, 0.5, 1.0);
    TS_ASSERT_EQUALS( b.getNPoints(), 100);
    b.refreshCache();
    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);
    b.setOnDisk(true);
    b.setInMemory(false);
    // Because it wasn't set, the # of points on disk is 0, so NPoints = data.size() + 0
    TS_ASSERT_EQUALS( b.getNPoints(), 100);
    b.setFileIndex(1234, 100);
    // Now it returns the cached number of points + the number in the data
    TS_ASSERT_EQUALS( b.getNPoints(), 200);
    // Still returns the signal/error
    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);
  }


  //-----------------------------------------------------------------------------------------
  /** Create a test .NXS file with some data for a MDBox<3>
   * 1000 events starting at position 500 of the file are made.
   *
   * @param goofyWeights :: weights increasing from 0 to 999
   * @param barefilename :: file to save to (no path)
   * @return filename with full path that was saved.
   * */
  static std::string do_saveNexus(bool goofyWeights = true, std::string barefilename = "MDBoxTest.nxs")
  {
    // Box with 1000 events evenly spread
    MDBox<MDLeanEvent<3>,3> b;
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

    // Start a NXS file
    std::string filename = (ConfigService::Instance().getString("defaultsave.directory") + barefilename);
    if (Poco::File(filename).exists())  Poco::File(filename).remove();
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_CREATE5);
    file->makeGroup("my_test_group", "NXdata", 1);

    // Must prepare the data.
    MDLeanEvent<3>::prepareNexusData(file, 2000);

    // Save it with some offset
    b.setFileIndex(500, 1000);
    b.saveNexus(file);

    file->closeData();
    file->closeGroup();
    file->close();

    return filename;
  }

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
  static ::NeXus::File * do_saveAndOpenNexus(MDBox<MDLeanEvent<3>,3> & box,
      std::string barefilename = "MDBoxTest.nxs", bool goofyWeights = true)
  {
    // Create the NXS file
    std::string filename = do_saveNexus(goofyWeights, barefilename);
    // Open the NXS file
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_RDWR);
    file->openGroup("my_test_group", "NXdata");
    // Must get ready to load in the data
    MDLeanEvent<3>::openNexusData(file);

    // Set it in the BoxController
    if (box.getBoxController())
      box.getBoxController()->setFile(file,filename, 2000);

    // Make the box know where it is in the file
    box.setFileIndex(500, 1000);
    box.setOnDisk(true);
    box.setInMemory(false);
    // This would be set on loading. Only makes sense with GoofyWeights == false
    box.setSignal(1000.0);
    box.setErrorSquared(1000.0);

    return file;
  }

  /** Deletes the file created by do_saveNexus */
  static void do_deleteNexusFile(std::string barefilename = "MDBoxTest.nxs")
  {
    std::string filename = (ConfigService::Instance().getString("defaultsave.directory") + barefilename);
    if (Poco::File(filename).exists())  Poco::File(filename).remove();
  }




  //-----------------------------------------------------------------------------------------
  /** Can we save to a file ? */
  void test_saveNexus()
  {
    std::string filename = do_saveNexus();
    TS_ASSERT(Poco::File(filename).exists());
    if (Poco::File(filename).exists()) Poco::File(filename).remove();
  }

  //-----------------------------------------------------------------------------------------
  /** Can we load it back? */
  void test_loadNexus()
  {
    // A box to load stuff from
    MDBox<MDLeanEvent<3>,3> c;
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);

    // Create and open the test NXS file
    ::NeXus::File * file = do_saveAndOpenNexus(c);
    c.setOnDisk(false); // Avoid touching DiskBuffer
    c.loadNexus(file);
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    const std::vector<MDLeanEvent<3> > & events = c.getEvents();

    // Try a couple of events to see if they are correct
    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);

    file->close();
  }


  //-----------------------------------------------------------------------------------------
  /** What if the box has no events, does it crash? */
  void test_loadNexus_noEvents()
  {
    // A box to load stuff from
    MDBox<MDLeanEvent<3>,3> c;
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    // Create and open the test NXS file
    ::NeXus::File * file = do_saveAndOpenNexus(c);
    // Tell it we actually have no events
    c.setFileIndex(500, 0);
    c.loadNexus(file);
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    file->close();
    do_deleteNexusFile();
  }

  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data */
  void test_fileBackEnd()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    // Handle the disk DiskBuffer values
    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 10000);
    DiskBuffer & dbuf = bc->getDiskBuffer();
    // It is empty now
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);

    // Create and open the test NXS file
    MDBox<MDLeanEvent<3>,3> c(bc, 0);
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);
    ::NeXus::File * file = do_saveAndOpenNexus(c);

    // Set the stuff that is handled outside the box itself
    c.setSignal(1234.5); // fake value loaded from disk
    c.setErrorSquared(456.78);

    // Now it gives the cached value
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);
    TSM_ASSERT("Data is not flagged as modified", !c.dataModified());

    // This should actually load the events from the file
    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
    TSM_ASSERT("Data is STILL not flagged as modified", !c.dataModified());
    // Try a couple of events to see if they are correct
    TS_ASSERT_EQUALS( events.size(), 1000);
    if (events.size() != 1000) return;
    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);

    // The box's data is busy
    TS_ASSERT( c.dataBusy() );
    // Done with the data.
    c.releaseEvents();
    TS_ASSERT( !c.dataBusy() );
    // Something in the to-write buffer
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 1000);

    // OK, using this fake way, let's keep it in memory
    c.setOnDisk(false);
    // Now this actually does it
    c.refreshCache();
    // The real values are back
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TS_ASSERT_DELTA( c.getSignal(), 499500.0, 1e-2);
    TS_ASSERT_DELTA( c.getErrorSquared(), 500000.0, 1e-2);

    // Flush out the cache
    c.setOnDisk(true);
    // This should NOT call the write method since we had const access. Hard to test though!
    dbuf.flushCache();

    file->close();
    do_deleteNexusFile();
  }



  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data.
   * This time, use no DiskBuffer so that reading/loading is done within the object itself */
  void test_fileBackEnd_noMRU()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    // Handle the disk DiskBuffer values
    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 0);
    // DiskBuffer won't be used
    TS_ASSERT( !bc->useWriteBuffer());
    DiskBuffer & dbuf = bc->getDiskBuffer();
    // It is empty now
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);

    // Create and open the test NXS file
    MDBox<MDLeanEvent<3>,3> c(bc, 0);
    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);
    ::NeXus::File * file = do_saveAndOpenNexus(c);

    // Set the stuff that is handled outside the box itself
    c.setSignal(1234.5); // fake value loaded from disk
    c.setErrorSquared(456.78);

    // Now it gives the cached value
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);
    TSM_ASSERT("Data is not flagged as modified", !c.dataModified());

    // This should actually load the events from the file
    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
    TSM_ASSERT("Data is STILL not flagged as modified", !c.dataModified());
    // Try a couple of events to see if they are correct
    TS_ASSERT_EQUALS( events.size(), 1000);
    if (events.size() != 1000) return;
    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);

    TSM_ASSERT_EQUALS( "DiskBuffer has nothing still - it wasn't used",  dbuf.getWriteBufferUsed(), 0);
    TSM_ASSERT("Data is busy", c.dataBusy() );
    TSM_ASSERT("Data is in memory", c.getInMemory() );
    // Done with the data.
    c.releaseEvents();
    TSM_ASSERT("Data is no longer busy", !c.dataBusy() );
    TSM_ASSERT("Data is not in memory", !c.getInMemory() );
    TSM_ASSERT_EQUALS( "DiskBuffer has nothing still - it wasn't used",  dbuf.getWriteBufferUsed(), 0);

    file->close();
    do_deleteNexusFile();
  }


  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data
   * in a non-const way, and writing it back out*/
  void test_fileBackEnd_nonConst_access()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    // Handle the disk DiskBuffer values
    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 10000);
    DiskBuffer & dbuf = bc->getDiskBuffer();
    // It is empty now
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);

    // A new empty box.
    MDBox<MDLeanEvent<3>,3> c(bc, 0);

    // Create and open the test NXS file
    ::NeXus::File * file = do_saveAndOpenNexus(c);

    // The # of points (from the file, not in memory)
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TSM_ASSERT("Data is not flagged as modified", !c.dataModified());

    // Non-const access to the events.
    std::vector<MDLeanEvent<3> > & events = c.getEvents();
    TSM_ASSERT("Data is flagged as modified", c.dataModified());
    TS_ASSERT_EQUALS( events.size(), 1000);
    if (events.size() != 1000) return;
    TS_ASSERT_DELTA( events[123].getSignal(), 123.0, 1e-5);

    // Modify the event
    events[123].setSignal(456.0);

    // Done with the events
    c.releaseEvents();

    // Flushing the cache will write out the events.
    dbuf.flushCache();

    // Now let's pretend we re-load that data into another box
    MDBox<MDLeanEvent<3>,3> c2(bc, 0);
    c2.setFileIndex(500, 1000);
    c2.setOnDisk(true);
    c2.setInMemory(false);
    // Is that event modified?
    std::vector<MDLeanEvent<3> > & events2 = c2.getEvents();
    TS_ASSERT_EQUALS( events2.size(), 1000);
    if (events2.size() != 1000) return;
    TS_ASSERT_DELTA( events2[123].getSignal(), 456.0, 1e-5);

    file->close();
    do_deleteNexusFile();
  }


  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data
   * where the number of events in the box is reduced or increased. */
  void test_fileBackEnd_nonConst_EventListChangesSize()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));

    // Handle the disk DiskBuffer values
    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 10000);
    DiskBuffer & dbuf = bc->getDiskBuffer();
    // It is empty now
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);

    // A new empty box.
    MDBox<MDLeanEvent<3>,3> c(bc, 0);

    // Create and open the test NXS file
    ::NeXus::File * file = do_saveAndOpenNexus(c);

    // The # of points (from the file, not in memory)
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TSM_ASSERT("Data is not flagged as modified", !c.dataModified());

    // Non-const access to the events.
    std::vector<MDLeanEvent<3> > & events = c.getEvents();
    TSM_ASSERT("Data is flagged as modified", c.dataModified());
    TS_ASSERT_EQUALS( events.size(), 1000);
    if (events.size() != 1000) return;
    TS_ASSERT_DELTA( events[123].getSignal(), 123.0, 1e-5);

    // Modify an event
    events[123].setSignal(456.0);
    // Also change the size of the event list
    events.resize(600);

    // Done with the events
    c.releaseEvents();

    // Flushing the cache will write out the events.
    dbuf.flushCache();

    // The size on disk should have been changed (but not the position since that was the only free spot)
    TS_ASSERT_EQUALS( c.getFileIndexStart(), 500);
    TS_ASSERT_EQUALS( c.getFileNumEvents(), 600);

    // Now let's pretend we re-load that data into another box
    MDBox<MDLeanEvent<3>,3> c2(bc, 0);
    c2.setFileIndex(500, 600);
    c2.setOnDisk(true);
    c2.setInMemory(false);
    // Is that event modified?
    std::vector<MDLeanEvent<3> > & events2 = c2.getEvents();
    TS_ASSERT_EQUALS( events2.size(), 600);
    if (events2.size() != 600) return;
    TS_ASSERT_DELTA( events2[123].getSignal(), 456.0, 1e-5);

    // Now we GROW the event list
    events2.resize(1500);
    events2[1499].setSignal(789.0);
    // And we finish and write it out
    c2.releaseEvents();
    dbuf.flushCache();
    // The new event list should have ended up at the end of the file
    TS_ASSERT_EQUALS( c2.getFileIndexStart(), 2000);
    TS_ASSERT_EQUALS( c2.getFileNumEvents(), 1500);
    // The file has now grown.
    TS_ASSERT_EQUALS( dbuf.getFileLength(), 3500);

    // This counts the number of events actually in the file.
    TS_ASSERT_EQUALS( file->getInfo().dims[0], 3500);

    // Now let's pretend we re-load that data into a 3rd box
    MDBox<MDLeanEvent<3>,3> c3(bc, 0);
    c3.setFileIndex(2000, 1500);
    c3.setOnDisk(true);
    c3.setInMemory(false);
    // Is that event modified?
    const std::vector<MDLeanEvent<3> > & events3 = c3.getEvents();
    TS_ASSERT_EQUALS( events3.size(), 1500);
    TS_ASSERT_DELTA( events3[1499].getSignal(), 789.0, 1e-5);
    c3.releaseEvents();

    file->closeData();
    file->close();
    do_deleteNexusFile();
  }



  //-----------------------------------------------------------------------------------------
  /** If a MDBox is file-backed, test that
   * you can add events to it without having to load the data from disk.
   */
  void test_fileBackEnd_addEvent()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));
    bc->setCacheParameters(sizeof(MDLeanEvent<3>),10000);
    DiskBuffer & dbuf = bc->getDiskBuffer();

    // Create and open the test NXS file
    MDBox<MDLeanEvent<3>,3> c(bc, 0);
    ::NeXus::File * file = do_saveAndOpenNexus(c, "MDBoxTest.nxs", false);
    TSM_ASSERT_EQUALS("1000 events on file", c.getFileNumEvents(), 1000);
    TSM_ASSERT("The data was NOT loaded from disk.", !c.getInMemory());
    TSM_ASSERT_DELTA("Correct cached signal", c.getSignal(), 1000.0, 1e-3);
    TSM_ASSERT("Data is not flagged as modified", !c.dataModified());
    TSM_ASSERT("Data is not flagged as 'added'", !c.dataAdded());

    // Add an event to it
    MDLeanEvent<3> ev(1.2, 3.4);
    ev.setCenter(0, 1.5);
    ev.setCenter(1, 2.5);
    ev.setCenter(2, 3.5);
    c.addEvent(ev);
    TSM_ASSERT("Data was added", c.dataAdded());
    TSM_ASSERT_EQUALS("Still 1000 events on file", c.getFileNumEvents(), 1000);
    TSM_ASSERT_EQUALS("But now 1001 events total because they are in two places.", c.getNPoints(), 1001);
    TSM_ASSERT("The data is STILL NOT loaded from disk.", !c.getInMemory());
    TSM_ASSERT_DELTA("At this point the cached signal is still incorrect - this is normal", c.getSignal(), 1000.0, 1e-3);

    // Get the const vector of events AFTER adding events
    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
    TSM_ASSERT("The data is ALL in memory right now.", c.getInMemory());
    TSM_ASSERT("Data still flagged as added", c.dataAdded());
    TSM_ASSERT("Data is not flagged as modified (const access)", !c.dataModified());
    TSM_ASSERT_EQUALS("The resulting event vector has concatenated both", events.size(), 1001);
    TSM_ASSERT_DELTA("The first event is the one that was manually added.", events[0].getSignal(), 1.2, 1e-4);
    c.releaseEvents();

    // Flush the cache to write out the modified data
    dbuf.flushCache();
    TSM_ASSERT("Data is not flagged as modified because it was written out to disk.", !c.dataModified());
    TSM_ASSERT("Data is not flagged as added because it was written out", !c.dataAdded());
    TSM_ASSERT_EQUALS("Now there are 1001 events on file", c.getFileNumEvents(), 1001);
    TSM_ASSERT_EQUALS("And the block must have been moved since it grew", c.getFilePosition(), 2000);
    TSM_ASSERT("And the data is no longer in memory.", !c.getInMemory());
    TSM_ASSERT("And the data is on disk.", c.getOnDisk());
    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1001);
    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1001.2, 1e-3);

    TSM_ASSERT_EQUALS("The size of the file's field matches the last available point", file->getInfo().dims[0], 3001);

    // Now getEvents in a const way then call addEvent()
    const std::vector<MDLeanEvent<3> > & events2 = c.getConstEvents();
    TSM_ASSERT("Data is not flagged as modified because it was accessed as const", !c.dataModified());
    (void) events2;
    c.addEvent(ev);
    TSM_ASSERT("Data flagged as added", c.dataAdded());
    TSM_ASSERT("Data is still not flagged as modified because it was accessed as const", !c.dataModified());
    TSM_ASSERT_EQUALS("Still 1001 events on file", c.getFileNumEvents(), 1001);
    TSM_ASSERT_EQUALS("But the number of points had grown.", c.getNPoints(), 1002);
    c.releaseEvents();
    dbuf.flushCache();
    TSM_ASSERT("Data is not flagged as modified because it was written out to disk.", !c.dataModified());
    TSM_ASSERT_EQUALS("Now there are 1002 events on file", c.getFileNumEvents(), 1002);
    TSM_ASSERT_EQUALS("And the block must have been moved since it grew", c.getFilePosition(), 3001);
    TSM_ASSERT("And the data is no longer in memory.", !c.getInMemory());
    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1002);
    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1002.4, 1e-3);

    // Now getEvents in a non-const way then call addEvent()
    std::vector<MDLeanEvent<3> > & events3 = c.getEvents();
    (void) events3;
    c.addEvent(ev);
    TSM_ASSERT_EQUALS("Still 1002 events on file", c.getFileNumEvents(), 1002);
    TSM_ASSERT_EQUALS("But the number of points had grown.", c.getNPoints(), 1003);
    c.releaseEvents();
    dbuf.flushCache();
    TSM_ASSERT_EQUALS("Now there are 1003 events on file", c.getFileNumEvents(), 1003);
    TSM_ASSERT_EQUALS("And the block must have been moved since it grew", c.getFilePosition(), 2000);
    TSM_ASSERT("And the data is no longer in memory.", !c.getInMemory());
    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1003);
    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1003.6, 1e-3);

    file->close();
    do_deleteNexusFile();
  }


  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data
   * by binning and stuff */
  void do_test_fileBackEnd_binningOperations(bool parallel)
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));
    MDBox<MDLeanEvent<3>,3> c(bc, 0);

    // Create and open the test NXS file
    ::NeXus::File * file = do_saveAndOpenNexus(c, "MDBoxBinningTest.nxs", false);

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

    file->close();
    do_deleteNexusFile("MDBoxBinningTest.nxs");
  }

  void test_fileBackEnd_binningOperations()
  {
    do_test_fileBackEnd_binningOperations(false);
  }

  void xtest_fileBackEnd_binningOperations_inParallel()
  {
    do_test_fileBackEnd_binningOperations(true);
  }

  void test_getIsMasked_Default()
  {
    MDBox<MDLeanEvent<1>, 1> box;
    TSM_ASSERT("Default should be for a MDBox not to be masked!", !box.getIsMasked());
  }

  void test_mask()
  {
    MDBox<MDLeanEvent<1>, 1> box;
    TSM_ASSERT("Default should be unmasked.", !box.getIsMasked());
    TS_ASSERT_THROWS_NOTHING(box.mask());
    TSM_ASSERT("Should have been masked.", box.getIsMasked());
  }

  void test_unmask()
  {
    MDBox<MDLeanEvent<1>, 1> box;
    TSM_ASSERT("Default should be unmasked.", !box.getIsMasked());
    TS_ASSERT_THROWS_NOTHING(box.unmask());
    TSM_ASSERT("Should have been masked.", !box.getIsMasked());
  }
};


#endif

