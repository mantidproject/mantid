// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DateAndTime.h"

#include <cxxtest/TestSuite.h>

using Mantid::Kernel::TimeInterval;
using Mantid::Types::Core::DateAndTime;

// some intervals to use
const TimeInterval ONE("2007-11-30T16:00:00", "2007-11-30T17:00:00");
const TimeInterval TWO("2007-11-30T16:30:00", "2007-11-30T17:30:00");
const TimeInterval THREE("2007-11-30T17:00:00", "2007-11-30T18:00:00");
const TimeInterval FOUR("2007-11-30T19:00:00", "2007-11-30T20:00:00");

class TimeIntervalTest : public CxxTest::TestSuite {

public:
  void test_constructors() {
    TimeInterval one(DateAndTime("2007-11-30T16:16:00"), DateAndTime("2007-11-30T16:18:50"));
    TimeInterval two("2007-11-30T16:16:00", "2007-11-30T16:18:50");
    TS_ASSERT_EQUALS(one, two);
  }

  void test_contains() {
    DateAndTime start("2007-11-30T16:00:00");
    DateAndTime stop("2007-11-30T17:00:00");
    TimeInterval interval(start, stop);

    TS_ASSERT(interval.contains(start));
    TS_ASSERT(interval.contains(DateAndTime("2007-11-30T16:30:00")));
    TS_ASSERT(!interval.contains(stop));
  }

  void test_overlaps() {
    // overlaps with self
    TS_ASSERT(ONE.overlaps(ONE));
    // overlaps with middle
    TS_ASSERT(ONE.overlaps(TWO));
    // overlaps with edge
    TS_ASSERT(ONE.overlaps(THREE));
    // doesn't overlap
    TS_ASSERT(!ONE.overlaps(FOUR));
  }

  void test_comparisions() {
    // self comparisons
    TS_ASSERT(!(ONE < ONE));
    TS_ASSERT(!(ONE > ONE));
    TS_ASSERT(ONE == ONE);

    // compare with distinct intervals
    TS_ASSERT(ONE < FOUR);
    TS_ASSERT(FOUR > ONE);
    TS_ASSERT(!(ONE == FOUR));

    // compare with shared edge
    TS_ASSERT(ONE < THREE);
    TS_ASSERT(THREE > ONE);
    TS_ASSERT(!(ONE == THREE));

    // compare with overlap never returns true
    TS_ASSERT(!(ONE < TWO));
    TS_ASSERT(!(TWO > ONE));
    TS_ASSERT(!(ONE == TWO));
  }
};
