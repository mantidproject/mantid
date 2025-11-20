// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TimeSplitter.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeROI.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::EventType;
using Mantid::API::IEventList;
using Mantid::API::MatrixWorkspace;
using Mantid::API::TableRow;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventSortType;
using Mantid::DataObjects::TimeSplitter;
using Mantid::Kernel::Logger;
using Mantid::Kernel::SplittingInterval;
using Mantid::Kernel::SplittingIntervalVec;
using Mantid::Kernel::TimeROI;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

namespace {
/// static Logger definition
Logger g_log("TimeSplitter");
} // namespace

namespace {
const DateAndTime ONE("2023-01-01T11:00:00");
const DateAndTime TWO("2023-01-01T12:00:00");
const DateAndTime THREE("2023-01-01T13:00:00");
const DateAndTime FOUR("2023-01-01T14:00:00");
const DateAndTime FIVE("2023-01-01T15:00:00");
const DateAndTime SIX("2023-01-01T16:00:00");

} // namespace

class TimeSplitterTest : public CxxTest::TestSuite {
private:
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

  /// Instantiate an EventList for every input destination index
  /// Insert an EventList for NO_TARGET if not provided in destinations
  std::map<int, EventList *> instantiatePartials(const std::vector<int> &destinations) {
    std::map<int, EventList *> partials;

    for (int destination : destinations) {
      if (partials.find(destination) != partials.cend())
        continue;
      partials[destination] = new EventList();
    }
    // insert an EventList for NO_TARGET if not provided in destinations
    if (partials.find(TimeSplitter::NO_TARGET) == partials.cend())
      partials[TimeSplitter::NO_TARGET] = new EventList();
    return partials;
  }

  /**
   * Helper function to generate a TimeSplitter object from a vector of times and
   * destination indexes.
   *
   * The size of vector `times` must be one plus the size of vector `indexes`. Thus,
   * any time `t` such that times[i] <= t < times[i+1] will be associated to
   * destination index `indexes[i]`
   *
   * Destination index '-1` is allowed and means no destination
   *
   * @param times : vector of times
   * @param indexes : vector of destination indexes
   */
  TimeSplitter generateSplitter(std::vector<DateAndTime> &times, std::vector<int> &indexes) {
    assert(times.size() == 1 + indexes.size());
    auto splitter = TimeSplitter();
    for (size_t i = 0; i < indexes.size(); i++)
      splitter.addROI(times[i], times[i + 1], indexes[i]);
    return splitter;
  }

  /**
   * Helper function to generate a TimeSplitter object from a vector of interval times,
   * destination indexes, and a starting DateAndTime.
   *
   * The size of vector `intervals` must be the same as `destinations`.
   *
   * Destination index '-1` is allowed and means no destination
   *
   * @param intervals : time intervals (in seconds) between consecutive DateAndTime boundaries
   * @param destinations : vector of destination indexes
   * @para startTime: first DateAndTime boundary
   */
  TimeSplitter generateSplitter(const DateAndTime &startTime, const std::vector<double> &intervals,
                                const std::vector<int> &destinations) {
    TimeSplitter splitter;
    assert(destinations.size() == intervals.size());
    DateAndTime start(startTime);
    DateAndTime stop(startTime);
    ;
    for (size_t i = 0; i < intervals.size(); i++) {
      stop += intervals[i]; // invokes DateAndTime &operator+=(const double sec);
      splitter.addROI(start, stop, destinations[i]);
      start = stop;
    }
    return splitter;
  }

  /**
   * Helper function to generate the event times associated to each event in the list as a string
   *
   * @param partial : the input event list
   * @param timeType : which time to select for each event (pulse, pulse+TOF, pulse+corrected_TOF)
   * @param factor : dimensionless quantity to rescale the TOF of each event
   * @param shift : TOF offset, in micro-seconds, to be applied after rescaling
   */
  std::vector<std::string> timesToStr(const EventList *partial, const EventSortType &timeType,
                                      const double &factor = 0.0, const double &shift = 0.0) {
    std::vector<DateAndTime> dates;
    switch (timeType) {
    case (EventSortType::PULSETIME_SORT):
      dates = partial->getPulseTimes();
      break;
    case (EventSortType::PULSETIMETOF_SORT):
      dates = partial->getPulseTOFTimes();
      break;
    case (EventSortType::TIMEATSAMPLE_SORT):
      // method getPulseTOFTimesAtSample requires argument `shift` in micro-seconds
      dates = partial->getPulseTOFTimesAtSample(factor, shift);
      break;
    default:
      throw std::runtime_error("timesToStr: Unhandled event sorting type");
    }
    std::vector<std::string> obtained;
    std::transform(dates.cbegin(), dates.cend(), std::back_inserter(obtained),
                   [](const DateAndTime &date) { return date.toSimpleString(); });
    return obtained;
  };

