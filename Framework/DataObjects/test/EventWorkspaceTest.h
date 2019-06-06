// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * EventWorkspaceTest.h
 *
 *  Created on: May 28, 2010
 *      Author: Janik Zikovsky
 */

#ifndef EVENTWORKSPACETEST_H_
#define EVENTWORKSPACETEST_H_

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include <string>

#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "PropertyManagerHelper.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Exception;
using namespace Mantid::API;

using std::cout;
using std::runtime_error;
using std::size_t;
using std::vector;
using namespace boost::posix_time;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::LinearGenerator;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

class EventWorkspaceTest : public CxxTest::TestSuite {
private:
  EventWorkspace_sptr ew;
  int NUMPIXELS, NUMBINS, NUMEVENTS, BIN_DELTA;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventWorkspaceTest *createSuite() { return new EventWorkspaceTest(); }
  static void destroySuite(EventWorkspaceTest *suite) { delete suite; }

  EventWorkspaceTest() {
    NUMPIXELS = 500;
    NUMBINS = 1025;
    NUMEVENTS = 100;
    BIN_DELTA = 1000;
  }

  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   */
  EventWorkspace_sptr createEventWorkspace(bool initialize_pixels, bool setX,
                                           bool evenTOFs = false) {

    EventWorkspace_sptr retVal(new EventWorkspace);
    if (initialize_pixels) {
      retVal->initialize(NUMPIXELS, 1, 1);

      // Make fake events
      for (int pix = 0; pix < NUMPIXELS; pix++) {
        for (int i = 0; i < NUMBINS - 1; i++) {
          double tof = 0;
          if (evenTOFs) {
            tof = (i + 0.5) * BIN_DELTA;
          } else {
            // Two events per bin
            tof = (pix + i + 0.5) * BIN_DELTA;
          }
          size_t pulse_time = static_cast<size_t>(tof);
          retVal->getSpectrum(pix) += TofEvent(tof, pulse_time);
          retVal->getSpectrum(pix) += TofEvent(tof, pulse_time);
        }
        retVal->getSpectrum(pix).addDetectorID(pix);
        retVal->getSpectrum(pix).setSpectrumNo(pix);
      }
    } else {
      retVal->initialize(1, 1, 1);
    }

    if (setX) {
      // Create the x-axis for histogramming.
      BinEdges axis(NUMBINS, LinearGenerator(0.0, BIN_DELTA));

      // Try setting a single axis, make sure it doesn't throw
      retVal->setX(2, axis.cowData());

      // Set all the histograms at once.
      retVal->setAllX(axis);
    }

    return retVal;
  }

  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   * 2 events per bin
   */
  EventWorkspace_sptr createFlatEventWorkspace() {
    return createEventWorkspace(true, true, true);
  }

  void setUp() override { ew = createEventWorkspace(true, true); }

