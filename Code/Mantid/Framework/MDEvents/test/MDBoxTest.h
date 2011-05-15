#ifndef MDBOXTEST_H
#define MDBOXTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MultiThreaded.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/BoxController.h"
#include <memory>
#include <map>
#include "MantidMDEvents/CoordTransformDistance.h"

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDBoxTest :    public CxxTest::TestSuite
{

public:
  void test_default_constructor()
  {
    MDBox<MDEvent<3>,3> b3;
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);
    TS_ASSERT_EQUALS( b3.getDepth(), 0);
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

  /** Adding events tracks the total signal */
  void test_addEvent()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getNPoints(), 1)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*1, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*1, 1e-5);
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

    TS_ASSERT_EQUALS( b.getNPoints(), num)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*num, 1e-5*num);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*num, 1e-5*num);

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
};


#endif