  // A helper method to create a table workspace for testing
  std::shared_ptr<Mantid::DataObjects::TableWorkspace>
  createTableWorkspace(const std::map<std::pair<double, double>, std::string> &splittingIntervals) {
    auto tws = std::make_shared<Mantid::DataObjects::TableWorkspace>(splittingIntervals.size() /*rows*/);

    // a table workspace used for event filtering must have 3 columns
    tws->addColumn("double", "start");
    tws->addColumn("double", "stop");
    tws->addColumn("str", "target"); // to be used as a suffix of the output workspace name

    TableRow row = tws->getFirstRow();
    for (const auto &iter : splittingIntervals) {
      row << iter.first.first << iter.first.second << iter.second;
      row.next();
    }

    return tws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeSplitterTest *createSuite() { return new TimeSplitterTest(); }
  static void destroySuite(TimeSplitterTest *suite) { delete suite; }

  void test_valueAtTime() {
    g_log.notice("\ntest_valueAtTime...");
    // to start everything is either in 0th output or masked
    TimeSplitter splitter(TWO, FOUR);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({0}));

    // add ROI for first half to go to 1st output
    splitter.addROI(TWO, THREE, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({0, 1}));

    // add ROI for second half to go to 2nd output
    splitter.addROI(THREE, FOUR, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({1, 2}));

    // have whole thing go to 3rd output
    splitter.addROI(TWO, FOUR, 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({3}));

    // prepend a section that goes to 1st output
    splitter.addROI(ONE, TWO, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({1, 3}));

    // append a section that goes to 2nd output
    splitter.addROI(FOUR, FIVE, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({1, 2, 3}));

    // set before the beginning to mask
    splitter.addROI(ONE, TWO, TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({2, 3}));

    // set after the end to mask
    splitter.addROI(FOUR, FIVE, TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({3}));
  }

