// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 *
 *  Created on: Sep 24, 2010
 *      Author: janik
 */

#pragma once

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeROI.h"
#include <ctime>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

class SplittingIntervalTest : public CxxTest::TestSuite {
public:
  //----------------------------------------------------------------------------
  /** Tests the AND operator checking for overlap between two
   * SplittingIntervals.
   */
  void test_SplittingInterval_AND() {
    DateAndTime start_a, stop_a, start_b, stop_b;

    start_a = DateAndTime("2007-11-30T16:17:10");
    stop_a = DateAndTime("2007-11-30T16:17:20");
    SplittingInterval a(start_a, stop_a, 0);

    SplittingInterval b, c;
    // b is all inside a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b = DateAndTime("2007-11-30T16:17:18");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_b);
    TS_ASSERT_EQUALS(c.stop(), stop_b);

    // a is all inside b
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b = DateAndTime("2007-11-30T16:17:23");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_a);
    TS_ASSERT_EQUALS(c.stop(), stop_a);

    // b goes past the end of a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b = DateAndTime("2007-11-30T16:17:25");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_b);
    TS_ASSERT_EQUALS(c.stop(), stop_a);

    // b starts before a and ends before
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b = DateAndTime("2007-11-30T16:17:15");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_a);
    TS_ASSERT_EQUALS(c.stop(), stop_b);

    // No overlap (b < a)
    start_b = DateAndTime("2007-11-30T16:17:01");
    stop_b = DateAndTime("2007-11-30T16:17:02");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT(!a.overlaps(b));
    TS_ASSERT_LESS_THAN_EQUALS(c.duration(), 0.0);

    // No overlap (a < b)
    start_b = DateAndTime("2007-11-30T16:17:30");
    stop_b = DateAndTime("2007-11-30T16:17:42");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a & b;
    TS_ASSERT(!a.overlaps(b));
    TS_ASSERT_LESS_THAN_EQUALS(c.duration(), 0.0);
  }

  //----------------------------------------------------------------------------
  /** Tests the AND operator checking for overlap between two
   * SplittingIntervals.
   */
  void test_SplittingInterval_OR() {
    DateAndTime start_a, stop_a, start_b, stop_b;

    start_a = DateAndTime("2007-11-30T16:17:10");
    stop_a = DateAndTime("2007-11-30T16:17:20");
    SplittingInterval a(start_a, stop_a, 0);

    SplittingInterval b, c;
    // b is all inside a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b = DateAndTime("2007-11-30T16:17:18");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_a);
    TS_ASSERT_EQUALS(c.stop(), stop_a);

    // a is all inside b
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b = DateAndTime("2007-11-30T16:17:23");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_b);
    TS_ASSERT_EQUALS(c.stop(), stop_b);

    // b goes past the end of a
    start_b = DateAndTime("2007-11-30T16:17:12");
    stop_b = DateAndTime("2007-11-30T16:17:25");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_a);
    TS_ASSERT_EQUALS(c.stop(), stop_b);

    // b starts before a and ends before
    start_b = DateAndTime("2007-11-30T16:17:05");
    stop_b = DateAndTime("2007-11-30T16:17:15");
    b = SplittingInterval(start_b, stop_b, 0);
    c = a | b;
    TS_ASSERT(a.overlaps(b));
    TS_ASSERT_EQUALS(c.start(), start_b);
    TS_ASSERT_EQUALS(c.stop(), stop_a);

    // No overlap (b < a) - This throws an exception because you need two
    // outputs!
    start_b = DateAndTime("2007-11-30T16:17:01");
    stop_b = DateAndTime("2007-11-30T16:17:02");
    b = SplittingInterval(start_b, stop_b, 0);
    TS_ASSERT(!a.overlaps(b));
    TS_ASSERT_THROWS(c = a | b;, const std::invalid_argument &);

    // No overlap (a < b)
    start_b = DateAndTime("2007-11-30T16:17:30");
    stop_b = DateAndTime("2007-11-30T16:17:42");
    b = SplittingInterval(start_b, stop_b, 0);
    TS_ASSERT(!a.overlaps(b));
    TS_ASSERT_THROWS(c = a | b;, const std::invalid_argument &);
  }

  //----------------------------------------------------------------------------
  void test_AND() {
    // Make a splitter
    DateAndTime start, stop;
    SplittingIntervalVec a, b;
    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:17:10");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:20");
    stop = DateAndTime("2007-11-30T16:17:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:40");
    stop = DateAndTime("2007-11-30T16:17:50");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:18:00");
    stop = DateAndTime("2007-11-30T16:18:10");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:18:20");
    stop = DateAndTime("2007-11-30T16:18:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    // Smaller than the first one
    start = DateAndTime("2007-11-30T16:17:01");
    stop = DateAndTime("2007-11-30T16:17:25");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:26");
    stop = DateAndTime("2007-11-30T16:17:27");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:45");
    stop = DateAndTime("2007-11-30T16:18:15");
    b.emplace_back(SplittingInterval(start, stop, 0));

    // Now AND the splitters (filters) together
    SplittingIntervalVec c;
    c = a & b;

    TS_ASSERT_EQUALS(c.size(), 5);
    if (c.size() < 5)
      return; // avoid segfaults if this part of the test fails

    SplittingInterval i;
    i = c[0];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:01"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:10"));
    i = c[1];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:20"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:25"));
    i = c[2];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:26"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:27"));
    i = c[3];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:45"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:50"));
    i = c[4];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:18:00"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:18:10"));
  }

  //----------------------------------------------------------------------------
  void test_OR() {
    // Make a splitter
    DateAndTime start, stop;
    SplittingIntervalVec a, b;
    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:17:10");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:20");
    stop = DateAndTime("2007-11-30T16:17:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:40");
    stop = DateAndTime("2007-11-30T16:17:50");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:18:00");
    stop = DateAndTime("2007-11-30T16:18:10");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:18:20");
    stop = DateAndTime("2007-11-30T16:18:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    // Smaller than the first one
    start = DateAndTime("2007-11-30T16:17:01");
    stop = DateAndTime("2007-11-30T16:17:25");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:26");
    stop = DateAndTime("2007-11-30T16:17:27");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:45");
    stop = DateAndTime("2007-11-30T16:18:15");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:18:50");
    stop = DateAndTime("2007-11-30T16:18:55");
    b.emplace_back(SplittingInterval(start, stop, 0));

    // Now AND the splitters (filters) together
    SplittingIntervalVec c;
    c = a | b;

    TS_ASSERT_EQUALS(c.size(), 4);
    if (c.size() < 4)
      return; // avoid segfaults if this part of the test fails

    SplittingInterval i;
    i = c[0];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:30"));
    i = c[1];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:40"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:18:15"));
    i = c[2];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:18:20"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:18:30"));
    i = c[3];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:18:50"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:18:55"));
  }

  //----------------------------------------------------------------------------
  void test_OR_with_a_bad_input() {
    // Make a splitter
    DateAndTime start, stop;
    SplittingIntervalVec a, b;

    start = DateAndTime("2007-11-30T16:17:20");
    stop = DateAndTime("2007-11-30T16:17:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    // A bad (reversed) interval
    start = DateAndTime("2007-11-30T16:17:32");
    stop = DateAndTime("2007-11-30T16:17:31");
    a.emplace_back(SplittingInterval(start, stop, 0));

    // REVERSED interval that is before the first one
    start = DateAndTime("2007-11-30T16:17:15");
    stop = DateAndTime("2007-11-30T16:17:00");
    b.emplace_back(SplittingInterval(start, stop, 0));

    // Another bad interval
    start = DateAndTime("2007-11-30T16:17:45");
    stop = DateAndTime("2007-11-30T16:17:35");
    b.emplace_back(SplittingInterval(start, stop, 0));

    // Now AND the splitters (filters) together
    SplittingIntervalVec c;
    c = a | b;

    TS_ASSERT_EQUALS(c.size(), 1);
    if (c.size() < 1)
      return; // avoid segfaults if this part of the test fails

    SplittingInterval i;
    i = c[0];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:20"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:30"));
  }

  //----------------------------------------------------------------------------
  void test_NOT_Normal() {
    DateAndTime start, stop;
    SplittingIntervalVec a, b, c;
    SplittingInterval i;

    //---- Normal Case ------
    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:17:10");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:20");
    stop = DateAndTime("2007-11-30T16:17:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    // Perform the NOT operation
    c = ~a;

    TS_ASSERT_EQUALS(c.size(), 3);
    if (c.size() < 3)
      return; // avoid segfaults if this part of the test fails

    i = c[0];
    TS_ASSERT_EQUALS(i.start(), DateAndTime::minimum());
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:00"));
    i = c[1];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:20"));
    i = c[2];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:30"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime::maximum());
  }

  //----------------------------------------------------------------------------
  void test_NOT_empty() {
    DateAndTime start, stop;
    SplittingIntervalVec a, b, c;
    SplittingInterval i;

    //---- Empty case ----------
    c = ~b;
    TS_ASSERT_EQUALS(c.size(), 1);
    if (c.size() < 1)
      return; // avoid segfaults if this part of the test fails
    i = c[0];
    TS_ASSERT_EQUALS(i.start(), DateAndTime::minimum());
    TS_ASSERT_EQUALS(i.stop(), DateAndTime::maximum());
  }

  //----------------------------------------------------------------------------
  void test_NOT_overlap() {
    DateAndTime start, stop;
    SplittingIntervalVec a, b, c;
    SplittingInterval i;
    // Overlapping case ------
    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:17:15");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:10");
    stop = DateAndTime("2007-11-30T16:17:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    c = ~a;
    TS_ASSERT_EQUALS(c.size(), 2);
    if (c.size() < 3)
      return; // avoid segfaults if this part of the test fails

    i = c[0];
    TS_ASSERT_EQUALS(i.start(), DateAndTime::minimum());
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:00"));
    i = c[1];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:30"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime::maximum());
  }

  //----------------------------------------------------------------------------
  void test_PLUS() {
    DateAndTime start, stop;
    SplittingIntervalVec a, b, c;
    SplittingInterval i;

    //  the splitter ------
    start = DateAndTime("2007-11-30T16:15:00");
    stop = DateAndTime("2007-11-30T16:16:00");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:18:00");
    b.emplace_back(SplittingInterval(start, stop, 1));

    start = DateAndTime("2007-11-30T16:18:00");
    stop = DateAndTime("2007-11-30T16:19:00");
    b.emplace_back(SplittingInterval(start, stop, 2));

    start = DateAndTime("2007-11-30T16:19:00");
    stop = DateAndTime("2007-11-30T16:20:00");
    b.emplace_back(SplittingInterval(start, stop, 3));

    // the filter ------
    start = DateAndTime("2007-11-30T16:16:50");
    stop = DateAndTime("2007-11-30T16:17:10");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:20");
    stop = DateAndTime("2007-11-30T16:17:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:40");
    stop = DateAndTime("2007-11-30T16:18:10");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:18:50");
    stop = DateAndTime("2007-11-30T16:18:55");
    a.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:22:20");
    stop = DateAndTime("2007-11-30T16:22:30");
    a.emplace_back(SplittingInterval(start, stop, 0));

    // Do the PLUS operation
    c = a + b;

    TS_ASSERT_EQUALS(c.size(), 5);
    if (c.size() < 5)
      return; // avoid segfaults if this part of the test fails

    i = c[0];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(i.index(), 1);

    i = c[1];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:20"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:17:30"));
    TS_ASSERT_EQUALS(i.index(), 1);

    i = c[2];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:17:40"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:18:00"));
    TS_ASSERT_EQUALS(i.index(), 1);

    i = c[3];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:18:00"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:18:10"));
    TS_ASSERT_EQUALS(i.index(), 2);

    i = c[4];
    TS_ASSERT_EQUALS(i.start(), DateAndTime("2007-11-30T16:18:50"));
    TS_ASSERT_EQUALS(i.stop(), DateAndTime("2007-11-30T16:18:55"));
    TS_ASSERT_EQUALS(i.index(), 2);

    // This fails since you can't add splitters together
    TS_ASSERT_THROWS(b + b, const std::invalid_argument &);
  }

  //----------------------------------------------------------------------------
  void test_sort() {
    DateAndTime start, stop;
    SplittingIntervalVec b;

    //  the splitter intentionally has overlapping intervals
    start = DateAndTime("2007-11-30T16:15:00");
    stop = DateAndTime("2007-11-30T16:16:00");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:19:00");
    stop = DateAndTime("2007-11-30T16:20:00");
    b.emplace_back(SplittingInterval(start, stop, 3));

    start = DateAndTime("2007-11-30T16:18:00");
    stop = DateAndTime("2007-11-30T16:19:00");
    b.emplace_back(SplittingInterval(start, stop, 2));

    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:18:00");
    b.emplace_back(SplittingInterval(start, stop, 1));

    // sort using the operator<
    std::sort(b.begin(), b.end());

    TS_ASSERT_EQUALS(b[0].start(), DateAndTime("2007-11-30T16:15:00"));
    TS_ASSERT_EQUALS(b[1].start(), DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(b[2].start(), DateAndTime("2007-11-30T16:18:00"));
    TS_ASSERT_EQUALS(b[3].start(), DateAndTime("2007-11-30T16:19:00"));
  }

  //----------------------------------------------------------------------------
  void test_find() {
    DateAndTime start, stop;
    SplittingIntervalVec b;

    //  the splitter ------
    start = DateAndTime("2007-11-30T16:15:00");
    stop = DateAndTime("2007-11-30T16:16:00");
    b.emplace_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:19:00");
    stop = DateAndTime("2007-11-30T16:20:00");
    b.emplace_back(SplittingInterval(start, stop, 3));

    start = DateAndTime("2007-11-30T16:18:00");
    stop = DateAndTime("2007-11-30T16:19:00");
    b.emplace_back(SplittingInterval(start, stop, 2));

    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:18:00");
    b.emplace_back(SplittingInterval(start, stop, 1));

    std::sort(b.begin(), b.end());

    SplittingIntervalVec::iterator sit;

    SplittingInterval temp1(DateAndTime("2007-11-30T16:17:00"), DateAndTime("2007-11-30T16:17:00"), -1);
    sit = std::lower_bound(b.begin(), b.end(), temp1);
    int index1 = int(sit - b.begin());
    TS_ASSERT_EQUALS(index1, 1);

    SplittingInterval temp2(DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:00"), -1);
    sit = std::lower_bound(b.begin(), b.end(), temp2);
    int index2 = int(sit - b.begin());
    TS_ASSERT_EQUALS(index2, 1);
  }

  //----------------------------------------------------------------------------
  void test_timeROIsFromSplitters() {
    DateAndTime start, stop;
    SplittingIntervalVec splitters;

    // splitter assigned to destination index 1
    start = DateAndTime("2007-11-30T16:15:00");
    stop = DateAndTime("2007-11-30T16:16:00");
    splitters.emplace_back(SplittingInterval(start, stop, 1));

    // splitter  assigned to nonsense destination index -1
    start = DateAndTime("2007-11-30T16:16:00");
    stop = DateAndTime("2007-11-30T16:17:00");
    splitters.emplace_back(SplittingInterval(start, stop, -1));

    // splitter assigned to destination index 1. It overlaps with the first splitter!
    start = DateAndTime("2007-11-30T16:14:00");
    stop = DateAndTime("2007-11-30T16:15:30");
    splitters.emplace_back(SplittingInterval(start, stop, 1));

    // splitter assigned to destination index 2
    start = DateAndTime("2007-11-30T16:17:00");
    stop = DateAndTime("2007-11-30T16:18:00");
    splitters.emplace_back(SplittingInterval(start, stop, 2));

    // splitter assigned to destination index 1. It doesn't overlap
    start = DateAndTime("2007-11-30T16:18:00");
    stop = DateAndTime("2007-11-30T16:19:00");
    splitters.emplace_back(SplittingInterval(start, stop, 1));

    // this splitter has same start and stop time. It will not be turned into a TIMEROI objed
    // splitter assigned to destination index 3
    start = DateAndTime("2007-11-30T16:20:00");
    stop = DateAndTime("2007-11-30T16:20:00");
    splitters.emplace_back(SplittingInterval(start, stop, 3));

    // map each workspace index to a splitter
    const std::map<int, Mantid::Kernel::TimeROI> &rois = timeROIsFromSplitters(splitters);

    // Assertions
    TS_ASSERT_EQUALS(rois.size(), 3); // workspace indexes -1, 1, and 3
    // assert the destination indexes
    std::vector<int> destinationIndexes;
    for (auto const &element : rois)
      destinationIndexes.push_back(element.first);
    std::sort(destinationIndexes.begin(), destinationIndexes.end());
    std::vector<int> expected{-1, 1, 2};
    for (size_t i = 0; i < destinationIndexes.size(); i++)
      TS_ASSERT_EQUALS(destinationIndexes[i], expected[i]);
    // assert the TimeROI's
    TS_ASSERT_EQUALS(rois.at(-1).debugStrPrint(), "0: 2007-Nov-30 16:16:00 to 2007-Nov-30 16:17:00\n");
    TS_ASSERT_EQUALS(
        rois.at(1).debugStrPrint(),
        "0: 2007-Nov-30 16:14:00 to 2007-Nov-30 16:16:00\n1: 2007-Nov-30 16:18:00 to 2007-Nov-30 16:19:00\n");
    TS_ASSERT_EQUALS(rois.at(2).debugStrPrint(), "0: 2007-Nov-30 16:17:00 to 2007-Nov-30 16:18:00\n");
  }
};
