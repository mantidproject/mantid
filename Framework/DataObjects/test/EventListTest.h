// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"

#include <cxxtest/TestSuite.h>

#include <boost/scoped_ptr.hpp>
#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;
using namespace Mantid::DataObjects;

using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;
using std::runtime_error;
using std::size_t;
using std::vector;

class EventListTest : public CxxTest::TestSuite {
private:
  EventList el;
  int NUMEVENTS;
  int MAX_TOF;
  int NUMBINS;
  int BIN_DELTA;
  int MAX_PULSE_TIME;

  /**
   * Helper function to generate a list of events.
   *
   * `eventsPerPulse` are spaced equally throughout `pulsePeriod`.
   *
   * The starting time is const DateAndTime TWO("2023-01-01T12:00:00")
   *
   * @param pulsePeriod : time span of a pulse, in seconds
   * @param nPulses : number of consecutive pulses
   * @param eventsPerPulse : number of events
   * @param eventType : one of enum EventType {TOF, WEIGHTED, WEIGHTED_NOTIME}
   */
  EventList generateEvents(const DateAndTime &startTime, double pulsePeriod, size_t nPulses, size_t eventsPerPulse,
                           EventType eventType = EventType::TOF) {
    UNUSED_ARG(eventsPerPulse);
    static constexpr int64_t nanosecInsec{1000000000};
    static constexpr double microsecInsec{1000000.0};
    int64_t pulsePeriodInNanosec = static_cast<int64_t>(pulsePeriod * nanosecInsec);
    // time between consecutive events, in microseconds.
    double eventPeriod = (pulsePeriod * microsecInsec) / static_cast<double>(eventsPerPulse);
    // loop over each pulse
    auto events = EventList();
    DateAndTime currentPulseTime{startTime};
    for (size_t iPulse = 0; iPulse < nPulses; iPulse++) {
      // instantiate each event in the current pulse
      double tof{0.0};
      for (size_t iEvent = 0; iEvent < eventsPerPulse; iEvent++) {
        auto event = TofEvent(tof, currentPulseTime);
        events.addEventQuickly(event);
        tof += eventPeriod;
      }
      currentPulseTime += pulsePeriodInNanosec;
    }
    events.switchTo(eventType);
    return events;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventListTest *createSuite() { return new EventListTest(); }
  static void destroySuite(EventListTest *suite) { delete suite; }

  EventListTest() {
    BIN_DELTA = 10000;
    NUMBINS = 160;
    MAX_TOF = 10000000;
    MAX_PULSE_TIME = 10000000;
    NUMEVENTS = 100;
  }

  void setUp() override {
    // Make a little event list with 3 events
    vector<TofEvent> mylist;
    mylist.emplace_back(TofEvent(100, 200));
    mylist.emplace_back(TofEvent(3.5, 400));
    mylist.emplace_back(TofEvent(50, 60));
    el = EventList(mylist);
  }

  void test_copyDataFrom() {
    Histogram1D histogram{Histogram::XMode::Points, Histogram::YMode::Counts};
    histogram.setHistogram(Points(1), Counts(1));
    EventList eventList;
    eventList.setHistogram(BinEdges{0.0, 2.0});
    eventList += TofEvent(1.0, 2);
    std::unique_ptr<const ISpectrum> specHist = std::make_unique<Histogram1D>(histogram);
    std::unique_ptr<const ISpectrum> specEvent = std::make_unique<EventList>(eventList);
    std::unique_ptr<ISpectrum> target = std::make_unique<EventList>();

    TS_ASSERT_THROWS_EQUALS(target->copyDataFrom(*specHist), const std::runtime_error &e, std::string(e.what()),
                            "Incompatible types in ISpectrum::copyDataFrom");

    TS_ASSERT_THROWS_NOTHING(target->copyDataFrom(*specEvent));
    TS_ASSERT(target->binEdges());
    TS_ASSERT_EQUALS(&target->binEdges()[0], &eventList.binEdges()[0]);
    TS_ASSERT_EQUALS(target->counts()[0], 1.0);
  }

  void test_copyDataFrom_does_not_copy_indices() {
    EventList eventList;
    eventList.setHistogram(BinEdges{0.0, 2.0});
    eventList += TofEvent(1.0, 2);
    std::unique_ptr<const ISpectrum> specEvent = std::make_unique<EventList>(eventList);
    std::unique_ptr<ISpectrum> target = std::make_unique<EventList>();
    target->setSpectrumNo(37);
    target->setDetectorID(42);

    TS_ASSERT_THROWS_NOTHING(target->copyDataFrom(*specEvent));
    TS_ASSERT(target->binEdges());
    TS_ASSERT_EQUALS(&target->binEdges()[0], &eventList.binEdges()[0]);
    TS_ASSERT_EQUALS(target->counts()[0], 1.0);
    TS_ASSERT_EQUALS(target->getSpectrumNo(), 37);
    TS_ASSERT_EQUALS(target->getDetectorIDs(), std::set<detid_t>{42});
  }

  void test_copyDataFrom_event_data_details() {
    EventList eventList;
    eventList.setHistogram(BinEdges{0.0, 2.0});
    eventList += TofEvent(1.0, 2);
    EventList target;

    target.copyDataFrom(eventList);
    TS_ASSERT_EQUALS(target.getEventType(), EventType::TOF)
    TS_ASSERT_EQUALS(target.getSortType(), eventList.getSortType());
    TS_ASSERT_EQUALS(target.getEvents(), eventList.getEvents());
    TS_ASSERT_THROWS(target.getWeightedEvents(), const std::runtime_error &);
    TS_ASSERT_THROWS(target.getWeightedEventsNoTime(), const std::runtime_error &);

    eventList.switchTo(EventType::WEIGHTED);
    target.copyDataFrom(eventList);
    TS_ASSERT_EQUALS(target.getEventType(), EventType::WEIGHTED)
    TS_ASSERT_EQUALS(target.getSortType(), eventList.getSortType());
    TS_ASSERT_THROWS(target.getEvents(), const std::runtime_error &);
    TS_ASSERT_EQUALS(target.getWeightedEvents(), eventList.getWeightedEvents());
    TS_ASSERT_THROWS(target.getWeightedEventsNoTime(), const std::runtime_error &);

    eventList.switchTo(EventType::WEIGHTED_NOTIME);
    target.copyDataFrom(eventList);
    TS_ASSERT_EQUALS(target.getEventType(), EventType::WEIGHTED_NOTIME)
    TS_ASSERT_EQUALS(target.getSortType(), eventList.getSortType());
    TS_ASSERT_THROWS(target.getEvents(), const std::runtime_error &);
    TS_ASSERT_THROWS(target.getWeightedEvents(), const std::runtime_error &);
    TS_ASSERT_EQUALS(target.getWeightedEventsNoTime(), eventList.getWeightedEventsNoTime());
  }

  void test_EventTypeConstructor() {
    EventList tof;
    TS_ASSERT_EQUALS(tof.getEventType(), TOF);

    EventList weighted(WEIGHTED);
    TS_ASSERT_EQUALS(weighted.getEventType(), WEIGHTED);

    EventList weightedNoTime(WEIGHTED_NOTIME);
    TS_ASSERT_EQUALS(weightedNoTime.getEventType(), WEIGHTED_NOTIME);
  }

  //==================================================================================
  //--- Basics  ----
  //==================================================================================

  void test_Init() {
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].tof(), 100);
    TS_ASSERT_EQUALS(rel[0].pulseTime(), 200);
    TS_ASSERT_EQUALS(rel[2].tof(), 50);
  }

  void test_AssignmentOperator() {
    // Modify EventList such that is does not contain default values.
    el.setSpectrumNo(42);
    MantidVec x{0.1, 0.2, 0.3};
    el.setX(make_cow<HistogramX>(x));
    el.setPointVariances(2);

    EventList other;
    other = el;

    TS_ASSERT_EQUALS(other, el);
    // operator== does not compare everything, so we do some extra comparisons
    TS_ASSERT_EQUALS(other.getSpectrumNo(), el.getSpectrumNo());
    TS_ASSERT_EQUALS(other.getDetectorIDs(), el.getDetectorIDs());
    TS_ASSERT_EQUALS(other.readX(), el.readX());
    TS_ASSERT_EQUALS(other.sharedDx(), el.sharedDx());
  }

  //==================================================================================
  //--- Plus Operators  ----
  //==================================================================================

  void test_PlusOperator() {
    vector<TofEvent> mylist{{45, 67}, {89, 12}, {34, 56}};
    el += mylist;
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].tof(), 45);
    TS_ASSERT_EQUALS(rel[5].tof(), 34);

    el += TofEvent(999, 888);
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 7);
    TS_ASSERT_EQUALS(rel[6].tof(), 999);

    EventList el2;
    el2 += TofEvent(1, 2);
    el2 += TofEvent(3, 4);
    el += el2;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 9);
    el += el;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 18);

    el.addEventQuickly(TofEvent(333, 444));
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 19);
  }

  template <class T> void do_test_memory_handling(EventList &el2, std::vector<T> &events) {
    std::vector<T> mylist{{45}, {89}, {34}};
    el2 += mylist;
    TS_ASSERT_EQUALS(events.size(), 3);
    TS_ASSERT_EQUALS(events.capacity(), 3);
    mylist.emplace_back(TofEvent(88, 88));
    el2 += mylist;
    TS_ASSERT_EQUALS(events.size(), 7);
    TS_ASSERT_EQUALS(events.capacity(), 7);
    el2.clear();
    TS_ASSERT_EQUALS(events.size(), 0);
    TS_ASSERT_EQUALS(events.capacity(), 0);
  }

  void test_Clear_AndOthers_FreesUpMemory() {
    // We want to make sure that clearing really releases the vector memory.
    EventList el2;
    el2 = EventList();
    do_test_memory_handling(el2, el2.getEvents());

    el2 = EventList();
    el2.switchTo(WEIGHTED);
    do_test_memory_handling(el2, el2.getWeightedEvents());

    el2 = EventList();
    el2.switchTo(WEIGHTED_NOTIME);
    do_test_memory_handling(el2, el2.getWeightedEventsNoTime());
  }

  //
  //  template<class T>
  //  void do_test_clearUnused(EventList & el2, typename std::vector<T> &
  //  events)
  //  {
  //    typename std::vector<T> mylist;
  //    mylist.emplace_back(T(45));
  //    mylist.emplace_back(T(89));
  //    mylist.emplace_back(T(34));
  //    el2 += mylist;
  //    TS_ASSERT_EQUALS(events.size(), 3);
  //    TS_ASSERT_EQUALS(events.capacity(), 3);
  //    mylist.emplace_back(TofEvent(88,88));
  //    el2 += mylist;
  //    TS_ASSERT_EQUALS(events.size(), 7);
  //    TS_ASSERT_EQUALS(events.capacity(), 7);
  //    el2.clear();
  //    TS_ASSERT_EQUALS(events.size(), 0);
  //    TS_ASSERT_EQUALS(events.capacity(), 0);
  //  }
  //
  //  void test_clearUnused()
  //  {
  //  }

  void test_PlusOperator2() {
    vector<TofEvent> rel;
    el += el;
    rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 6);
    TS_ASSERT_EQUALS(rel[3].tof(), 100);
    TS_ASSERT_EQUALS(rel[5].tof(), 50);
  }

  void test_DetectorIDs() {
    EventList el1;
    el1.addDetectorID(14);
    TS_ASSERT_EQUALS(el1.getDetectorIDs().size(), 1);
    el1.addDetectorID(21);
    TS_ASSERT_EQUALS(el1.getDetectorIDs().size(), 2);
    el1.addDetectorID(21);
    TS_ASSERT_EQUALS(el1.getDetectorIDs().size(), 2);

    EventList el2;
    el2.addDetectorID(7);
    el2.addDetectorID(14);
    el2.addDetectorID(28);
    TS_ASSERT_EQUALS(el2.getDetectorIDs().size(), 3);

    // One detID was repeated, so it doesn't appear twice
    el2 += el1;
    TS_ASSERT_EQUALS(el2.getDetectorIDs().size(), 4);
    // Find the right stuff
    for (int i = 7; i < 35; i += 7)
      TS_ASSERT(el2.hasDetectorID(i));
    TS_ASSERT(!el2.hasDetectorID(0));
  }

  //==================================================================================
  //--- Switching to Weighted Events ----
  //==================================================================================

  //----------------------------------
  void test_switchToWeightedEvents() {
    // Start with a bit of fake data
    this->fake_data();
    TS_ASSERT_EQUALS(el.getEvents().size(), NUMEVENTS);
    TS_ASSERT_EQUALS(el.getNumberEvents(), NUMEVENTS);
    TS_ASSERT_THROWS(el.getWeightedEvents(), const std::runtime_error &);
    TS_ASSERT_THROWS(el.getWeightedEventsNoTime(), const std::runtime_error &);

    el.switchTo(WEIGHTED);
    TS_ASSERT_THROWS(el.getEvents(), const std::runtime_error &);
    TS_ASSERT_THROWS(el.getWeightedEventsNoTime(), const std::runtime_error &);
    TS_ASSERT_EQUALS(el.getWeightedEvents().size(), NUMEVENTS);
    TS_ASSERT_EQUALS(el.getNumberEvents(), NUMEVENTS);
    TS_ASSERT_EQUALS(el.getEvent(0).weight(), 1.0);
    TS_ASSERT_EQUALS(el.getEvent(0).error(), 1.0);
  }

  //----------------------------------
  void test_switchToWeightedEventsNoTime() {
    // Start with a bit of fake data
    this->fake_data();
    el.switchTo(WEIGHTED_NOTIME);
    TS_ASSERT_THROWS(el.getEvents(), const std::runtime_error &);
    TS_ASSERT_THROWS(el.getWeightedEvents(), const std::runtime_error &);
    TS_ASSERT_EQUALS(el.getWeightedEventsNoTime().size(), NUMEVENTS);
    TS_ASSERT_EQUALS(el.getNumberEvents(), NUMEVENTS);
    TS_ASSERT_EQUALS(el.getWeightedEventsNoTime()[0].weight(), 1.0);
    TS_ASSERT_EQUALS(el.getWeightedEventsNoTime()[0].error(), 1.0);
  }

  //----------------------------------
  void test_switch_on_the_fly_when_adding_single_event() {
    fake_data();
    TS_ASSERT_EQUALS(el.getEventType(), TOF);

    // Add a weighted event = everything switches
    WeightedEvent we(123, 456, 2.0, 3.0 * 3.0);
    el += we;
    TS_ASSERT_EQUALS(el.getEventType(), WEIGHTED);
    TS_ASSERT_EQUALS(el.getEvent(0).weight(), 1.0);
    TS_ASSERT_EQUALS(el.getEvent(0).error(), 1.0);
    // New one is at the end
    TS_ASSERT_EQUALS(el.getWeightedEvents()[NUMEVENTS], we);

    // But you can still add a plain one
    TofEvent e(789, 654);
    el += e;
    TS_ASSERT_EQUALS(el.getWeightedEvents()[NUMEVENTS + 1], static_cast<WeightedEvent>(e));
    TS_ASSERT_EQUALS(el.getEvent(NUMEVENTS + 1).weight(), 1.0);
  }

  //----------------------------------
  /** Nine possibilies of adding event lists together
   * (3 lhs x 3 rhs types).
   */
  void test_switch_on_the_fly_when_appending_lists_all_nine_possibilities() {
    EventList lhs, rhs;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        // Copy and switch
        lhs = el;
        lhs.switchTo(static_cast<EventType>(i));

        // Copy and switch
        rhs = el;
        rhs.switchTo(static_cast<EventType>(j));

        // Use the += operator to append
        TS_ASSERT_THROWS_NOTHING(lhs += rhs;);

        // The Ending type is whatever is HIGHER in the hierarchy
        // TOF->WEIGHTED->WEIGHTED_NOTIME
        int expected = i;
        if (j > i)
          expected = j;
        TS_ASSERT_EQUALS(static_cast<int>(lhs.getEventType()), expected);

        // The final list has 6 events
        TS_ASSERT_EQUALS(lhs.getNumberEvents(), 6);
        // And each element's TOF is what we expect.
        TS_ASSERT_DELTA(lhs.getEvent(0).tof(), 100, 1e-5);
        TS_ASSERT_DELTA(lhs.getEvent(1).tof(), 3.5, 1e-5);
        TS_ASSERT_DELTA(lhs.getEvent(2).tof(), 50, 1e-5);
        TS_ASSERT_DELTA(lhs.getEvent(3).tof(), 100, 1e-5);
        TS_ASSERT_DELTA(lhs.getEvent(4).tof(), 3.5, 1e-5);
        TS_ASSERT_DELTA(lhs.getEvent(5).tof(), 50, 1e-5);
      }
    }
  }

  //==================================================================================
  //--- Minus Operation ----
  //==================================================================================

  /// Make a big bin holding all events
  cow_ptr<HistogramX> one_big_bin() { return make_cow<HistogramX>(std::initializer_list<double>{0, 1e10}); }

  void test_MinusOperator_all_9_possibilites() {
    EventList lhs, rhs;
    for (size_t i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        this->fake_uniform_data();
        // Copy and switch
        lhs = el;
        lhs.switchTo(static_cast<EventType>(i));

        // Copy and switch
        rhs = el;
        rhs.switchTo(static_cast<EventType>(j));

        // Do the minus!
        std::ostringstream mess;
        mess << "Minus operation of types " << i << " -= " << j << ".";
        TSM_ASSERT_THROWS_NOTHING(mess.str(), lhs -= rhs);

        TSM_ASSERT_EQUALS(mess.str(), lhs.getNumberEvents(), 2 * el.getNumberEvents());

        // Put a single big bin with all events
        lhs.setX(one_big_bin());
        // But the total neutrons is 0.0! They've been cancelled out :)
        boost::scoped_ptr<MantidVec> Y(lhs.makeDataY());
        boost::scoped_ptr<MantidVec> E(lhs.makeDataE());
        TS_ASSERT_DELTA((*Y)[0], 0.0, 1e-6);
        TS_ASSERT_DELTA((*E)[0], sqrt((double)lhs.getNumberEvents()), 1e-6);
      }
    }
  }

  /** Perform THIS -= THIS, e.g. clear the event list */
  void test_MinusOperator_inPlace_3cases() {
    EventList lhs, rhs;
    for (size_t i = 0; i < 3; i++) {
      this->fake_uniform_data();
      // Copy and switch
      lhs = el;
      lhs.switchTo(static_cast<EventType>(i));

      // Do the minus!
      std::ostringstream mess;
      mess << "Minus operation of type " << i << ".";
      TSM_ASSERT_THROWS_NOTHING(mess.str(), lhs -= lhs);

      TSM_ASSERT_EQUALS(mess.str(), lhs.getNumberEvents(), 0);

      // Put a single big bin with all events
      lhs.setX(one_big_bin());
      // But the total neutrons is 0.0! They've been cancelled out :)
      boost::scoped_ptr<MantidVec> Y(lhs.makeDataY());
      boost::scoped_ptr<MantidVec> E(lhs.makeDataE());
      TS_ASSERT_DELTA((*Y)[0], 0.0, 1e-6);
      TS_ASSERT_DELTA((*E)[0], sqrt((double)lhs.getNumberEvents()), 1e-6);
    }
  }

  //==================================================================================
  //--- Multiplying  ----
  //==================================================================================

  void test_multiply_scalar_simple() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));

      // Perform the multiply; no error on the scalar
      TS_ASSERT_THROWS_NOTHING(el.multiply(2.0, 0.0));
      TS_ASSERT_DELTA(el.getEvent(0).weight(), 2.0, 1e-5);
      TS_ASSERT_DELTA(el.getEvent(0).error(), 2.0, 1e-5);

      this->fake_uniform_data();
      // Multiply by zero with error
      TS_ASSERT_THROWS_NOTHING(el.multiply(0.0, 1.0));
      TS_ASSERT_DELTA(el.getEvent(0).weight(), 0.0, 1e-5);
      // Error is preserved!
      TS_ASSERT_DELTA(el.getEvent(0).error(), 1.0, 1e-5);
    }
  }

  void test_multiply_by_one_doesnt_give_weights() {
    // No weights
    this->fake_uniform_data();
    // Perform the multiply by one without error.
    el.multiply(1.0, 0.0);
    TS_ASSERT_EQUALS(el.getEventType(), TOF);
  }

  void test_divide_by_one_doesnt_give_weights() {
    // No weights
    this->fake_uniform_data();
    // Perform the multiply by one without error.
    el.divide(1.0, 0.0);
    TS_ASSERT_EQUALS(el.getEventType(), TOF);
  }

  //-----------------------------------------------------------------------------------------------
  void test_multiply_scalar() {
    // Weight 2, error (2.5)
    this->fake_uniform_data_weights();
    // Perform the multiply
    el.multiply(2.0, 0.5);

    TS_ASSERT_DELTA(el.getEvent(0).weight(), 4.0, 1e-5);
    // Error^2 = 2.5*2.5 * 2.0*2.0 + 2.0*2.0*0.5*0.5
    TS_ASSERT_DELTA(el.getEvent(0).errorSquared(), (2.5 * 2.5 * 2.0 * 2.0 + 2.0 * 2.0 * 0.5 * 0.5), 1e-5);

    // Go through each possible EventType as the input
    for (int this_type = 1; this_type < 3; this_type++) {
      // Try it with no scalar error
      this->fake_uniform_data_weights();
      el.switchTo(static_cast<EventType>(this_type));
      el.multiply(2.0);
      TS_ASSERT_DELTA(el.getEvent(0).weight(), 4.0, 1e-5);
      TS_ASSERT_DELTA(el.getEvent(0).error(), 1.25 * 4.0, 1e-5);

      // *= operator
      this->fake_uniform_data_weights();
      el.switchTo(static_cast<EventType>(this_type));
      el *= 2.0;
      TS_ASSERT_DELTA(el.getEvent(0).weight(), 4.0, 1e-5);
      TS_ASSERT_DELTA(el.getEvent(0).error(), 1.25 * 4.0, 1e-5);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_multiply_histogram() {
    // Make the histogram we are multiplying.
    MantidVec X, Y, E;
    // one tenth of the # of bins
    double step = BIN_DELTA * 10;
    X = this->makeX(step, NUMBINS / 10 + 1);
    for (std::size_t i = 0; i < X.size() - 1; i++) {
      Y.emplace_back(static_cast<double>(i + 1));
      E.emplace_back(sqrt(static_cast<double>(i + 1)));
    }

    // Go through each possible EventType as the input
    for (int this_type = 1; this_type < 3; this_type++) {
      // Make the data and multiply: 2.0+-2.5
      this->fake_uniform_data_weights();
      el.switchTo(static_cast<EventType>(this_type));

      // Perform the histogram multiplication
      el.multiply(X, Y, E);

      // The event list is of the right length and type
      TS_ASSERT_EQUALS(el.getNumberEvents(), 2000);
      TS_ASSERT_EQUALS(el.getEventType(), static_cast<EventType>(this_type));

      for (std::size_t i = 0; i < el.getNumberEvents(); i++) {
        double tof = el.getEvent(i).tof();
        if (tof >= step && tof < BIN_DELTA * NUMBINS) {
          double value = std::floor(tof / step);
          double errorsquared = value;
          // Check the formulae for value and error
          TS_ASSERT_DELTA(el.getEvent(i).weight(), 2.0 * value, 1e-6);
          TS_ASSERT_DELTA(el.getEvent(i).errorSquared(), 2.5 * 2.5 * value * value + 2.0 * 2.0 * errorsquared, 1e-6);
        }
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_divide_scalar_simple() {
    this->fake_uniform_data();
    el.divide(2.0, 0.0);
    TS_ASSERT_DELTA(el.getEvent(0).weight(), 0.5, 1e-5);
    TS_ASSERT_DELTA(el.getEvent(0).error(), 0.5, 1e-5);

    this->fake_uniform_data();
    el.divide(2.0);
    TS_ASSERT_DELTA(el.getEvent(0).weight(), 0.5, 1e-5);
    TS_ASSERT_DELTA(el.getEvent(0).error(), 0.5, 1e-5);
  }

  void test_divide_scalar() {
    // Weight 2, error 2.5
    this->fake_uniform_data_weights();
    el.divide(2.0, 0.5);

    TS_ASSERT_DELTA(el.getEvent(0).weight(), 1.0, 1e-5);
    // Relative errors sum, so (sqrt(2.5)/2)^2+0.25^2 = 1.625; error is
    // sqrt(1.625 * 1.0)
    TS_ASSERT_DELTA(el.getEvent(0).error(), sqrt(1.625), 1e-5);

    // Try it with no scalar error
    this->fake_uniform_data_weights();
    el.divide(2.0);
    TS_ASSERT_DELTA(el.getEvent(0).weight(), 1.0, 1e-5);
    // Same relative error of 1.25
    TS_ASSERT_DELTA(el.getEvent(0).error(), 1.25, 1e-5);

    // *= operator
    this->fake_uniform_data_weights();
    el /= 2.0;
    TS_ASSERT_DELTA(el.getEvent(0).weight(), 1.0, 1e-5);
    TS_ASSERT_DELTA(el.getEvent(0).error(), 1.25, 1e-5);
  }

  void test_divide_by_zero() {
    // Perform the multiply
    TS_ASSERT_THROWS(el.divide(0.0, 0.5), const std::invalid_argument &);
    TS_ASSERT_THROWS(el.divide(0.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(el /= 0, const std::invalid_argument &);
  }

  //-----------------------------------------------------------------------------------------------
  void test_divide_histogram() {
    // Make the histogram by which we'll divide
    MantidVec X, Y, E;
    // one tenth of the # of bins
    double step = BIN_DELTA * 10;
    for (double tof = step; tof < BIN_DELTA * (NUMBINS + 1); tof += step) {
      X.emplace_back(tof);
    }
    for (std::size_t i = 0; i < X.size() - 1; i++) {
      // Have one zero bin in there
      if (i == 6)
        Y.emplace_back(0.0);
      else
        Y.emplace_back(2.0);
      E.emplace_back(0.5);
    }

    // Go through each possible EventType as the input
    for (int this_type = 1; this_type < 3; this_type++) {
      // Make the data and multiply: 2.0+-2.5
      this->fake_uniform_data_weights();
      el.switchTo(static_cast<EventType>(this_type));

      // Now we divide
      TS_ASSERT_THROWS_NOTHING(el.divide(X, Y, E));

      // The event list is of the right length and type
      TS_ASSERT_EQUALS(el.getNumberEvents(), 2000);
      TS_ASSERT_EQUALS(el.getEventType(), static_cast<EventType>(this_type));

      for (std::size_t i = 0; i < el.getNumberEvents(); i++) {
        double tof = el.getEvent(i).tof();
        if (tof >= step && tof < BIN_DELTA * NUMBINS) {
          int bini = static_cast<int>(tof / step);
          if (bini == 7) {
            // That was zeros
            TS_ASSERT(std::isnan(el.getEvent(i).weight()));
            TS_ASSERT(std::isnan(el.getEvent(i).errorSquared()));
          } else {
            // Same weight error as dividing by a scalar with error before,
            // since we divided by 2+-0.5 again
            TS_ASSERT_DELTA(el.getEvent(i).weight(), 1.0, 1e-5);
            TS_ASSERT_DELTA(el.getEvent(i).error(), sqrt(1.625), 1e-5);
          }
        }
      }
    }
  }

  void test_divide_by_a_scalar_without_error___then_histogram() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      // Make the data
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      // Divide by 2, no error = result should be 1 +- 0.707
      TS_ASSERT_THROWS_NOTHING(el.divide(2.0, 0));

      // Make the histogram we are multiplying.
      MantidVec Y, E;
      MantidVec X = this->makeX(BIN_DELTA, 10);
      el.generateHistogram(X, Y, E);

      for (std::size_t i = 0; i < Y.size(); i++) {
        TSM_ASSERT_DELTA(this_type, Y[i], 1.0, 1e-5);
        TS_ASSERT_DELTA(E[i], 0.5 * M_SQRT2, 1e-5);
      }
    }
  }

  void test_divide_by_a_scalar_with_error___then_histogram() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      // Make the data
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      // Divide by two with error sqrt(2) = result has less error than if you
      // had started from a histogram.
      TS_ASSERT_THROWS_NOTHING(el.divide(2.0, M_SQRT2));

      // Make the histogram we are multiplying.
      MantidVec Y, E;
      MantidVec X = this->makeX(BIN_DELTA, 10);
      el.generateHistogram(X, Y, E);

      for (std::size_t i = 0; i < Y.size(); i++) {
        TS_ASSERT_DELTA(Y[i], 1.0, 1e-5);
        TS_ASSERT_DELTA(E[i], sqrt(0.75), 1e-5);
      }
    }
  }

  void test_multiply_by_a_scalar_without_error___then_histogram() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      // Make the data
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      // multiply
      TS_ASSERT_THROWS_NOTHING(el.multiply(2.0, 0.0));

      // Make the histogram we are multiplying.
      MantidVec Y, E;
      MantidVec X = this->makeX(BIN_DELTA);
      el.generateHistogram(X, Y, E);

      for (std::size_t i = 0; i < Y.size(); i++) {
        TS_ASSERT_DELTA(Y[i], 4.0, 1e-5);
        TS_ASSERT_DELTA(E[i], 4.0 * M_SQRT1_2, 1e-5);
      }
    }
  }

  void test_multiply_by_a_scalar_with_error___then_histogram() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      // Make the data
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      // Multiply with an error
      TS_ASSERT_THROWS_NOTHING(el.multiply(2.0, M_SQRT2));
      MantidVec Y, E;
      MantidVec X = this->makeX(BIN_DELTA);
      el.generateHistogram(X, Y, E);

      for (std::size_t i = 0; i < Y.size(); i++) {
        TSM_ASSERT_DELTA(this_type, Y[i], 4.0, 1e-5);
        TS_ASSERT_DELTA(E[i], sqrt(12.0), 1e-5);
      }
    }
  }

  //==================================================================================
  //--- Sorting Tests ---
  //==================================================================================

  void test_SortTOF_simple() {
    el.sortTof();
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].tof(), 3.5);
    TS_ASSERT_EQUALS(rel[1].tof(), 50);
    TS_ASSERT_EQUALS(rel[2].tof(), 100);
  }

  /// Test for all event types
  void test_SortTOF_all_types() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_data();
      el.switchTo(static_cast<EventType>(this_type));
      el.sort(TOF_SORT);
      for (size_t i = 1; i < 100; i++) {
        TSM_ASSERT_LESS_THAN_EQUALS(this_type, el.getEvent(i - 1).tof(), el.getEvent(i).tof());
      }
    }
  }

  void test_SortPulseTime_simple() {
    el.sortPulseTime();
    vector<TofEvent> rel = el.getEvents();
    TS_ASSERT_EQUALS(rel.size(), 3);
    TS_ASSERT_EQUALS(rel[0].pulseTime(), 60);
    TS_ASSERT_EQUALS(rel[1].pulseTime(), 200);
    TS_ASSERT_EQUALS(rel[2].pulseTime(), 400);
  }

  void test_SortPulseTime_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_data();
      el.switchTo(static_cast<EventType>(this_type));
      el.sort(PULSETIME_SORT);
      for (size_t i = 1; i < 100; i++) {
        TSM_ASSERT_LESS_THAN_EQUALS(this_type, el.getEvent(i - 1).pulseTime(), el.getEvent(i).pulseTime());
      }
    }
  }

  void test_SortPulseTime_weights() {
    this->fake_data();
    el.switchTo(WEIGHTED);
    el.sort(PULSETIME_SORT);
    vector<WeightedEvent> rwel = el.getWeightedEvents();
    for (size_t i = 1; i < 100; i++) {
      TS_ASSERT_LESS_THAN_EQUALS(rwel[i - 1].pulseTime(), rwel[i].pulseTime());
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_reverse_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_data();
      el.switchTo(static_cast<EventType>(this_type));
      el.sortTof();

      double oldFirst = el.getEvent(0).tof();
      double oldLast = el.getEvent(el.getNumberEvents() - 1).tof();
      size_t oldNum = el.getNumberEvents();

      el.reverse();

      double newFirst = el.getEvent(0).tof();
      double newLast = el.getEvent(el.getNumberEvents() - 1).tof();
      size_t newNum = el.getNumberEvents();

      TS_ASSERT_EQUALS(oldNum, newNum);
      TS_ASSERT_EQUALS(oldFirst, newLast);
      TS_ASSERT_EQUALS(oldLast, newFirst);
    }
  }

  //==================================================================================
  //--- Comparison Operators
  //==================================================================================

  void test_EqualityOperator() {
    EventList el1, el2;
    el1.addEventQuickly(TofEvent(1.5, 5));
    TS_ASSERT(!(el1 == el2));
    TS_ASSERT((el1 != el2));
    el2.addEventQuickly(TofEvent(1.5, 5));
    TS_ASSERT((el1 == el2));
    TS_ASSERT(!(el1 != el2));

    TS_ASSERT(el1.equals(el2, 0., 0., 0));
  }

  //==================================================================================
  //--- Histogramming Tests ---
  //==================================================================================

  void test_setX() {
    // Generate the histrogram bins
    MantidVec shared_x;
    double tof; // in ns
    for (tof = 0; tof < BIN_DELTA * (NUMBINS + 1); tof += BIN_DELTA) {
      // bins of 10 microsec
      shared_x.emplace_back(tof);
    }
    el.setX(make_cow<HistogramX>(shared_x));
    // Do we have the same data in X?
    const EventList el2(el);
    TS_ASSERT(el2.readX() == shared_x);
  }

  void test_dataX() {
    el = EventList();
    MantidVec inVec(10, 1.0);
    el.dataX() = inVec;
    const MantidVec &vec = el.dataX();
    TS_ASSERT_EQUALS(vec, inVec);
  }

  void test_setX_empty_constructor() {
    el = EventList();
    // Generate the histrogram bins
    MantidVec shared_x;
    double tof; // in ns
    for (tof = 0; tof < 16e3 * 1e3; tof += 1e4) {
      // bins of 10 microsec
      shared_x.emplace_back(tof);
    }
    el.setX(make_cow<HistogramX>(shared_x));
    // Do we have the same data in X?
    const EventList el2(el);
    TS_ASSERT(el2.readX() == shared_x);
  }

  void test_empty_histogram() {
    // Make sure there's no data
    el.clear();
    const EventList el2(el);

    // Getting data before setting X returns empty vector
    boost::scoped_ptr<MantidVec> Y2(el2.makeDataY());
    TS_ASSERT_EQUALS(Y2->size(), 0);

    // Now do set up an X axis.
    this->test_setX();
    const EventList el3(el);
    MantidVec X = el3.readX();
    boost::scoped_ptr<MantidVec> Y3(el3.makeDataY());
    // Histogram is 0, since I cleared all the events
    for (std::size_t i = 0; i < X.size() - 1; i++) {
      TS_ASSERT_EQUALS((*Y3)[i], 0);
    }
  }

  void test_no_histogram_x() {
    // Make sure there's no data and no X
    el.clear();
    // Now give it some fake data, with NUMEVENTS events in it.
    this->fake_data();
    const EventList el4(el);
    boost::scoped_ptr<MantidVec> Y(el4.makeDataY());
    TS_ASSERT_EQUALS(Y->size(), 0);
  }

  void test_histogram_all_types() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));

      this->test_setX();       // Set it up
      const EventList el3(el); // need to copy to a const method in order to
                               // access the data directly.
      MantidVec X = el3.readX();
      boost::scoped_ptr<MantidVec> Y(el3.makeDataY());
      boost::scoped_ptr<MantidVec> E(el3.makeDataE());
      TS_ASSERT_EQUALS(Y->size(), X.size() - 1);
      // The data was created so that there should be exactly 2 events per bin
      // The last bin entry will be 0 since we use it as the top boundary of
      // i-1.
      for (std::size_t i = 0; i < Y->size(); i++) {
        TS_ASSERT_EQUALS((*Y)[i], 2.0);
        TS_ASSERT_DELTA((*E)[i], M_SQRT2, 1e-5);
      }
    }
  }

  void test_histogram_tof_event_by_pulse_time() {
    // Generate TOF events with Pulse times uniformly distributed.
    EventList eList = this->fake_uniform_pulse_data();

    // Generate the histrogram bins
    MantidVec shared_x;
    for (int pulse_time = 0; pulse_time < BIN_DELTA * (NUMBINS + 1); pulse_time += BIN_DELTA) {
      shared_x.emplace_back(pulse_time);
    }

    eList.setX(make_cow<HistogramX>(shared_x));
    // Do we have the same data in X?
    TS_ASSERT(eList.readX() == shared_x);

    MantidVec X = eList.readX();
    MantidVec Y;
    MantidVec E;

    eList.generateHistogramPulseTime(X, Y, E);

    for (std::size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_EQUALS(Y[i], 2.0);
      TS_ASSERT_DELTA(E[i], M_SQRT2, 1e-5);
    }

    // check uniform counts histogram.
    size_t hist1 = Y.size();
    MantidVec Y1(hist1, 0);
    eList.generateCountsHistogramPulseTime(X[0], X[hist1], Y1);
    for (std::size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_EQUALS(Y[i], Y1[i]);
    }
  }

  void test_histogram_weighed_event_by_pulse_time_throws() {
    EventList eList = this->fake_uniform_pulse_data(WEIGHTED);

    // Generate the histrogram bins
    MantidVec shared_x;
    for (int pulse_time = 0; pulse_time < BIN_DELTA * (NUMBINS + 1); pulse_time += BIN_DELTA) {
      shared_x.emplace_back(pulse_time);
    }

    eList.setX(make_cow<HistogramX>(shared_x));
    // Do we have the same data in X?
    TS_ASSERT(eList.readX() == shared_x);

    MantidVec X = eList.readX();
    MantidVec Y;
    MantidVec E;

    TSM_ASSERT_THROWS("We don't support WeightedEvents with this feature at present.",
                      eList.generateHistogramPulseTime(X, Y, E), const std::runtime_error &);
  }

  void test_histogram_by_time_at_sample_pulse_only() {
    // Generate TOF events with Pulse times uniformly distributed.
    EventList eList = this->fake_uniform_pulse_data();

    // Generate the histrogram bins
    MantidVec shared_x;
    for (int time_at_sample = 0; time_at_sample < BIN_DELTA * (NUMBINS + 1); time_at_sample += BIN_DELTA) {
      shared_x.emplace_back(time_at_sample);
    }

    eList.setX(make_cow<HistogramX>(shared_x));
    // Do we have the same data in X?
    TS_ASSERT(eList.readX() == shared_x);

    MantidVec X = eList.readX();
    MantidVec Y;
    MantidVec E;

    const double tofFactor = 0;
    const double tofOffset = 0;
    // Should be doing the same job as generatehistogrampulsetime with tofFactor
    // and tofOffset = 0.
    eList.generateHistogramTimeAtSample(X, Y, E, tofFactor, tofOffset);

    for (std::size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_EQUALS(Y[i], 2.0);
      TS_ASSERT_DELTA(E[i], M_SQRT2, 1e-5);
    }
  }

  void test_get_min_pulse_time() {
    EventList eList = this->fake_uniform_pulse_data();
    auto vec = eList.getPulseTimes();
    int64_t expectedResult = std::min_element(vec.begin(), vec.end())->totalNanoseconds();
    TS_ASSERT_EQUALS(expectedResult, eList.getPulseTimeMin().totalNanoseconds());
  }

  void test_get_max_pulse_time() {
    EventList eList = this->fake_uniform_pulse_data();
    auto vec = eList.getPulseTimes();
    int64_t expectedResult = std::max_element(vec.begin(), vec.end())->totalNanoseconds();
    TS_ASSERT_EQUALS(expectedResult, eList.getPulseTimeMax().totalNanoseconds());
  }

  void test_histogram_by_time_at_sample() {
    // Generate TOF events with Pulse times of zero, and TOF uniformly
    // distributed int the range 100 microseconds to MAX_TOF, in steps
    // calculated to be BIN_DELTA/events_per_bin.
    const double events_per_bin = 2;
    const bool randomPulse = false;
    this->fake_uniform_data(events_per_bin, randomPulse);

    // Generate the histrogram bins
    MantidVec shared_x;
    /*
     * Note in the loop below, steps are made in stages of BIN_DELTA, while we
     * populated the orignal event list with events distributed in steps of 1
     * TOFevent per BIN_DELTA/2,
     * therefore we should expect 2 events in each bin in our output.
     */
    for (int time_at_sample = 100; time_at_sample < MAX_TOF; time_at_sample += BIN_DELTA) {
      shared_x.emplace_back(time_at_sample * 1e3); // Have x-axis in nanoseconds.
                                                   // Tof values are stored as
                                                   // microseconds.
    }

    el.setX(make_cow<HistogramX>(shared_x));
    // Do we have the same data in X?
    TS_ASSERT(el.readX() == shared_x);

    MantidVec X = el.readX();
    MantidVec Y;
    MantidVec E;

    const double tofFactor = 1;
    const double tofOffset = 0;
    // Should be doing the same job as generatehistogram with tofFactor = 1, and
    // tofOffset = 0 and pulse times of zero.
    el.generateHistogramTimeAtSample(X, Y, E, tofFactor, tofOffset);

    for (std::size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_EQUALS(Y[i], 2.0);
      TS_ASSERT_DELTA(E[i], M_SQRT2, 1e-5);
    }
  }

  void test_get_min_time_at_sample() {
    this->fake_data();
    const double tofFactor = 1;
    const double tofOffset = 0;
    this->el.sortTimeAtSample(tofFactor, tofOffset);
    TofEvent firstEvent = el.getEvent(0);
    const int64_t expectedMinTimeAtSample = calculatedTAtSample(firstEvent, tofFactor, tofOffset);

    TS_ASSERT_EQUALS(expectedMinTimeAtSample, el.getTimeAtSampleMin(tofFactor, tofOffset).totalNanoseconds());
  }

  void test_get_max_time_at_sample() {
    this->fake_data();
    const double tofFactor = 1;
    const double tofOffset = 0;
    this->el.sortTimeAtSample(tofFactor, tofOffset);
    TofEvent lastEvent = el.getEvent(NUMEVENTS - 1);
    const int64_t expectedMaxTimeAtSample = calculatedTAtSample(lastEvent, tofFactor, tofOffset);

    TS_ASSERT_EQUALS(expectedMaxTimeAtSample, el.getTimeAtSampleMax(tofFactor, tofOffset).totalNanoseconds());
  }

  void test_histogram_weights_simple() {
    // 5 events per bin, simple non-weighted
    this->fake_uniform_data(5.0);
    this->test_setX();

    // Multiply by a simple scalar
    el *= 3.2;

    TS_ASSERT_EQUALS(el.getEventType(), WEIGHTED);

    const EventList el3(el); // need to copy to a const method in order to
                             // access the data directly.
    MantidVec X = el3.readX();
    boost::scoped_ptr<MantidVec> Y(el3.makeDataY());
    boost::scoped_ptr<MantidVec> E(el3.makeDataE());
    TS_ASSERT_EQUALS(Y->size(), X.size() - 1);
    for (std::size_t i = 0; i < Y->size(); i++) {
      // 5 events, each with a weight of 3.2
      TS_ASSERT_DELTA((*Y)[i], 5 * 3.2, 1e-6);
      // Error should be scaled the same, by a factor of 3.2 - maintaining the
      // same signal/error ratio.
      TS_ASSERT_DELTA((*E)[i], sqrt((double)5.0) * 3.2, 1e-6);
    }
  }

  void test_histogram_weights() {
    // This one has a weight of 2.0, error is 2.5
    this->fake_uniform_data_weights();

    this->test_setX();       // Set it up
    const EventList el3(el); // need to copy to a const method in order to
                             // access the data directly.
    MantidVec X = el3.readX();
    boost::scoped_ptr<MantidVec> Y(el3.makeDataY());
    boost::scoped_ptr<MantidVec> E(el3.makeDataE());
    TS_ASSERT_EQUALS(Y->size(), X.size() - 1);
    // The data was created so that there should be exactly 2 events per bin
    // The last bin entry will be 0 since we use it as the top boundary of i-1.
    for (std::size_t i = 0; i < Y->size(); i++) {
      TS_ASSERT_EQUALS((*Y)[i], 4.0);
      // Two errors of (2.5) adds up to sqrt(2 * 2.5*2.5)
      TS_ASSERT_DELTA((*E)[i], sqrt(2 * 2.5 * 2.5), 1e-5);
    }
  }

  void test_histogram_with_first_bin_higher_than_first_event() {
    // Make sure the algorithm handles it if the first bin > then the first
    // event tof
    this->fake_uniform_data();

    // Generate the histrogram bins starting at 1000
    MantidVec shared_x;
    for (double tof = BIN_DELTA * 10; tof < BIN_DELTA * (NUMBINS + 1); tof += BIN_DELTA)
      shared_x.emplace_back(tof);
    el.setX(make_cow<HistogramX>(shared_x));

    // Get them back
    const EventList el3(el); // need to copy to a const method in order to
                             // access the data directly.
    MantidVec X = el3.readX();
    boost::scoped_ptr<MantidVec> Y(el3.makeDataY());
    TS_ASSERT_EQUALS(Y->size(), X.size() - 1);

    // The data was created so that there should be exactly 2 events per bin.
    // The first 10 bins (20 events) are empty.
    for (std::size_t i = 0; i < Y->size(); i++) {
      TS_ASSERT_EQUALS((*Y)[i], 2.0);
    }
  }

  void test_histogram_with_first_bin_higher_than_first_event_Weights() {
    // Make sure the algorithm handles it if the first bin > then the first
    // event tof
    this->fake_uniform_data_weights();
    MantidVec shared_x;
    for (double tof = BIN_DELTA * 10; tof < BIN_DELTA * (NUMBINS + 1); tof += BIN_DELTA)
      shared_x.emplace_back(tof);
    el.setX(make_cow<HistogramX>(shared_x));
    const EventList el3(el); // need to copy to a const method in order to
                             // access the data directly.
    MantidVec X = el3.readX();
    boost::scoped_ptr<MantidVec> Y(el3.makeDataY());
    TS_ASSERT_EQUALS(Y->size(), X.size() - 1);
    for (std::size_t i = 0; i < Y->size(); i++) {
      TS_ASSERT_EQUALS((*Y)[i], 4.0);
    }
  }

  void test_random_histogram() {
    this->fake_data();
    this->test_setX();
    const EventList el3(el);
    MantidVec X = el3.readX();
    boost::scoped_ptr<MantidVec> Y(el3.makeDataY());
    TS_ASSERT_EQUALS(Y->size(), X.size() - 1);
    for (std::size_t i = 0; i < X.size() - 1; i++) {
      // No data was generated above 10 ms.
      if (X[i] > 10e6)
        TS_ASSERT_EQUALS((*Y)[i], 0.0);
    }
  }

  void test_histogram_const_call() {
    this->fake_uniform_data();
    this->test_setX(); // Set it up WITH THE default binning
    // Ok, we have this many bins
    TS_ASSERT_EQUALS(this->el.ptrX()->size(), NUMBINS + 1);

    // Make one with half the bins
    MantidVec some_other_x;
    double tof; // in ns
    for (tof = 0; tof < BIN_DELTA * (NUMBINS + 1); tof += BIN_DELTA * 2)
      some_other_x.emplace_back(tof);

    const EventList el3(el); // need to copy to a const method in order to
                             // access the data directly.
    MantidVec Y, E;
    el3.generateHistogram(some_other_x, Y, E);
    TS_ASSERT_EQUALS(Y.size(), some_other_x.size() - 1);
    TS_ASSERT_EQUALS(E.size(), some_other_x.size() - 1);
    // Now there are 4 events per bin
    for (std::size_t i = 0; i < Y.size(); i++)
      TS_ASSERT_EQUALS(Y[i], 4.0);

    // With all this jazz, the original element is unchanged
    TS_ASSERT_EQUALS(this->el.ptrX()->size(), NUMBINS + 1);
  }

  //  void test_histogram_static_function()
  //  {
  //    std::vector<WeightedEvent> events;
  //    events.emplace_back(WeightedEvent(1.0, 0, 2.0, 16.0) );
  //    MantidVec X, Y, E;
  //    X.emplace_back(0.0);
  //    X.emplace_back(10.0);
  //    EventList::histogramForWeightsHelper(events, X, Y, E);
  //    TS_ASSERT_EQUALS(Y.size(), 1 );
  //    TS_ASSERT_DELTA(Y[0], 2.0, 1e-5 );
  //    TS_ASSERT_DELTA(E[0], 4.0, 1e-5 );
  //  }

  //-----------------------------------------------------------------------------------------------
  void test_integrate_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));

      TSM_ASSERT_EQUALS(this_type, el.integrate(0, MAX_TOF, false), el.getNumberEvents());
      TSM_ASSERT_EQUALS(this_type, el.integrate(10, 1, true), el.getNumberEvents());
      // Two events per bin
      TSM_ASSERT_EQUALS(this_type, el.integrate(0, BIN_DELTA, false), 2);
      TSM_ASSERT_EQUALS(this_type, el.integrate(BIN_DELTA * 10, BIN_DELTA * 20, false), 20);
      // Exactly on the first event's TOF?
      TS_ASSERT_EQUALS(el.integrate(100, 100, false), 1);
      // Go past the ends?
      TS_ASSERT_EQUALS(el.integrate(-MAX_TOF, MAX_TOF * 2, false), el.getNumberEvents());
      // Give max < min ?
      TS_ASSERT_EQUALS(el.integrate(1000, 100, false), 0);
    }
  }

  void test_integrate_weighted() {
    this->fake_uniform_data_weights();
    TS_ASSERT_EQUALS(el.integrate(0, MAX_TOF, false), static_cast<double>(el.getNumberEvents()) * 2.0);
    TS_ASSERT_EQUALS(el.integrate(10, 1, true), static_cast<double>(el.getNumberEvents()) * 2.0);
    // Two events per bin
    TS_ASSERT_EQUALS(el.integrate(0, BIN_DELTA, false), 2 * 2.0);
    TS_ASSERT_EQUALS(el.integrate(BIN_DELTA * 10, BIN_DELTA * 20, false), 20 * 2.0);
    // Exactly on the first event's TOF?
    TS_ASSERT_EQUALS(el.integrate(100, 100, false), 1 * 2.0);
    // Go past the ends?
    TS_ASSERT_EQUALS(el.integrate(-MAX_TOF, MAX_TOF * 2, false), static_cast<double>(el.getNumberEvents()) * 2.0);
    // Give max < min ?
    TS_ASSERT_EQUALS(el.integrate(1000, 100, false), 0);
  }

  //-----------------------------------------------------------------------------------------------
  void test_maskTof_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));

      // tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      // How many events did we make?
      TS_ASSERT_EQUALS(el.getNumberEvents(), 2 * MAX_TOF / BIN_DELTA);
      // Mask out 5-10 milliseconds
      double min = MAX_TOF * 0.25;
      double max = MAX_TOF * 0.5;
      el.maskTof(min, max);
      for (std::size_t i = 0; i < el.getNumberEvents(); i++) {
        // No tofs in that range
        TS_ASSERT((el.getEvent(i).tof() < min) || (el.getEvent(i).tof() > max));
      }
      TS_ASSERT_EQUALS(el.getNumberEvents(), 0.75 * 2 * MAX_TOF / BIN_DELTA);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_maskCondition_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));

      // tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      // How many events did we make?
      TS_ASSERT_EQUALS(el.getNumberEvents(), 2 * MAX_TOF / BIN_DELTA);

      // Mask out 5-10 milliseconds
      auto nlen = el.getNumberEvents();
      std::vector<bool> mask(nlen, true);

      // first check no removal
      el.maskCondition(mask);
      TS_ASSERT_EQUALS(el.getNumberEvents(), 2 * MAX_TOF / BIN_DELTA);

      double min = MAX_TOF * 0.25;
      double max = MAX_TOF * 0.5;
      for (size_t i = 0; i < nlen; i++) {
        if ((el.getEvent(i).tof() >= min) && (el.getEvent(i).tof() <= max)) {
          mask[i] = false;
        }
      }
      el.maskCondition(mask);
      for (std::size_t i = 0; i < el.getNumberEvents(); i++) {
        // No tofs in that range
        TS_ASSERT((el.getEvent(i).tof() < min) || (el.getEvent(i).tof() > max));
      }
      TS_ASSERT_EQUALS(el.getNumberEvents(), 0.75 * 2 * MAX_TOF / BIN_DELTA);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_getTofs_and_setTofs() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));

      // Grab original data as it will become "new" data
      MantidVec T;
      el.getTofs(T);

      // Convert to make values something different
      this->el.convertTof(4.0, 2.0);
      double old_value = this->el.getEvent(0).tof();
      size_t old_size = this->el.getNumberEvents();

      // Set "new" data
      this->el.setTofs(T);
      double new_value = this->el.getEvent(0).tof();
      size_t new_size = this->el.getNumberEvents();

      TSM_ASSERT_EQUALS(this_type, old_size, new_size);
      TSM_ASSERT_DELTA(this_type, old_value, new_value * 4.0 + 2.0, 1e-5);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_getPulseTimes() {
    this->fake_uniform_time_data();
    std::vector<DateAndTime> times = el.getPulseTimes();
    TS_ASSERT_EQUALS(times[0].totalNanoseconds(), 0);
    TS_ASSERT_EQUALS(times[1].totalNanoseconds(), 1);
    TS_ASSERT_EQUALS(times[2].totalNanoseconds(), 2);
  }

  void test_getPulseTOFTimes() {
    const DateAndTime startTime{"2023-01-01T12:00:00"};
    const double pulsePeriod{60.0}; // in seconds
    const size_t nPulses{2};
    const size_t eventsPerPulse{3};
    // event list with two pulses, each pulse containing three equally spaced events.
    EventList el = this->generateEvents(startTime, pulsePeriod, nPulses, eventsPerPulse);
    std::vector<DateAndTime> times = el.getPulseTOFTimes();
    std::vector<std::string> dates;
    std::transform(times.cbegin(), times.cend(), std::back_inserter(dates),
                   [](const DateAndTime &time) { return time.toSimpleString(); });
    TS_ASSERT_EQUALS(dates[0], "2023-Jan-01 12:00:00");
    TS_ASSERT_EQUALS(dates[1], "2023-Jan-01 12:00:20");
    TS_ASSERT_EQUALS(dates[2], "2023-Jan-01 12:00:40");
    TS_ASSERT_EQUALS(dates[3], "2023-Jan-01 12:01:00");
    TS_ASSERT_EQUALS(dates[4], "2023-Jan-01 12:01:20");
    TS_ASSERT_EQUALS(dates[5], "2023-Jan-01 12:01:40");
  }

  /**
   *
   * @param factor
   * @param shift
   */
  void test_getPulseTOFTimesAtSample() {
    const DateAndTime startTime{"2023-01-01T12:00:00"};
    const double pulsePeriod{60.0}; // in seconds
    const size_t nPulses{2};
    const size_t eventsPerPulse{3};
    // event list with two pulses, each pulse containing three equally spaced events.
    EventList el = this->generateEvents(startTime, pulsePeriod, nPulses, eventsPerPulse);
    const double factor{0.5};
    const double shift{3000000}; // three seconds in units of microseconds
    std::vector<DateAndTime> times = el.getPulseTOFTimesAtSample(factor, shift);
    std::vector<std::string> dates;
    std::transform(times.cbegin(), times.cend(), std::back_inserter(dates),
                   [](const DateAndTime &time) { return time.toSimpleString(); });
    TS_ASSERT_EQUALS(dates[0], "2023-Jan-01 12:00:03");
    TS_ASSERT_EQUALS(dates[1], "2023-Jan-01 12:00:13");
    TS_ASSERT_EQUALS(dates[2], "2023-Jan-01 12:00:23");
    TS_ASSERT_EQUALS(dates[3], "2023-Jan-01 12:01:03");
    TS_ASSERT_EQUALS(dates[4], "2023-Jan-01 12:01:13");
    TS_ASSERT_EQUALS(dates[5], "2023-Jan-01 12:01:23");
  }
  //-----------------------------------------------------------------------------------------------
  void test_convertTof_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      size_t old_num = this->el.getNumberEvents();
      // Initial X values
      TS_ASSERT_DELTA(this->el.readX()[0], 0.0, 1e-4);
      TS_ASSERT_DELTA(this->el.readX()[1], MAX_TOF, 1e-4);

      // Do convert
      this->el.convertTof(2.5, 1.);
      // Unchanged size
      TS_ASSERT_EQUALS(old_num, this->el.getNumberEvents());
      // Original tofs were 100, 5100, 10100, etc.)
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(0).tof(), 251.0);
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(1).tof(), 12751.0);
      // Modified X values
      TS_ASSERT_DELTA(this->el.readX()[0], 1.0, 1e-4);
      TS_ASSERT_DELTA(this->el.readX()[1], MAX_TOF * 2.5 + 1.0, 1e-4);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_addTof_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      size_t old_num = this->el.getNumberEvents();
      // Do convert
      this->el.addTof(123.0);
      // Unchanged size
      TS_ASSERT_EQUALS(old_num, this->el.getNumberEvents());
      // Original tofs were 100, 5100, 10100, etc.)
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(0).tof(), 223.0);
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(1).tof(), 5223.0);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_scaleTof_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      size_t old_num = this->el.getNumberEvents();
      // Do convert
      this->el.scaleTof(2.5);
      // Unchanged size
      TS_ASSERT_EQUALS(old_num, this->el.getNumberEvents());
      // Original tofs were 100, 5100, 10100, etc.)
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(0).tof(), 250.0);
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(1).tof(), 12750.0);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_convertUnitsQuickly_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      size_t old_num = this->el.getNumberEvents();
      this->el.convertUnitsQuickly(3.0, 2.0);
      // Unchanged size
      TS_ASSERT_EQUALS(old_num, this->el.getNumberEvents());
      // Original tofs were 100, 5100, 10100, etc.). This becomes 3^x^2
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(0).tof(), 3 * 100 * 100.);
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(1).tof(), 3 * 5100 * 5100.);
    }
  }

  /// Dummy unit for testing conversion
  class DummyUnit1 : public Mantid::Kernel::Units::Degrees {
    double singleToTOF(const double x) const override { return x * 10.; }
    double singleFromTOF(const double tof) const override { return tof / 10.; }
  };

  /// Dummy unit for testing conversion
  class DummyUnit2 : public Mantid::Kernel::Units::Degrees {
    double singleToTOF(const double x) const override { return x / 20.; }
    double singleFromTOF(const double tof) const override { return tof * 20.; }
  };

  //-----------------------------------------------------------------------------------------------
  void test_convertUnitsViaTof_failures() {
    DummyUnit1 fromUnit;
    DummyUnit2 toUnit;
    TS_ASSERT_THROWS_ANYTHING(el.convertUnitsViaTof(nullptr, nullptr));
    // Not initalized
    TS_ASSERT_THROWS_ANYTHING(el.convertUnitsViaTof(&fromUnit, &toUnit));
  }

  //-----------------------------------------------------------------------------------------------
  void test_convertUnitsViaTof_allTypes() {
    DummyUnit1 fromUnit;
    DummyUnit2 toUnit;
    fromUnit.initialize(1, 2, {});
    toUnit.initialize(1, 2, {});
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_data();
      el.switchTo(static_cast<EventType>(this_type));
      size_t old_num = this->el.getNumberEvents();
      this->el.convertUnitsViaTof(&fromUnit, &toUnit);
      // Unchanged size
      TS_ASSERT_EQUALS(old_num, this->el.getNumberEvents());
      // Original tofs were 100, 5100, 10100, etc.). This becomes x * 200.
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(0).tof(), 100 * 200.);
      TSM_ASSERT_EQUALS(this_type, this->el.getEvent(1).tof(), 5100 * 200.);
    }
  }

  void test_addPulseTime_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_time_data();
      el.switchTo(static_cast<EventType>(this_type));
      size_t old_num = this->el.getNumberEvents();
      // Do convert
      if (static_cast<EventType>(this_type) == WEIGHTED_NOTIME) {
        TS_ASSERT_THROWS_ANYTHING(this->el.addPulsetime(123e-9);)
      } else {
        this->el.addPulsetime(123e-9);
        // Unchanged size
        TS_ASSERT_EQUALS(old_num, this->el.getNumberEvents());
        // original times were 0, 1, etc. nansoeconds
        TSM_ASSERT_EQUALS(this_type, this->el.getEvent(0).pulseTime().totalNanoseconds(), 123);
        TSM_ASSERT_EQUALS(this_type, this->el.getEvent(1).pulseTime().totalNanoseconds(), 124);
        TSM_ASSERT_EQUALS(this_type, this->el.getEvent(2).pulseTime().totalNanoseconds(), 125);
      }
    }
  }

  void test_addPulseTimes_vector_throws_if_size_not_match_number_events() {
    // Go through each possible EventType as the input
    const std::vector<double> offsets = {1, 2, 3, 4, 5, 6};
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_time_data();
      el.switchTo(static_cast<EventType>(this_type));
      // Do convert
      TS_ASSERT_THROWS(this->el.addPulsetimes(offsets), std::runtime_error &);
    }
  }

  void test_addPulseTimes_vector_allTypes() {
    // Go through each possible EventType as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      this->fake_uniform_time_data();
      el.switchTo(static_cast<EventType>(this_type));
      const size_t old_num = this->el.getNumberEvents();
      std::vector<double> offsets(old_num, 123e-9);
      // Do convert
      if (static_cast<EventType>(this_type) == WEIGHTED_NOTIME) {
        TS_ASSERT_THROWS_ANYTHING(this->el.addPulsetimes(offsets))
      } else {
        this->el.addPulsetimes(offsets);
        // Unchanged size
        TS_ASSERT_EQUALS(old_num, this->el.getNumberEvents());
        // original times were 0, 1, etc. nansoeconds
        TSM_ASSERT_EQUALS(this_type, this->el.getEvent(0).pulseTime().totalNanoseconds(), 123);
        TSM_ASSERT_EQUALS(this_type, this->el.getEvent(1).pulseTime().totalNanoseconds(), 124);
        TSM_ASSERT_EQUALS(this_type, this->el.getEvent(2).pulseTime().totalNanoseconds(), 125);
      }
    }
  }

  void test_sortByTimeAtSample_uniform_pulse_time() {
    // Go through each possible EventType (except the no-time one) as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      EventType curType = static_cast<EventType>(this_type);
      // Since input data has varying pulse time, and same TOF on all events.
      // Nothing interesting happens here.
      if (curType == WEIGHTED_NOTIME) {
        continue;
      }

      EventList el = this->fake_uniform_pulse_data(curType, 1);
      el.switchTo(curType);

      const double tofFactor = 1; // L1 / (L1 + L2)
      const double tofShift = 0;
      TS_ASSERT_THROWS_NOTHING(el.sortTimeAtSample(tofFactor, tofShift));
      if (curType != WEIGHTED_NOTIME) {
        for (size_t i = 1; i < el.getNumberEvents(); i++) {
          TSM_ASSERT_LESS_THAN_EQUALS(this_type, el.getEvent(i - 1).pulseTime(), el.getEvent(i).pulseTime());
        }
      }
    }
  }

  void test_sortByTimeAtSample_random_tof_time() {
    for (int this_type = 0; this_type < 3; this_type++) {
      EventType curType = static_cast<EventType>(this_type);
      if (curType == WEIGHTED_NOTIME)
        continue;

      EventList el = this->fake_random_tof_constant_pulse_data(curType, 10);
      el.switchTo(curType);

      const double tofFactor = 1; // L1 / (L1 + L2)
      const double tofShift = 0;

      TS_ASSERT_THROWS_NOTHING(el.sortTimeAtSample(tofFactor, tofShift));

      for (size_t i = 1; i < el.getNumberEvents(); i++) {
        TSM_ASSERT_LESS_THAN_EQUALS(this_type, el.getEvent(i - 1).tof(), el.getEvent(i).tof());
      }
    }
  }

  void test_sortByTimeAtSample_random_tof_and_pulse_time() {
    for (int this_type = 0; this_type < 3; this_type++) {
      EventType curType = static_cast<EventType>(this_type);
      EventList el = this->fake_data();
      el.switchTo(curType);

      if (curType == WEIGHTED_NOTIME) {
        continue;
      }

      const double tofFactor = 1; // L1 / (L1 + L2)
      const double tofShift = 0;

      TS_ASSERT_THROWS_NOTHING(el.sortTimeAtSample(tofFactor, tofShift));

      for (size_t i = 1; i < el.getNumberEvents(); i++) {
        auto tAtSample1 =
            el.getEvent(i - 1).pulseTime().totalNanoseconds() + static_cast<int64_t>(el.getEvent(i - 1).tof() * 1e3);
        auto tAtSample2 =
            el.getEvent(i).pulseTime().totalNanoseconds() + static_cast<int64_t>(el.getEvent(i).tof() * 1e3);
        TSM_ASSERT_LESS_THAN_EQUALS(this_type, tAtSample1, tAtSample2);
      }
    }
  }

  void test_sortByPulseTime_random_tof_and_pulse_time() {
    for (int this_type = 0; this_type < 3; this_type++) {
      EventType curType = static_cast<EventType>(this_type);
      EventList el = this->fake_data();
      el.switchTo(curType);

      if (curType == WEIGHTED_NOTIME) {
        continue;
      }

      TS_ASSERT_THROWS_NOTHING(el.sortPulseTime());

      for (size_t i = 1; i < el.getNumberEvents(); i++) {
        auto tAtSample1 = el.getEvent(i - 1).pulseTime().totalNanoseconds();
        auto tAtSample2 = el.getEvent(i).pulseTime().totalNanoseconds();
        TSM_ASSERT_LESS_THAN_EQUALS(this_type, tAtSample1, tAtSample2);
      }
    }
  }

  void test_sortByPulseTimeTOF_random_tof_and_pulse_time() {
    for (int this_type = 0; this_type < 3; this_type++) {
      EventType curType = static_cast<EventType>(this_type);
      EventList el = this->fake_data();
      el.switchTo(curType);

      if (curType == WEIGHTED_NOTIME) {
        continue;
      }

      TS_ASSERT_THROWS_NOTHING(el.sortPulseTimeTOF());

      for (size_t i = 1; i < el.getNumberEvents(); i++) {
        TSM_ASSERT_LESS_THAN_EQUALS(this_type, el.getEvent(i - 1).pulseTime(), el.getEvent(i).pulseTime());
        if (el.getEvent(i - 1).pulseTime() == el.getEvent(i).pulseTime())
          TSM_ASSERT_LESS_THAN_EQUALS(this_type, el.getEvent(i - 1).tof(), el.getEvent(i).tof());
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  void test_filterByPulseTime() {
    // Go through each possible EventType (except the no-time one) as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      EventType curType = static_cast<EventType>(this_type);
      this->fake_data();
      el.switchTo(curType);

      // Filter into this
      EventList out;
      // Manually set a sort mode to verify that is it is switched afterward
      out.setSortOrder(Mantid::DataObjects::TOF_SORT);

      if (curType == WEIGHTED_NOTIME) {
        TS_ASSERT_THROWS(el.filterByPulseTime(100, 200, out), const std::runtime_error &);
      } else {
        TS_ASSERT_THROWS_NOTHING(el.filterByPulseTime(100, 200, out););

        int numGood = 0;
        for (std::size_t i = 0; i < el.getNumberEvents(); i++)
          if ((el.getEvent(i).pulseTime() >= 100) && (el.getEvent(i).pulseTime() < 200))
            numGood++;

        // Good # of events.
        TS_ASSERT_EQUALS(numGood, out.getNumberEvents());
        TS_ASSERT_EQUALS(curType, out.getEventType());
        TS_ASSERT_EQUALS(Mantid::DataObjects::PULSETIME_SORT, out.getSortType());

        for (std::size_t i = 0; i < out.getNumberEvents(); i++) {
          // Check that the times are within the given limits.
          TSM_ASSERT_LESS_THAN_EQUALS(this_type, DateAndTime(100), out.getEvent(i).pulseTime());
          TS_ASSERT_LESS_THAN(out.getEvent(i).pulseTime(), DateAndTime(200));
        }
      }
    }
  }

  void test_filterByPulseTime_output_same_as_input_throws() {
    TS_ASSERT_THROWS(el.filterByPulseTime(100, 200, el), const std::invalid_argument &);
  }

  void test_filterByPulseTime_withTimeROI() {
    // Go through each possible EventType (except the no-time one) as the input
    for (int this_type = 0; this_type < 3; this_type++) {
      EventType curType = static_cast<EventType>(this_type);
      this->fake_data();
      el.switchTo(curType);

      // Filter into this
      EventList *out = new EventList();
      // Manually set a sort mode to verify it has switched
      out->setSortOrder(Mantid::DataObjects::TOF_SORT);
      Kernel::TimeROI *timeRoi = nullptr;
      TS_ASSERT_THROWS(el.filterByPulseTime(timeRoi, out), const std::invalid_argument &);
      timeRoi = new Kernel::TimeROI();
      timeRoi->addROI(100, 200);
      timeRoi->addROI(250, 300);
      if (curType == WEIGHTED_NOTIME) {
        TS_ASSERT_THROWS(el.filterByPulseTime(timeRoi, out), const std::runtime_error &);
      } else {
        TS_ASSERT_THROWS_NOTHING(el.filterByPulseTime(timeRoi, out));

        int numGood = 0;
        for (std::size_t i = 0; i < el.getNumberEvents(); i++)
          if (((el.getEvent(i).pulseTime() >= 100) && (el.getEvent(i).pulseTime() < 200)) ||
              ((el.getEvent(i).pulseTime() >= 250) && (el.getEvent(i).pulseTime() < 300)))
            numGood++;

        // Good # of events.
        TS_ASSERT_EQUALS(numGood, out->getNumberEvents());
        TS_ASSERT_EQUALS(curType, out->getEventType());

        for (std::size_t i = 0; i < out->getNumberEvents(); i++) {
          // Check that the times are within the given limits.
          TSM_ASSERT_LESS_THAN_EQUALS(this_type, DateAndTime(100), out->getEvent(i).pulseTime());
          TS_ASSERT_LESS_THAN(out->getEvent(i).pulseTime(), DateAndTime(300));
        }
      }
    }
  }

  /**
   * Helper method to calculate the epoch time in nanoseconds of the event at
   * the sample. Assuming elastic scattering.
   * @param event
   * @return
   */
  int64_t calculatedTAtSample(const TofEvent &event, const double &L1, const double &L2) {
    return event.pulseTime().totalNanoseconds() + static_cast<int64_t>(event.tof() * 1e3 * L1 / (L1 + L2));
  }

  //-----------------------------------------------------------------------------------------------
  void do_testSplit_FilterInPlace(bool weighted) {
    this->fake_uniform_time_data();
    if (weighted)
      el *= 3.0;

    Kernel::TimeROI *timeRoi = new TimeROI();
    timeRoi->addROI(100, 200);
    timeRoi->addROI(150, 250);
    timeRoi->addROI(300, 350);

    // Do the splitting
    el.filterInPlace(timeRoi);

    // 100-249; 300-349 are in the output, everything else is gone.
    TS_ASSERT_EQUALS(el.getNumberEvents(), 200);
    if (weighted) {
      // First event is at 100.
      TS_ASSERT_EQUALS(el.getEvent(0).pulseTime(), 100);
      TS_ASSERT_EQUALS(el.getEvent(149).pulseTime(), 249);
      TS_ASSERT_EQUALS(el.getEvent(150).pulseTime(), 300);
      TS_ASSERT_EQUALS(el.getEvent(199).pulseTime(), 349);
      TS_ASSERT_EQUALS(el.getEvent(0).weight(), 3.0);
    } else {
      // First event is at 100.
      TS_ASSERT_EQUALS(el.getEvent(0).pulseTime(), 100);
      TS_ASSERT_EQUALS(el.getEvent(149).pulseTime(), 249);
      TS_ASSERT_EQUALS(el.getEvent(150).pulseTime(), 300);
      TS_ASSERT_EQUALS(el.getEvent(199).pulseTime(), 349);
    }
  }

  //-----------------------------------------------------------------------------------------------
  void do_testSplit_FilterInPlace_Nothing(bool weighted) {
    this->fake_uniform_time_data();
    if (weighted)
      el.switchTo(WEIGHTED);

    Kernel::TimeROI *timeRoi = new TimeROI();
    timeRoi->addROI(1500, 1700);

    // Do the splitting
    el.filterInPlace(timeRoi);

    // Nothing left
    TS_ASSERT_EQUALS(el.getNumberEvents(), 0);
  }

  //-----------------------------------------------------------------------------------------------
  void do_testSplit_FilterInPlace_Everything(bool weighted) {
    this->fake_uniform_time_data();
    if (weighted)
      el *= 3.0;

    Kernel::TimeROI *timeRoi = new TimeROI();
    timeRoi->addROI(-10, 1700);

    // Do the splitting
    el.filterInPlace(timeRoi);

    // Nothing left
    TS_ASSERT_EQUALS(el.getNumberEvents(), 1000);
    if (weighted) {
      TS_ASSERT_EQUALS(el.getEvent(0).weight(), 3.0);
    }
  }

  void test_filterInPlace_all_permutations() {
    do_testSplit_FilterInPlace(false);
    do_testSplit_FilterInPlace_Nothing(false);
    do_testSplit_FilterInPlace_Everything(false);
    do_testSplit_FilterInPlace(true);
    do_testSplit_FilterInPlace_Nothing(true);
    do_testSplit_FilterInPlace_Everything(true);
  }

  void test_filterInPlace_notime_throws() {
    this->fake_uniform_time_data();
    el.switchTo(WEIGHTED_NOTIME);
    Kernel::TimeROI *timeRoi = new TimeROI();
    timeRoi->addROI(0, 100);
    TS_ASSERT_THROWS(el.filterInPlace(timeRoi), const std::runtime_error &)
  }

  void test_filterInPlace_emptyROI_throws() {
    this->fake_uniform_time_data();
    Kernel::TimeROI *timeRoi = new TimeROI();
    TS_ASSERT_THROWS(el.filterInPlace(timeRoi), const std::invalid_argument &)
  }

  void test_filterInPlace_nullptr_throws() {
    this->fake_uniform_time_data();
    Kernel::TimeROI *timeRoi = nullptr;
    TS_ASSERT_THROWS(el.filterInPlace(timeRoi), const std::runtime_error &)
  }

  //----------------------------------------------------------------------------------------------
  void test_ParallelizedSorting() {
    for (int this_type = 0; this_type < 3; this_type++) {
      bool verbose = false;
      if (verbose) {
        std::cout << "\n";
        NUMEVENTS = 100000000;
      } else {
        NUMEVENTS = 100;
      }

      if (verbose)
        std::cout << NUMEVENTS << " events:\n";
      Timer timer1;
      fake_data();
      el.switchTo(static_cast<EventType>(this_type));

      if (verbose)
        std::cout << "   - " << timer1.elapsed() << " seconds to create.\n";

      Timer timer2;
      el.sortTof();
      if (verbose)
        std::cout << "   - " << timer2.elapsed() << " seconds to sortTof (original).\n";
      TS_ASSERT(checkSort("sortTof"));
    }
  }

  //----------------------------------------------------------------------------------------------
  void test_compressEvents_InPlace_or_Not() {
    for (int this_type = 0; this_type < 3; this_type++) {
      for (size_t inplace = 0; inplace < 2; inplace++) {
        el = EventList();
        el.addEventQuickly(TofEvent(1.0, 22));
        el.addEventQuickly(TofEvent(1.2, 33));
        el.addEventQuickly(TofEvent(30.3, 44));
        el.addEventQuickly(TofEvent(30.2, 55));
        el.addEventQuickly(TofEvent(30.25, 66));
        el.addEventQuickly(TofEvent(34.0, 55));

        el.switchTo(static_cast<EventType>(this_type));

        double mult = 1.0;
        if (this_type > 0) {
          mult = 2.0;
          el *= mult;
        }

        EventList *el_out = &el;
        if (!inplace) {
          el_out = new EventList();
        }

        TS_ASSERT_THROWS_NOTHING(el.compressEvents(1.0, el_out);)

        // Right number of events, of the type without times
        TS_ASSERT_EQUALS(el_out->getEventType(), WEIGHTED_NOTIME);
        TS_ASSERT_EQUALS(el_out->getNumberEvents(), 3);
        TS_ASSERT(el_out->isSortedByTof());

        if (el_out->getNumberEvents() == 3) {
          TS_ASSERT_DELTA(el_out->getEvent(0).tof(), 1.1, 1e-5);
          TS_ASSERT_DELTA(el_out->getEvent(0).weight(), 2 * mult, 1e-5);
          // Error squared is multiplied by mult (squared)
          TS_ASSERT_DELTA(el_out->getEvent(0).errorSquared(), 2 * mult * mult, 1e-5);

          TS_ASSERT_DELTA(el_out->getEvent(1).tof(), 30.25, 1e-5);
          TS_ASSERT_DELTA(el_out->getEvent(1).weight(), 3 * mult, 1e-5);
          TS_ASSERT_DELTA(el_out->getEvent(1).errorSquared(), 3 * mult * mult, 1e-5);

          TS_ASSERT_DELTA(el_out->getEvent(2).tof(), 34.0, 1e-5);
          TS_ASSERT_DELTA(el_out->getEvent(2).weight(), 1 * mult, 1e-5);
          TS_ASSERT_DELTA(el_out->getEvent(2).errorSquared(), 1 * mult * mult, 1e-5);

          // Now the memory must be well used
          TS_ASSERT_EQUALS(el_out->getWeightedEventsNoTime().capacity(), 3);
        }

        if (!inplace)
          delete el_out;
      } // inplace
    }   // starting event type
  }

  void test_compressFatEvents() {
    // no pulse time should throw an exception
    EventList el_notime_output;
    EventList el_notime = this->fake_data(WEIGHTED_NOTIME);
    TS_ASSERT_THROWS(el_notime.compressFatEvents(10., DateAndTime(0), 10., &el_notime_output),
                     const std::invalid_argument &);

    // integration range
    const double XMIN = 0.;
    const double XMAX = 1.e7;

    // regular events should compress decently well
    EventList el_output;
    this->fake_uniform_data_weights(TOF);
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(20000., el.getPulseTimeMin(), 5., &el_output));
    // TS_ASSERT_EQUALS(el_output.getNumberEvents(), 401); // osx creates 400
    TS_ASSERT_EQUALS(el_output.integrate(XMIN, XMAX, true), el.integrate(XMIN, XMAX, true));

    // weighted events with time is the main use case
    EventList el_weight_output;
    this->fake_uniform_data_weights(WEIGHTED);
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(20000., el.getPulseTimeMin(), 5., &el_weight_output));
    // TS_ASSERT_EQUALS(el_weight_output.getNumberEvents(), 401); // osx creates
    // 400
    TS_ASSERT_EQUALS(el_weight_output.integrate(XMIN, XMAX, true), el.integrate(XMIN, XMAX, true));

    // change the start time to see that events don't make it in
    el_weight_output.clear();
    // previous run's this->fake_uniform_data_weights(WEIGHTED);
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(20000., el.getPulseTimeMax(), 5., &el_weight_output));
    TS_ASSERT_EQUALS(el_weight_output.getNumberEvents(), 1);
    TS_ASSERT_DELTA(el_weight_output.integrate(XMIN, XMAX, true), 2., .0001);
  }

  void test_compressWeightedEvents() {
    this->fake_uniform_data_weights(WEIGHTED);
    EventList uniformOut;
    TS_ASSERT_THROWS_NOTHING(el.compressEvents(1., &uniformOut));
    TS_ASSERT_DIFFERS(el, uniformOut);

    // compress again and see that the results are the same as doing it once
    EventList uniformOut2;
    TS_ASSERT_THROWS_NOTHING(el.compressEvents(10., &uniformOut2));
    TS_ASSERT_THROWS_NOTHING(uniformOut.compressEvents(10., &uniformOut));
    TS_ASSERT_EQUALS(uniformOut, uniformOut2);

    // test with varying weights
    this->fake_uniform_data_changing_weights();
    EventList varyingOut(el);
    TS_ASSERT_THROWS_NOTHING(el.compressEvents(1., &varyingOut));
    TS_ASSERT_DIFFERS(el, varyingOut);

    // they should give different results because they have different weighting
    // structures
    TS_ASSERT_DIFFERS(uniformOut, varyingOut);

    // compress again to see that changing weights works
    EventList varyingOut2;
    TS_ASSERT_THROWS_NOTHING(el.compressEvents(10., &varyingOut2));
    TS_ASSERT_THROWS_NOTHING(varyingOut.compressEvents(10., &varyingOut));
    TS_ASSERT_EQUALS(varyingOut, varyingOut2);
  }

  void test_compressWeightedFatEvents() {
    this->fake_uniform_data_weights(WEIGHTED);
    EventList uniformOut;
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(10000., el.getPulseTimeMin(), 30., &uniformOut));
    TS_ASSERT_DIFFERS(el, uniformOut);

    this->fake_uniform_data_changing_weights();
    EventList varyingOut(el);
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(10000., el.getPulseTimeMin(), 30., &varyingOut));
    TS_ASSERT_DIFFERS(el, varyingOut);

    // they should give different results because they have different weighting
    // structures
    TS_ASSERT_DIFFERS(uniformOut, varyingOut);
  }

  void test_getEventsFrom() {
    std::vector<TofEvent> *rel;
    TS_ASSERT_THROWS_NOTHING(getEventsFrom(el, rel));
    TS_ASSERT_EQUALS(rel->size(), 3);
    el *= 2.0;

    std::vector<WeightedEvent> *rel2;
    TS_ASSERT_THROWS_NOTHING(getEventsFrom(el, rel2));
    TS_ASSERT_EQUALS(rel2->size(), 3);

    el.compressEvents(0, &el);

    std::vector<WeightedEventNoTime> *rel3;
    TS_ASSERT_THROWS_NOTHING(getEventsFrom(el, rel3));
    TS_ASSERT_EQUALS(rel3->size(), 3);
  }

  void test_getWeights() {
    std::vector<double> result;

    // TOF data should return 1.0
    fake_data();
    TS_ASSERT_EQUALS(el.getEventType(), TOF);
    TS_ASSERT_THROWS_NOTHING(result = el.getWeights());
    TS_ASSERT_EQUALS(result.size(), el.getNumberEvents());
    // right number of entries
    TS_ASSERT_DELTA(result[0], 1.0, 0.000001);
    // first value
    TS_ASSERT_DELTA(result[el.getNumberEvents() - 1], 1.0, 0.000001);
    // last value

    // weighted data test data has 2.0 uniform data
    fake_uniform_data_weights();
    TS_ASSERT_THROWS_NOTHING(result = el.getWeights());
    TS_ASSERT_EQUALS(el.getEventType(), WEIGHTED);
    TS_ASSERT_THROWS_NOTHING(result = el.getWeights());
    TS_ASSERT_EQUALS(result.size(), el.getNumberEvents());
    // right number of entries
    TS_ASSERT_DELTA(result[0], 2.0, 0.000001);
    // first value
    TS_ASSERT_DELTA(result[el.getNumberEvents() - 1], 2.0, 0.000001);
    // last value

    // compress the events to no time weighted events
    el.compressEvents(0, &el);
    TS_ASSERT_EQUALS(el.getEventType(), WEIGHTED_NOTIME);
    TS_ASSERT_THROWS_NOTHING(result = el.getWeights());
    TS_ASSERT_EQUALS(result.size(), el.getNumberEvents());
    // right number of entries
    TS_ASSERT_DELTA(result[0], 2.0, 0.000001);
    // first value
    TS_ASSERT_DELTA(result[el.getNumberEvents() - 1], 2.0, 0.000001);
    // last value
  }

  void test_getWeightErrors() {
    std::vector<double> result;

    // TOF data should return 1.0
    fake_data();
    TS_ASSERT_EQUALS(el.getEventType(), TOF);
    TS_ASSERT_THROWS_NOTHING(result = el.getWeightErrors());
    TS_ASSERT_EQUALS(result.size(), el.getNumberEvents());
    // right number of entries
    TS_ASSERT_DELTA(result[0], 1.0, 0.000001);
    // first value
    TS_ASSERT_DELTA(result[el.getNumberEvents() - 1], 1.0, 0.000001);
    // last value

    // weighted data test data has 2.5 uniform data
    fake_uniform_data_weights();
    TS_ASSERT_EQUALS(el.getEventType(), WEIGHTED);
    TS_ASSERT_THROWS_NOTHING(result = el.getWeightErrors());
    TS_ASSERT_EQUALS(result.size(), el.getNumberEvents());
    // right number of entries
    TS_ASSERT_DELTA(result[0], 2.5, 0.000001);
    // first value
    TS_ASSERT_DELTA(result[el.getNumberEvents() - 1], 2.5, 0.000001);
    // last value

    // compress the events to no time weighted events
    el.compressEvents(0, &el);
    TS_ASSERT_EQUALS(el.getEventType(), WEIGHTED_NOTIME);
    TS_ASSERT_THROWS_NOTHING(result = el.getWeightErrors());
    TS_ASSERT_EQUALS(result.size(), el.getNumberEvents());
    // right number of entries
    TS_ASSERT_DELTA(result[0], 2.5, 0.000001);
    // first value
    TS_ASSERT_DELTA(result[el.getNumberEvents() - 1], 2.5, 0.000001);
    // last value
  }

  void test_compressEvents_log() {
    this->fake_uniform_data(10000.);

    // First lets compare histogramming with compression versus without.
    // This may only work exactly when the smallest tof is equal to the minimum rebin parameter,
    // in this case 100
    MantidVec X, expected_Y, expected_E, Y, E;
    VectorHelper::createAxisFromRebinParams({100., -1., 819200}, X, true);

    // get expected results
    el.generateHistogram(X, expected_Y, expected_E);

    // do compress events with log binning then histogram to compare to un-compressed
    EventList el_output;
    TS_ASSERT_THROWS_NOTHING(el.compressEvents(-1, &el_output));
    el_output.generateHistogram(X, Y, E);

    for (size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_EQUALS(expected_Y[i], Y[i]);
      TS_ASSERT_EQUALS(expected_E[i], E[i]);
    }

    // now check individual events
    TS_ASSERT_EQUALS(el.getNumberEvents(), 9999900);
    TS_ASSERT_EQUALS(el_output.getNumberEvents(), 17);

    // event weights should double for each one, 100, 200, 400, 800...
    // event tofs should double, it will be roughly 150, 300, 600, 1200...

    // don't check last event as bin will be partially filled
    for (int i = 0; i < 16; i++) {
      TS_ASSERT_EQUALS(el_output.getEvent(i).weight(), 100 * pow(2, i))
      TS_ASSERT_EQUALS(el_output.getEvent(i).errorSquared(), 100 * pow(2, i))
      TS_ASSERT_DELTA(el_output.getEvent(i).tof(), 150 * pow(2, i), 0.5)
    }
  }

  void test_compressEvents_log2() {
    // Check that for very sparse events
    // Add four events, one that is repeated
    // Should result in 3 events, with the second of weight 2
    el = EventList();
    el += TofEvent(100, 0);
    el += TofEvent(1000, 0);
    el += TofEvent(1000, 0);
    el += TofEvent(100000, 0);

    // do compress events with log binning
    EventList el_output;
    TS_ASSERT_THROWS_NOTHING(el.compressEvents(-0.01, &el_output))

    // now check individual events
    TS_ASSERT_EQUALS(el_output.getNumberEvents(), 3)

    TS_ASSERT_EQUALS(el_output.getEvent(0).weight(), 1)
    TS_ASSERT_EQUALS(el_output.getEvent(0).errorSquared(), 1)
    TS_ASSERT_DELTA(el_output.getEvent(0).tof(), 100, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(1).weight(), 2)
    TS_ASSERT_EQUALS(el_output.getEvent(1).errorSquared(), 2)
    TS_ASSERT_DELTA(el_output.getEvent(1).tof(), 1000, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(2).weight(), 1)
    TS_ASSERT_EQUALS(el_output.getEvent(2).errorSquared(), 1)
    TS_ASSERT_DELTA(el_output.getEvent(2).tof(), 100000, 1e-5)
  }

  void test_compressEvents_log3() {
    // check the behavior when TOF is zero or negative
    el = EventList();
    el += TofEvent(0, 0);
    el += TofEvent(0.5, 0);
    el += TofEvent(1, 0);

    // Do compress events with log binning
    // Since there is a tof==0 then the first bin_end should be 1
    EventList el_output;
    TS_ASSERT_THROWS_NOTHING(el.compressEvents(-1, &el_output))

    // now check individual events
    TS_ASSERT_EQUALS(el_output.getNumberEvents(), 2)

    TS_ASSERT_EQUALS(el_output.getEvent(0).weight(), 2)
    TS_ASSERT_EQUALS(el_output.getEvent(0).errorSquared(), 2)
    TS_ASSERT_DELTA(el_output.getEvent(0).tof(), 0.25, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(1).weight(), 1)
    TS_ASSERT_EQUALS(el_output.getEvent(1).errorSquared(), 1)
    TS_ASSERT_DELTA(el_output.getEvent(1).tof(), 1, 1e-5)

    // now add a negative TOF and it should throw
    el += TofEvent(-1, 0);
    TS_ASSERT_THROWS(el.compressEvents(-1, &el_output), const std::runtime_error &)
  }

  void test_compressFatEvents_log() {
    el = EventList();
    for (int pulseTime = 0; pulseTime < 5; pulseTime++)
      for (double tof = 100; tof < 51200; tof++)
        el += TofEvent(tof, DateAndTime{pulseTime, 0});

    // First lets compare histogramming with compression versus without.
    // This may only work exactly when the smallest tof is equal to the minimum rebin parameter,
    // in this case 100
    MantidVec X, expected_Y, expected_E, Y, E;
    VectorHelper::createAxisFromRebinParams({100., -1., 51200}, X, true);

    // get expected results
    el.generateHistogram(X, expected_Y, expected_E);

    /// do compress events with log binning
    EventList el_output;
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(-1, DateAndTime{0}, 2., &el_output));

    // check individual events
    // with a pulsetime delta of 2 seconds we should end up with 3 pulsetime groups with 9 events in each group, so 27
    // events total

    TS_ASSERT_EQUALS(el.getNumberEvents(), 255500);
    TS_ASSERT_EQUALS(el_output.getNumberEvents(), 27);

    // First pulse group with pulsetime = 500ms
    // Event weights should double for each one, 200, 400, 800, 1600...
    // Event tofs should double, it will be roughly 150, 300, 600, 1200...
    for (int i = 0; i < 9; i++) {
      TS_ASSERT_EQUALS(el_output.getEvent(i).pulseTime().totalNanoseconds(), 500000000)
      TS_ASSERT_EQUALS(el_output.getEvent(i).weight(), 200 * pow(2, i))
      TS_ASSERT_EQUALS(el_output.getEvent(i).errorSquared(), 200 * pow(2, i))
      TS_ASSERT_DELTA(el_output.getEvent(i).tof(), 150 * pow(2, i), 0.5)
    }

    // Second pulse group with pulsetime = 2500ms
    // Event weights should double for each one, 200, 400, 800, 1600...
    // Event tofs should double, it will be roughly 150, 300, 600, 1200...
    for (int i = 9; i < 18; i++) {
      TS_ASSERT_EQUALS(el_output.getEvent(i).pulseTime().totalNanoseconds(), 2500000000)
      TS_ASSERT_EQUALS(el_output.getEvent(i).weight(), 200 * pow(2, i - 9))
      TS_ASSERT_EQUALS(el_output.getEvent(i).errorSquared(), 200 * pow(2, i - 9))
      TS_ASSERT_DELTA(el_output.getEvent(i).tof(), 150 * pow(2, i - 9), 0.5)
    }

    // Third pulse group with pulsetime = 4000ms
    // Event weights should double for each one, 100, 200, 400, 800...
    // Event tofs should double, it will be roughly 150, 300, 600, 1200...
    for (int i = 18; i < 27; i++) {
      TS_ASSERT_EQUALS(el_output.getEvent(i).pulseTime().totalNanoseconds(), 4000000000)
      TS_ASSERT_EQUALS(el_output.getEvent(i).weight(), 100 * pow(2, i - 18))
      TS_ASSERT_EQUALS(el_output.getEvent(i).errorSquared(), 100 * pow(2, i - 18))
      TS_ASSERT_DELTA(el_output.getEvent(i).tof(), 150 * pow(2, i - 18), 0.5)
    }

    // histogram to compare to un-compressed
    el_output.generateHistogram(X, Y, E);

    for (size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_EQUALS(expected_Y[i], Y[i]);
      TS_ASSERT_EQUALS(expected_E[i], E[i]);
    }
  }

  void test_compressFatEvents_log2() {
    el = EventList();
    el += TofEvent(100, DateAndTime{0, 0});
    el += TofEvent(1000, DateAndTime{0, 0});
    el += TofEvent(1001, DateAndTime{0, 0});
    el += TofEvent(100, DateAndTime{3, 0});
    el += TofEvent(1000, DateAndTime{3, 0});
    el += TofEvent(1001, DateAndTime{3, 0});
    el += TofEvent(100, DateAndTime{6, 0});
    el += TofEvent(1000, DateAndTime{6, 0});
    el += TofEvent(1001, DateAndTime{6, 0});

    // Do compress events with log binning.
    // With a large pulseTime delta of 10 seconds we should end up with 2 events with same pulsetime but different TOFs
    EventList el_output;
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(-1, DateAndTime{0}, 10, &el_output));

    TS_ASSERT_EQUALS(el.getNumberEvents(), 9)
    TS_ASSERT_EQUALS(el_output.getNumberEvents(), 2)

    TS_ASSERT_EQUALS(el_output.getEvent(0).weight(), 3)
    TS_ASSERT_EQUALS(el_output.getEvent(0).errorSquared(), 3)
    TS_ASSERT_DELTA(el_output.getEvent(0).tof(), 100, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(0).pulseTime().totalNanoseconds(), 3000000000, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(1).weight(), 6)
    TS_ASSERT_EQUALS(el_output.getEvent(1).errorSquared(), 6)
    TS_ASSERT_DELTA(el_output.getEvent(1).tof(), 1000.5, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(1).pulseTime().totalNanoseconds(), 3000000000, 1e-5)

    // Do compress events with log binning.
    // With a pulseTime delta of 5 seconds we should end up with 4 events
    el_output = EventList();
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(-1, DateAndTime{0}, 5, &el_output));

    TS_ASSERT_EQUALS(el.getNumberEvents(), 9)
    TS_ASSERT_EQUALS(el_output.getNumberEvents(), 4)

    TS_ASSERT_EQUALS(el_output.getEvent(0).weight(), 2)
    TS_ASSERT_EQUALS(el_output.getEvent(0).errorSquared(), 2)
    TS_ASSERT_DELTA(el_output.getEvent(0).tof(), 100, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(0).pulseTime().totalNanoseconds(), 1500000000, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(1).weight(), 4)
    TS_ASSERT_EQUALS(el_output.getEvent(1).errorSquared(), 4)
    TS_ASSERT_DELTA(el_output.getEvent(1).tof(), 1000.5, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(1).pulseTime().totalNanoseconds(), 1500000000, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(2).weight(), 1)
    TS_ASSERT_EQUALS(el_output.getEvent(2).errorSquared(), 1)
    TS_ASSERT_DELTA(el_output.getEvent(2).tof(), 100, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(2).pulseTime().totalNanoseconds(), 6000000000, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(3).weight(), 2)
    TS_ASSERT_EQUALS(el_output.getEvent(3).errorSquared(), 2)
    TS_ASSERT_DELTA(el_output.getEvent(3).tof(), 1000.5, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(3).pulseTime().totalNanoseconds(), 6000000000, 1e-5)
  }

  void test_compressFatEvents_log3() {
    // check the behavior when TOF is zero or negative
    el = EventList();
    el += TofEvent(0.5, 1);
    el += TofEvent(1, 2);
    el += TofEvent(0, 3);
    el += TofEvent(1, 15000000000); // 15 seconds, one event in second wall clock bin

    // Do compress events with log binning
    // Since there is a tof==0 then the first bin_end should be 1
    EventList el_output;
    TS_ASSERT_THROWS_NOTHING(el.compressFatEvents(-1, DateAndTime{0}, 10, &el_output))

    // now check individual events
    TS_ASSERT_EQUALS(el_output.getNumberEvents(), 3)

    TS_ASSERT_EQUALS(el_output.getEvent(0).weight(), 2)
    TS_ASSERT_EQUALS(el_output.getEvent(0).errorSquared(), 2)
    TS_ASSERT_DELTA(el_output.getEvent(0).tof(), 0.25, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(0).pulseTime().totalNanoseconds(), 2, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(1).weight(), 1)
    TS_ASSERT_EQUALS(el_output.getEvent(1).errorSquared(), 1)
    TS_ASSERT_DELTA(el_output.getEvent(1).tof(), 1, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(1).pulseTime().totalNanoseconds(), 2, 1e-5)

    TS_ASSERT_EQUALS(el_output.getEvent(2).weight(), 1)
    TS_ASSERT_EQUALS(el_output.getEvent(2).errorSquared(), 1)
    TS_ASSERT_DELTA(el_output.getEvent(2).tof(), 1, 1e-5)
    TS_ASSERT_DELTA(el_output.getEvent(2).pulseTime().totalNanoseconds(), 15000000000, 1e-5)

    // now add a negative TOF and it should throw
    el += TofEvent(-1, 0);
    TS_ASSERT_THROWS(el.compressFatEvents(-1, DateAndTime{0}, 10, &el_output), const std::runtime_error &)
  }

  //==================================================================================
  // Mocking functions
  //==================================================================================

  EventList fake_data(EventType eventType = TOF) {
    // Clear the list
    el = EventList();
    if (eventType != TOF)
      el.switchTo(eventType);

    // Create some mostly-reasonable fake data.
    srand(1234); // Fixed random seed
    switch (eventType) {
    case TOF:
      for (int i = 0; i < NUMEVENTS; i++) {
        // Random tof up to 10 ms
        // Random pulse time up to 1000
        el += TofEvent(1e7 * (rand() * 1.0 / RAND_MAX), rand() % 1000);
      }
      break;
    case WEIGHTED:
      for (int i = 0; i < NUMEVENTS; i++) {
        // Random tof up to 10 ms
        // Random pulse time up to 1000
        el += WeightedEvent(TofEvent(1e7 * (rand() * 1.0 / RAND_MAX), rand() % 1000));
      }
      break;
    case WEIGHTED_NOTIME:
      for (int i = 0; i < NUMEVENTS; i++) {
        // Random tof up to 10 ms
        // Random pulse time up to 1000
        el += TofEvent(1e7 * (rand() * 1.0 / RAND_MAX), rand() % 1000);
      }
      break;
    }
    return el;
  }

  /*
   Make some uniformly distributed fake event data distributed by pulse time,
   WITH A CONSTANT TOF.
   */
  EventList fake_uniform_pulse_data(EventType eventType = TOF, double events_per_bin = 2) {
    EventList el;
    if (eventType == TOF) {
      for (double pulse_time = 0; pulse_time < MAX_PULSE_TIME; pulse_time += BIN_DELTA / events_per_bin) {
        el += TofEvent(100, static_cast<size_t>(pulse_time));
      }
    } else if (eventType == WEIGHTED) {
      for (double pulse_time = 0; pulse_time < MAX_PULSE_TIME; pulse_time += BIN_DELTA / events_per_bin) {
        el += WeightedEvent(TofEvent(100, static_cast<size_t>(pulse_time)));
      }
    } else {
      throw std::runtime_error("not implemented: fake_uniform_pulse_data");
    }
    return el;
  }

  EventList fake_random_tof_constant_pulse_data(EventType eventType = TOF, const size_t nEvents = 1000) {
    EventList el;
    if (eventType == TOF) {
      for (size_t i = 0; i < nEvents; ++i) {
        // Random tof up to 10 ms
        el += TofEvent(1e7 * (rand() * 1.0 / RAND_MAX), 0);
      }
    } else if (eventType == WEIGHTED) {
      for (size_t i = 0; i < nEvents; ++i) {
        // Random tof up to 10 ms
        el += WeightedEvent(TofEvent(1e7 * (rand() * 1.0 / RAND_MAX), 0));
      }
    } else {
      throw std::runtime_error("not implemented: fake_random_tof_constant_pulse_data");
    }
    return el;
  }

  /** Create a uniform event list with no weights*/
  void fake_uniform_data(double events_per_bin = 2, bool randomPulseTime = true) {
    // Clear the list
    el = EventList();
    // Create some mostly-reasonable fake data.
    srand(1234); // Fixed random seed
    for (double tof = 100; tof < MAX_TOF; tof += BIN_DELTA / events_per_bin) {
      // tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      if (randomPulseTime) {
        el += TofEvent(tof, rand() % 1000);
      } else {
        el += TofEvent(tof, 0);
      }
    }
    // Create an X axis
    MantidVec X;
    X.resize(2);
    X[0] = 0;
    X[1] = MAX_TOF;
    el.dataX() = X;
  }

  /** Create a uniform event list with each event weight of 2.0, error 2.5 */
  void fake_uniform_data_weights(EventType eventType = WEIGHTED) {
    // Clear the list
    el = EventList();
    if (eventType != TOF)
      el.switchTo(WEIGHTED);

    // Create some mostly-reasonable fake data.
    srand(1234); // Fixed random seed
    int64_t pulsetime = 0;
    // 10^6 nanoseconds for pulse time delta
    const int64_t PULSETIME_DELTA = static_cast<int64_t>(BIN_DELTA / 1000);
    for (double tof = 100; tof < MAX_TOF; tof += BIN_DELTA / 2) {
      // add some jitter into the pulse time with rand
      pulsetime = 10000 * (static_cast<int64_t>(tof) / PULSETIME_DELTA) + (rand() % 1000);
      // tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      if (eventType == TOF) {
        el += TofEvent(tof, pulsetime);
      } else if (eventType == WEIGHTED) {
        el += WeightedEvent(tof, pulsetime, 2.0, 2.5 * 2.5);
      }
    }
  }

  void fake_uniform_data_changing_weights() {
    // Clear the list
    el = EventList();
    el.switchTo(WEIGHTED);

    // Create some mostly-reasonable fake data.
    srand(1234); // Fixed random seed
    int64_t pulsetime = 0;
    // 10^6 nanoseconds for pulse time delta
    const int64_t PULSETIME_DELTA = static_cast<int64_t>(BIN_DELTA / 1000);
    for (double tof = 100; tof < MAX_TOF; tof += BIN_DELTA / 2) {
      // add some jitter into the pulse time with rand
      pulsetime = 10000 * (static_cast<int64_t>(tof) / PULSETIME_DELTA) + (rand() % 1000);
      // tof steps of 5 microseconds, starting at 100 ns, up to 20 msec
      el += WeightedEvent(tof, pulsetime, 2.0,
                          tof / 100); // want non-uniform weights
    }
  }

  void fake_uniform_time_data() {
    // Clear the list
    el = EventList();
    // Create some mostly-reasonable fake data.
    srand(1234); // Fixed random seed
    for (int time = 0; time < 1000; time++) {
      // All pulse times from 0 to 999 in seconds
      el += TofEvent(rand() % 1000,
                     time); // Types::Core::DateAndTime(time*1.0, 0.0) );
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Fake uniform time data more close to SNS case
   */
  void fake_uniform_time_sns_data() {
    // Clear the list
    el = EventList();

    // Create some mostly-reasonable fake data.
    srand(1234); // Fixed random seed
    for (int time = 0; time < 1000; time++) {
      // All pulse times from 0 to 999 in seconds
      DateAndTime pulsetime(static_cast<int64_t>(time * 1000000));
      el += TofEvent(rand() % 1000,
                     pulsetime); // Types::Core::DateAndTime(time*1.0, 0.0) );
    }
  }

  void fake_data_only_two_times(DateAndTime time1, DateAndTime time2) {
    // Clear the list
    el = EventList();
    el += TofEvent(rand() % 1000, time1);
    el += TofEvent(rand() % 1000, time2);
  }

  /** Make a X-vector for histogramming, starting at step and going up in step
   */
  MantidVec makeX(double step, int numbins = 10) {
    MantidVec X;
    for (double tof = step; tof < step * numbins; tof += step) {
      X.emplace_back(tof);
    }
    return X;
  }

  bool checkSort(std::string context) {
    TSM_ASSERT_EQUALS(context, el.getNumberEvents(), NUMEVENTS);
    bool ret = true;
    for (std::size_t i = 1; i < el.getNumberEvents(); i++) {
      if (el.getEvent(i - 1).tof() > el.getEvent(i).tof())
        ret = false;
      if (!ret)
        return ret;
    }
    return ret;
  }

  void test_readYE_throws_without_MRU() {
    const EventList el;
    TS_ASSERT_THROWS(el.readY(), const std::runtime_error &);
    TS_ASSERT_THROWS(el.dataY(), const std::runtime_error &);
    TS_ASSERT_THROWS(el.readE(), const std::runtime_error &);
    TS_ASSERT_THROWS(el.dataE(), const std::runtime_error &);
  }

  void test_counts_works_without_MRU() {
    EventList el;
    TS_ASSERT_THROWS_NOTHING(el.counts());
    TS_ASSERT_THROWS_NOTHING(el.countStandardDeviations());
  }

  void test_setPoints_fails() {
    EventList el;
    el.setHistogram(HistogramData::BinEdges{0, 2});
    TS_ASSERT_THROWS_NOTHING(el.setBinEdges(HistogramData::BinEdges{0, 2}));
    TS_ASSERT_THROWS(el.setPoints(1), const std::runtime_error &);
    // Uncertainties for X are always for Points, this must work.
    TS_ASSERT_THROWS_NOTHING(el.setPointVariances(1));
    TS_ASSERT_THROWS_NOTHING(el.setPointStandardDeviations(1));
  }

  void test_setCounts_fails() {
    EventList el;
    el.setHistogram(HistogramData::BinEdges{0, 2});
    TS_ASSERT_THROWS(el.setCounts(1), const std::runtime_error &);
    TS_ASSERT_THROWS(el.setCountVariances(1), const std::runtime_error &);
    TS_ASSERT_THROWS(el.setCountStandardDeviations(1), const std::runtime_error &);
  }

  void test_setFrequencies_fails() {
    EventList el;
    el.setHistogram(HistogramData::BinEdges{0, 2});
    TS_ASSERT_THROWS(el.setFrequencies(1), const std::runtime_error &);
    TS_ASSERT_THROWS(el.setFrequencyVariances(1), const std::runtime_error &);
    TS_ASSERT_THROWS(el.setFrequencyStandardDeviations(1), const std::runtime_error &);
  }

  void test_setShared_fails() {
    EventList el;
    TS_ASSERT_THROWS_NOTHING(el.setSharedX(el.sharedX()));
    TS_ASSERT_THROWS(el.setSharedY(el.sharedY()), const std::runtime_error &);
    TS_ASSERT_THROWS(el.setSharedE(el.sharedE()), const std::runtime_error &);
  }

  void test_mutable_access_fails() {
    EventList el;
    TS_ASSERT_THROWS_NOTHING(el.mutableX());
    TS_ASSERT_THROWS(el.mutableY(), const std::runtime_error &);
    TS_ASSERT_THROWS(el.mutableE(), const std::runtime_error &);
  }

  void test_histogram() {
    EventList el;
    el += TofEvent(1);
    el.setHistogram(HistogramData::BinEdges{0, 2, 4});
    auto histogram = el.histogram();
    TS_ASSERT(histogram.sharedY());
    TS_ASSERT(histogram.sharedE());
    el += TofEvent(1);
    el += TofEvent(3);
    TS_ASSERT_EQUALS(histogram.y()[0], 1.0);
    TS_ASSERT_EQUALS(histogram.y()[1], 0.0);
    auto updated = el.histogram();
    TS_ASSERT_EQUALS(updated.y()[0], 2.0);
    TS_ASSERT_EQUALS(updated.y()[1], 1.0);
    TS_ASSERT_EQUALS(updated.e()[0], M_SQRT2);
    TS_ASSERT_EQUALS(updated.e()[1], 1.0);
  }

  void test_histogram_no_mru() {
    EventList el;
    auto hist1 = el.histogram();
    auto hist2 = el.histogram();
    TS_ASSERT_EQUALS(hist1.sharedX(), hist2.sharedX());
    TS_ASSERT_DIFFERS(hist1.sharedY(), hist2.sharedY());
    TS_ASSERT_DIFFERS(hist1.sharedE(), hist2.sharedE());
  }

  void test_setHistogram() {
    EventList el;
    HistogramData::Histogram histogram(HistogramData::BinEdges{0, 2, 4});
    TS_ASSERT_THROWS_NOTHING(el.setHistogram(histogram));
    TS_ASSERT_EQUALS(el.sharedX(), histogram.sharedX());
    histogram.setCounts(2);
    TS_ASSERT_THROWS(el.setHistogram(histogram), const std::runtime_error &);
    HistogramData::Histogram points(HistogramData::Points{0, 2});
    TS_ASSERT_THROWS(el.setHistogram(points), const std::runtime_error &);
  }

  void test_YMode() {
    EventList e;
    TS_ASSERT_EQUALS(e.yMode(), HistogramData::Histogram::YMode::Counts);
  }

  void test_setHistogram_rejects_YMode_Frequencies() {
    EventList e;
    HistogramData::Histogram h(BinEdges(0));
    h.setYMode(HistogramData::Histogram::YMode::Counts);
    TS_ASSERT_THROWS_NOTHING(e.setHistogram(h));
    h.setYMode(HistogramData::Histogram::YMode::Frequencies);
    TS_ASSERT_THROWS(e.setHistogram(h), const std::runtime_error &);
  }

  void test_setHistogram_preserves_YMode_when_setting_uninitialized() {
    EventList e;
    HistogramData::Histogram h(BinEdges(0));
    TS_ASSERT_EQUALS(h.yMode(), HistogramData::Histogram::YMode::Uninitialized);
    TS_ASSERT_THROWS_NOTHING(e.setHistogram(h));
    TS_ASSERT_EQUALS(e.yMode(), HistogramData::Histogram::YMode::Counts);
  }

  void test_histogram_has_correct_YMode() {
    EventList e;
    e.setYMode(HistogramData::Histogram::YMode::Frequencies);
    TS_ASSERT_EQUALS(e.histogram().yMode(), HistogramData::Histogram::YMode::Frequencies);
  }

  void test_YMode_affects_event_data_interpretation() {
    // Data generated from the events is put into the histogram as Y and E.
    // Depending on the YMode, this is interpreted as Counts or Frequencies. In
    // particular, data generated from events is *not* forcibly intepreted as
    // counts, i.e., internally we do not use Histogram.setCounts(), since the
    // bin width may be absorbed into the event weight.
    EventList e;
    e += TofEvent(1);
    e += TofEvent(1);
    e += TofEvent(3);
    e.setHistogram(HistogramData::BinEdges{0, 2, 4});
    auto countHist = e.histogram();
    TS_ASSERT_EQUALS(countHist.counts()[0], 2.0);
    TS_ASSERT_EQUALS(countHist.counts()[1], 1.0);
    // Intepret events as if weighted by bin width, happens, e.g., in 'Divide'.
    e.setYMode(HistogramData::Histogram::YMode::Frequencies);
    auto freqHist = e.histogram();
    TS_ASSERT_EQUALS(freqHist.counts()[0], 4.0);
    TS_ASSERT_EQUALS(freqHist.counts()[1], 2.0);
  }

  void test_generateHistogramUnsortedLinear_TOF() {
    const auto e = createLinearTestData();
    run_generateHistogramUnsortedTest(e, {0., 0.1, 100.}, 999.);
    run_generateHistogramUnsortedTest(e, {50., 1.0, 100.}, 490.);
  }

  void test_generateHistogramUnsortedLinear_WEIGHTED() {
    const auto e = createLinearTestData(WEIGHTED);
    run_generateHistogramUnsortedTest(e, {0., 0.1, 100.}, 999.);
    run_generateHistogramUnsortedTest(e, {50., 1.0, 100.}, 490.);
  }

  void test_generateHistogramUnsortedLinear_WEIGHTED_NOTIME() {
    const auto e = createLinearTestData(WEIGHTED_NOTIME);
    run_generateHistogramUnsortedTest(e, {0., 0.1, 100.}, 999.);
    run_generateHistogramUnsortedTest(e, {50., 1.0, 100.}, 490.);
  }

  void test_generateHistogramUnsortedLog_TOF() {
    const auto e = createLogTestData();
    run_generateHistogramUnsortedTest(e, {1., -0.001, 1.1}, 95.);
    run_generateHistogramUnsortedTest(e, {1.05, -0.002, 1.1}, 45.);
  }

  void test_generateHistogramUnsortedLog_WEIGHTED() {
    const auto e = createLogTestData(WEIGHTED);
    run_generateHistogramUnsortedTest(e, {1., -0.001, 1.1}, 95.);
    run_generateHistogramUnsortedTest(e, {1.05, -0.002, 1.1}, 45.);
  }

  void test_generateHistogramUnsortedLog_WEIGHTED_NOTIME() {
    const auto e = createLogTestData(WEIGHTED_NOTIME);
    run_generateHistogramUnsortedTest(e, {1., -0.001, 1.1}, 95.);
    run_generateHistogramUnsortedTest(e, {1.05, -0.002, 1.1}, 45.);
  }

  void test_generateHistogramUnsortedLinear_TOF_bad_params() {
    // putting incorrect parameters in generateHistogram should not cause segfault
    const auto e = createLinearTestData();

    std::vector<double> rebinParams = {0., 0.1, 100.};
    MantidVec X, expected_Y, expected_E, Y, E;
    VectorHelper::createAxisFromRebinParams(rebinParams, X, true);

    TS_ASSERT(!e.isSortedByTof());

    // setting step size to 0.001 will cause findLinearBin to get bin out of range of X
    TS_ASSERT_THROWS_NOTHING(e.generateHistogram(0.01, X, Y, E));

    TS_ASSERT_DELTA(std::reduce(Y.begin(), Y.end()), 101, 1e-8);
  }

  void test_generateHistogramUnsortedLinear_WEIGHTED_bad_params() {
    // putting incorrect parameters in generateHistogram should not cause segfault
    const auto e = createLinearTestData(WEIGHTED);

    std::vector<double> rebinParams = {0., 0.1, 100.};
    MantidVec X, expected_Y, expected_E, Y, E;
    VectorHelper::createAxisFromRebinParams(rebinParams, X, true);

    TS_ASSERT(!e.isSortedByTof());

    // setting step size to 0.001 will cause findLinearBin to get bin out of range of X
    TS_ASSERT_THROWS_NOTHING(e.generateHistogram(0.01, X, Y, E));

    TS_ASSERT_DELTA(std::reduce(Y.begin(), Y.end()), 101, 1e-8);
  }

  void test_generateHistogramUnsortedLog_bad_params() {
    // putting incorrect parameters in generateHistogram should not cause segfault
    const auto e = createLogTestData();

    std::vector<double> rebinParams = {1., -0.001, 1.1};
    MantidVec X, expected_Y, expected_E, Y, E;
    VectorHelper::createAxisFromRebinParams(rebinParams, X, true);

    TS_ASSERT(!e.isSortedByTof());

    // setting step size to -0.001 will cause findLogBin to get bin out of range of X
    TS_ASSERT_THROWS_NOTHING(e.generateHistogram(-0.0001, X, Y, E));

    TS_ASSERT_DELTA(std::reduce(Y.begin(), Y.end()), 10, 1e-8);
  }

  void run_generateHistogramUnsortedTest(EventList e, std::vector<double> rebinParams,
                                         const double expected_total = 0) {
    MantidVec X, expected_Y, expected_E, Y, E;
    VectorHelper::createAxisFromRebinParams(rebinParams, X, true);

    TS_ASSERT(!e.isSortedByTof());
    // set the values of Y to be one so we can check that the values are zeroed out
    Y.resize(X.size() - 1, 1.);

    // do unsorted histogram then compare and check still unsorted
    e.generateHistogram(rebinParams[1], X, Y, E);
    TS_ASSERT(!e.isSortedByTof());

    // do sorted method to get expected results
    e.generateHistogram(X, expected_Y, expected_E);
    TS_ASSERT(e.isSortedByTof());

    double total_counts = 0;

    for (size_t i = 1; i < Y.size(); i++) {
      TS_ASSERT_EQUALS(expected_Y[i], Y[i]);
      total_counts += Y[i];
      TS_ASSERT_EQUALS(expected_E[i], E[i]);
    }

    TS_ASSERT_DELTA(total_counts, expected_total, 1e-8);
  }

  const EventList createLinearTestData(EventType eventType = TOF) {
    EventList e;

    if (eventType != TOF)
      el.switchTo(eventType);

    // some of these values will go to incorrect bins when simply converted from tof to bin_number when binned with {0.,
    // 0.1, 100.}

    switch (eventType) {
    case TOF:
      for (int x = -10; x < 1010; x++)
        e += TofEvent(x * 0.1);
      break;
    case WEIGHTED:
      for (int x = -10; x < 1010; x++)
        e += WeightedEvent(x * 0.1);
      break;
    case WEIGHTED_NOTIME:
      for (int x = -10; x < 1010; x++)
        e += TofEvent(x * 0.1);
      break;
    }

    return e;
  }

  const EventList createLogTestData(EventType eventType = TOF) {
    EventList e;

    if (eventType != TOF)
      el.switchTo(eventType);

    // some of these values will go to incorrect bins when simply converted from tof to bin_number when binned with {1.,
    // -0.001, 1.1}

    switch (eventType) {
    case TOF:
      for (int x = 0; x < 100; x++)
        e += TofEvent(pow(1.001, x));
      break;
    case WEIGHTED:
      for (int x = 0; x < 100; x++)
        e += WeightedEvent(pow(1.001, x));
      break;
    case WEIGHTED_NOTIME:
      for (int x = 0; x < 100; x++)
        e += TofEvent(pow(1.001, x));
      break;
    }

    return e;
  }
};

//==========================================================================================
/** Performance tests for event lists.
 * Just runs some of the slowest code with lots of events.
 * Tries to isolate sorting from other code by feeding
 * in pre-sorted event lists in some cases.
 * */
class EventListTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventListTestPerformance *createSuite() { return new EventListTestPerformance(); }
  static void destroySuite(EventListTestPerformance *suite) { delete suite; }

  /** Constructor */
  EventListTestPerformance() {
    // Source for a randome event list
    el_random_source.clear();
    for (size_t i = 0; i < 2000000; i++)
      el_random_source += TofEvent((rand() % 200000) * 0.05, rand() % 1000);

    // 10 million events, up to 1e5 tof
    el_sorted_original.clear();
    for (size_t i = 0; i < 10000000; i++)
      el_sorted_original += TofEvent(static_cast<double>(i) / 100.0, rand() % 1000);
    el_sorted_original.setSortOrder(TOF_SORT);

    el_sorted_weighted.clear();
    for (size_t i = 0; i < 10000000; i++)
      el_sorted_weighted += WeightedEvent(static_cast<double>(i) / 100.0, rand() % 1000, 2.34, 4.56);
    el_sorted_weighted.setSortOrder(TOF_SORT);

    // A vector for histogramming, 100,000 steps of 1.0
    for (double i = 0; i < 100000; i += 1.0)
      fineX.emplace_back(i);
    // Coarse vector, 1000 bins.
    for (double i = 0; i < 100000; i += 100)
      coarseX.emplace_back(i);

    // Create FrameworkManager such that the effect of config option
    // `MultiThreaded.MaxCores` is visible: The FrameworkManager sets the TBB
    // thread count according to this value if applicable. TBB threading is used
    // when sorting events.
    Mantid::API::FrameworkManager::Instance();
  }

  EventList el_random, el_random_source, el_sorted, el_sorted_original, el_sorted_weighted, el4, el5;
  MantidVec fineX;
  MantidVec coarseX;

  void setUp() override {
    // Reset the random event list
    el_random.clear();
    el_random += el_random_source;
    // And the sorted one
    el_sorted.clear();
    el_sorted += el_sorted_original;
    el_sorted.setSortOrder(TOF_SORT);
  }

  void tearDown() override {}

  void test_sort_tof() { el_random.sortTof(); }

  void test_compressEvents() {
    EventList out_el;
    el_sorted.compressEvents(10.0, &out_el);
  }

  void test_compressEvents_Parallel() {
    EventList out_el;
    el_sorted.compressEvents(10.0, &out_el);
  }

  void test_multiply() { el_random *= 2.345; }

  void test_convertTof() { el_random.convertTof(2.5, 6.78); }

  void test_getTofs_setTofs() {
    std::vector<double> tofs;
    el_random.getTofs(tofs);
    TS_ASSERT_EQUALS(tofs.size(), el_random.getNumberEvents());
    el_random.setTofs(tofs);
  }

  void test_histogram_fine() {
    MantidVec Y, E;
    el_sorted.generateHistogram(fineX, Y, E);
    el_sorted_weighted.generateHistogram(fineX, Y, E);
  }

  void test_histogram_coarse() {
    MantidVec Y, E;
    el_sorted.generateHistogram(coarseX, Y, E);
    el_sorted_weighted.generateHistogram(coarseX, Y, E);
  }

  void test_maskTof() {
    TS_ASSERT_EQUALS(el_sorted.getNumberEvents(), 10000000);
    el_sorted.maskTof(25e3, 75e3);
    TS_ASSERT_EQUALS(el_sorted.getNumberEvents(), 5000000 - 1);
  }

  void test_integrate() {
    TS_ASSERT_EQUALS(el_sorted.getNumberEvents(), 10000000);
    double integ = el_sorted.integrate(25e3, 75e3, false);
    TS_ASSERT_DELTA(integ, 5e6, 1);
  }
};
