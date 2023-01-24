// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeROI.h"

using Mantid::Kernel::TimeROI;
using Mantid::Types::Core::DateAndTime;

constexpr double ONE_DAY_DURATION{24 * 3600};

const std::string DECEMBER_START("2022-12-01T00:01");
const std::string DECEMBER_STOP("2023-01-01T00:01");
const TimeROI DECEMBER{DateAndTime(DECEMBER_START), DateAndTime(DECEMBER_STOP)};

const std::string HANUKKAH_START("2022-12-19T00:01");
const std::string HANUKKAH_STOP("2022-12-26T00:01");
constexpr double HANUKKAH_DURATION{7. * ONE_DAY_DURATION};

const std::string CHRISTMAS_START("2022-12-25T00:01");
const std::string CHRISTMAS_STOP("2022-12-26T00:01"); // same as HANUKKAH_STOP
const TimeROI CHRISTMAS{CHRISTMAS_START, CHRISTMAS_STOP};

const std::string NEW_YEARS_START("2022-12-31T00:01");
const std::string NEW_YEARS_STOP("2023-01-01T00:01");

const DateAndTime ONE("2023-01-01T00:01");
const DateAndTime TWO("2023-01-02T00:01");
const DateAndTime THREE("2023-01-03T00:01");
const DateAndTime FOUR("2023-01-04T00:01");
const DateAndTime FIVE("2023-01-05T00:01");
const DateAndTime SIX("2023-01-06T00:01");

