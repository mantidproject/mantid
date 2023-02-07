// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeSplitter.h"

using Mantid::Kernel::TimeSplitter;
using Mantid::Types::Core::DateAndTime;

class TimeSplitterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeSplitterTest *createSuite() { return new TimeSplitterTest(); }
  static void destroySuite(TimeSplitterTest *suite) { delete suite; }

  void test_valueAtTime() {
    const DateAndTime before("2023-01-01T11:00:00");
    const DateAndTime start("2023-01-01T12:00:00");
    const DateAndTime middle("2023-01-01T13:00:00");
    const DateAndTime stop("2023-01-01T14:00:00");
    const DateAndTime after("2023-01-01T15:00:00");

    // to start everything is either in 0th output or masked
    TimeSplitter splitter(start, stop);
    TS_ASSERT_EQUALS(splitter.valueAtTime(before), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(start), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(middle), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(stop), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(after), -1);

    // add ROI for first half to go to 1st output
    splitter.addROI(start, middle, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(before), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(start), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(middle), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(stop), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(after), -1);

    // add ROI for second half to go to 2nd output
    splitter.addROI(middle, stop, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(before), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(start), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(middle), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(stop), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(after), -1);

    // have whole thing go to 3rd output
    splitter.addROI(start, stop, 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(before), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(start), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(middle), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(stop), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(after), -1);

    // prepend a section that goes to 1st output
    splitter.addROI(before, start, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(before), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(start), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(middle), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(stop), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(after), -1);

    // prepend a section that goes to 2nd output
    splitter.addROI(stop, after, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(before), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(start), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(middle), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(stop), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(after), -1);
  }

  void test_emptySplitter() {
    TimeSplitter splitter;
    TS_ASSERT_EQUALS(splitter.valueAtTime(DateAndTime("2023-01-01T11:00:00")), -1);
  }
};