  void test_constructor() {
    TS_ASSERT_EQUALS(ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS(ew->blocksize(), NUMBINS - 1);
    TS_ASSERT_EQUALS(ew->size(), (NUMBINS - 1) * NUMPIXELS);

    // Are the returned arrays the right size?
    const EventList el(ew->getSpectrum(1));
    TS_ASSERT_EQUALS(el.readX().size(), NUMBINS);
    boost::scoped_ptr<MantidVec> Y(el.makeDataY());
    boost::scoped_ptr<MantidVec> E(el.makeDataE());
    TS_ASSERT_EQUALS(Y->size(), NUMBINS - 1);
    TS_ASSERT_EQUALS(E->size(), NUMBINS - 1);
    TS_ASSERT(el.hasDetectorID(1));
  }

  void test_getMemorySize() {
    // Because of the way vectors allocate, we can only know the minimum amount
    // of memory that can be used.
    size_t min_memory = (ew->getNumberEvents() * sizeof(TofEvent) +
                         NUMPIXELS * sizeof(EventList));
    TS_ASSERT_LESS_THAN_EQUALS(min_memory, ew->getMemorySize());
  }

  void testUnequalBins() {
    ew = createEventWorkspace(true, false);
    // normal behavior
    TS_ASSERT_EQUALS(ew->blocksize(), 1);
    TS_ASSERT(ew->isCommonBins());
    TS_ASSERT_EQUALS(ew->size(), 500);

    // set the first histogram to have 2 bins
    ew->getSpectrum(0).setHistogram(BinEdges({0., 10., 20.}));
    TS_ASSERT_THROWS(ew->blocksize(), const std::logic_error &);
    TS_ASSERT(!(ew->isCommonBins()));
    TS_ASSERT_EQUALS(ew->size(), 501);
  }

  void test_destructor() {
    EventWorkspace *ew2 = new EventWorkspace();
    delete ew2;
  }

  void test_constructor_setting_default_x() {
    // Do the workspace, but don't set x explicity
    ew = createEventWorkspace(true, false);
    TS_ASSERT_EQUALS(ew->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS(ew->blocksize(), 1);
    TS_ASSERT_EQUALS(ew->size(), 500);

    // Didn't set X? well all the histograms show a single bin
    const EventList el(ew->getSpectrum(1));
    TS_ASSERT_EQUALS(el.readX().size(), 2);
    TS_ASSERT_EQUALS(el.readX()[0], 0.0);
    TS_ASSERT_EQUALS(el.readX()[1], std::numeric_limits<double>::min());
    boost::scoped_ptr<MantidVec> Y(el.makeDataY());
    TS_ASSERT_EQUALS(Y->size(), 1);
    TS_ASSERT_EQUALS((*Y)[0], 0.0);
    boost::scoped_ptr<MantidVec> E(el.makeDataE());
    TS_ASSERT_EQUALS(E->size(), 1);
    TS_ASSERT_EQUALS((*E)[0], 0.0);
  }

  void test_maskWorkspaceIndex() {
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
            1, 10, false /*dont clear the events*/);
    TS_ASSERT_EQUALS(ws->getSpectrum(2).getNumberEvents(), 200);
    ws->getSpectrum(2).clearData();
    ws->mutableSpectrumInfo().setMasked(2, true);
    TS_ASSERT_EQUALS(ws->getSpectrum(2).getNumberEvents(), 0);
  }

  void test_uneven_pixel_ids() {
    EventWorkspace_sptr uneven(new EventWorkspace);
    uneven->initialize(NUMPIXELS / 10, 1, 1);

    // Make fake events. Pixel IDs start at 5 increment by 10
    size_t wi = 0;
    for (int pix = 5; pix < NUMPIXELS; pix += 10) {
      for (int i = 0; i < pix; i++) {
        uneven->getSpectrum(wi) += TofEvent((pix + i + 0.5) * BIN_DELTA, 1);
      }
      uneven->getSpectrum(wi).addDetectorID(pix);
      uneven->getSpectrum(wi).setSpectrumNo(pix);
      wi++;
    }

    // Set all the histograms at once.
    uneven->setAllX(BinEdges(NUMBINS, LinearGenerator(0.0, BIN_DELTA)));

    TS_ASSERT_EQUALS(uneven->getNumberHistograms(), NUMPIXELS / 10);
    TS_ASSERT_EQUALS(uneven->blocksize(), (NUMBINS - 1));
    TS_ASSERT_EQUALS(uneven->size(), (NUMBINS - 1) * NUMPIXELS / 10);

    // Axis 1 is the map between spectrum # and the workspace index.
    TS_ASSERT_EQUALS(uneven->getAxis(1)->spectraNo(0), 5);
    TS_ASSERT_EQUALS(uneven->getAxis(1)->spectraNo(5), 55);
    TS_ASSERT_EQUALS(uneven->getAxis(1)->length(), NUMPIXELS / 10);

    // The spectra map should take each workspace index and point to the right
    // pixel id: 5,15,25, etc.
    for (int wi = 0; wi < static_cast<int>(uneven->getNumberHistograms());
         wi++) {
      TS_ASSERT_EQUALS(*uneven->getSpectrum(wi).getDetectorIDs().begin(),
                       5 + wi * 10);
    }

    // Workspace index 0 is at pixelid 5 and has 5 events
    const EventList el0(uneven->getSpectrum(0));
    TS_ASSERT_EQUALS(el0.getNumberEvents(), 5);
    // And so on, the # of events = pixel ID
    const EventList el1(uneven->getSpectrum(1));
    TS_ASSERT_EQUALS(el1.getNumberEvents(), 15);
    const EventList el5(uneven->getSpectrum(5));
    TS_ASSERT_EQUALS(el5.getNumberEvents(), 55);

    // Out of range
    TS_ASSERT_THROWS(uneven->dataX(-3), const std::range_error &);
    TS_ASSERT_THROWS(uneven->dataX(NUMPIXELS / 10), const std::range_error &);
  }

  void test_data_access() {
    // Non-const access throws errors for Y & E - not for X
    TS_ASSERT_THROWS_NOTHING(ew->dataX(1));
    TS_ASSERT_THROWS(ew->dataY(2), const NotImplementedError &);
    TS_ASSERT_THROWS(ew->dataE(3), const NotImplementedError &);
    // Out of range
    TS_ASSERT_THROWS(ew->dataX(-123), const std::range_error &);
    TS_ASSERT_THROWS(ew->dataX(5123), const std::range_error &);
    TS_ASSERT_THROWS(ew->dataE(5123), const NotImplementedError &);
    TS_ASSERT_THROWS(ew->dataY(5123), const NotImplementedError &);

    // Can't try the const access; copy constructors are not allowed.
  }

  void test_setX_individually() {
    // Create A DIFFERENT x-axis for histogramming.
    auto axis = Kernel::make_cow<HistogramData::HistogramX>(
        NUMBINS / 2, LinearGenerator(0.0, 2.0 * BIN_DELTA));

    ew->setX(0, axis);
    const EventList el(ew->getSpectrum(0));
    TS_ASSERT_EQUALS(el.readX()[0], 0);
    TS_ASSERT_EQUALS(el.readX()[1], BIN_DELTA * 2);

    // Are the returned arrays the right size?
    TS_ASSERT_EQUALS(el.readX().size(), NUMBINS / 2);

    boost::scoped_ptr<MantidVec> Y(el.makeDataY());
    boost::scoped_ptr<MantidVec> E(el.makeDataE());
    TS_ASSERT_EQUALS(Y->size(), NUMBINS / 2 - 1);
    TS_ASSERT_EQUALS(E->size(), NUMBINS / 2 - 1);

    // Now there are 4 events in each bin
    TS_ASSERT_EQUALS((*Y)[0], 4);
    TS_ASSERT_EQUALS((*Y)[NUMBINS / 2 - 2], 4);

    // But pixel 1 is the same, 2 events in the bin
    const EventList el1(ew->getSpectrum(1));
    TS_ASSERT_EQUALS(el1.readX()[1], BIN_DELTA * 1);
    boost::scoped_ptr<MantidVec> Y1(el1.makeDataY());
    TS_ASSERT_EQUALS((*Y1)[1], 2);
  }

  void testIntegrateSpectra_entire_range() {
    EventWorkspace_sptr ws = createFlatEventWorkspace();
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 0, 0, true);
    for (int i = 0; i < NUMPIXELS; ++i) {
      TS_ASSERT_EQUALS(sums[i], (NUMBINS - 1) * 2.0);
      ;
    }
  }
  void testIntegrateSpectra_empty_range() {
    EventWorkspace_sptr ws = createFlatEventWorkspace();
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 10, 5, false);
    for (int i = 0; i < NUMPIXELS; ++i) {
      TS_ASSERT_EQUALS(sums[i], 0.0);
      ;
    }
  }

  void testIntegrateSpectra_partial_range() {
    EventWorkspace_sptr ws = createFlatEventWorkspace();
    MantidVec sums;
    // This will include a single bin
    ws->getIntegratedSpectra(sums, BIN_DELTA * 1.9, BIN_DELTA * 3.1, false);
    for (int i = 0; i < NUMPIXELS; ++i) {
      TS_ASSERT_EQUALS(sums[i], 2);
      ;
    }
  }

  void test_histogram_cache() {
    // Try caching and most-recently-used MRU list.
    EventWorkspace_const_sptr ew2 =
        boost::dynamic_pointer_cast<const EventWorkspace>(ew);

    // Are the returned arrays the right size?
    MantidVec data1 = ew2->dataY(1);
    TS_ASSERT_EQUALS(data1.size(), NUMBINS - 1);
    // A single cached value now
    TS_ASSERT_EQUALS(ew2->MRUSize(), 1);

    // This should get the cached one
    MantidVec data2 = ew2->dataY(1);
    TS_ASSERT_EQUALS(data2.size(), NUMBINS - 1);
    // Still a single cached value
    TS_ASSERT_EQUALS(ew2->MRUSize(), 1);

    // All elements are the same
    for (std::size_t i = 0; i < data1.size(); i++)
      TS_ASSERT_EQUALS(data1[i], data2[i]);

    // Now test the caching. The first 100 will load in memory
    for (int i = 0; i < 100; i++)
      data1 = ew2->dataY(i);

    // Check the bins contain 2
    data1 = ew2->dataY(0);
    TS_ASSERT_DELTA(ew2->dataY(0)[1], 2.0, 1e-6);
    TS_ASSERT_DELTA(data1[1], 2.0, 1e-6);
    // Cache should now be full
    TS_ASSERT_EQUALS(ew2->MRUSize(), 50);

    int last = 100;
    // Read more;
    for (int i = last; i < last + 100; i++)
      data1 = ew2->dataY(i);

    // Cache should now be full still
    TS_ASSERT_EQUALS(ew2->MRUSize(), 50);

    // Do it some more
    last = 200;
    for (int i = last; i < last + 100; i++)
      data1 = ew2->dataY(i);

    //----- Now we test that setAllX clears the memory ----

    // Yes, our eventworkspace MRU is full
    TS_ASSERT_EQUALS(ew->MRUSize(), 50);
    TS_ASSERT_EQUALS(ew2->MRUSize(), 50);
    ew->setAllX(BinEdges(10, LinearGenerator(0.0, BIN_DELTA)));

    // MRU should have been cleared now
    TS_ASSERT_EQUALS(ew->MRUSize(), 0);
    TS_ASSERT_EQUALS(ew2->MRUSize(), 0);
  }

  void test_histogram_cache_dataE() {
    // Try caching and most-recently-used MRU list.
    EventWorkspace_const_sptr ew2 = ew;
    // Are the returned arrays the right size?
    MantidVec data1 = ew2->dataE(1);
    TS_ASSERT_EQUALS(data1.size(), NUMBINS - 1);
    // This should get the cached one
    MantidVec data2 = ew2->dataE(1);
    TS_ASSERT_EQUALS(data2.size(), NUMBINS - 1);
    // All elements are the same
    for (std::size_t i = 0; i < data1.size(); i++)
      TS_ASSERT_EQUALS(data1[i], data2[i]);

    // Now test the caching. The first 100 will load in memory
    for (int i = 0; i < 100; i++)
      data1 = ew2->dataE(i);

    // Check the bins contain 2
    data1 = ew2->dataE(0);
    TS_ASSERT_DELTA(ew2->dataE(0)[1], M_SQRT2, 1e-6);
    TS_ASSERT_DELTA(data1[1], M_SQRT2, 1e-6);
    // But the Y is still 2.0
    TS_ASSERT_DELTA(ew2->dataY(0)[1], 2.0, 1e-6);

    int last = 100;
    // Read more; memory use should be the same?
    for (int i = last; i < last + 100; i++)
      data1 = ew2->dataE(i);

    // Do it some more
    last = 200;
    for (int i = last; i < last + 100; i++)
      data1 = ew2->dataE(i);
  }

  void test_histogram_pulse_time_throws_if_index_too_large() {
    const size_t nHistos = 10;
    EventWorkspace_sptr ws = boost::make_shared<EventWorkspace>();
    ws->initialize(nHistos, 1, 1);

    MantidVec X, Y, E;
    TSM_ASSERT_THROWS("Number of histograms is out of range, should throw",
                      ws->generateHistogramPulseTime(nHistos + 1, X, Y, E),
                      const std::range_error &);
  }

  void do_test_binning(EventWorkspace_sptr ws, const BinEdges &axis,
                       size_t expected_occupancy_per_bin) {
    MantidVec Y(NUMBINS - 1);
    MantidVec E(NUMBINS - 1);
    // Required since we are rebinning in place.
    ws->setAllX(axis);
    // perform binning
    ws->generateHistogramPulseTime(0, axis.rawData(), Y, E);
    // Check results
    for (size_t j = 0; j < Y.size(); ++j) {
      TS_ASSERT_EQUALS(expected_occupancy_per_bin, Y[j]);
    }
  }

  void test_histogram_pulse_time() {
    EventWorkspace_sptr ws =
        createEventWorkspace(true, false); // Creates TOF events with
                                           // pulse_time intervals of
                                           // BIN_DELTA/2

    // Create bin steps = 4*BIN_DELTA.
    BinEdges axis1(NUMBINS / 4, LinearGenerator(0.0, 4.0 * BIN_DELTA));
    size_t expected_occupancy = 8; // Because there are two events with
                                   // pulse_time in each BIN_DELTA interval.
    do_test_binning(ws, axis1, expected_occupancy);

    // Create bin steps = 2*BIN_DELTA.
    BinEdges axis2(NUMBINS / 2, LinearGenerator(0.0, 2.0 * BIN_DELTA));
    expected_occupancy = 4; // Because there are two events with pulse_time in
                            // each BIN_DELTA interval.
    do_test_binning(ws, axis2, expected_occupancy);

    // Create bin steps = BIN_DELTA.
    BinEdges axis3(NUMBINS, LinearGenerator(0.0, BIN_DELTA));
    expected_occupancy = 2; // Because there are two events with pulse_time in
                            // each BIN_DELTA interval.
    do_test_binning(ws, axis3, expected_occupancy);
  }

  void test_get_pulse_time_max() {
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(1);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(1, 2, 1);
    ws->getSpectrum(0) += TofEvent(0, min); // min
    ws->getSpectrum(0) += TofEvent(0, max); // max;

    TS_ASSERT_EQUALS(max, ws->getPulseTimeMax());
  }

  void test_get_pulse_time_min() {
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(1);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(1, 2, 1);
    ws->getSpectrum(0) += TofEvent(0, min); // min
    ws->getSpectrum(0) += TofEvent(0, max); // max;

    TS_ASSERT_EQUALS(min, ws->getPulseTimeMin());
  }

  void test_get_time_at_sample_max_min_with_colocated_detectors() {
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(4);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(2, 2, 1);
    // First spectrum
    ws->getSpectrum(0) += TofEvent(0, min + int64_t(1));
    ws->getSpectrum(0) += TofEvent(0, max); // max in spectra 1
    // Second spectrum
    ws->getSpectrum(1) += TofEvent(0, min); // min in spectra 2
    ws->getSpectrum(1) += TofEvent(0, max - int64_t(1));

    V3D source(0, 0, 0);
    V3D sample(10, 0, 0);
    // First detector pos
    // Second detector sits on the first.
    std::vector<V3D> detectorPositions{{11, 1, 0}, {11, 1, 0}};

    WorkspaceCreationHelper::createInstrumentForWorkspaceWithDistances(
        ws, source, sample, detectorPositions);

    DateAndTime foundMin = ws->getTimeAtSampleMin();
    DateAndTime foundMax = ws->getTimeAtSampleMax();

    TS_ASSERT_EQUALS(max, foundMax);
    TS_ASSERT_EQUALS(min, foundMin);
  }

  void test_get_time_at_sample_min() {
    /*
    DateAndTime min = DateAndTime(0);
    DateAndTime max = DateAndTime(1);

    EventWorkspace_sptr ws(new EventWorkspace);
    ws->initialize(1,2,1);
    ws->getSpectrum(0) += TofEvent(0, min); // min
    ws->getSpectrum(0) += TofEvent(0, max); // max;

    TS_ASSERT_EQUALS(min, ws->getPulseTimeMin());
    */
  }

  void test_droppingOffMRU() {
    // Try caching and most-recently-used MRU list.
    EventWorkspace_const_sptr ew2 =
        boost::dynamic_pointer_cast<const EventWorkspace>(ew);

    // OK, we grab data0 from the MRU.
    const auto &inSpec = ew2->getSpectrum(0);
    const auto &inSpec300 = ew2->getSpectrum(300);

    const MantidVec &data0 = inSpec.readY();
    const MantidVec &e300 = inSpec300.readE();
    TS_ASSERT_EQUALS(data0.size(), NUMBINS - 1);

    // Fill up the MRU to make data0 drop off
    for (size_t i = 0; i < 200; i++)
      MantidVec otherData = ew2->readY(i);

    // data0 and e300 are now invalid references!
    TS_ASSERT_DIFFERS(&data0, &inSpec.readY());
    TS_ASSERT_DIFFERS(&e300, &inSpec.readE());

    // MRU is full
    TS_ASSERT_EQUALS(ew2->MRUSize(), 50);
  }

  void test_sortAll_TOF() {
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createRandomEventWorkspace(NUMBINS, NUMPIXELS);

    test_in->sortAll(TOF_SORT, nullptr);

    EventWorkspace_sptr outWS = test_in;
    for (int wi = 0; wi < NUMPIXELS; wi++) {
      std::vector<TofEvent> ve = outWS->getSpectrum(wi).getEvents();
      TS_ASSERT_EQUALS(ve.size(), NUMBINS);
      for (size_t i = 0; i < ve.size() - 1; i++)
        TS_ASSERT_LESS_THAN_EQUALS(ve[i].tof(), ve[i + 1].tof());
    }
  }

  /** Test sortAll() when there are more cores available than pixels.
   * This test will only work on machines with 2 cores at least.
   */
  void test_sortAll_SingleEventList() {
    int numEvents = 30;
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createRandomEventWorkspace(numEvents, 1);

    test_in->sortAll(TOF_SORT, nullptr);

    EventWorkspace_sptr outWS = test_in;
    std::vector<TofEvent> ve = outWS->getSpectrum(0).getEvents();
    TS_ASSERT_EQUALS(ve.size(), numEvents);
    for (size_t i = 0; i < ve.size() - 1; i++)
      TS_ASSERT_LESS_THAN_EQUALS(ve[i].tof(), ve[i + 1].tof());
  }

  /** Test sortAll() when there are more cores available than pixels.
   * This test will only work on machines with 2 cores at least.
   */
  void test_sortAll_byTime_SingleEventList() {
    int numEvents = 30;
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createRandomEventWorkspace(numEvents, 1);

    test_in->sortAll(PULSETIME_SORT, nullptr);

    EventWorkspace_sptr outWS = test_in;
    std::vector<TofEvent> ve = outWS->getSpectrum(0).getEvents();
    TS_ASSERT_EQUALS(ve.size(), numEvents);
    for (size_t i = 0; i < ve.size() - 1; i++)
      TS_ASSERT_LESS_THAN_EQUALS(ve[i].pulseTime(), ve[i + 1].pulseTime());
  }

  void test_sortAll_ByTime() {
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createRandomEventWorkspace(NUMBINS, NUMPIXELS);

    test_in->sortAll(PULSETIME_SORT, nullptr);

    EventWorkspace_sptr outWS = test_in;
    for (int wi = 0; wi < NUMPIXELS; wi++) {
      std::vector<TofEvent> ve = outWS->getSpectrum(wi).getEvents();
      TS_ASSERT_EQUALS(ve.size(), NUMBINS);
      for (size_t i = 0; i < ve.size() - 1; i++)
        TS_ASSERT_LESS_THAN_EQUALS(ve[i].pulseTime(), ve[i + 1].pulseTime());
    }
  }

  /** Nov 29 2010, ticket #1974
   * SegFault on data access through MRU list.
   * Test that parallelization is thread-safe
   *
   */
  void xtestSegFault() ///< Disabled because ~2.5 seconds.
  {
    int numpix = 100000;
    EventWorkspace_const_sptr ew1 =
        WorkspaceCreationHelper::createRandomEventWorkspace(50, numpix);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < numpix; i++) {
      for (int j = 0; j < 10; j++) {
        MantidVec Y = ew1->dataY(i);
        const MantidVec &E = ew1->dataE(i);
        MantidVec E2 = E;
      }
    }
  }

  /** Refs #2649: Add a dirty flag when changing X on an event list */
  void do_test_dirtyFlag(bool do_parallel) {
    // 50 pixels, 100 bins, 2 events in each
    int numpixels = 900;
    EventWorkspace_sptr ew1 =
        WorkspaceCreationHelper::createEventWorkspace2(numpixels, 100);
    PARALLEL_FOR_IF(do_parallel)
    for (int i = 0; i < numpixels; i += 3) {
      const MantidVec &Y = ew1->readY(i);
      TS_ASSERT_DELTA(Y[0], 2.0, 1e-5);
      const MantidVec &E = ew1->readE(i);
      TS_ASSERT_DELTA(E[0], M_SQRT2, 1e-5);

      // Vector with 10 bins, 10 wide
      MantidVec X;
      for (size_t j = 0; j < 11; j++)
        X.push_back(static_cast<double>(j) * 10.0);
      ew1->setX(i, make_cow<HistogramX>(X));

      // Now it should be 20 in that spot
      const MantidVec &Y_now = ew1->readY(i);
      TS_ASSERT_DELTA(Y_now[0], 20.0, 1e-5);
      const MantidVec &E_now = ew1->readE(i);
      TS_ASSERT_DELTA(E_now[0], sqrt(20.0), 1e-5);

      // But the other pixel is still 2.0
      const MantidVec &Y_other = ew1->readY(i + 1);
      TS_ASSERT_DELTA(Y_other[0], 2.0, 1e-5);
      const MantidVec &E_other = ew1->readE(i + 1);
      TS_ASSERT_DELTA(E_other[0], M_SQRT2, 1e-5);
    }
    // suppress unused argument when built without openmp.
    UNUSED_ARG(do_parallel)
  }

  void test_dirtyFlag() { do_test_dirtyFlag(false); }

  void test_dirtyFlag_parallel() { do_test_dirtyFlag(true); }

  void test_getEventXMinMax() {
    EventWorkspace_sptr wksp = createEventWorkspace(true, true, true);
    TS_ASSERT_DELTA(wksp->getEventXMin(), 500, .01);
    TS_ASSERT_DELTA(wksp->getEventXMax(), 1023500, .01);
  }

  /**
   * Test declaring an input EventWorkspace and retrieving as const_sptr or sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    EventWorkspace_sptr wsInput(new EventWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    EventWorkspace_const_sptr wsConst;
    EventWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<EventWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst =
                                 manager.getValue<EventWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    EventWorkspace_const_sptr wsCastConst;
    EventWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (EventWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (EventWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  /**
   * Test declaring an input IEventWorkspace and retrieving as const_sptr or
   * sptr
   */
  void testGetProperty_IEventWS_const_sptr() {
    const std::string wsName = "InputWorkspace";
    IEventWorkspace_sptr wsInput(new EventWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    IEventWorkspace_const_sptr wsConst;
    IEventWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<IEventWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(
        wsNonConst = manager.getValue<IEventWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    IEventWorkspace_const_sptr wsCastConst;
    IEventWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (IEventWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (IEventWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  void test_writeAccessInvalidatesCommonBinsFlagIsSet() {
    const int numEvents = 2;
    const int numHistograms = 2;
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createRandomEventWorkspace(numEvents,
                                                            numHistograms);
    // Calling isCommonBins() sets the flag m_isCommonBinsFlagSet
    TS_ASSERT(ws->isCommonBins())
    // Calling dataX should unset the flag m_isCommonBinsFlagSet
    ws->dataX(0)[0] += 0.0;
    // m_isCommonBinsFlagSet is false, so this will re-validate and notice that
    // dataX is still identical.
    TS_ASSERT(ws->isCommonBins())
    ws->dataX(0)[0] += 0.1;
    // m_isCommonBinsFlagSet is false, so this will re-validate and notice that
    // dataX(0) is now different from dataX(1).
    TS_ASSERT(!ws->isCommonBins())
    const BinEdges edges{-0.5, 0.5, 1.3};
    // Check methods not inherited from MatrixWorkspace
    ws->setAllX(edges);
    TS_ASSERT(ws->isCommonBins())
    ws->dataX(0)[0] -= 0.1;
    TS_ASSERT(!ws->isCommonBins())
    ws->resetAllXToSingleBin();
    TS_ASSERT(ws->isCommonBins())
  }

  void test_readYE() {
    int numEvents = 2;
    int numHistograms = 2;
    EventWorkspace_const_sptr ws =
        WorkspaceCreationHelper::createRandomEventWorkspace(numEvents,
                                                            numHistograms);
    TS_ASSERT_THROWS_NOTHING(ws->readY(0));
    TS_ASSERT_THROWS_NOTHING(ws->dataY(0));
    TS_ASSERT_THROWS_NOTHING(ws->readE(0));
    TS_ASSERT_THROWS_NOTHING(ws->dataE(0));
  }

  void test_histogram() {
    int numEvents = 2;
    int numHistograms = 2;
    EventWorkspace_const_sptr ws =
        WorkspaceCreationHelper::createRandomEventWorkspace(numEvents,
                                                            numHistograms);
    auto hist1 = ws->histogram(0);
    auto hist2 = ws->histogram(0);
    TS_ASSERT_EQUALS(hist1.sharedX(), hist2.sharedX());
    // Y and E are in the MRU
    TS_ASSERT_EQUALS(hist1.sharedY(), hist2.sharedY());
    TS_ASSERT_EQUALS(hist1.sharedE(), hist2.sharedE());
  }

  void test_clearing_EventList_clears_MRU() {
    auto ws = WorkspaceCreationHelper::createRandomEventWorkspace(2, 1);
    auto y = ws->sharedY(0);
    TS_ASSERT_EQUALS(y.use_count(), 2);
    ws->getSpectrum(0).clear();
    TS_ASSERT_EQUALS(y.use_count(), 1);
  }

  void test_swapping_spectrum_numbers_does_not_break_MRU() {
    int numEvents = 2;
    int numHistograms = 2;
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createRandomEventWorkspace(numEvents,
                                                            numHistograms);
    // put two items into MRU
    auto &yOld0 = ws->y(0);
    auto &yOld1 = ws->y(1);
    TS_ASSERT_DIFFERS(&yOld0, &yOld1);
    TS_ASSERT_EQUALS(ws->getSpectrum(0).getSpectrumNo(), 0);
    TS_ASSERT_EQUALS(ws->getSpectrum(1).getSpectrumNo(), 1);
    TS_ASSERT_DIFFERS(&(ws->y(0)), &yOld1);
    // swap their spectrum numbers
    ws->getSpectrum(0).setSpectrumNo(1);
    ws->getSpectrum(1).setSpectrumNo(0);
    TS_ASSERT_EQUALS(ws->getSpectrum(0).getSpectrumNo(), 1);
    // spectrum number of index 0 is now 1, MRU should not mix up data
    TS_ASSERT_DIFFERS(&(ws->y(0)), &yOld1);
  }

  void test_deleting_spectra_removes_them_from_MRU() {
    auto ws = WorkspaceCreationHelper::createRandomEventWorkspace(2, 1);
    auto y = ws->sharedY(0);
    TS_ASSERT_EQUALS(y.use_count(), 2);

    auto &eventList = ws->getSpectrum(0);
    auto *memory = &eventList;

    // Explicit destructor call should remove y from MRU
    eventList.~EventList();
    TS_ASSERT_EQUALS(y.use_count(), 1);

    // Placement-new to put ws back into valid state (avoid double-destruct)
    static_cast<void>(new (memory) EventList());
  }
};

#endif /* EVENTWORKSPACETEST_H_ */
