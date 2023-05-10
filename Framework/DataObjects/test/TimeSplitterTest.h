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
#include "MantidKernel/TimeROI.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::EventType;
using Mantid::API::IEventList;
using Mantid::API::MatrixWorkspace;
using Mantid::API::TableRow;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventSortType;
using Mantid::DataObjects::TimeSplitter;
using Mantid::Kernel::SplittingInterval;
using Mantid::Kernel::TimeROI;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

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
  std::map<int, EventList *> instantiatePartials(const std::vector<int> &destinations) {
    std::map<int, EventList *> partials;
    for (int destination : destinations) {
      if (partials.find(destination) != partials.cend())
        throw std::runtime_error("cannot have duplicate destinations");
      partials[destination] = new EventList();
    }
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
   * Destination index '-1` is allowed.
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
   * Destination index '-1` is allowed.
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

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeSplitterTest *createSuite() { return new TimeSplitterTest(); }
  static void destroySuite(TimeSplitterTest *suite) { delete suite; }

  void test_valueAtTime() {
    // to start everything is either in 0th output or masked
    TimeSplitter splitter(TWO, FOUR);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({0}));

    // add ROI for first half to go to 1st output
    splitter.addROI(TWO, THREE, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({0, 1}));

    // add ROI for second half to go to 2nd output
    splitter.addROI(THREE, FOUR, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 2}));

    // have whole thing go to 3rd output
    splitter.addROI(TWO, FOUR, 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({3}));

    // prepend a section that goes to 1st output
    splitter.addROI(ONE, TWO, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 3}));

    // append a section that goes to 2nd output
    splitter.addROI(FOUR, FIVE, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 2, 3}));

    // set before the beginning to mask
    splitter.addROI(ONE, TWO, TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({2, 3}));

    // set after the end to mask
    splitter.addROI(FOUR, FIVE, TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({3}));
  }

  void test_emptySplitter() {
    TimeSplitter splitter;
    TS_ASSERT_EQUALS(splitter.valueAtTime(DateAndTime("2023-01-01T11:00:00")), TimeSplitter::NO_TARGET);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 0);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({}));
  }

  void test_gap() {
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
    TS_ASSERT(splitter.getTimeROI(0).useAll());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.useAll());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);

    // add the same output index, but with a gap from the previous
    splitter.addROI(FOUR, FIVE, 1);
    roi = splitter.getTimeROI(TimeSplitter::NO_TARGET - 1); // intentionally trying a "big" negative for ignore filter
    TS_ASSERT(!roi.useAll());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);
    TS_ASSERT(splitter.getTimeROI(0).useAll());
    TS_ASSERT(splitter.getTimeROI(0).useAll());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.useAll());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);
  }

  void test_timeSplitterFromMatrixWorkspaceAbsoluteTimes() {
    TimeSplitter splitter;
    splitter.addROI(DateAndTime(0, 0), DateAndTime(10, 0), 1);
    splitter.addROI(DateAndTime(10, 0), DateAndTime(15, 0), 3);
    splitter.addROI(DateAndTime(15, 0), DateAndTime(20, 0), 2);

    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 3);

    auto &X = ws->dataX(0);
    auto &Y = ws->dataY(0);
    // X[0] is 0 by default, unit is seconds.
    X[1] = 10.0;
    X[2] = 15.0;
    X[3] = 20.0;

    Y[0] = 1.0;
    Y[1] = 3.0;
    Y[2] = 2.0;
    auto convertedSplitter = new TimeSplitter(ws);

    TS_ASSERT(splitter.numRawValues() == convertedSplitter->numRawValues() && convertedSplitter->numRawValues() == 4);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(0, 0)) == convertedSplitter->valueAtTime(DateAndTime(0, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(0, 0)) == 1);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(12, 0)) == convertedSplitter->valueAtTime(DateAndTime(12, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(12, 0)) == 3);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(20, 0)) == convertedSplitter->valueAtTime(DateAndTime(20, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(20, 0)) == TimeSplitter::NO_TARGET);
  }

  void test_timeSplitterFromMatrixWorkspaceRelativeTimes() {
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
    auto tws = std::make_shared<Mantid::DataObjects::TableWorkspace>(3 /*rows*/);

    // by design, a table workspace used for event filtering must have 3 columns
    tws->addColumn("double", "start");
    tws->addColumn("double", "stop");
    tws->addColumn("str", "target"); // index of a workspace where the filtered time events will be output

    // by design, all times in the table must be in seconds
    const double time1_s{1.0e-5};
    const double time2_s{1.5e-5};
    const double time3_s{2.0e-5};
    const double time4_s{3.0e-5};
    const double time5_s{4.0e-5};
    const double time6_s{5.0e-5};

    TableRow row = tws->getFirstRow();
    row << time1_s << time2_s << "1";
    row.next();
    row << time3_s << time4_s << "3";
    row.next();
    row << time5_s << time6_s << std::to_string(TimeSplitter::NO_TARGET);

    // create a TimeSplitter object from the table
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
    auto tws = std::make_shared<Mantid::DataObjects::TableWorkspace>(3 /*rows*/);

    // by design, a table workspace used for event filtering must have 3 columns
    tws->addColumn("double", "start");
    tws->addColumn("double", "stop");
    tws->addColumn("str", "target"); // index of a workspace where the filtered time events will be output

    // by design, all times in the table must be in seconds
    const double time1_s{1.0e-5};
    const double time2_s{1.5e-5};
    const double time3_s{2.0e-5};
    const double time4_s{3.0e-5};
    const double time5_s{4.0e-5};
    const double time6_s{5.0e-5};

    TableRow row = tws->getFirstRow();
    row << time1_s << time2_s << "1";
    row.next();
    row << time3_s << time4_s << "3";
    row.next();
    row << time5_s << time6_s << std::to_string(TimeSplitter::NO_TARGET);

    // create a TimeSplitter object from the table. By design, the table user must know whether
    // the table holds absolute or relative times. In the latter case the user must have
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

  /** Test that a TimeSplitter object constructed from a SplittersWorkspace object
   * is equivalent to a TimeSplitter object built by successively adding time ROIs.
   * @brief test_timeSplitterFromSplittersWorkspace
   */
  void test_timeSplitterFromSplittersWorkspace() {
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
    DateAndTime startTime{TWO};
    // return the times associated to each event in the list as a string
    // factor is a dimensionless quantity, and shift is a time in micro-seconds
    auto timesToStr = [](const EventList *partial, const EventSortType &timeType, const double &factor = 0.0,
                         const double &shift = 0.0) {
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
    // Generate the events. Six events, the first at "2023-Jan-01 12:00:00" and then every 30 seconds. The last
    // event happening at "2023-Jan-01 12:02:30".
    double pulsePeriod{60.}; // time between consecutive pulses, in seconds
    size_t nPulses{3};
    size_t eventsPerPulse{2};
    EventType eventType = EventType::TOF;
    EventList events = this->generateEvents(startTime, pulsePeriod, nPulses, eventsPerPulse, eventType);
    // Generate a splitter with three intervals:
    // interval ["2023-Jan-01 12:00:00", "2023-Jan-01 12:02:00") with destination 0
    // interval ["2023-Jan-01 12:02:00", "2023-Jan-01 12:03:00") with destination 1
    // interval ["2023-Jan-01 12:03:00", "2023-Jan-01 12:04:00") with destination NO_TARGET
    std::vector<double> intervals{120.0, 60.0, 60.0};
    const std::vector<int> destinations{0, 1, TimeSplitter::NO_TARGET};
    TimeSplitter splitter = this->generateSplitter(startTime, intervals, destinations);
    // Generate the output partial event lists
    std::map<int, EventList *> partials = this->instantiatePartials(destinations);
    //
    /// Split events according to pulse time
    splitter.splitEventList(events, partials);
    // Check the pulse times of the events landing in the partials
    std::vector<std::string> expected{"2023-Jan-01 12:00:00", "2023-Jan-01 12:00:00", "2023-Jan-01 12:01:00",
                                      "2023-Jan-01 12:01:00"};
    TS_ASSERT(timesToStr(partials[0], EventSortType::PULSETIME_SORT) == expected);
    expected = {"2023-Jan-01 12:02:00", "2023-Jan-01 12:02:00"};
    TS_ASSERT(timesToStr(partials[1], EventSortType::PULSETIME_SORT) == expected);
    expected = {};
    TS_ASSERT(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::PULSETIME_SORT) == expected);
    //
    /// Split events according to pulse time + TOF
    bool pulseTof{true};
    intervals = {90.0, 90.0, 60.0};
    splitter = this->generateSplitter(startTime, intervals, destinations);
    splitter.splitEventList(events, partials, pulseTof);
    expected = {"2023-Jan-01 12:00:00", "2023-Jan-01 12:00:30", "2023-Jan-01 12:01:00"};
    TS_ASSERT(timesToStr(partials[0], EventSortType::PULSETIMETOF_SORT) == expected);
    expected = {"2023-Jan-01 12:01:30", "2023-Jan-01 12:02:00", "2023-Jan-01 12:02:30"};
    TS_ASSERT(timesToStr(partials[1], EventSortType::PULSETIMETOF_SORT) == expected);
    expected = {};
    TS_ASSERT(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::PULSETIMETOF_SORT) == expected);
    //
    /// Split events according to pulse time + shifted TOF
    bool tofCorrect{true};
    double factor{1.0};
    double shift{30.0 * 1.0E6}; // add 30 seconds to each TOF, in units of micro-seconds
    splitter.splitEventList(events, partials, pulseTof, tofCorrect, factor, shift);
    expected = {"2023-Jan-01 12:00:30", "2023-Jan-01 12:01:00"};
    TS_ASSERT(timesToStr(partials[0], EventSortType::TIMEATSAMPLE_SORT, factor, shift) == expected);
    expected = {"2023-Jan-01 12:01:30", "2023-Jan-01 12:02:00", "2023-Jan-01 12:02:30"};
    TS_ASSERT(timesToStr(partials[1], EventSortType::TIMEATSAMPLE_SORT, factor, shift) == expected);
    expected = {"2023-Jan-01 12:03:00"};
    TS_ASSERT(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::TIMEATSAMPLE_SORT, factor, shift) ==
              expected);
    //
    /// Split events according to pulse time + contracted TOF
    factor = 0.5; // shrink TOF by half
    shift = 0.0;
    splitter.splitEventList(events, partials, pulseTof, tofCorrect, factor, shift);
    expected = {"2023-Jan-01 12:00:00", "2023-Jan-01 12:00:15", "2023-Jan-01 12:01:00", "2023-Jan-01 12:01:15"};
    TS_ASSERT(timesToStr(partials[0], EventSortType::TIMEATSAMPLE_SORT, factor, shift) == expected);
    expected = {"2023-Jan-01 12:02:00", "2023-Jan-01 12:02:15"};
    TS_ASSERT(timesToStr(partials[1], EventSortType::TIMEATSAMPLE_SORT, factor, shift) == expected);
    expected = {};
    TS_ASSERT(timesToStr(partials[TimeSplitter::NO_TARGET], EventSortType::TIMEATSAMPLE_SORT, factor, shift) ==
              expected);
  }
};