  void test_emptySplitter() {
    g_log.notice("\ntest_emptySplitter...");
    TimeSplitter splitter;
    TS_ASSERT_EQUALS(splitter.valueAtTime(DateAndTime("2023-01-01T11:00:00")), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 0);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::set<int>({}));
  }

  void test_addAdjacentROI() {
    g_log.notice("\ntest_addAdjacentROI...");
    // append to ROI with touching boundary
    TimeSplitter splitter;
    splitter.addROI(ONE, TWO, 1);
    splitter.addROI(TWO, THREE, 2);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 2);

    // prepend to ROI with touching boundary
    TimeSplitter splitter2;
    splitter2.addROI(TWO, THREE, 2);
    splitter2.addROI(ONE, TWO, 1);
    TS_ASSERT_EQUALS(splitter2.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter2.valueAtTime(TWO), 2);
  }

  void test_gap() {
    g_log.notice("\ntest_gap...");
    TimeSplitter splitter;
    // create a splitter with a gap
    splitter.addROI(ONE, TWO, 0);
    splitter.addROI(THREE, FOUR, 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);

    // fill in the gap with a different value
    splitter.addROI(TWO, THREE, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);

    // fill in the gap with the same value as before and after
    splitter.addROI(TWO, THREE, 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
  }

  void test_getTimeROI() {
    g_log.notice("\ntest_getTimeROI...");
    // start with empty TimeSplitter
    TimeSplitter splitter;
    TS_ASSERT(splitter.getTimeROI(TimeSplitter::NO_TARGET).useAll());
    TS_ASSERT(splitter.getTimeROI(0).useAll());

    // place to put the output
    TimeROI roi;

    // add a single TimeROI
    splitter.addROI(ONE, THREE, 1);
    TS_ASSERT(splitter.getTimeROI(TimeSplitter::NO_TARGET).useAll());
    TS_ASSERT(splitter.getTimeROI(0).useAll());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.useAll());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);

    // add the same output index, but with a gap from the previous
    splitter.addROI(FOUR, FIVE, 1);
    roi =
        splitter.getTimeROI(TimeSplitter::NO_TARGET - 1); // intentionally trying a "bigger" negative for ignore filter
    TS_ASSERT(!roi.useAll());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);
    TS_ASSERT(splitter.getTimeROI(0).useAll());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.useAll());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);
  }

  void test_toSplitters() {
    g_log.notice("\ntest_toSplitters...");
    TimeSplitter splitter;
    splitter.addROI(ONE, TWO, 1);
    splitter.addROI(TWO, THREE, 2);
    splitter.addROI(FOUR, FIVE, 3); // a gap with the previous ROI
    const SplittingIntervalVec splitVec = splitter.getSplittingIntervals(true);
    TS_ASSERT_EQUALS(splitVec.size(), 4);
    TS_ASSERT(splitVec[0] == SplittingInterval(ONE, TWO, 1));
    TS_ASSERT(splitVec[1] == SplittingInterval(TWO, THREE, 2));
    TS_ASSERT(splitVec[2] == SplittingInterval(THREE, FOUR, TimeSplitter::NO_TARGET));
    TS_ASSERT(splitVec[3] == SplittingInterval(FOUR, FIVE, 3));
    const SplittingIntervalVec splitVecNoTarget = splitter.getSplittingIntervals(false);
    TS_ASSERT_EQUALS(splitVecNoTarget.size(), 3);
    TS_ASSERT(splitVecNoTarget[0] == SplittingInterval(ONE, TWO, 1));
    TS_ASSERT(splitVecNoTarget[1] == SplittingInterval(TWO, THREE, 2));
    TS_ASSERT(splitVecNoTarget[2] == SplittingInterval(FOUR, FIVE, 3));
  }

  /** Test that a TimeSplitter object constructed from a MatrixWorkspace object
   * containing absolute times is equivalent to a TimeSplitter object built by
   * successively adding time ROIs. Also test that TimeSplitter supports
   * "OutputWorkspaceIndexedFrom1" property of FilterEvents class: optionally shift
   * all target indexes (just for the output naming purposes) so they start from 1.
   * @brief test_timeSplitterFromMatrixWorkspaceAbsoluteTimes
   */
  void test_timeSplitterFromMatrixWorkspaceAbsoluteTimes() {
    g_log.notice("\ntest_timeSplitterFromMatrixWorkspaceAbsoluteTimes...");
    TimeSplitter splitter;
    splitter.addROI(DateAndTime(0, 0), DateAndTime(10, 0), 0);
    splitter.addROI(DateAndTime(10, 0), DateAndTime(15, 0), 3);
    splitter.addROI(DateAndTime(15, 0), DateAndTime(20, 0), 2);

    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 3);

    auto &X = ws->dataX(0);
    auto &Y = ws->dataY(0);
    // X[0] is 0 by default, unit is seconds.
    X[1] = 10.0;
    X[2] = 15.0;
    X[3] = 20.0;

    Y[0] = 0.0;
    Y[1] = 3.0;
    Y[2] = 2.0;
    auto convertedSplitter = new TimeSplitter(ws);

    TS_ASSERT(splitter.numRawValues() == convertedSplitter->numRawValues() && convertedSplitter->numRawValues() == 4);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(0, 0)) == convertedSplitter->valueAtTime(DateAndTime(0, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(0, 0)) == 0);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(12, 0)) == convertedSplitter->valueAtTime(DateAndTime(12, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(12, 0)) == 3);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(20, 0)) == convertedSplitter->valueAtTime(DateAndTime(20, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(20, 0)) == TimeSplitter::NO_TARGET);

    // test shifing all input indexes by 1
    TS_ASSERT(convertedSplitter->getWorkspaceIndexName(0, 1) == "1"); // 0 becomes 1
    TS_ASSERT(convertedSplitter->getWorkspaceIndexName(3, 1) == "4"); // 3 becomes 4
    TS_ASSERT(convertedSplitter->getWorkspaceIndexName(2, 1) == "3"); // 2 becomes 3
  }

  /** Test that a TimeSplitter object constructed from a MatrixWorkspace object
   * containing relative times is equivalent to a TimeSplitter object built by
   * successively adding time ROIs.
   * @brief test_timeSplitterFromMatrixWorkspaceRelativeTimes
   */
  void test_timeSplitterFromMatrixWorkspaceRelativeTimes() {
    g_log.notice("\ntest_timeSplitterFromMatrixWorkspaceRelativeTimes...");
    TimeSplitter splitter;
    int64_t offset_ns{TWO.totalNanoseconds()};
    splitter.addROI(DateAndTime(0) + offset_ns, DateAndTime(10, 0) + offset_ns, 1);

    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto &X = ws->dataX(0);
    auto &Y = ws->dataY(0);
    // X[0] is 0 by default, unit is seconds.
    X[1] = 10.0;
    Y[0] = 1.0;
    auto convertedSplitter = new TimeSplitter(ws, TWO);
    // New starting point of converted splitter is TWO
    TS_ASSERT(splitter.valueAtTime(DateAndTime(0)) == convertedSplitter->valueAtTime(DateAndTime(0)) &&
              convertedSplitter->valueAtTime(DateAndTime(0)) == TimeSplitter::NO_TARGET);
    TS_ASSERT(splitter.valueAtTime(TWO) == convertedSplitter->valueAtTime(TWO) &&
              convertedSplitter->valueAtTime(TWO) == 1);
    TS_ASSERT(splitter.valueAtTime(TWO + offset_ns) == convertedSplitter->valueAtTime(TWO + offset_ns) &&
              convertedSplitter->valueAtTime(TWO + offset_ns) == TimeSplitter::NO_TARGET);
  }

  void test_timeSplitterFromMatrixWorkspaceError() {
    g_log.notice("\ntest_timeSplitterFromMatrixWorkspaceError...");
    // Testing the case where an X value in the MatrixWorkspace is negative.
    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 3);
    auto &X = ws->dataX(0);
    auto &Y = ws->dataY(0);
    X[0] = -5.0;
    X[1] = 10.0;
    X[2] = 15.0;
    X[3] = 20.0;
    Y[0] = 1.0;
    Y[1] = 3.0;
    Y[2] = 2.0;
    TS_ASSERT_THROWS(new TimeSplitter(ws), std::runtime_error &);
  }

  /** Test that a TimeSplitter object constructed from a TableWorkspace object
   * containing absolute times is equivalent to a TimeSplitter object built by
   * successively adding time ROIs.
   * @brief test_timeSplitterFromTableWorkspaceAbsoluteTimes
   */
  void test_timeSplitterFromTableWorkspaceAbsoluteTimes() {
    g_log.notice("\ntest_timeSplitterFromTableWorkspaceAbsoluteTimes...");

    // Create a small table workspace with some targets
    // By design, for a table workspace all times must be in seconds
    const double time1_s{1.0e-5};
    const double time2_s{1.5e-5};
    const double time3_s{2.0e-5};
    const double time4_s{3.0e-5};
    const double time5_s{4.0e-5};
    const double time6_s{5.0e-5};

    std::map<std::pair<double, double>, std::string> splittingIntervals{
        {{time1_s, time2_s}, "1"},
        {{time3_s, time4_s}, "3"},
        {{time5_s, time6_s}, std::to_string(TimeSplitter::NO_TARGET)}};
    auto tws = createTableWorkspace(splittingIntervals);

    // Create a time splitter from the table workspace
    TimeSplitter workspaceDerivedSplitter(tws, DateAndTime(0, 0));

    // build a reference time splitter

    // create all time objects
    DateAndTime time1_abs(time1_s, 0.0);
    DateAndTime time2_abs(time2_s, 0.0);
    DateAndTime time3_abs(time3_s, 0.0);
    DateAndTime time4_abs(time4_s, 0.0);
    DateAndTime time5_abs(time5_s, 0.0);
    DateAndTime time6_abs(time6_s, 0.0);

    // build the reference splitter by adding ROIs
    TimeSplitter referenceSplitter;
    referenceSplitter.addROI(time1_abs, time2_abs, 1);
    referenceSplitter.addROI(time3_abs, time4_abs, 3);
    referenceSplitter.addROI(time5_abs, time6_abs, TimeSplitter::NO_TARGET);

    TS_ASSERT_EQUALS(referenceSplitter.numRawValues(), workspaceDerivedSplitter.numRawValues());
    TS_ASSERT(referenceSplitter.valueAtTime(time1_abs) == workspaceDerivedSplitter.valueAtTime(time1_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time2_abs) == workspaceDerivedSplitter.valueAtTime(time2_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time3_abs) == workspaceDerivedSplitter.valueAtTime(time3_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time4_abs) == workspaceDerivedSplitter.valueAtTime(time4_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time5_abs) == workspaceDerivedSplitter.valueAtTime(time5_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time6_abs) == workspaceDerivedSplitter.valueAtTime(time6_abs));
  }

  /** Test that a TimeSplitter object constructed from a TableWorkspace object
   * containing relative times is equivalent to a TimeSplitter object built by
   * successively adding time ROIs.
   * @brief test_timeSplitterFromTableWorkspaceRelativeTimes
   */
  void test_timeSplitterFromTableWorkspaceRelativeTimes() {
    g_log.notice("\ntest_timeSplitterFromTableWorkspaceRelativeTimes...");

    // Create a small table workspace with some targets
    // By design, for a table workspace all times must be in seconds
    const double time1_s{1.0e-5};
    const double time2_s{1.5e-5};
    const double time3_s{2.0e-5};
    const double time4_s{3.0e-5};
    const double time5_s{4.0e-5};
    const double time6_s{5.0e-5};

    std::map<std::pair<double, double>, std::string> splittingIntervals{
        {{time1_s, time2_s}, "1"},
        {{time3_s, time4_s}, "3"},
        {{time5_s, time6_s}, std::to_string(TimeSplitter::NO_TARGET)}};
    auto tws = createTableWorkspace(splittingIntervals);

    // create a TimeSplitter object from the table. By design, the table owner must know whether
    // the table holds absolute or relative times. In the latter case the user must specify
    // a time offset to be used with the table.
    DateAndTime offset(THREE);
    TimeSplitter workspaceDerivedSplitter(tws, offset);

    // build a reference time splitter

    // create all time objects and offset them
    DateAndTime time1_abs(time1_s, 0.0);
    DateAndTime time2_abs(time2_s, 0.0);
    DateAndTime time3_abs(time3_s, 0.0);
    DateAndTime time4_abs(time4_s, 0.0);
    DateAndTime time5_abs(time5_s, 0.0);
    DateAndTime time6_abs(time6_s, 0.0);

    int64_t offset_ns = offset.totalNanoseconds();
    time1_abs += offset_ns;
    time2_abs += offset_ns;
    time3_abs += offset_ns;
    time4_abs += offset_ns;
    time5_abs += offset_ns;
    time6_abs += offset_ns;

    // build the reference splitter by adding ROIs
    TimeSplitter referenceSplitter;
    referenceSplitter.addROI(time1_abs, time2_abs, 1);
    referenceSplitter.addROI(time3_abs, time4_abs, 3);
    referenceSplitter.addROI(time5_abs, time6_abs, TimeSplitter::NO_TARGET);

    TS_ASSERT_EQUALS(referenceSplitter.numRawValues(), workspaceDerivedSplitter.numRawValues());
    TS_ASSERT(referenceSplitter.valueAtTime(time1_abs) == workspaceDerivedSplitter.valueAtTime(time1_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time2_abs) == workspaceDerivedSplitter.valueAtTime(time2_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time3_abs) == workspaceDerivedSplitter.valueAtTime(time3_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time4_abs) == workspaceDerivedSplitter.valueAtTime(time4_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time5_abs) == workspaceDerivedSplitter.valueAtTime(time5_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time6_abs) == workspaceDerivedSplitter.valueAtTime(time6_abs));
  }

  /** Test that a TimeSplitter object constructed from a TableWorkspace object
   * containing non-numeric targets is equivalent to a TimeSplitter object built by
   * successively adding time ROIs. Also check that the TimeSplitter internal mapping
   * of the target names is correct.
   * @brief test_timeSplitterFromTableWorkspaceRelativeTimes
   */
  void test_timeSplitterFromTableWorkspaceWithNonNumericTargets() {
    g_log.notice("\ntest_timeSplitterFromTableWorkspaceWithNonNumericTargets...");

    // Create a small table workspace with some targets
    // By design, for a table workspace all times must be in seconds
    const double time1_s{1.0e-5};
    const double time2_s{1.5e-5};
    const double time3_s{2.0e-5};
    const double time4_s{3.0e-5};
    const double time5_s{4.0e-5};
    const double time6_s{5.0e-5};

    std::map<std::pair<double, double>, std::string> splittingIntervals{
        {{time1_s, time2_s}, "A"},
        {{time3_s, time4_s}, std::to_string(TimeSplitter::NO_TARGET)},
        {{time5_s, time6_s}, "B"}};
    auto tws = createTableWorkspace(splittingIntervals);

    // create a TimeSplitter object from the table. By design, the table user must know whether
    // the table holds absolute or relative times. In the latter case the user must specify
    // a time offset to be used with the table.
    DateAndTime offset(THREE);
    TimeSplitter workspaceDerivedSplitter(tws, offset);

    // build the reference time splitter

    // first create all time objects and offset them
    DateAndTime time1_abs(time1_s, 0.0);
    DateAndTime time2_abs(time2_s, 0.0);
    DateAndTime time3_abs(time3_s, 0.0);
    DateAndTime time4_abs(time4_s, 0.0);
    DateAndTime time5_abs(time5_s, 0.0);
    DateAndTime time6_abs(time6_s, 0.0);

    int64_t offset_ns = offset.totalNanoseconds();
    time1_abs += offset_ns;
    time2_abs += offset_ns;
    time3_abs += offset_ns;
    time4_abs += offset_ns;
    time5_abs += offset_ns;
    time6_abs += offset_ns;

    // now build the reference splitter by adding ROIs
    TimeSplitter referenceSplitter;
    referenceSplitter.addROI(time1_abs, time2_abs, 0);
    referenceSplitter.addROI(time3_abs, time4_abs, TimeSplitter::NO_TARGET);
    referenceSplitter.addROI(time5_abs, time6_abs, 1);

    // check that the two time splitters are the same internally
    TS_ASSERT_EQUALS(referenceSplitter.numRawValues(), workspaceDerivedSplitter.numRawValues());
    TS_ASSERT(referenceSplitter.valueAtTime(time1_abs) == workspaceDerivedSplitter.valueAtTime(time1_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time2_abs) == workspaceDerivedSplitter.valueAtTime(time2_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time3_abs) == workspaceDerivedSplitter.valueAtTime(time3_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time4_abs) == workspaceDerivedSplitter.valueAtTime(time4_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time5_abs) == workspaceDerivedSplitter.valueAtTime(time5_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time6_abs) == workspaceDerivedSplitter.valueAtTime(time6_abs));

    // check that non-numeric target names are internally mapped to consecutive indexes starting from 0
    TS_ASSERT(workspaceDerivedSplitter.getWorkspaceIndexName(0) == "A");
    TS_ASSERT(workspaceDerivedSplitter.getWorkspaceIndexName(1) == "B");
    // check that "no target" index name is mapped to the correct value
    TS_ASSERT(workspaceDerivedSplitter.getWorkspaceIndexName(TimeSplitter::NO_TARGET) ==
              std::to_string(TimeSplitter::NO_TARGET));
  }

  /** Test that a TimeSplitter object constructed from a SplittersWorkspace object
   * is equivalent to a TimeSplitter object built by successively adding time ROIs.
   * @brief test_timeSplitterFromSplittersWorkspace
   */
  void test_timeSplitterFromSplittersWorkspace() {
    g_log.notice("\ntest_timeSplitterFromSplittersWorkspace...");

    // create time objects for testing. All input times are in nanoseconds.
    DateAndTime time1(10000);
    DateAndTime time2(15000);
    DateAndTime time3(20000);
    DateAndTime time4(30000);
    DateAndTime time5(40000);
    DateAndTime time6(50000);

    SplittingInterval s1(time1, time2, 1);
    SplittingInterval s2(time3, time4, 3);
    SplittingInterval s3(time5, time6, TimeSplitter::NO_TARGET);

    auto sws = std::make_shared<Mantid::DataObjects::SplittersWorkspace>();
    sws->addSplitter(s1);
    sws->addSplitter(s2);
    sws->addSplitter(s3);

    // create a TimeSplitter object from the workspace
    TimeSplitter workspaceDerivedSplitter(sws);

    // build a reference TimeSplitter by adding ROIs
    TimeSplitter referenceSplitter;
    referenceSplitter.addROI(time1, time2, 1);
    referenceSplitter.addROI(time3, time4, 3);
    referenceSplitter.addROI(time5, time6, TimeSplitter::NO_TARGET);

    TS_ASSERT_EQUALS(referenceSplitter.numRawValues(), workspaceDerivedSplitter.numRawValues());
    TS_ASSERT(referenceSplitter.valueAtTime(time1) == workspaceDerivedSplitter.valueAtTime(time1));
    TS_ASSERT(referenceSplitter.valueAtTime(time2) == workspaceDerivedSplitter.valueAtTime(time2));
    TS_ASSERT(referenceSplitter.valueAtTime(time3) == workspaceDerivedSplitter.valueAtTime(time3));
    TS_ASSERT(referenceSplitter.valueAtTime(time4) == workspaceDerivedSplitter.valueAtTime(time4));
    TS_ASSERT(referenceSplitter.valueAtTime(time5) == workspaceDerivedSplitter.valueAtTime(time5));
    TS_ASSERT(referenceSplitter.valueAtTime(time6) == workspaceDerivedSplitter.valueAtTime(time6));
  }
  // Verify keys in TimeSplitter::m_roi_map are sorted
  void test_keysSorted() {
    g_log.notice("\ntest_keysSorted...");
    TimeSplitter splitter;
    splitter.addROI(FIVE, SIX, 0);
    splitter.addROI(THREE, FOUR, 0);
    splitter.addROI(ONE, TWO, 0);
    auto iter = splitter.getSplittersMap().begin();
    TS_ASSERT_EQUALS(iter->first, ONE);
    TS_ASSERT_EQUALS(iter->second, 0);
    iter++;
    TS_ASSERT_EQUALS(iter->first, TWO);
    TS_ASSERT_EQUALS(iter->second, TimeSplitter::NO_TARGET);
    iter++;
    TS_ASSERT_EQUALS(iter->first, THREE);
    TS_ASSERT_EQUALS(iter->second, 0);
    iter++;
    TS_ASSERT_EQUALS(iter->first, FOUR);
    TS_ASSERT_EQUALS(iter->second, TimeSplitter::NO_TARGET);
    iter++;
    TS_ASSERT_EQUALS(iter->first, FIVE);
    TS_ASSERT_EQUALS(iter->second, 0);
    iter++;
    TS_ASSERT_EQUALS(iter->first, SIX);
    TS_ASSERT_EQUALS(iter->second, TimeSplitter::NO_TARGET);
    iter++;
  }

  void test_splitEventList() {
    g_log.notice("\ntest_splitEventList...");
    const DateAndTime startTime{TWO};
    // Generate the events. Six events, the first at "2023-Jan-01 12:00:00" and then every 30 seconds. The last
    // event happening at "2023-Jan-01 12:02:30".
    double pulsePeriod{60.}; // time between consecutive pulses, in seconds
    size_t nPulses{3};
    size_t eventsPerPulse{2};
    EventType eventType = EventType::TOF;
    EventList events = this->generateEvents(startTime, pulsePeriod, nPulses, eventsPerPulse, eventType);

    // --------------------
    // Split events according to pulse time
    // --------------------
    // Generate a splitter with three intervals:
    // interval ["2023-Jan-01 12:00:00", "2023-Jan-01 12:02:00") with destination 0
    // interval ["2023-Jan-01 12:02:00", "2023-Jan-01 12:03:00") with destination 1
    // interval ["2023-Jan-01 12:03:00", "2023-Jan-01 12:04:00") with destination NO_TARGET
    std::vector<double> intervals{120.0, 60.0, 60.0};
    const std::vector<int> destinations{0, 1, TimeSplitter::NO_TARGET};
    TimeSplitter splitter = this->generateSplitter(startTime, intervals, destinations);
    // Generate the output partial event lists
    std::map<int, EventList *> partials = this->instantiatePartials(destinations);

    splitter.splitEventList(events, partials);
    TS_ASSERT_EQUALS(partials[0]->getNumberEvents(), 4);
    TS_ASSERT_EQUALS(partials[1]->getNumberEvents(), 2);
    TS_ASSERT_EQUALS(partials[TimeSplitter::NO_TARGET]->getNumberEvents(), 0);
    // Check the pulse times of the events landing in the partials
    std::vector<std::string> expected{"2023-Jan-01 12:00:00", "2023-Jan-01 12:00:00", "2023-Jan-01 12:01:00",
                                      "2023-Jan-01 12:01:00"};
    TS_ASSERT_EQUALS(timesToStr(partials[0], EventSortType::PULSETIME_SORT), expected);
    expected = {"2023-Jan-01 12:02:00", "2023-Jan-01 12:02:00"};
    TS_ASSERT_EQUALS(timesToStr(partials[1], EventSortType::PULSETIME_SORT), expected);
    expected = {};
    TS_ASSERT_EQUALS(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::PULSETIME_SORT), expected);

    // --------------------
    // Split events according to pulse time + TOF
    // --------------------
    bool pulseTof{true};
    intervals = {90.0, 90.0, 60.0};
    partials = this->instantiatePartials(destinations); // splitting doesn't initialize partials
    splitter = this->generateSplitter(startTime, intervals, destinations);
    splitter.splitEventList(events, partials, pulseTof);
    TS_ASSERT_EQUALS(partials[0]->getNumberEvents(), 3);
    TS_ASSERT_EQUALS(partials[1]->getNumberEvents(), 3);
    TS_ASSERT_EQUALS(partials[TimeSplitter::NO_TARGET]->getNumberEvents(), 0);
    expected = {"2023-Jan-01 12:00:00", "2023-Jan-01 12:00:30", "2023-Jan-01 12:01:00"};
    TS_ASSERT_EQUALS(timesToStr(partials[0], EventSortType::PULSETIMETOF_SORT), expected);
    expected = {"2023-Jan-01 12:01:30", "2023-Jan-01 12:02:00", "2023-Jan-01 12:02:30"};
    TS_ASSERT_EQUALS(timesToStr(partials[1], EventSortType::PULSETIMETOF_SORT), expected);
    expected = {};
    TS_ASSERT_EQUALS(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::PULSETIMETOF_SORT), expected);

    // --------------------
    // Split events according to pulse time + shifted TOF
    // --------------------
    bool tofCorrect{true};
    double factor{1.0};
    double shift{30.0 * 1.0E6};                         // add 30 seconds to each TOF, in units of micro-seconds
    partials = this->instantiatePartials(destinations); // splitting doesn't initialize partials
    splitter.splitEventList(events, partials, pulseTof, tofCorrect, factor, shift);
    TS_ASSERT_EQUALS(partials[0]->getNumberEvents(), 2);
    TS_ASSERT_EQUALS(partials[1]->getNumberEvents(), 3);
    TS_ASSERT_EQUALS(partials[TimeSplitter::NO_TARGET]->getNumberEvents(), 1);
    expected = {"2023-Jan-01 12:00:30", "2023-Jan-01 12:01:00"};
    TS_ASSERT_EQUALS(timesToStr(partials[0], EventSortType::TIMEATSAMPLE_SORT, factor, shift), expected);
    expected = {"2023-Jan-01 12:01:30", "2023-Jan-01 12:02:00", "2023-Jan-01 12:02:30"};
    TS_ASSERT_EQUALS(timesToStr(partials[1], EventSortType::TIMEATSAMPLE_SORT, factor, shift), expected);
    expected = {"2023-Jan-01 12:03:00"};
    TS_ASSERT_EQUALS(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::TIMEATSAMPLE_SORT, factor, shift),
                     expected);

    // --------------------
    // Split events according to pulse time + contracted TOF
    // --------------------
    factor = 0.5; // shrink TOF by half
    shift = 0.0;
    partials = this->instantiatePartials(destinations); // splitting doesn't initialize partials
    splitter.splitEventList(events, partials, pulseTof, tofCorrect, factor, shift);
    TS_ASSERT_EQUALS(partials[0]->getNumberEvents(), 4);
    TS_ASSERT_EQUALS(partials[1]->getNumberEvents(), 2);
    TS_ASSERT_EQUALS(partials[TimeSplitter::NO_TARGET]->getNumberEvents(), 0);
    expected = {"2023-Jan-01 12:00:00", "2023-Jan-01 12:00:15", "2023-Jan-01 12:01:00", "2023-Jan-01 12:01:15"};
    TS_ASSERT_EQUALS(timesToStr(partials[0], EventSortType::TIMEATSAMPLE_SORT, factor, shift), expected);
    expected = {"2023-Jan-01 12:02:00", "2023-Jan-01 12:02:15"};
    TS_ASSERT_EQUALS(timesToStr(partials[1], EventSortType::TIMEATSAMPLE_SORT, factor, shift), expected);
    expected = {};
    TS_ASSERT_EQUALS(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::TIMEATSAMPLE_SORT, factor, shift),
                     expected);
  }

  // This test aims to test a TimeSplitter containing a splitter that will end up holding no events
  void test_splitEventListLeapingTimes() {
    g_log.notice("\ntest_splitEventListLeapingTimes...");
    // Generate the events. Six events, the first at "2023-Jan-01 12:00:00" and then every 30 seconds. The last
    // event happening at "2023-Jan-01 12:02:30".
    DateAndTime startTime{TWO};
    double pulsePeriod{60.}; // time between consecutive pulses, in seconds
    size_t nPulses{3};
    size_t eventsPerPulse{2};
    EventType eventType = EventType::TOF;
    EventList events = this->generateEvents(startTime, pulsePeriod, nPulses, eventsPerPulse, eventType);
    // Generate a splitter with six intervals:
    // interval ["2023-Jan-01 12:00:00", "2023-Jan-01 12:00:30") with destination 0
    // interval ["2023-Jan-01 12:00:30", "2023-Jan-01 12:00:45") with destination 1
    // interval ["2023-Jan-01 12:00:45", "2023-Jan-01 12:01:00") with destination 2
    // interval ["2023-Jan-01 12:01:00", "2023-Jan-01 12:02:00") with destination 3
    // interval ["2023-Jan-01 12:02:00", "2023-Jan-01 12:02:10") with destination 1
    // interval ["2023-Jan-01 12:02:10", "2023-Jan-01 12:02:20") with destination 2
    std::vector<double> intervals{30, 15, 15, 60, 10, 10};
    const std::vector<int> destinations{0, 1, 2, 3, 1, 2};
    TimeSplitter splitter = this->generateSplitter(startTime, intervals, destinations);
    // Generate the output partial event lists
    std::map<int, EventList *> partials = this->instantiatePartials(destinations);
    /// Split events according to pulse time + TOF
    bool pulseTof{true};
    splitter.splitEventList(events, partials, pulseTof);
    /// Check which event landed on which partial event list
    std::vector<std::string> expected;
    expected = {"2023-Jan-01 12:00:00"};
    TS_ASSERT(timesToStr(partials[0], EventSortType::PULSETIMETOF_SORT) == expected);
    expected = {"2023-Jan-01 12:00:30", "2023-Jan-01 12:02:00"};
    TS_ASSERT(timesToStr(partials[1], EventSortType::PULSETIMETOF_SORT) == expected);
    expected = {}; // no events for this workspace
    TS_ASSERT(timesToStr(partials[2], EventSortType::PULSETIMETOF_SORT) == expected);
    expected = {"2023-Jan-01 12:01:00", "2023-Jan-01 12:01:30"};
    TS_ASSERT(timesToStr(partials[3], EventSortType::PULSETIMETOF_SORT) == expected);
    expected = {"2023-Jan-01 12:02:30"};
    TS_ASSERT(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::PULSETIMETOF_SORT) == expected);
  }

  void test_copyAndAssignment() {
    // Create a small table workspace with some targets
    // By design, for a table workspace all times must be in seconds
    const double time1_s{1.0e-5};
    const double time2_s{1.5e-5};
    const double time3_s{2.0e-5};
    const double time4_s{3.0e-5};
    const double time5_s{4.0e-5};
    const double time6_s{5.0e-5};

    std::map<std::pair<double, double>, std::string> splittingIntervals{
        {{time1_s, time2_s}, "A"},
        {{time3_s, time4_s}, std::to_string(TimeSplitter::NO_TARGET)},
        {{time5_s, time6_s}, "B"}};
    auto tws = createTableWorkspace(splittingIntervals);

    // Create a time splitter from the table workspace
    TimeSplitter splitter1(tws, DateAndTime(0, 0));

    // Copy splitter1 to splitter2
    TimeSplitter splitter2(splitter1);

    // Compare splitter maps
    TS_ASSERT_EQUALS(splitter2.getSplittersMap(), splitter1.getSplittersMap())
    // Compare "name:target" and "target:name" maps
    TS_ASSERT_EQUALS(splitter2.getNameTargetMap(), splitter1.getNameTargetMap())
    TS_ASSERT_EQUALS(splitter2.getTargetNameMap(), splitter1.getTargetNameMap())

    // Assign splitter1 to splitter3
    TimeSplitter splitter3;
    splitter3 = splitter1;

    // Compare splitter maps
    TS_ASSERT_EQUALS(splitter3.getSplittersMap(), splitter1.getSplittersMap())
    // Compare "name:target" and "target:name" maps
    TS_ASSERT_EQUALS(splitter3.getNameTargetMap(), splitter1.getNameTargetMap())
    TS_ASSERT_EQUALS(splitter3.getTargetNameMap(), splitter1.getTargetNameMap())
  }

  void test_calculate_target_indices() {
    TimeSplitter splitter;
    splitter.addROI(ONE, TWO, 1);
    splitter.addROI(TWO, THREE, 2);
    splitter.addROI(FOUR, FIVE, 3); // a gap with the previous ROI

    std::vector<DateAndTime> times{ONE - 100.0,  ONE + 100.0,  TWO + 100.0, THREE + 100.0,
                                   FOUR - 100.0, FOUR + 100.0, FIVE + 100.0};
    const auto target_to_pulse_indices = splitter.calculate_target_indices(times);
    TS_ASSERT_EQUALS(target_to_pulse_indices.size(), 3);

    {
      auto [target, pulse_indices] = target_to_pulse_indices.at(0);
      TS_ASSERT_EQUALS(target, 1);
      auto [start, stop] = pulse_indices;
      TS_ASSERT_EQUALS(start, 1);
      TS_ASSERT_EQUALS(stop, 2);
    }
    {
      auto [target, pulse_indices] = target_to_pulse_indices.at(1);
      TS_ASSERT_EQUALS(target, 2);
      auto [start, stop] = pulse_indices;
      TS_ASSERT_EQUALS(start, 2);
      TS_ASSERT_EQUALS(stop, 3);
    }
    {
      auto [target, pulse_indices] = target_to_pulse_indices.at(2);
      TS_ASSERT_EQUALS(target, 3);
      auto [start, stop] = pulse_indices;
      TS_ASSERT_EQUALS(start, 5);
      TS_ASSERT_EQUALS(stop, 6);
    }
  }

  void test_combinedTimeROI() {
    TimeSplitter splitter;
    splitter.addROI(ONE, TWO, 0);
    splitter.addROI(THREE, FOUR, 1);
    splitter.addROI(FOUR, FIVE, 2);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 5);

    const auto roi = splitter.combinedTimeROI();
    TS_ASSERT_EQUALS(roi.numberOfRegions(), 2);

    // the second and third ROIs should be merged
    auto times = roi.getAllTimes();
    TS_ASSERT_EQUALS(times.size(), 4);
    TS_ASSERT_EQUALS(times[0], ONE);
    TS_ASSERT_EQUALS(times[1], TWO);
    TS_ASSERT_EQUALS(times[2], THREE);
    TS_ASSERT_EQUALS(times[3], FIVE);
  }
};
