// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/SplittingInterval.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::FilteredTimeSeriesProperty;
using Mantid::Types::Core::DateAndTime;

class FilteredTimeSeriesPropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilteredTimeSeriesPropertyTest *createSuite() { return new FilteredTimeSeriesPropertyTest(); }
  static void destroySuite(FilteredTimeSeriesPropertyTest *suite) { delete suite; }

  void test_FilteredProperty_Has_Same_Name_As_Original() {
    using Mantid::Kernel::TimeSeriesProperty;
    const std::string name = "seriesName";
    auto source = createTestSeries(name);
    auto filter = createTestFilter();

    auto filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(std::move(source), filter);
    TS_ASSERT_EQUALS(filtered->name(), name);
  }

  void test_Transferring_Ownership_Makes_Unfiltered_Property_Return_The_Original() {
    constexpr bool transferOwnership(true);
    doOwnershipTest(transferOwnership);
  }

  void test_Retaining_Ownership_With_Caller_Makes_Unfiltered_Property_A_Clone() {
    constexpr bool transferOwnership(false);
    doOwnershipTest(transferOwnership);
  }

  void test_Construction_Yields_A_Filtered_Property_When_Accessing_Through_The_Filtered_Object() {
    auto source = createTestSeries("name");
    auto filter = createTestFilter();

    auto filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(std::move(source), filter);

    TS_ASSERT_EQUALS(filtered->size(), 2);
    TS_ASSERT_EQUALS(filtered->nthInterval(0).begin_str(), "2007-Nov-30 16:17:25");
    TS_ASSERT_EQUALS(filtered->nthInterval(0).end_str(), "2007-Nov-30 16:17:30");
    TS_ASSERT_EQUALS(filtered->nthValue(0), 3);

    TS_ASSERT_EQUALS(filtered->nthInterval(1).begin_str(), "2007-Nov-30 16:17:30");
    TS_ASSERT_EQUALS(filtered->nthInterval(1).end_str(), "2007-Nov-30 16:17:39");
    TS_ASSERT_EQUALS(filtered->nthValue(1), 4);
  }

  // Create a small TSP<int>. Callee owns the returned object.
  FilteredTimeSeriesProperty<int> *createIntegerTSP(int numberOfValues) {
    FilteredTimeSeriesProperty<int> *log = new FilteredTimeSeriesProperty<int>("intProp");
    DateAndTime startTime("2007-11-30T16:17:00");
    for (int value = 0; value < numberOfValues; ++value) {
      DateAndTime time = startTime + value * 10.0;
      TS_ASSERT_THROWS_NOTHING(log->addValue(time, value + 1));
    }
    return log;
  }

  void test_ComparisonOperator() {
    // Setup two logs and two filters so that logs have different sizes but are
    // the same size after applying the filter

    FilteredTimeSeriesProperty<int> *log1 = new FilteredTimeSeriesProperty<int>("count_rate");
    log1->addValue("2016-03-17T00:00:00", 1);
    log1->addValue("2016-03-17T00:30:00", 2);
    log1->addValue("2016-03-17T01:00:00", 3);
    log1->addValue("2016-03-17T01:30:00", 4);
    log1->addValue("2016-03-17T02:00:00", 5);
    FilteredTimeSeriesProperty<bool> *filter1 = new FilteredTimeSeriesProperty<bool>("filter");
    filter1->addValue("2016-Mar-17 00:00:00", 1);
    filter1->addValue("2016-Mar-17 01:00:00", 0);
    log1->filterWith(filter1);

    FilteredTimeSeriesProperty<int> *log2 = new FilteredTimeSeriesProperty<int>("count_rate");
    log2->addValue("2016-03-17T03:00:00", 1);
    log2->addValue("2016-03-17T04:00:00", 2);
    log2->addValue("2016-03-17T05:00:00", 3);
    log2->addValue("2016-03-17T06:00:0", 4);
    FilteredTimeSeriesProperty<bool> *filter2 = new FilteredTimeSeriesProperty<bool>("filter");
    filter2->addValue("2016-Mar-17 03:00:00", 1);
    filter2->addValue("2016-Mar-17 05:00:00", 0);
    log2->filterWith(filter2);

    TS_ASSERT(!(*log1 == *log2));

    delete log1;
    delete log2;
    delete filter1;
    delete filter2;
  }

  /*
   * Test filterWith() and clear filter
   */
  void test_filter() {
    // 1. Create a base property
    Mantid::Types::Core::DateAndTime tStart("2007-11-30T16:17:00");
    std::vector<double> deltaTs;
    std::vector<double> valueXs;
    for (int i = 0; i < 20; i++) {
      deltaTs.emplace_back(static_cast<double>(i) * 10.0);
      valueXs.emplace_back(static_cast<double>(i) + 1.0);
    }
    FilteredTimeSeriesProperty<double> *p1 = new FilteredTimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // b) Copy size and interval information in order to verify clearFilter()
    size_t origsize = p1->size();
    std::vector<Mantid::Kernel::TimeInterval> dts;

    for (size_t i = 0; i < origsize; i++) {
      dts.emplace_back(p1->nthInterval(static_cast<int>(i)));
    }

    // 2. Create a filter
    FilteredTimeSeriesProperty<bool> *filter = new FilteredTimeSeriesProperty<bool>("Filter");
    filter->addValue("2007-11-30T16:17:06", true);
    filter->addValue("2007-11-30T16:17:16", false);
    filter->addValue("2007-11-30T16:18:40", true);
    filter->addValue("2007-11-30T16:19:30", false);

    p1->filterWith(filter);

    // 4. Formal check (1) Size  (2) Number of Interval
    p1->countSize();
    TS_ASSERT_EQUALS(p1->size(), 7);

    Mantid::Kernel::TimeInterval dt1 = p1->nthInterval(1);
    TS_ASSERT_EQUALS(dt1.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(dt1.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));

    Mantid::Kernel::TimeInterval dt2 = p1->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:18:40"));
    TS_ASSERT_EQUALS(dt2.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:18:50"));

    // 4. Clear filter
    p1->clearFilter();
    p1->countSize();

    size_t finalsize = p1->size();
    TS_ASSERT_EQUALS(finalsize, origsize);

    if (finalsize == origsize) {
      for (size_t i = 0; i < finalsize; i++) {
        Mantid::Kernel::TimeInterval dt = p1->nthInterval(static_cast<int>(i));
        TS_ASSERT_EQUALS(dt.begin(), dts[i].begin());
        TS_ASSERT_EQUALS(dt.end(), dts[i].end());
      }
    }

    // -1. Clean
    delete p1;
    delete filter;

    return;
  }

  void test_filter_with_single_value_in_series() {
    auto p1 = std::make_shared<FilteredTimeSeriesProperty<double>>("SingleValueTSP");
    p1->addValue("2007-11-30T16:17:00", 1.5);

    auto filterEndsBefore = std::make_shared<FilteredTimeSeriesProperty<bool>>("EndsBefore");
    filterEndsBefore->addValue("2007-11-30T16:16:30", false);
    filterEndsBefore->addValue("2007-11-30T16:16:58", true);
    p1->filterWith(filterEndsBefore.get());
    TS_ASSERT_EQUALS(1, p1->size());

    p1->clearFilter();
    auto filterEndsAfter = std::make_shared<FilteredTimeSeriesProperty<bool>>("EndsAfter");
    filterEndsAfter->addValue("2007-11-30T16:16:30", false);
    filterEndsAfter->addValue("2007-11-30T16:17:01", true);
    p1->filterWith(filterEndsAfter.get());
    TS_ASSERT_EQUALS(1, p1->size());
  }

  /*
   * Test filterWith() on different boundary conditions
   * Filter_T0 < Log_T0 < LogTf < Filter_Tf, T... F... T... F...
   * Log will be extended to Filter_T0
   */
  void test_filterBoundary1() {
    // 1. Create a base property
    Mantid::Types::Core::DateAndTime tStart("2007-11-30T16:17:00");
    std::vector<double> deltaTs;
    std::vector<double> valueXs;
    for (int i = 0; i < 20; i++) {
      deltaTs.emplace_back(static_cast<double>(i) * 10.0);
      valueXs.emplace_back(static_cast<double>(i) + 1.0);
    }
    FilteredTimeSeriesProperty<double> *p1 = new FilteredTimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    FilteredTimeSeriesProperty<bool> *filter = new FilteredTimeSeriesProperty<bool>("Filter");
    filter->addValue("2007-11-30T16:16:06", true);
    filter->addValue("2007-11-30T16:17:16", false);
    filter->addValue("2007-11-30T16:18:40", true);
    filter->addValue("2007-11-30T17:19:30", false);

    p1->filterWith(filter);

    // 3. Check size
    p1->countSize();
    TS_ASSERT_EQUALS(p1->size(), 12);

    // 4. Check interval & Value
    Mantid::Kernel::TimeInterval dt0 = p1->nthInterval(0);
    TS_ASSERT_EQUALS(dt0.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(dt0.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    double v0 = p1->nthValue(0);
    TS_ASSERT_DELTA(v0, 1, 0.00000001);

    Mantid::Kernel::TimeInterval dt1 = p1->nthInterval(1);
    TS_ASSERT_EQUALS(dt1.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(dt1.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    double v1 = p1->nthValue(1);
    TS_ASSERT_DELTA(v1, 2, 0.00000001);

    Mantid::Kernel::TimeInterval dt2 = p1->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:18:40"));
    TS_ASSERT_EQUALS(dt2.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:18:50"));
    double v2 = p1->nthValue(2);
    TS_ASSERT_DELTA(v2, 11, 0.00000001);

    Mantid::Kernel::TimeInterval dt12 = p1->nthInterval(11);
    TS_ASSERT_EQUALS(dt12.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:20:10"));
    TS_ASSERT_EQUALS(dt12.end(), Mantid::Types::Core::DateAndTime("2007-11-30T17:19:30"));
    double v12 = p1->nthValue(11);
    TS_ASSERT_DELTA(v12, 20, 1.0E-8);

    // 5. Clear filter
    p1->clearFilter();

    // -1. Clean
    delete p1;
    delete filter;

    return;
  }

  /*
   * Test filterWith() on different boundary conditions
   * Filter_T0 < Log_T0 < LogTf < Filter_Tf, F... T... F... T... F...
   */
  void test_filterBoundary2() {
    // 1. Create a base property
    Mantid::Types::Core::DateAndTime tStart("2007-11-30T16:17:00");
    std::vector<double> deltaTs;
    std::vector<double> valueXs;
    for (int i = 0; i < 20; i++) {
      deltaTs.emplace_back(static_cast<double>(i) * 10.0);
      valueXs.emplace_back(static_cast<double>(i) + 1.0);
    }
    FilteredTimeSeriesProperty<double> *p1 = new FilteredTimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    FilteredTimeSeriesProperty<bool> *filter = new FilteredTimeSeriesProperty<bool>("Filter");
    filter->addValue("2007-11-30T16:16:06", false);
    filter->addValue("2007-11-30T16:17:16", true);
    filter->addValue("2007-11-30T16:18:40", false);
    filter->addValue("2007-11-30T17:19:30", true);

    p1->filterWith(filter);

    // 3. Check size
    p1->countSize();
    TS_ASSERT_EQUALS(p1->size(), 10);

    // 4. Check interval
    Mantid::Kernel::TimeInterval dt0 = p1->nthInterval(0);
    TS_ASSERT_EQUALS(dt0.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    TS_ASSERT_EQUALS(dt0.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    double v0 = p1->nthValue(0);
    TS_ASSERT_DELTA(v0, 2, 1.0E-8);

    // 5. Clear filter
    p1->clearFilter();

    // -1. Clean
    delete p1;
    delete filter;

    return;
  }

  /*
   * Test filterWith() on different boundary conditions
   * Log_T0 < Filter_T0 <  < Filter_Tf  LogTf, T... F... T... F...
   */
  void test_filterBoundary3() {
    // 1. Create a base property
    Mantid::Types::Core::DateAndTime tStart("2007-11-30T16:17:00");
    std::vector<double> deltaTs;
    std::vector<double> valueXs;
    for (int i = 0; i < 20; i++) {
      deltaTs.emplace_back(static_cast<double>(i) * 10.0);
      valueXs.emplace_back(static_cast<double>(i) + 1.0);
    }
    FilteredTimeSeriesProperty<double> *p1 = new FilteredTimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    FilteredTimeSeriesProperty<bool> *filter = new FilteredTimeSeriesProperty<bool>("Filter");
    filter->addValue("2007-11-30T16:17:06", true);
    filter->addValue("2007-11-30T16:17:16", false);
    filter->addValue("2007-11-30T16:18:40", true);
    filter->addValue("2007-11-30T16:19:30", false);

    p1->filterWith(filter);

    // 3. Check size
    p1->countSize();
    TS_ASSERT_EQUALS(p1->size(), 7);

    // 4. Check interval
    Mantid::Kernel::TimeInterval dt1 = p1->nthInterval(1);
    TS_ASSERT_EQUALS(dt1.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(dt1.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    double v1 = p1->nthValue(1);
    TS_ASSERT_DELTA(v1, 2, 1.0E-8);

    Mantid::Kernel::TimeInterval dt2 = p1->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:18:40"));
    TS_ASSERT_EQUALS(dt2.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:18:50"));
    double v2 = p1->nthValue(2);
    TS_ASSERT_DELTA(v2, 11, 1.0E-8);

    // 5. Clear filter
    p1->clearFilter();

    // -1. Clean
    delete p1;
    delete filter;

    return;
  }

  /*
   * Test filterWith() on different boundary conditions
   * Log_T0 < Filter_T0 <  < Filter_Tf  LogTf,  F... T... F... T... F...
   */

  void test_filterBoundary4() {
    // 1. Create a base property
    Mantid::Types::Core::DateAndTime tStart("2007-11-30T16:17:00");
    std::vector<double> deltaTs;
    std::vector<double> valueXs;
    for (int i = 0; i < 20; i++) {
      deltaTs.emplace_back(static_cast<double>(i) * 10.0);
      valueXs.emplace_back(static_cast<double>(i) + 1.0);
    }
    FilteredTimeSeriesProperty<double> *p1 = new FilteredTimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    FilteredTimeSeriesProperty<bool> *filter = new FilteredTimeSeriesProperty<bool>("Filter");
    filter->addValue("2007-11-30T16:17:06", false);
    filter->addValue("2007-11-30T16:17:16", true);
    filter->addValue("2007-11-30T16:18:40", false);
    filter->addValue("2007-11-30T16:19:30", true);

    p1->filterWith(filter);

    // 3. Check size
    p1->countSize();
    TS_ASSERT_EQUALS(p1->size(), 14);

    // 4. Check interval
    Mantid::Kernel::TimeInterval dt0 = p1->nthInterval(0);
    TS_ASSERT_EQUALS(dt0.begin(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    TS_ASSERT_EQUALS(dt0.end(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    double v0 = p1->nthValue(0);
    TS_ASSERT_DELTA(v0, 2, 1.0E-8);

    // 5. Clear filter
    p1->clearFilter();

    // -1. Clean
    delete p1;
    delete filter;

    return;
  }

  void test_filter_by_first_value() {
    FilteredTimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", expectedFilteredValue);
    series.addValue("2000-11-30T01:01:02", 2);

    const double actualFilteredValue = filterByStatistic(&series, Mantid::Kernel::Math::FirstValue);
    TSM_ASSERT_EQUALS("Filtering by FirstValue is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_last_value() {
    FilteredTimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue);

    const double actualFilteredValue = filterByStatistic(&series, Mantid::Kernel::Math::LastValue);
    TSM_ASSERT_EQUALS("Filtering by LastValue is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_minimum_value() {
    FilteredTimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 3);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // minimum. 1 < 3 < 4
    series.addValue("2000-11-30T01:01:03", 4);

    const double actualFilteredValue = filterByStatistic(&series, Mantid::Kernel::Math::Minimum);
    TSM_ASSERT_EQUALS("Filtering by Minimum is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_maximum_value() {
    FilteredTimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0.1);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // maximum. 1 > 0.9 > 0.1
    series.addValue("2000-11-30T01:01:03", 0.9);

    const double actualFilteredValue = filterByStatistic(&series, Mantid::Kernel::Math::Maximum);
    TSM_ASSERT_EQUALS("Filtering by Maximum is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_mean_value() {
    FilteredTimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // time series mean = value at T =
                                            // (T1 + T2 + T3) / 3
    series.addValue("2000-11-30T01:01:03", 2);

    const double actualFilteredValue = filterByStatistic(&series, Mantid::Kernel::Math::Mean);
    TSM_ASSERT_EQUALS("Filtering by Mean Time is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_median() {
    FilteredTimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 2;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", 1);
    series.addValue("2000-11-30T01:01:03",
                    expectedFilteredValue); // Median time.
    series.addValue("2000-11-30T01:01:04", 4);
    series.addValue("2000-11-30T01:02:00", 5);

    const double actualFilteredValue = filterByStatistic(&series, Mantid::Kernel::Math::Median);
    TSM_ASSERT_EQUALS("Filtering by Median Time is not working.", expectedFilteredValue, actualFilteredValue);
  }

  //----------------------------------------------------------------------------

  /** A test for filter nothing
   */
  void test_filterByTime_out_of_range_filters_nothing() {
    FilteredTimeSeriesProperty<int> *log = createIntegerTSP(6);

    size_t original_size = log->realSize();

    TS_ASSERT_EQUALS(original_size, 6);

    DateAndTime start = DateAndTime("2007-11-30T15:00:00"); // Much earlier than first time series value
    DateAndTime stop = DateAndTime("2007-11-30T17:00:00");  // Much later than last time series value

    log->filterByTime(start, stop);

    TSM_ASSERT_EQUALS("Shouldn't be filtering anything!", original_size, log->realSize());

    delete log;
  }

  /// Test that getStatistics respects the filter
  void test_getStatistics_filtered() {
    const auto &log = getFilteredTestLog();

    // Get the stats and compare to expected values
    const auto &stats = log->getStatistics();
    TS_ASSERT_DELTA(stats.minimum, 1.0, 1e-6);
    TS_ASSERT_DELTA(stats.maximum, 10.0, 1e-6);
    TS_ASSERT_DELTA(stats.median, 6.0, 1e-6);
    TS_ASSERT_DELTA(stats.mean, 5.77778, 1e-3);
    TS_ASSERT_DELTA(stats.duration, 85.0, 1e-6);
    TS_ASSERT_DELTA(stats.standard_deviation, 2.8974, 1e-4);
  }

  /// Test that timeAverageValue respects the filter
  void test_timeAverageValue_filtered() {
    const auto &log = getFilteredTestLog();
    TS_ASSERT_DELTA(log->timeAverageValue(), 5.588, 1e-3);
  }

  void test_filteredValuesAsVector() {
    const auto &log = getFilteredTestLog();

    const auto &unfilteredValues = log->valuesAsVector();
    const auto &filteredValues = log->filteredValuesAsVector();

    TS_ASSERT_DIFFERS(unfilteredValues.size(), filteredValues.size());
    TS_ASSERT_EQUALS(unfilteredValues.size(), 11);
    TS_ASSERT_EQUALS(filteredValues.size(), 9);
  }

  void test_getSplittingIntervals_repeatedEntries() {
    const auto &log = getTestLog();
    // Add the filter
    auto filter = std::make_unique<FilteredTimeSeriesProperty<bool>>("Filter");
    Mantid::Types::Core::DateAndTime firstStart("2007-11-30T16:17:00"), firstEnd("2007-11-30T16:17:15"),
        secondStart("2007-11-30T16:18:35"), secondEnd("2007-11-30T16:18:40");
    filter->addValue(firstStart.toISO8601String(), true);
    filter->addValue(firstEnd.toISO8601String(), false);
    filter->addValue("2007-11-30T16:17:25", false);
    filter->addValue(secondStart.toISO8601String(), true);
    filter->addValue("2007-11-30T16:18:38", true);
    filter->addValue(secondEnd.toISO8601String(), false);
    log->filterWith(filter.get());
    const auto &intervals = log->getSplittingIntervals();
    TS_ASSERT_EQUALS(intervals.size(), 2);
    if (intervals.size() == 2) {
      const auto &firstRange = intervals.front(), &secondRange = intervals.back();
      TS_ASSERT_EQUALS(firstRange.start(), firstStart);
      TS_ASSERT_EQUALS(firstRange.stop(), firstEnd);
      TS_ASSERT_EQUALS(secondRange.start(), secondStart);
      TS_ASSERT_EQUALS(secondRange.stop(), secondEnd);
    }
  }

  void test_getSplittingIntervals_startEndTimes() {
    const auto &log = getTestLog();
    // Add the filter
    auto filter = std::make_unique<FilteredTimeSeriesProperty<bool>>("Filter");
    Mantid::Types::Core::DateAndTime firstEnd("2007-11-30T16:17:05"), secondStart("2007-11-30T16:17:10"),
        secondEnd("2007-11-30T16:17:15"), thirdStart("2007-11-30T16:18:35");
    filter->addValue(log->firstTime(), true);
    filter->addValue(firstEnd.toISO8601String(), false);
    filter->addValue(secondStart.toISO8601String(), true);
    filter->addValue(secondEnd.toISO8601String(), false);
    filter->addValue(thirdStart.toISO8601String(), true);
    log->filterWith(filter.get());
    const auto &intervals = log->getSplittingIntervals();
    TS_ASSERT_EQUALS(intervals.size(), 3);
    if (intervals.size() == 3) {
      TS_ASSERT_EQUALS(intervals[0].start(), log->firstTime());
      TS_ASSERT_EQUALS(intervals[0].stop(), firstEnd);
      TS_ASSERT_EQUALS(intervals[1].start(), secondStart);
      TS_ASSERT_EQUALS(intervals[1].stop(), secondEnd);
      TS_ASSERT_EQUALS(intervals[2].start(), thirdStart);
      TS_ASSERT(intervals[2].stop() > thirdStart);
    }
  }

private:
  void doOwnershipTest(const bool transferOwnership) {
    auto source = createTestSeries("name");
    auto filter = createTestFilter();

    std::unique_ptr<FilteredTimeSeriesProperty<double>> filtered = nullptr;

    // Pointer comparison
    if (transferOwnership) {
      auto copy = std::unique_ptr<Mantid::Kernel::TimeSeriesProperty<double>>(source->clone());
      TS_ASSERT_THROWS_NOTHING(filtered =
                                   std::make_unique<FilteredTimeSeriesProperty<double>>(std::move(source), filter));
      source = std::move(copy);
    } else {
      TS_ASSERT_THROWS_NOTHING(filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(source.get(), filter));
      TS_ASSERT_DIFFERS(source.get(), filtered->unfiltered());
    }

    // Object equality
    TS_ASSERT_EQUALS(*source,
                     *(filtered->unfiltered())); // Objects are considered equal
  }

  /// Create the test source property
  std::unique_ptr<Mantid::Kernel::TimeSeriesProperty<double>> createTestSeries(const std::string &name) {
    auto source = std::make_unique<Mantid::Kernel::TimeSeriesProperty<double>>(name);
    source->addValue("2007-11-30T16:17:00", 1);
    source->addValue("2007-11-30T16:17:10", 2);
    source->addValue("2007-11-30T16:17:20", 3);
    source->addValue("2007-11-30T16:17:30", 4);
    source->addValue("2007-11-30T16:17:40", 5);
    return source;
  }

  /// Create test filter
  Mantid::Kernel::TimeSeriesProperty<bool> createTestFilter() {
    auto filter = Mantid::Kernel::TimeSeriesProperty<bool>("filter");
    filter.addValue("2007-11-30T16:16:50", false);
    filter.addValue("2007-11-30T16:17:25", true);
    filter.addValue("2007-11-30T16:17:39", false);
    return filter;
  }

  /// Generate a test log
  std::unique_ptr<FilteredTimeSeriesProperty<double>> getTestLog() {
    // Build the log
    auto log = std::make_unique<FilteredTimeSeriesProperty<double>>("DoubleLog");
    Mantid::Types::Core::DateAndTime logTime("2007-11-30T16:17:00");
    const double incrementSecs(10.0);
    for (int i = 1; i < 12; ++i) {
      const double val = static_cast<double>(i);
      log->addValue(logTime.toISO8601String(), val);
      logTime += incrementSecs;
    }
    return log;
  }

  /// Generate a test log that has been filtered
  std::unique_ptr<FilteredTimeSeriesProperty<double>> getFilteredTestLog() {
    // Build the log
    auto log = getTestLog();
    // Add the filter
    auto filter = std::make_unique<FilteredTimeSeriesProperty<bool>>("Filter");
    filter->addValue("2007-11-30T16:17:00", true);
    filter->addValue("2007-11-30T16:17:15", false);
    filter->addValue("2007-11-30T16:17:25", true);
    filter->addValue("2007-11-30T16:18:35", false);
    log->filterWith(filter.get());
    return log;
  }
};
