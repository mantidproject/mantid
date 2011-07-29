#ifndef MDBOXTEST_H
#define MDBOXTEST_H

#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/CoordTransformDistance.h"
#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidNexus/NeXusFile.hpp"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::MDEvents;
using Mantid::Kernel::ConfigService;
using Mantid::Geometry::MDDimensionExtents;
using Mantid::Kernel::CPUTimer;

class MDBoxTest :    public CxxTest::TestSuite
{

public:
  void test_default_constructor()
  {
    MDBox<MDEvent<3>,3> b3;
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);
    TS_ASSERT_EQUALS( b3.getDepth(), 0);

    //std::cout << sizeof(b3) << " bytes per MDBox(3)" << std::endl;
  }

  void test_constructor()
  {
    BoxController_sptr sc( new BoxController(3));
    MDBox<MDEvent<3>,3> b3(sc, 2);
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
    extents[0].min=1.23;
    extents[0].max=2.34;
    MDBox<MDEvent<1>,1> box(sc, 2, extents);
    TS_ASSERT_EQUALS( box.getNumDims(), 1);
    TS_ASSERT_EQUALS( box.getBoxController(), sc);
    TS_ASSERT_EQUALS( box.getNPoints(), 0);
    TS_ASSERT_EQUALS( box.getDepth(), 2);
    TS_ASSERT_EQUALS( box.getNumMDBoxes(), 1);
    TS_ASSERT_DELTA( box.getExtents(0).min, 1.23, 1e-5);
    TS_ASSERT_DELTA( box.getExtents(0).max, 2.34, 1e-5);
  }


  /** Adding events tracks the total signal */
  void test_addEvent()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
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
  }


  /** Add a vector of events */
  void test_addEvents()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    std::vector< MDEvent<2> > vec;
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
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    std::vector< MDEvent<2> > vec;
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    for (size_t i=0; i<10; i++)
      vec.push_back(ev);

    b.addEvents(vec, 5, 8);
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
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);

    int num = 5e5;
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
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
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

  void test_clear()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
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
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getEvents().size(), 3);
    TS_ASSERT_EQUALS( b.getEvents()[2].getSignal(), 4.0);
  }

  void test_getEventsCopy()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    std::vector<MDEvent<2> > * events;
    events = b.getEventsCopy();
    TS_ASSERT_EQUALS( events->size(), 3);
    TS_ASSERT_EQUALS( (*events)[2].getSignal(), 4.0);
  }

  void test_sptr()
  {
    typedef MDBox<MDEvent<3>,3> mdbox3;
    TS_ASSERT_THROWS_NOTHING( mdbox3::sptr a( new mdbox3()); )
  }

  void test_bad_splitter()
  {
    BoxController_sptr sc( new BoxController(4));
    sc->setSplitThreshold(10);
    typedef MDBox<MDEvent<3>,3> MACROS_ARE_DUMB; //...since they get confused by commas
    TS_ASSERT_THROWS( MACROS_ARE_DUMB b3(sc), std::invalid_argument);
  }


  void test_splitter()
  {
    BoxController_sptr sc( new BoxController(3));
    sc->setSplitThreshold(10);
    MDBox<MDEvent<3>,3> b3(sc);
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);

    MDEvent<3> ev(1.2, 3.4);
    std::vector< MDEvent<3> > vec;
    for(size_t i=0; i < 12; i++) vec.push_back(ev);
    b3.addEvents( vec );

    TS_ASSERT_EQUALS( b3.getBoxController(), sc);
  }


  void test_centerpointBin()
  {
    MDBox<MDEvent<2>,2> box;
    for (coord_t x=0.5; x < 10.0; x += 1.0)
      for (coord_t y=0.5; y < 10.0; y += 1.0)
      {
        MDEvent<2> ev(1.0, 1.5);
        ev.setCenter(0, x);
        ev.setCenter(1, y);
        box.addEvent(ev);
      }
    TS_ASSERT_EQUALS(box.getNPoints(), 100);
    // First, a bin object that holds everything
    MDBin<MDEvent<2>,2> bin;
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
  void dotest_integrateSphere(MDBox<MDEvent<3>,3> & box, coord_t x, coord_t y, coord_t z, const coord_t radius, double numExpected)
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
    MDBox<MDEvent<3>,3> box;
    for (coord_t x=1.0; x < 10.0; x += 1.0)
      for (coord_t y=1.0; y < 10.0; y += 1.0)
        for (coord_t z=1.0; z < 10.0; z += 1.0)
        {
          MDEvent<3> ev(1.0, 1.5);
          ev.setCenter(0, x);
          ev.setCenter(1, y);
          ev.setCenter(2, z);
          box.addEvent(ev);
        }

    TS_ASSERT_EQUALS( box.getNPoints(), 9*9*9);

    dotest_integrateSphere(box, 5.0,5.0,5.0,  0.5,   1.0);
    dotest_integrateSphere(box, 0.5,0.5,0.5,  0.5,   0.0);
    dotest_integrateSphere(box, 5.0,5.0,5.0,  1.1,   7.0);
    dotest_integrateSphere(box, 5.0,5.0,5.0,  10., 9*9*9);
  }

  //-----------------------------------------------------------------------------------------
  /** refreshCache() tracks the centroid */
  void test_refreshCentroid()
  {
    MDBox<MDEvent<2>,2> b;

    MDEvent<2> ev(2.0, 2.0);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);

    MDEvent<2> ev2(4.0, 4.0);
    ev2.setCenter(0, 4.0);
    ev2.setCenter(1, 4.0);
    b.addEvent(ev2);

    // Must call the signal cache first.
    b.refreshCache();
    b.refreshCentroid();

    // This should be the weighted centroid
    TS_ASSERT_DELTA( b.getCentroid(0), 3.333, 0.001);
    TS_ASSERT_DELTA( b.getCentroid(1), 3.666, 0.001);
  }


  /** Centroid of an empty MDBox is 0.0 */
  void test_refreshCache_withCentroid_emptyMDBox()
  {
    MDBox<MDEvent<2>,2> b;
    b.refreshCache();
    b.refreshCentroid();
    TS_ASSERT_DELTA( b.getCentroid(0), 0.000, 0.001);
    TS_ASSERT_DELTA( b.getCentroid(1), 0.000, 0.001);
  }


  //-----------------------------------------------------------------------------------------
  void test_centroidSphere()
  {
    MDBox<MDEvent<2>,2> b;

    MDEvent<2> ev(2.0, 2.0);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);

    MDEvent<2> ev2(4.0, 4.0);
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
      centroid[d] /= signal;

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
      centroid[d] /= signal;
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
    MDBox<MDEvent<2>,2> b;
    MDEventsTestHelper::feedMDBox(&b, 1, 10, 0.5, 1.0);
    TS_ASSERT_EQUALS( b.getNPoints(), 100);
    b.refreshCache();
    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);
    b.setOnDisk(true);
    // Because it wasn't set, the # of points on disk is 0
    TS_ASSERT_EQUALS( b.getNPoints(), 0);
    b.setFileIndex(1234, 100);
    // Now it returns the cached number of points
    TS_ASSERT_EQUALS( b.getNPoints(), 100);
    // Still returns the signal/error
    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);

  }


  //-----------------------------------------------------------------------------------------
  /** Create a test .NXS file with some data */
  std::string do_saveNexus(bool goofyWeights = true)
  {
    // Box with 1000 events evenly spread
    MDBox<MDEvent<3>,3> b;
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
    std::string filename = (ConfigService::Instance().getString("defaultsave.directory") + "MDBoxTest.nxs");
    if (Poco::File(filename).exists())  Poco::File(filename).remove();
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_CREATE5);
    file->makeGroup("my_test_group", "NXdata", 1);

    // Must prepare the data. Make a 2000-sized array
    MDEvent<3>::prepareNexusData(file, 2000);

    // Save it with some offset
    b.setFileIndex(500, 1000);
    b.saveNexus(file);

    file->closeData();
    file->closeGroup();
    file->close();

    return filename;
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
    std::string filename = do_saveNexus();

    // Open the NXS file
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_READ);
    file->openGroup("my_test_group", "NXdata");

    // Must get ready to load in the data
    MDEvent<3>::openNexusData(file);

    // A box to load stuff from
    MDBox<MDEvent<3>,3> c;
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    CPUTimer tim;
    c.setFileIndex(500, 1000);
    c.loadNexus(file);
    std::cout << tim << " for the LoadNexus call alone." << std::endl;
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    const std::vector<MDEvent<3> > & events = c.getEvents();

    // Try a couple of events to see if they are correct
    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);

    file->close();
    if (Poco::File(filename).exists()) Poco::File(filename).remove();
  }


  //-----------------------------------------------------------------------------------------
  /** What if the box has no events, does it crash? */
  void test_loadNexus_noEvents()
  {
    // Open the NXS file
    std::string filename = do_saveNexus();
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_READ);
    file->openGroup("my_test_group", "NXdata");
    MDEvent<3>::openNexusData(file);

    // A box to load stuff from
    MDBox<MDEvent<3>,3> c;
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    c.setFileIndex(500, 0);
    c.loadNexus(file);
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    file->close();
    if (Poco::File(filename).exists()) Poco::File(filename).remove();
  }

  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data */
  void test_fileBackEnd()
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));
    MDBox<MDEvent<3>,3> c(bc, 0);

    // Open the NXS file
    std::string filename = do_saveNexus();
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_RDWR);
    file->openGroup("my_test_group", "NXdata");
    MDEvent<3>::openNexusData(file);

    // Set it in the controller for back-end
    bc->setFile(file);

    // Nothing on it to start
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    // Set the stuff that is handled outside the box itself
    c.setFileIndex(500, 1000);
    c.setOnDisk(true);
    c.setSignal(1234.5); // fake value loaded from disk
    c.setErrorSquared(456.78);

    // Now it gives the cached value
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);

    // This should actually load the events from the file
    const std::vector<MDEvent<3> > & events = c.getEvents();
    // Try a couple of events to see if they are correct
    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);

    // This won't do anything because the value is cached
    c.refreshCache();
    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);

    // OK, let's just keep it in memory
    c.setOnDisk(false);
    // Now this actually does it
    c.refreshCache();
    // The real values are back
    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
    TS_ASSERT_DELTA( c.getSignal(), 499500.0, 1e-2);
    TS_ASSERT_DELTA( c.getErrorSquared(), 500000.0, 1e-2);

    // Pretend we're letting go of the events. This should clear the list
    c.setOnDisk(true);
    c.releaseEvents();

    c.setOnDisk(false);
    TS_ASSERT_EQUALS( c.getNPoints(), 0);

    file->close();
    if (Poco::File(filename).exists()) Poco::File(filename).remove();
  }



  //-----------------------------------------------------------------------------------------
  /** Set up the file back end and test accessing data */
  void do_test_fileBackEnd_binningOperations(bool parallel)
  {
    // Create a box with a controller for the back-end
    BoxController_sptr bc(new BoxController(3));
    MDBox<MDEvent<3>,3> c(bc, 0);

    // Open the NXS file
    std::string filename = do_saveNexus(false);
    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_RDWR);
    file->openGroup("my_test_group", "NXdata");
    MDEvent<3>::openNexusData(file);

    // Set it in the controller for back-end
    bc->setFile(file);
    // Set the stuff that is handled outside the box itself
    c.setFileIndex(500, 1000);
    c.setOnDisk(true);

    PARALLEL_FOR_IF(parallel)
    for (int i=0; i<20; i++)
    {
      //std::cout << "Bin try " << i << "\n";
      // Try a bin, 2x2x2 so 8 events should be in there
      MDBin<MDEvent<3>,3> bin;
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

  }

  void test_fileBackEnd_binningOperations()
  {
    do_test_fileBackEnd_binningOperations(false);
  }

  void xtest_fileBackEnd_binningOperations_inParallel()
  {
    do_test_fileBackEnd_binningOperations(true);
  }
};


#endif

