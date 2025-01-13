// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/MDBin.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidNexusCpp/NeXusFile.hpp"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class MDBoxTest : public CxxTest::TestSuite {
  BoxController_sptr sc;

  MDBoxTest() { sc = BoxController_sptr(new BoxController(3)); }

public:
  static MDBoxTest *createSuite() { return new MDBoxTest(); }
  static void destroySuite(MDBoxTest *suite) { delete suite; }

  void test_default_constructor() {
    MDBox<MDLeanEvent<3>, 3> b3(sc.get());
    TS_ASSERT_EQUALS(b3.getNumDims(), 3);
    TS_ASSERT_EQUALS(b3.getNPoints(), 0);
    TS_ASSERT_EQUALS(b3.getDepth(), 0);
  }

  void test_constructor() {

    MDBox<MDLeanEvent<3>, 3> b3(sc.get(), 2);
    TS_ASSERT_EQUALS(b3.getNumDims(), 3);
    TS_ASSERT(b3.getBoxController() == sc.get());
    TS_ASSERT_EQUALS(b3.getNPoints(), 0);
    TS_ASSERT_EQUALS(b3.getDepth(), 2);
    TS_ASSERT_EQUALS(b3.getNumMDBoxes(), 1);
  }

  void test_constructorWithExtents() {
    BoxController_sptr sc(new BoxController(1));
    std::vector<MDDimensionExtents<coord_t>> extents(1);
    extents[0].setExtents(123, 234);
    MDBox<MDLeanEvent<1>, 1> box(sc.get(), 2, extents);
    TS_ASSERT_EQUALS(box.getNumDims(), 1);
    TS_ASSERT(box.getBoxController() == sc.get());
    TS_ASSERT_EQUALS(box.getNPoints(), 0);
    TS_ASSERT_EQUALS(box.getDepth(), 2);
    TS_ASSERT_EQUALS(box.getNumMDBoxes(), 1);
    TS_ASSERT_DELTA(box.getExtents(0).getMin(), 123, 1e-5);
    TS_ASSERT_DELTA(box.getExtents(0).getMax(), 234, 1e-5);
  }

  void test_copy_constructor() {
    BoxController_sptr sc(new BoxController(1));
    std::vector<MDDimensionExtents<coord_t>> extents(1);
    extents[0].setExtents(123, 234);
    MDBox<MDLeanEvent<1>, 1> box1(sc.get(), 2, extents);
    MDLeanEvent<1> ev(1.23, 2.34);
    for (size_t i = 0; i < 15; i++) {
      ev.setCenter(0, static_cast<coord_t>(i));
      box1.addEvent(ev);
    }
    // Do the copy
    MDBox<MDLeanEvent<1>, 1> box2(box1, box1.getBoxController());
    // Compare
    std::vector<MDLeanEvent<1>> events = box2.getEvents();
    TS_ASSERT_EQUALS(box2.getNumDims(), 1);
    TS_ASSERT(box2.getBoxController() == sc.get());
    TS_ASSERT_EQUALS(box2.getNPoints(), 15);
    TS_ASSERT_EQUALS(events.size(), 15);
    TS_ASSERT_DELTA(events[7].getCenter(0), 7.0, 1e-4);
    TS_ASSERT_EQUALS(box2.getDepth(), 2);
    TS_ASSERT_EQUALS(box2.getNumMDBoxes(), 1);
    TS_ASSERT_DELTA(box2.getExtents(0).getMin(), 123, 1e-5);
    TS_ASSERT_DELTA(box2.getExtents(0).getMax(), 234, 1e-5);

    TS_ASSERT_EQUALS(box1.getBoxController(), box2.getBoxController());
  }

  /** Adding events tracks the total signal */
  void test_addEvent() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    TS_ASSERT_EQUALS(1, b.addEvent(ev));
    TS_ASSERT_EQUALS(b.getNPoints(), 1)

    b.refreshCache();

    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * 1, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * 1, 1e-5);
    // Weight of 1.0 per event.
    TS_ASSERT_EQUALS(b.getTotalWeight(), 1.0);
  }
  /** Adding events tracks the total signal */
  void test_BuildAndAddEvent() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    std::vector<coord_t> coord(2, 2.);
    coord[1] = 3;

    b.buildAndAddEvent(1.2, 3.4, coord, 0, 0, 0);
    TS_ASSERT_EQUALS(b.getNPoints(), 1)

    b.refreshCache();

    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * 1, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * 1, 1e-5);
    // Weight of 1.0 per event.
    TS_ASSERT_EQUALS(b.getTotalWeight(), 1.0);
  }

  /** Adding events in unsafe way also works */
  void test_addEventUnsafe() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    TS_ASSERT_EQUALS(1, b.addEventUnsafe(ev));
    TS_ASSERT_EQUALS(b.getNPoints(), 1)

    b.refreshCache();

    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * 1, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * 1, 1e-5);
  }

  /** Add a vector of events */
  void test_addEvents() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    MDLeanEvent<2> ev(1.2, 3.4);
    std::vector<MDLeanEvent<2>> vec;
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    vec.emplace_back(ev);
    vec.emplace_back(ev);
    vec.emplace_back(ev);
    b.addEvents(vec);

    b.refreshCache();

    TS_ASSERT_EQUALS(b.getNPoints(), 3)
    TS_ASSERT_DELTA(b.getEvents()[2].getSignal(), 1.2, 1e-5)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * 3, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * 3, 1e-5);
  }

  /** Add a vector of events */
  void test_BuildAndAddLeanEvents() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    std::vector<signal_t> SigErrSq(3 * 2, 1.2);
    std::vector<coord_t> Coord(3 * 2, 2);
    std::vector<uint16_t> ind;
    std::vector<uint16_t> goniometerIndex;
    std::vector<uint32_t> detID;
    SigErrSq[1] = SigErrSq[3] = SigErrSq[5] = 3.4;
    Coord[1] = Coord[3] = Coord[5] = 3.0;

    b.buildAndAddEvents(SigErrSq, Coord, ind, goniometerIndex, detID);

    b.refreshCache();

    TS_ASSERT_EQUALS(b.getNPoints(), 3)
    TS_ASSERT_DELTA(b.getEvents()[2].getSignal(), 1.2, 1e-5)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * 3, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * 3, 1e-5);
  }

  /** Add a vector of events */
  void test_BuildAndAddFatEvents() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDEvent<2>, 2> b(sc.get());
    std::vector<signal_t> SigErrSq(3 * 2, 1.2);
    std::vector<coord_t> Coord(3 * 2, 2);
    std::vector<uint16_t> ind(3, 10);
    std::vector<uint16_t> goniometerIndex(3, 42);
    std::vector<uint32_t> detID(3, 20);
    SigErrSq[1] = SigErrSq[3] = SigErrSq[5] = 3.4;
    Coord[1] = Coord[3] = Coord[5] = 3.0;

    b.buildAndAddEvents(SigErrSq, Coord, ind, goniometerIndex, detID);

    b.refreshCache();

    TS_ASSERT_EQUALS(b.getNPoints(), 3)
    TS_ASSERT_DELTA(b.getEvents()[2].getSignal(), 1.2, 1e-5)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * 3, 1e-5);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * 3, 1e-5);

    TS_ASSERT_EQUALS(b.getEvents()[2].getExpInfoIndex(), 10);
    TS_ASSERT_EQUALS(b.getEvents()[2].getGoniometerIndex(), 42);
    TS_ASSERT_EQUALS(b.getEvents()[2].getDetectorID(), 20);
  }

  ///** We hopefully do not need this
  //   Add a vector of events and give start/end spots*/
  // void xest_addEvents_with_start_stop()
  //{
  //  BoxController_sptr sc( new BoxController(2));
  //  MDBox<MDLeanEvent<2>,2> b(sc.get());
  //  MDLeanEvent<2> ev(1.2, 3.4);
  //  std::vector< MDLeanEvent<2> > vec;
  //  ev.setCenter(0, 2.0);
  //  ev.setCenter(1, 3.0);
  //  for (size_t i=0; i<10; i++)
  //    vec.emplace_back(ev);

  //  b.addEvents(vec, 5, 8);
  //  b.refreshCache();

  //  TS_ASSERT_EQUALS( b.getNPoints(), 3)
  //  TS_ASSERT_DELTA( b.getEvents()[2].getSignal(), 1.2, 1e-5)
  //  // Did it keep a running total of the signal and error?
  //  TS_ASSERT_DELTA( b.getSignal(), 1.2*3, 1e-5);
  //  TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*3, 1e-5);
  //}

  /** Try to add a large number of events in parallel
   * to the same MDBox, to make sure it is thread-safe.
   */
  void test_addEvent_inParallel() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);

    int num = 500000;
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < num; i++) {
      b.addEvent(ev);
    }

    b.refreshCache();

    TS_ASSERT_EQUALS(b.getNPoints(), num)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * num, 1e-5 * num);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * num, 1e-5 * num);
  }

  /** Try to add a large number of events in parallel
   * to the same MDBox, to make sure it is thread-safe.
   */
  void test_BuildAndAddEvent_inParallel() {
    BoxController_sptr sc(new BoxController(4));
    MDBox<MDLeanEvent<4>, 4> b(sc.get());
    std::vector<coord_t> Coord(4, 2.);

    int num = 500000;
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < num; i++) {
      b.buildAndAddEvent(1.2, 3.4, Coord, 1, 0, 10);
    }

    b.refreshCache();

    TS_ASSERT_EQUALS(b.getNPoints(), num)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA(b.getSignal(), 1.2 * num, 1e-5 * num);
    TS_ASSERT_DELTA(b.getErrorSquared(), 3.4 * num, 1e-5 * num);
  }

  void test_calculateDimensionStats() {
    MDDimensionStats stats[2];
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    MDLeanEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);
    ev.setCenter(0, 4.0);
    ev.setCenter(1, 5.0);
    b.addEvent(ev);
    TS_ASSERT_THROWS_NOTHING(b.calculateDimensionStats(stats);)
    TS_ASSERT_DELTA(stats[0].getMean(), 3.0, 1e-3);
    TS_ASSERT_DELTA(stats[1].getMean(), 4.0, 1e-3);
    TS_ASSERT_DELTA(stats[0].getApproxVariance(), 0.5, 1e-3);
    TS_ASSERT_DELTA(stats[1].getApproxVariance(), 0.5, 1e-3);
  }

  void test_transformDimensions() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
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
    const std::vector<MDLeanEvent<2>> events = b.getConstEvents();
    TS_ASSERT_DELTA(events[0].getCenter(0), 7.0, 1e-3);
    TS_ASSERT_DELTA(events[0].getCenter(1), 10.0, 1e-3);
    TS_ASSERT_DELTA(events[1].getCenter(0), 13.0, 1e-3);
    TS_ASSERT_DELTA(events[1].getCenter(1), 16.0, 1e-3);
    b.releaseEvents();
  }

  void test_clear() {
    BoxController_sptr bc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(bc.get());
    MDLeanEvent<2> ev(1.2, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);

    b.refreshCache();

    TS_ASSERT_EQUALS(b.getNPoints(), 2)
    TS_ASSERT_DELTA(b.getSignal(), 2.4, 1e-5)
    b.clear();
    TS_ASSERT_EQUALS(b.getNPoints(), 0)
    TS_ASSERT_DELTA(b.getSignal(), 0.0, 1e-5)
    TS_ASSERT_DELTA(b.getErrorSquared(), 0.0, 1e-5)
  }

  void test_getEvents() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    MDLeanEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    TS_ASSERT_EQUALS(b.getEvents().size(), 3);
    TS_ASSERT_EQUALS(b.getEvents()[2].getSignal(), 4.0);
  }

  void test_getEventsCopy() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    MDLeanEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    std::vector<MDLeanEvent<2>> *events = b.getEventsCopy();
    TS_ASSERT_EQUALS(events->size(), 3);
    TS_ASSERT_EQUALS((*events)[2].getSignal(), 4.0);
    delete events;
  }

  void test_sptr() {
    using mdbox3 = MDBox<MDLeanEvent<3>, 3>;
    TS_ASSERT_THROWS_NOTHING(mdbox3::sptr a(new mdbox3(sc.get()));)
  }

  void test_bad_splitter() {
    BoxController_sptr sc(new BoxController(4));
    sc->setSplitThreshold(10);
    using MACROS_ARE_DUMB = MDBox<MDLeanEvent<3>, 3>; //...since they get confused by commas
    TS_ASSERT_THROWS(MACROS_ARE_DUMB b3(sc.get()), const std::invalid_argument &);
  }

  void test_splitter() {
    BoxController_sptr sc(new BoxController(3));
    sc->setSplitThreshold(10);
    MDBox<MDLeanEvent<3>, 3> b3(sc.get());
    TS_ASSERT_EQUALS(b3.getNumDims(), 3);
    TS_ASSERT_EQUALS(b3.getNPoints(), 0);

    MDLeanEvent<3> ev(1.2, 3.4);
    std::vector<MDLeanEvent<3>> vec;
    for (size_t i = 0; i < 12; i++)
      vec.emplace_back(ev);
    b3.addEvents(vec);

    TS_ASSERT(b3.getBoxController() == sc.get());
  }

  void test_centerpointBin() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> box(sc.get());
    for (double x = 0.5; x < 10.0; x += 1.0)
      for (double y = 0.5; y < 10.0; y += 1.0) {
        MDLeanEvent<2> ev(1.0, 1.5);
        ev.setCenter(0, static_cast<coord_t>(x));
        ev.setCenter(1, static_cast<coord_t>(y));
        box.addEvent(ev);
      }
    TS_ASSERT_EQUALS(box.getNPoints(), 100);
    // First, a bin object that holds everything
    MDBin<MDLeanEvent<2>, 2> bin;
    // Perform the centerpoint binning
    box.centerpointBin(bin, nullptr);
    // 100 events = 100 weight.
    TS_ASSERT_DELTA(bin.m_signal, 100.0, 1e-4);
    TS_ASSERT_DELTA(bin.m_errorSquared, 150.0, 1e-4);

    // Next, a more restrictive bin. a 2.0 x 2.0 square, with 4 events
    bin.m_signal = 0;
    bin.m_errorSquared = 0;
    bin.m_min[0] = 4.0;
    bin.m_max[0] = 6.0;
    bin.m_min[1] = 1.0;
    bin.m_max[1] = 3.0;
    box.centerpointBin(bin, nullptr);
    TS_ASSERT_DELTA(bin.m_signal, 4.0, 1e-4);
    TS_ASSERT_DELTA(bin.m_errorSquared, 6.0, 1e-4);
  }

  /** For test_integrateSphere,
   *
   * @param box
   * @param radius :: radius to integrate
   * @param numExpected :: how many events should be in there
   */
  void dotest_integrateSphere(MDBox<MDLeanEvent<3>, 3> &box, coord_t x, coord_t y, coord_t z, const coord_t radius,
                              double numExpected) {
    // The sphere transformation
    bool dimensionsUsed[3] = {true, true, true};
    coord_t center[3] = {x, y, z};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    signal_t signal = 0;
    signal_t errorSquared = 0;
    box.integrateSphere(sphere, radius * radius, signal, errorSquared);
    TS_ASSERT_DELTA(signal, 1.0 * numExpected, 1e-5);
    TS_ASSERT_DELTA(errorSquared, 1.5 * numExpected, 1e-5);
  }

  void dotest_integrateSphereWithInnerRadius(MDBox<MDLeanEvent<3>, 3> &box, coord_t x, coord_t y, coord_t z,
                                             const coord_t radius, const coord_t innerRadius, const bool useOnePercent,
                                             double numExpected) {
    // The sphere transformation
    bool dimensionsUsed[3] = {true, true, true};
    coord_t center[3] = {x, y, z};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    signal_t signal = 0;
    signal_t errorSquared = 0;
    box.integrateSphere(sphere, radius * radius, signal, errorSquared, innerRadius * innerRadius, useOnePercent);
    TS_ASSERT_DELTA(signal, 1.0 * numExpected, 1e-5);
    TS_ASSERT_DELTA(errorSquared, 1.5 * numExpected, 1e-5);
  }

  void test_integrateSphere() {
    // One event at each integer coordinate value between 1 and 9
    MDBox<MDLeanEvent<3>, 3> box(sc.get());
    for (double x = 1.0; x < 10.0; x += 1.0)
      for (double y = 1.0; y < 10.0; y += 1.0)
        for (double z = 1.0; z < 10.0; z += 1.0) {
          MDLeanEvent<3> ev(1.0, 1.5);
          ev.setCenter(0, x);
          ev.setCenter(1, y);
          ev.setCenter(2, z);
          box.addEvent(ev);
        }

    TS_ASSERT_EQUALS(box.getNPoints(), 9 * 9 * 9);

    dotest_integrateSphere(box, 5.0, 5.0, 5.0, 0.5, 1.0);
    dotest_integrateSphere(box, 0.5, 0.5, 0.5, 0.5, 0.0);
    dotest_integrateSphere(box, 5.0, 5.0, 5.0, 1.1f, 7.0);
    dotest_integrateSphere(box, 5.0, 5.0, 5.0, 10., 9 * 9 * 9);
  }

  void test_integrateSphereWithLowerRadiusBoundAndNoOnePercentCutoff() {
    // One event at each integer coordinate value between 1 and 9
    MDBox<MDLeanEvent<3>, 3> box(sc.get());
    for (double x = 1.0; x < 10.0; x += 1.0)
      for (double y = 1.0; y < 10.0; y += 1.0)
        for (double z = 1.0; z < 10.0; z += 1.0) {
          MDLeanEvent<3> ev(1.0, 1.5);
          ev.setCenter(0, x);
          ev.setCenter(1, y);
          ev.setCenter(2, z);
          box.addEvent(ev);
        }

    TS_ASSERT_EQUALS(box.getNPoints(), 9 * 9 * 9);

    // Too small shell
    dotest_integrateSphereWithInnerRadius(box, 5.0f, 5.0f, 5.0f, 0.5f, 0.4f, false, 0.0);

    // The 0.7 shell contains 2 but the 1.15 inner shell excludes one of them
    dotest_integrateSphereWithInnerRadius(box, 5.6f, 5.0f, 5.0f, 0.7f, 0.5f, false, 1.0);

    // The 1.3 shell contains 2 and the 1.05 inner shell contains 2
    dotest_integrateSphereWithInnerRadius(box, 5.6f, 5.0f, 5.0f, 0.7f, 0.4f, false, 2.0);
  }

  //-----------------------------------------------------------------------------------------
  /** refreshCache() tracks the centroid */
  void test_calculateCentroid() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());

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
    coord_t centroid[2];
    b.calculateCentroid(centroid);
    TS_ASSERT_DELTA(centroid[0], 3.333, 0.001);
    TS_ASSERT_DELTA(centroid[1], 3.666, 0.001);

    //    b.refreshCentroid();
    // #ifdef MDBOX_TRACK_CENTROID
    //    // This should be the weighted centroid
    //    TS_ASSERT_DELTA( b.getCentroid(0), 3.333, 0.001);
    //    TS_ASSERT_DELTA( b.getCentroid(1), 3.666, 0.001);
    // #endif
  }

  /** Centroid of an empty MDBox is 0.0 */
  void test_refreshCache_withCentroid_emptyMDBox() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());
    b.refreshCache();

    coord_t centroid[2];
    b.calculateCentroid(centroid);
    TS_ASSERT_DELTA(centroid[0], 0.000, 0.001);
    TS_ASSERT_DELTA(centroid[1], 0.000, 0.001);
  }

  //-----------------------------------------------------------------------------------------
  void test_centroidSphere() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());

    MDLeanEvent<2> ev(2.0, 2.0);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);

    MDLeanEvent<2> ev2(4.0, 4.0);
    ev2.setCenter(0, 4.0);
    ev2.setCenter(1, 4.0);
    b.addEvent(ev2);

    // The sphere transformation
    bool dimensionsUsed[2] = {true, true};
    coord_t center[2] = {0, 0};
    CoordTransformDistance sphere(2, center, dimensionsUsed);

    // Set up the data for the centroid
    coord_t centroid[2] = {0, 0};
    signal_t signal = 0.0;
    b.centroidSphere(sphere, 400., centroid, signal);
    for (float &d : centroid)
      d /= static_cast<coord_t>(signal);

    // This should be the weighted centroid
    TS_ASSERT_DELTA(signal, 6.000, 0.001);
    TS_ASSERT_DELTA(centroid[0], 3.333, 0.001);
    TS_ASSERT_DELTA(centroid[1], 3.666, 0.001);

    // --- Reset and reduce the radius ------
    signal = 0;
    for (float &d : centroid)
      d = 0.0;
    b.centroidSphere(sphere, 16., centroid, signal);
    for (float &d : centroid)
      d /= static_cast<coord_t>(signal);
    // Only one event was contained
    TS_ASSERT_DELTA(signal, 2.000, 0.001);
    TS_ASSERT_DELTA(centroid[0], 2.000, 0.001);
    TS_ASSERT_DELTA(centroid[1], 3.000, 0.001);
  }

  void test_getIsMasked_Default() {
    BoxController_sptr sc(new BoxController(1));
    MDBox<MDLeanEvent<1>, 1> box(sc.get());
    TSM_ASSERT("Default should be for a MDBox not to be masked!", !box.getIsMasked());
  }

  void test_mask() {
    BoxController_sptr sc(new BoxController(1));
    MDBox<MDLeanEvent<1>, 1> box(sc.get());
    TSM_ASSERT("Default should be unmasked.", !box.getIsMasked());
    TS_ASSERT_THROWS_NOTHING(box.mask());
    TSM_ASSERT("Should have been masked.", box.getIsMasked());
  }

  void test_unmask() {
    BoxController_sptr sc(new BoxController(1));
    MDBox<MDLeanEvent<1>, 1> box(sc.get());
    TSM_ASSERT("Default should be unmasked.", !box.getIsMasked());
    TS_ASSERT_THROWS_NOTHING(box.unmask());
    TSM_ASSERT("Should have been masked.", !box.getIsMasked());
  }

  void test_reserve() {
    BoxController_sptr sc(new BoxController(2));
    MDBox<MDLeanEvent<2>, 2> b(sc.get());

    b.reserveMemoryForLoad(3);
    TS_ASSERT_EQUALS(b.getEvents().capacity(), 3);
  }
};
