// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TimeSplitter.h"
#include "MantidKernel/TimeROI.h"

using Mantid::API::TableRow;
using Mantid::DataObjects::TimeSplitter;
using Mantid::Kernel::TimeROI;
using Mantid::Types::Core::DateAndTime;

namespace {
const DateAndTime ONE("2023-01-01T11:00:00");
const DateAndTime TWO("2023-01-01T12:00:00");
const DateAndTime THREE("2023-01-01T13:00:00");
const DateAndTime FOUR("2023-01-01T14:00:00");
const DateAndTime FIVE("2023-01-01T15:00:00");
} // namespace

class TimeSplitterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeSplitterTest *createSuite() { return new TimeSplitterTest(); }
  static void destroySuite(TimeSplitterTest *suite) { delete suite; }

  void test_valueAtTime() {
    // to start everything is either in 0th output or masked
    TimeSplitter splitter(TWO, FOUR);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({0}));

    // add ROI for first half to go to 1st output
    splitter.addROI(TWO, THREE, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({0, 1}));

    // add ROI for second half to go to 2nd output
    splitter.addROI(THREE, FOUR, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 2}));

    // have whole thing go to 3rd output
    splitter.addROI(TWO, FOUR, 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({3}));

    // prepend a section that goes to 1st output
    splitter.addROI(ONE, TWO, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 3}));

    // append a section that goes to 2nd output
    splitter.addROI(FOUR, FIVE, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 2, 3}));

    // set before the beginning to mask
    splitter.addROI(ONE, TWO, -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({2, 3}));

    // set after the end to mask
    splitter.addROI(FOUR, FIVE, -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({3}));
  }

  void test_emptySplitter() {
    TimeSplitter splitter;
    TS_ASSERT_EQUALS(splitter.valueAtTime(DateAndTime("2023-01-01T11:00:00")), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 0);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({}));
  }

  void test_gap() {
    TimeSplitter splitter;
    // create a splitter with a gap
    splitter.addROI(ONE, TWO, 0);
    splitter.addROI(THREE, FOUR, 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);

    // fill in the gap with a different value
    splitter.addROI(TWO, THREE, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);

    // fill in the gap with the same value as before and after
    splitter.addROI(TWO, THREE, 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
  }

  void test_getTimeROI() {

    // start with empty TimeSplitter
    TimeSplitter splitter;
    TS_ASSERT(splitter.getTimeROI(-1).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());

    // place to put the output
    TimeROI roi;

    // add a single TimeROI
    splitter.addROI(ONE, THREE, 1);
    TS_ASSERT(splitter.getTimeROI(-1).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.empty());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);

    // add the same output index, but with a gap from the previous
    splitter.addROI(FOUR, FIVE, 1);
    roi = splitter.getTimeROI(-2); // intentionally trying a "big" negative for ignore filter
    TS_ASSERT(!roi.empty());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);
    TS_ASSERT(splitter.getTimeROI(0).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.empty());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);
  }

  /** Test that a TimeSplitter object constructed from a TableWorkspace object
   * is equivalent to a TimeSplitter object built by successively adding the corresponding time ROIs.
   * @brief test_createFromTableWorkspace
   */
  void test_createFromTableWorkspace() {
    auto tws = std::make_shared<Mantid::DataObjects::TableWorkspace>(3 /*rows*/);

    // by design, a table workspace used for event filtering must have 3 columns
    tws->addColumn("double", "start");
    tws->addColumn("double", "stop");
    tws->addColumn("str", "target"); // index of a target workspace where the filtered time events will be output

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
    row << time5_s << time6_s << "2";

    // create a TimeSplitter object from the table. By design, the table creator must know whether
    // the table holds absolute or relative times. In the latter case the creator must have
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
    referenceSplitter.addROI(time5_abs, time6_abs, 2);

    TS_ASSERT_EQUALS(referenceSplitter.numRawValues(), workspaceDerivedSplitter.numRawValues());
    TS_ASSERT(referenceSplitter.valueAtTime(time1_abs) == workspaceDerivedSplitter.valueAtTime(time1_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time2_abs) == workspaceDerivedSplitter.valueAtTime(time2_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time3_abs) == workspaceDerivedSplitter.valueAtTime(time3_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time4_abs) == workspaceDerivedSplitter.valueAtTime(time4_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time5_abs) == workspaceDerivedSplitter.valueAtTime(time5_abs));
    TS_ASSERT(referenceSplitter.valueAtTime(time6_abs) == workspaceDerivedSplitter.valueAtTime(time6_abs));
  }
};
