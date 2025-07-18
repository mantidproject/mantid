// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"

using Mantid::Kernel::TimeROI;
using Mantid::Kernel::TimeSeriesProperty;
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
    TS_ASSERT(value.useAll());
    TS_ASSERT_EQUALS(value.numBoundaries(), 0);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 0);
  }

  void test_badRegions() {
    TimeROI value;
    TS_ASSERT_THROWS(value.addROI(NEW_YEARS_STOP, NEW_YEARS_START), const std::runtime_error &);
    TS_ASSERT_EQUALS(value.numBoundaries(), 0);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 0);
    TS_ASSERT_THROWS(value.addMask(NEW_YEARS_STOP, NEW_YEARS_START), const std::runtime_error &);
    TS_ASSERT_EQUALS(value.numBoundaries(), 0);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 0);
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

  void test_replaceFromTSP() {
    TimeROI value{CHRISTMAS_START, CHRISTMAS_STOP};

    TimeSeriesProperty<bool> tsp("junk");
    value.replaceROI(&tsp);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 0.);

    tsp.addValue(CHRISTMAS_START, true);
    tsp.addValue(CHRISTMAS_STOP, false);
    value.replaceROI(&tsp);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);

    tsp.addValue(DECEMBER_START, false); // should get ignored
    tsp.addValue(CHRISTMAS_STOP, true);  // should override previous value
    tsp.addValue(DECEMBER_STOP, false);  // new endpoint
    value.replaceROI(&tsp);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 7.);
  }

  void test_sortedROI() {
    TimeROI value;
    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);
    TS_ASSERT(!value.useAll());

    // add New Year's eve
    value.addROI(NEW_YEARS_START, NEW_YEARS_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION + ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 2);

    // add Christmas - fully contained in existing TimeROI
    value.addROI(CHRISTMAS_START, CHRISTMAS_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION + ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 2);

    // get rid of entries that have no effect
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 2);
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

  void test_addROI2() {
    TimeROI value{TWO, THREE};
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    value.addROI(ONE, TWO);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);
    value.addROI(THREE, FOUR);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    value.addROI(TWO, FIVE);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);
  }

  void test_addMask() {
    TimeROI value{ONE, TWO}; // 1-2
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    value.addROI(THREE, FIVE); // 1-2,3-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    value.addMask(THREE, FOUR); // 1-2,4-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);

    value.addROI(THREE, FIVE); // 1-2,3-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    value.addMask(FOUR, SIX); // 1-2,3-4
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);

    value.addROI(TWO, FIVE); // 1-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);
    value.addMask(ONE, TWO); // 2-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    value.addMask(THREE, FOUR); // 1-3,4-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);
    value.addMask(THREE, FOUR); // 1-3,4-5 still
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);
  }

  void test_addOverlapping() {
    TimeROI value{ONE, FOUR}; // 1-4
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    // extend one day past the end is 1-5
    value.addROI(THREE, FIVE);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    // add in time from the middle is still 1-5
    value.addROI(TWO, THREE);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 4.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    // now remove regions
    value.addMask(TWO, THREE); // 1-2, 3-5 is left
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 2);

    value.addMask(TWO, FOUR); // 1-2, 4-5 is left
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 2);

    value.addMask(THREE, FIVE); // 1-2 is left
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    // remove the rest
    value.addMask(ONE, FOUR);
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 0.);
    TS_ASSERT(value.useAll());

    // add back an ROI then remove parts until nothing is left
    value.addROI(TWO, FIVE); // 2-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 3.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    value.addMask(ONE, THREE); // 3-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 2.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    value.addMask(ONE, FOUR); // 4-5
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    value.addMask(FOUR, FIVE); // empty
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 0.);
    TS_ASSERT(value.useAll());
  }

  void test_redundantValues() {
    TimeROI value;
    value.addROI(CHRISTMAS_START, CHRISTMAS_STOP);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);
    value.addROI(CHRISTMAS_START, CHRISTMAS_STOP);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);
  }

  void test_reversesortedROI() {
    TimeROI value;
    // add New Year's eve
    value.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value.durationInSeconds() / ONE_DAY_DURATION, 1.);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 1);

    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), ONE_DAY_DURATION + HANUKKAH_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
    TS_ASSERT_EQUALS(value.numberOfRegions(), 2);
  }

  void test_onlyMask() {
    TimeROI value;
    // the result is empty
    value.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT(value.useAll());

    // since it ends with "on" the duration is infinite
    TS_ASSERT_EQUALS(value.durationInSeconds(), 0.);
    TS_ASSERT(value.useAll());
  }

  void test_overwrite() {
    // mask first
    TimeROI value1;
    value1.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    // since it ends with "on" the duration is infinite
    TS_ASSERT_EQUALS(value1.durationInSeconds(), 0.);
    TS_ASSERT(value1.useAll());

    value1.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value1.durationInSeconds(), ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value1.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value1.numberOfRegions(), 1);

    // roi first
    TimeROI value2;
    value2.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value2.durationInSeconds(), ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value2.numBoundaries(), 2);
    TS_ASSERT_EQUALS(value2.numberOfRegions(), 1);

    value2.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value2.durationInSeconds(), 0.);
    TS_ASSERT_EQUALS(value2.numBoundaries(), 0);
    TS_ASSERT_EQUALS(value2.numberOfRegions(), 0);
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

  void test_intersection_no_overlap() { runIntersectionTest(CHRISTMAS, TimeROI{NEW_YEARS_START, NEW_YEARS_STOP}, -1.); }

  void test_intersection_one_empty() { runIntersectionTest(CHRISTMAS, TimeROI(), 0.); }

  // this is a test lifted from a situation that arrises in DataHandlingTest and FilterLog
  void test_intersection_ISISRunLogs() {
    TimeROI left;
    left.addROI("2008-Jun-17 11:20:09", "2008-Jun-17 11:20:11");
    left.addROI("2008-Jun-17 11:20:21", "2008-Jun-17 11:20:23");
    left.addROI("2008-Jun-17 11:20:32", "2008-Jun-17 11:20:33");
    left.addROI("2008-Jun-17 11:20:41", "2008-Jun-17 11:20:45");
    left.addROI("2008-Jun-17 11:20:53", "2008-Jun-17 11:20:57");
    left.addROI("2008-Jun-17 11:21:06", "2008-Jun-17 11:21:07");
    left.addROI("2008-Jun-17 11:21:16", "2008-Jun-17 11:21:18");
    left.addROI("2008-Jun-17 11:21:19", "2008-Jun-17 11:22:29");

    TimeROI right;
    right.addROI("2008-Jun-17 11:12:11", "2008-Jun-17 11:12:21");
    right.addROI("2008-Jun-17 11:17:57", "2008-Jun-17 11:18:07");
    right.addROI("2008-Jun-17 11:21:08", "2008-Jun-17 11:21:19");

    // this is the hand calculated answer
    TimeROI intersection;
    intersection.addROI("2008-Jun-17 11:21:16", "2008-Jun-17 11:21:18");

    // intersection with the right answer should yeild the right answer
    runIntersectionTest(left, intersection, intersection.durationInSeconds());
    runIntersectionTest(right, intersection, intersection.durationInSeconds());
    runIntersectionTest(left, right, intersection.durationInSeconds());
  }

  /*
   * This test is similar to test_intersection_one_empty, except the function that is called will replace the TimeROI
   * with the non-empty one.
   */
  void test_replace_intersection() {
    TimeROI one(CHRISTMAS);
    one.update_or_replace_intersection(TimeROI());
    TS_ASSERT_EQUALS(one.durationInSeconds() / ONE_DAY_DURATION, CHRISTMAS.durationInSeconds() / ONE_DAY_DURATION);

    TimeROI two;
    two.update_or_replace_intersection(CHRISTMAS);
    TS_ASSERT_EQUALS(two.durationInSeconds() / ONE_DAY_DURATION, CHRISTMAS.durationInSeconds() / ONE_DAY_DURATION);
  }

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

  void test_union_one_empty() { runUnionTest(CHRISTMAS, TimeROI(), CHRISTMAS.durationInSeconds()); }

  /**
   * This copies the TimeSeriesProperty<bool> used in Kernel::LogFilterTest which ends in "use"
   */
  void test_badTimeSeriesPropertyTest() {
    DateAndTime one("2007-11-30T16:16:50");
    DateAndTime two("2007-11-30T16:17:25");
    DateAndTime three("2007-11-30T16:17:39");

    TimeSeriesProperty<bool> tsp("filter");
    tsp.addValue(one, true);
    tsp.addValue(two, false);
    tsp.addValue(three, true);

    TimeROI roi(&tsp);
    // should be two roi with the specified values being consistent
    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);
    TS_ASSERT_EQUALS(roi.numberOfRegions(), 2);
    TS_ASSERT_EQUALS(roi.valueAtTime(one), true);
    TS_ASSERT_EQUALS(roi.valueAtTime(two), false);
    TS_ASSERT_EQUALS(roi.valueAtTime(three), true);

    // a full duration past the end is false
    const auto duration = three - one;
    DateAndTime four = three + duration;
    TS_ASSERT_EQUALS(roi.valueAtTime(four), false);
  }

  void test_getEffectiveTime() {
    TimeROI roi{HANUKKAH_START, HANUKKAH_STOP};
    TS_ASSERT_EQUALS(roi.getEffectiveTime(DECEMBER_START), HANUKKAH_START);
    TS_ASSERT_EQUALS(roi.getEffectiveTime(HANUKKAH_START), HANUKKAH_START);
    TS_ASSERT_EQUALS(roi.getEffectiveTime(CHRISTMAS_START), CHRISTMAS_START);
    TS_ASSERT_THROWS(roi.getEffectiveTime(DECEMBER_STOP), const std::runtime_error &);
  }

  void test_invalidROI() {
    TS_ASSERT(TimeROI::USE_NONE.useNone());
    TS_ASSERT_EQUALS(TimeROI::USE_NONE.durationInSeconds(), -1);
  }

  void test_debugStrPrint() {
    TimeROI roi{HANUKKAH_START, HANUKKAH_STOP};
    roi.addROI(NEW_YEARS_START, NEW_YEARS_STOP);
    TS_ASSERT_EQUALS(
        roi.debugStrPrint(0),
        "0: 2022-Dec-19 00:01:00 to 2022-Dec-26 00:01:00\n1: 2022-Dec-31 00:01:00 to 2023-Jan-01 00:01:00\n");
    TS_ASSERT_EQUALS(roi.debugStrPrint(1),
                     "2022-Dec-19 00:01:00 2022-Dec-26 00:01:00 2022-Dec-31 00:01:00 2023-Jan-01 00:01:00 \n");
  }

  void test_calculate_indices() {
    TimeROI roi;
    roi.addROI(ONE, TWO);
    roi.addROI(THREE, FOUR);
    roi.addROI(FIVE, SIX);
    roi.addROI(SIX + 100.0, SIX + 200.0); // region that is not included in the times

    std::vector<DateAndTime> times{ONE + 100.0, TWO + 100.0, THREE, FOUR - 100.0, FIVE + 100.0};

    auto indices = roi.calculate_indices(times);
    TS_ASSERT_EQUALS(indices.size(), 3);
    TS_ASSERT_EQUALS(indices[0].first, 0);                                   // ONE
    TS_ASSERT_EQUALS(indices[0].second, 1);                                  // TWO
    TS_ASSERT_EQUALS(indices[1].first, 2);                                   // THREE
    TS_ASSERT_EQUALS(indices[1].second, 4);                                  // FOUR
    TS_ASSERT_EQUALS(indices[2].first, 4);                                   // FIVE
    TS_ASSERT_EQUALS(indices[2].second, std::numeric_limits<size_t>::max()); // SIX
  }
};