class TimeROITest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeROITest *createSuite() { return new TimeROITest(); }
  static void destroySuite(TimeROITest *suite) { delete suite; }

  void test_emptyROI() {
    TimeROI value;
    TS_ASSERT_EQUALS(value.durationInSeconds(), 0.);
    TS_ASSERT(value.empty());
    TS_ASSERT_EQUALS(value.numBoundaries(), 0);
    value.removeRedundantEntries();
  }

  void test_badRegions() {
    TimeROI value;
    TS_ASSERT_THROWS(value.addROI(NEW_YEARS_STOP, NEW_YEARS_START), const std::runtime_error &);
    TS_ASSERT_EQUALS(value.numBoundaries(), 0);
    TS_ASSERT_THROWS(value.addMask(NEW_YEARS_STOP, NEW_YEARS_START), const std::runtime_error &);
    TS_ASSERT_EQUALS(value.numBoundaries(), 0);
  }

  void test_durations() {
    TimeROI value{HANUKKAH_START, HANUKKAH_STOP};

    // verify the full duration
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION);
    TS_ASSERT_EQUALS(value.durationInSeconds(HANUKKAH_START, HANUKKAH_STOP), HANUKKAH_DURATION);

    // window parameter order matters
    TS_ASSERT_THROWS(value.durationInSeconds(HANUKKAH_STOP, HANUKKAH_START), const std::runtime_error &);

    // window entirely outside of TimeROI gives zero
    TS_ASSERT_EQUALS(value.durationInSeconds(DECEMBER_START, HANUKKAH_START), 0.);
    TS_ASSERT_EQUALS(value.durationInSeconds(HANUKKAH_STOP, NEW_YEARS_STOP), 0.);

    // from the beginning
    TS_ASSERT_EQUALS(value.durationInSeconds(DECEMBER_START, CHRISTMAS_START) / ONE_DAY_DURATION, 6.);
    TS_ASSERT_EQUALS(value.durationInSeconds(HANUKKAH_START, CHRISTMAS_START) / ONE_DAY_DURATION, 6.);

    // past the end
    TS_ASSERT_EQUALS(value.durationInSeconds(CHRISTMAS_START, HANUKKAH_STOP) / ONE_DAY_DURATION, 1.);
    TS_ASSERT_EQUALS(value.durationInSeconds(CHRISTMAS_START, NEW_YEARS_STOP) / ONE_DAY_DURATION, 1.);
  }

  void test_sortedROI() {
    TimeROI value;
    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT(!value.empty());

    // add New Year's eve
    value.addROI(NEW_YEARS_START, NEW_YEARS_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION + ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);

    // add Christmas - fully contained in existing TimeROI
    value.addROI(CHRISTMAS_START, CHRISTMAS_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION + ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);

    // get rid of entries that have no effect
    value.removeRedundantEntries();
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
  }

  void test_addROI() {
    TimeROI value{THREE, FOUR}; // 3-4
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);

    value.addROI(TWO, FIVE); // 2-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);

    value.addROI(TWO, SIX); // 2-6
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);

    value.addROI(THREE, FIVE); // 2-6
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);

    value.addROI(ONE, TWO); // 1-6
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 5.);

    value.addMask(ONE, SIX); // empty
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 0.);
  }

  void test_addOverlapping() {
    TimeROI value{ONE, FOUR}; // 1-4
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    // extend one day past the end is 1-5
    value.addROI(THREE, FIVE);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    // add in time from the middle is still 1-5
    value.addROI(TWO, THREE);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    // now remove regions
    value.addMask(TWO, THREE); // 1-2, 3-5 is left
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);

    value.addMask(TWO, FOUR); // 1-2, 4-5 is left
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);

    value.addMask(THREE, FIVE); // 1-2 is left
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    // remove the rest
    value.addMask(ONE, FOUR);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 0.);
    TS_ASSERT(value.empty());

    // add back an ROI then remove parts until nothing is left
    value.addROI(TWO, FIVE); // 2-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    value.addMask(ONE, THREE); // 3-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    value.addMask(ONE, FOUR); // 4-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    value.addMask(FOUR, FIVE); // empty
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 0.);
    TS_ASSERT(value.empty());
  }

  void test_redundantValues() {
    TimeROI value;
    value.addROI(CHRISTMAS_START, CHRISTMAS_STOP);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    value.addROI(CHRISTMAS_START, CHRISTMAS_STOP);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    value.removeRedundantEntries();
  }

  void test_reversesortedROI() {
    TimeROI value;
    // add New Year's eve
    value.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), ONE_DAY_DURATION + HANUKKAH_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
  }

  void test_onlyMask() {
    TimeROI value;
    // the result is empty
    value.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT(value.empty());

    // since it ends with "on" the duration is infinite
    TS_ASSERT_EQUALS(value.durationInSeconds(), 0.);
    value.removeRedundantEntries();
    TS_ASSERT(value.empty());
  }

  void test_overwrite() {
    // mask first
    TimeROI value1;
    value1.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    // since it ends with "on" the duration is infinite
    TS_ASSERT_EQUALS(value1.durationInSeconds(), 0.);
    TS_ASSERT(value1.empty());

    value1.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value1.durationInSeconds(), ONE_DAY_DURATION);
    value1.removeRedundantEntries();
    TS_ASSERT_EQUALS(value1.numBoundaries(), 2);

    // roi first
    TimeROI value2;
    value2.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value2.durationInSeconds(), ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value2.numBoundaries(), 2);

    value2.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value2.durationInSeconds(), 0.);
    value2.removeRedundantEntries();
    TS_ASSERT_EQUALS(value2.numBoundaries(), 0);
  }

  void test_valueAtTime() {
    // to understand the checks, note that
    // USE = true
    // IGNORE = false

    // values outside of the TimeROI should be ignore
    TS_ASSERT_EQUALS(CHRISTMAS.valueAtTime(DECEMBER_START), false);
    TS_ASSERT_EQUALS(CHRISTMAS.valueAtTime(DECEMBER_STOP), false);

    // tests for more interesting values
    TS_ASSERT_EQUALS(DECEMBER.valueAtTime(DECEMBER_START), true);  // first in region
    TS_ASSERT_EQUALS(DECEMBER.valueAtTime(CHRISTMAS_START), true); // middle of region
    TS_ASSERT_EQUALS(DECEMBER.valueAtTime(DECEMBER_STOP), false);  // last of region
  }

  void runIntersectionTest(const TimeROI &left, const TimeROI &right, const double exp_duration) {
    // left intersecting with right
    TimeROI one(left);
    one.update_intersection(right);
    TS_ASSERT_EQUALS(one.durationInSeconds() / ONE_DAY_DURATION, exp_duration / ONE_DAY_DURATION);

    // right intersecting with left
    TimeROI two(right);
    two.update_intersection(left);
    TS_ASSERT_EQUALS(two.durationInSeconds() / ONE_DAY_DURATION, exp_duration / ONE_DAY_DURATION);

    // the values should be identical
    TS_ASSERT_EQUALS(one, two);
  }

  void test_intersection_same_date() { runIntersectionTest(CHRISTMAS, CHRISTMAS, CHRISTMAS.durationInSeconds()); }

  void test_intersection_full_overlap() { runIntersectionTest(DECEMBER, CHRISTMAS, CHRISTMAS.durationInSeconds()); }

  void test_intersection_partial_overlap() {
    TimeROI left{HANUKKAH_START, NEW_YEARS_START};
    TimeROI right{HANUKKAH_STOP, NEW_YEARS_STOP};
    runIntersectionTest(left, right, 5. * ONE_DAY_DURATION);
  }

  void test_intersection_no_overlap() { runIntersectionTest(CHRISTMAS, TimeROI{NEW_YEARS_START, NEW_YEARS_STOP}, 0.); }

  void runUnionTest(const TimeROI &left, const TimeROI &right, const double exp_duration) {
    // left union with right
    TimeROI one(left);
    one.update_union(right);
    TS_ASSERT_EQUALS(one.durationInSeconds() / ONE_DAY_DURATION, exp_duration / ONE_DAY_DURATION);

    // right union with left
    TimeROI two(right);
    two.update_union(left);
    TS_ASSERT_EQUALS(two.durationInSeconds() / ONE_DAY_DURATION, exp_duration / ONE_DAY_DURATION);

    // the values should be identical
    TS_ASSERT_EQUALS(one, two);
  }

  void test_union_same_date() { runUnionTest(CHRISTMAS, CHRISTMAS, CHRISTMAS.durationInSeconds()); }

  void test_union_full_overlap() { runUnionTest(DECEMBER, CHRISTMAS, DECEMBER.durationInSeconds()); }

  void test_union_partial_overlap() {
    TimeROI left{HANUKKAH_START, NEW_YEARS_START};
    TimeROI right{HANUKKAH_STOP, NEW_YEARS_STOP};
    runUnionTest(left, right, 13. * ONE_DAY_DURATION);
  }

  void test_union_no_overlap() {
    runUnionTest(CHRISTMAS, TimeROI{NEW_YEARS_START, NEW_YEARS_STOP}, 2. * ONE_DAY_DURATION);
  }
};
