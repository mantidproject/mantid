// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/LogFilter.h"
#include <ctime>

using namespace Mantid::Kernel;

class LogFilterTest : public CxxTest::TestSuite {
  FilteredTimeSeriesProperty<double> *p;

public:
  static LogFilterTest *createSuite() { return new LogFilterTest(); }
  static void destroySuite(LogFilterTest *suite) { delete suite; }

  LogFilterTest() : p(new FilteredTimeSeriesProperty<double>("test")) {
    p->addValue("2007-11-30T16:17:00", 1);
    p->addValue("2007-11-30T16:17:10", 2);
    p->addValue("2007-11-30T16:17:20", 3);
    p->addValue("2007-11-30T16:17:30", 4);
    p->addValue("2007-11-30T16:17:40", 5);
  }

  ~LogFilterTest() override { delete p; }

  void testnthValue() {

    TS_ASSERT_EQUALS(p->size(), 5);
    TS_ASSERT_EQUALS(p->nthValue(0), 1);
    TS_ASSERT_EQUALS(p->nthValue(1), 2);
    TS_ASSERT_EQUALS(p->nthValue(2), 3);
    TS_ASSERT_EQUALS(p->nthValue(3), 4);
    TS_ASSERT_EQUALS(p->nthValue(4), 5);
    TS_ASSERT_EQUALS(p->nthValue(5), 5);

    TS_ASSERT_EQUALS(p->nthInterval(0).begin_str(), "2007-Nov-30 16:17:00");
    TS_ASSERT_EQUALS(p->nthInterval(0).end_str(), "2007-Nov-30 16:17:10");

    TS_ASSERT_EQUALS(p->nthInterval(1).begin_str(), "2007-Nov-30 16:17:10");
    TS_ASSERT_EQUALS(p->nthInterval(1).end_str(), "2007-Nov-30 16:17:20");

    TS_ASSERT_EQUALS(p->nthInterval(2).begin_str(), "2007-Nov-30 16:17:20");
    TS_ASSERT_EQUALS(p->nthInterval(2).end_str(), "2007-Nov-30 16:17:30");

    TS_ASSERT_EQUALS(p->nthInterval(3).begin_str(), "2007-Nov-30 16:17:30");
    TS_ASSERT_EQUALS(p->nthInterval(3).end_str(), "2007-Nov-30 16:17:40");

    TS_ASSERT_EQUALS(p->nthInterval(4).begin_str(), "2007-Nov-30 16:17:40");
    TS_ASSERT_EQUALS(p->nthInterval(4).end_str(),
                     "2007-Nov-30 16:17:50"); // nth interval changed to use
                                              // previous interval now.

    // test out of bounds check given filter applied
    auto testFilter = createTestFilter(1);
    LogFilter flt(p);
    flt.addFilter(*testFilter);
    TS_ASSERT_THROWS(flt.data()->nthInterval(5), const std::runtime_error &);
  }

  void testFilterWithTrueAtStart() {
    auto testFilter = createTestFilter(1);

    LogFilter flt(p);
    flt.addFilter(*testFilter);

    TS_ASSERT_EQUALS(flt.data()->size(), 5);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:00");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:10");
    TS_ASSERT_EQUALS(flt.data()->nthValue(0), 1);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:10");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:17:20");
    TS_ASSERT_EQUALS(flt.data()->nthValue(1), 2);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(2).begin_str(), "2007-Nov-30 16:17:20");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(2).end_str(), "2007-Nov-30 16:17:25");
    TS_ASSERT_EQUALS(flt.data()->nthValue(2), 3);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(3).begin_str(), "2007-Nov-30 16:17:39");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(3).end_str(), "2007-Nov-30 16:17:40");
    TS_ASSERT_EQUALS(flt.data()->nthValue(3), 4);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(4).begin_str(), "2007-Nov-30 16:17:40");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(4).end_str(), "2007-Nov-30 16:17:41");
    TS_ASSERT_EQUALS(flt.data()->nthValue(4), 5);
  }

  void testFilterWithFalseAtStart() {
    auto testFilter = createTestFilter(2);

    LogFilter flt(p);
    flt.addFilter(*testFilter);

    TS_ASSERT_EQUALS(flt.data()->size(), 2);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:25");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:30");
    TS_ASSERT_EQUALS(flt.data()->nthValue(0), 3);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:30");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:17:39");
    TS_ASSERT_EQUALS(flt.data()->nthValue(1), 4);
  }

  /*
   * this is a test for two filters doing "AND" operation according to previous
   * unit test.
   */
  void test_Filters_Combine_Using_AND_Operation() {
    auto testFilter1 = createTestFilter(1);
    auto testFilter2 = createTestFilter(3);

    LogFilter flt(p);
    flt.addFilter(*testFilter1);
    flt.addFilter(*testFilter2);

    TS_ASSERT_EQUALS(flt.data()->size(), 5);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:00");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:05");
    TS_ASSERT_EQUALS(flt.data()->nthValue(0), 1);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:12");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:17:20");
    TS_ASSERT_EQUALS(flt.data()->nthValue(1), 2);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(2).begin_str(), "2007-Nov-30 16:17:20");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(2).end_str(), "2007-Nov-30 16:17:25");
    TS_ASSERT_EQUALS(flt.data()->nthValue(2), 3);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(3).begin_str(), "2007-Nov-30 16:17:39");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(3).end_str(), "2007-Nov-30 16:17:40");
    TS_ASSERT_EQUALS(flt.data()->nthValue(3), 4);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(4).begin_str(), "2007-Nov-30 16:17:40");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(4).end_str(), "2007-Nov-30 16:17:41");
    TS_ASSERT_EQUALS(flt.data()->nthValue(4), 5);
  }

  void test_LogFilter_Can_Combine_Filters_When_No_Property_Is_Set() {
    auto mask1 = createTestFilter(1);
    auto mask2 = createTestFilter(3);
    LogFilter filterer(*mask1);
    filterer.addFilter(*mask2);
    const TimeSeriesProperty<bool> *finalFilter = filterer.filter();

    TSM_ASSERT("Filter is NULL", finalFilter);
    if (!finalFilter)
      return;

    TS_ASSERT_EQUALS(5, finalFilter->size());

    TS_ASSERT_EQUALS(finalFilter->nthInterval(0).begin_str(), "2007-Nov-30 16:17:00");
    TS_ASSERT_EQUALS(finalFilter->nthInterval(0).end_str(), "2007-Nov-30 16:17:05");
    TS_ASSERT_EQUALS(finalFilter->nthValue(0), true);

    TS_ASSERT_EQUALS(finalFilter->nthInterval(1).begin_str(), "2007-Nov-30 16:17:05");
    TS_ASSERT_EQUALS(finalFilter->nthInterval(1).end_str(), "2007-Nov-30 16:17:12");
    TS_ASSERT_EQUALS(finalFilter->nthValue(1), false);

    TS_ASSERT_EQUALS(finalFilter->nthInterval(2).begin_str(), "2007-Nov-30 16:17:12");
    TS_ASSERT_EQUALS(finalFilter->nthInterval(2).end_str(), "2007-Nov-30 16:17:25");
    TS_ASSERT_EQUALS(finalFilter->nthValue(2), true);

    TS_ASSERT_EQUALS(finalFilter->nthInterval(3).begin_str(), "2007-Nov-30 16:17:25");
    TS_ASSERT_EQUALS(finalFilter->nthInterval(3).end_str(), "2007-Nov-30 16:17:39");
    TS_ASSERT_EQUALS(finalFilter->nthValue(3), false);

    TS_ASSERT_EQUALS(finalFilter->nthInterval(4).begin_str(), "2007-Nov-30 16:17:39");
    TS_ASSERT_EQUALS(finalFilter->nthInterval(4).end_str(), "2007-Nov-30 16:17:53");
    TS_ASSERT_EQUALS(finalFilter->nthValue(4), true);
  }

  void test_filtered_size_when_combined_filter_is_invalid() {
    FilteredTimeSeriesProperty<double> *p = new FilteredTimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2009-Apr-28 09:20:52", -0.00161));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2009-Apr-28 09:21:57", -0.00161));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2009-Apr-28 09:23:01", -0.00161));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2009-Apr-28 09:25:10", -0.00161));

    FilteredTimeSeriesProperty<bool> *running = new FilteredTimeSeriesProperty<bool>("running");
    running->addValue("2009-Apr-28 09:20:30", true);
    running->addValue("2009-Apr-28 09:20:51", false);

    // goes from before the previous one through all time
    // the intersection of the two is "simply" the previous filter
    FilteredTimeSeriesProperty<bool> *period = new FilteredTimeSeriesProperty<bool>("period 1");
    period->addValue("2009-Apr-28 09:20:29", true);

    LogFilter filter(p);
    filter.addFilter(*running);
    filter.addFilter(*period);
    TS_ASSERT_EQUALS(filter.data()->size(), 1);
    delete p;
    delete period;
    delete running;
  }

  void testF3() {
    auto testFilter = createTestFilter(4);

    LogFilter flt(p);
    flt.addFilter(*testFilter);

    TS_ASSERT_EQUALS(flt.data()->size(), 2);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).begin_str(), "2007-Nov-30 16:17:40");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(0).end_str(), "2007-Nov-30 16:17:45");
    TS_ASSERT_EQUALS(flt.data()->nthValue(0), 5);

    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).begin_str(), "2007-Nov-30 16:17:50");
    TS_ASSERT_EQUALS(flt.data()->nthInterval(1).end_str(), "2007-Nov-30 16:18:00");
    TS_ASSERT_EQUALS(flt.data()->nthValue(1), 5);

    return;
  }

  void testFilterByPeriod() // Test for Wendou to look at
  {
    FilteredTimeSeriesProperty<double> height_log("height_log");
    height_log.addValue("2008-Jun-17 11:10:44", -0.86526);
    height_log.addValue("2008-Jun-17 11:10:45", -1.17843);
    height_log.addValue("2008-Jun-17 11:10:47", -1.27995);
    height_log.addValue("2008-Jun-17 11:20:15", -1.38216);
    height_log.addValue("2008-Jun-17 11:20:16", -1.87435);
    height_log.addValue("2008-Jun-17 11:20:17", -2.70547);
    height_log.addValue("2008-Jun-17 11:20:19", -2.99125);
    height_log.addValue("2008-Jun-17 11:20:20", -3);
    height_log.addValue("2008-Jun-17 11:20:27", -2.98519);
    height_log.addValue("2008-Jun-17 11:20:29", -2.68904);
    height_log.addValue("2008-Jun-17 11:20:30", -2.5);
    height_log.addValue("2008-Jun-17 11:20:38", -2.45909);
    height_log.addValue("2008-Jun-17 11:20:39", -2.08764);
    height_log.addValue("2008-Jun-17 11:20:40", -2);
    height_log.addValue("2008-Jun-17 11:20:50", -1.85174);
    height_log.addValue("2008-Jun-17 11:20:51", -1.51258);
    height_log.addValue("2008-Jun-17 11:20:52", -1.5);
    height_log.addValue("2008-Jun-17 11:21:01", -1.48566);
    height_log.addValue("2008-Jun-17 11:21:02", -1.18799);
    height_log.addValue("2008-Jun-17 11:21:04", -1);
    height_log.addValue("2008-Jun-17 11:21:11", -0.98799);
    height_log.addValue("2008-Jun-17 11:21:13", -0.63694);
    height_log.addValue("2008-Jun-17 11:21:14", -0.5);
    height_log.addValue("2008-Jun-17 11:21:23", -0.46247);
    height_log.addValue("2008-Jun-17 11:21:24", -0.08519);
    height_log.addValue("2008-Jun-17 11:21:25", 0);

    FilteredTimeSeriesProperty<bool> period_log("period 7");
    period_log.addValue("2008-Jun-17 11:11:13", false);
    period_log.addValue("2008-Jun-17 11:11:13", false);
    period_log.addValue("2008-Jun-17 11:11:18", false);
    period_log.addValue("2008-Jun-17 11:11:30", false);
    period_log.addValue("2008-Jun-17 11:11:42", false);
    period_log.addValue("2008-Jun-17 11:11:52", false);
    period_log.addValue("2008-Jun-17 11:12:01", false);
    period_log.addValue("2008-Jun-17 11:12:11", false);
    period_log.addValue("2008-Jun-17 11:12:21", true); //
    period_log.addValue("2008-Jun-17 11:12:32", false);
    period_log.addValue("2008-Jun-17 11:12:42", false);
    period_log.addValue("2008-Jun-17 11:12:52", false);
    period_log.addValue("2008-Jun-17 11:13:02", false);
    period_log.addValue("2008-Jun-17 11:16:55", false);
    period_log.addValue("2008-Jun-17 11:17:00", false);
    period_log.addValue("2008-Jun-17 11:17:16", false);
    period_log.addValue("2008-Jun-17 11:17:28", false);
    period_log.addValue("2008-Jun-17 11:17:37", false);
    period_log.addValue("2008-Jun-17 11:17:48", false);
    period_log.addValue("2008-Jun-17 11:17:57", false);
    period_log.addValue("2008-Jun-17 11:18:07", true); //
    period_log.addValue("2008-Jun-17 11:18:18", false);
    period_log.addValue("2008-Jun-17 11:18:28", false);
    period_log.addValue("2008-Jun-17 11:18:38", false);
    period_log.addValue("2008-Jun-17 11:18:48", false);
    period_log.addValue("2008-Jun-17 11:20:07", false);
    period_log.addValue("2008-Jun-17 11:20:11", false);
    period_log.addValue("2008-Jun-17 11:20:24", false);
    period_log.addValue("2008-Jun-17 11:20:34", false);
    period_log.addValue("2008-Jun-17 11:20:46", false);
    period_log.addValue("2008-Jun-17 11:20:58", false);
    period_log.addValue("2008-Jun-17 11:21:08", false);
    period_log.addValue("2008-Jun-17 11:21:19", true); //

    TS_ASSERT_EQUALS(height_log.size(), 26);

    LogFilter filter(&height_log);
    filter.addFilter(period_log);
    const FilteredTimeSeriesProperty<double> *filteredLog = filter.data();
    TS_ASSERT_EQUALS(filteredLog->size(), 6)
  }

  /**
   * This is based on the usage scenario in
   * docs/source/tutorials/python_in_mantid/further_alg_ws/04_run_logs.rst
   */
  void test_usage_docs() {
    Mantid::DateAndTime EPOCH("1990-Jan-01 00:00:00");
    const std::size_t NUM_VALUES{100};
    constexpr double FREQUENCY{2 * M_PI / 100.};

    // integral of sine from zero to pi is 2
    // divide by that range is 2/pi
    constexpr double MEAN_EXP{2 / M_PI};

    // Times start at GPS_EPOCH and are every second for 100 values
    // The "sinewave" is a simple sine curve starting at 0 with a period of 100s and amplitude of one
    // The "positive" log is true for the first half of the values
    // The "negative" log is true for the second half of the values
    TimeSeriesProperty<double> sinewave("sinewave");
    TimeSeriesProperty<bool> negative("negative");
    TimeSeriesProperty<bool> positive("positive");
    for (std::size_t i = 0; i < NUM_VALUES; ++i) {
      const double value = std::sin(static_cast<double>(i) * FREQUENCY);
      const Mantid::DateAndTime time = EPOCH + static_cast<double>(i);
      const bool isPositive(value >= 0);
      sinewave.addValue(time, value);
      negative.addValue(time, !isPositive);
      positive.addValue(time, isPositive);
    }

    // verify logs are setup correctly
    TS_ASSERT_EQUALS(sinewave.size(), NUM_VALUES);
    TS_ASSERT_DELTA(sinewave.timeAverageValue(), 0., .001);
    TS_ASSERT_EQUALS(negative.size(), NUM_VALUES);
    TS_ASSERT_EQUALS(positive.size(), NUM_VALUES);

    // logfilter with negative
    LogFilter negativeFilter(&sinewave);
    negativeFilter.addFilter(negative);
    const FilteredTimeSeriesProperty<double> *negativeFilterLog = negativeFilter.data();
    TS_ASSERT_EQUALS(negativeFilterLog->size(), 50);
    TS_ASSERT_DELTA(negativeFilterLog->timeAverageValue(), -1 * MEAN_EXP, .001);

    // logfilter with positive
    LogFilter positiveFilter(&sinewave);
    positiveFilter.addFilter(positive);
    const FilteredTimeSeriesProperty<double> *positiveFilterLog = positiveFilter.data();
    TS_ASSERT_EQUALS(positiveFilterLog->size(), 50);
    TS_ASSERT_DELTA(positiveFilterLog->timeAverageValue(), MEAN_EXP, .001);
  }

private:
  /// Creates a test boolean filter
  /// @param type :: Which variant to create
  std::shared_ptr<TimeSeriesProperty<bool>> createTestFilter(const int type) {
    auto filter = std::make_shared<TimeSeriesProperty<bool>>("filter");
    if (type == 1) {
      filter->addValue("2007-11-30T16:16:50", true);
      filter->addValue("2007-11-30T16:17:25", false);
      filter->addValue("2007-11-30T16:17:39", true);
    } else if (type == 2) {
      filter->addValue("2007-11-30T16:16:50", false);
      filter->addValue("2007-11-30T16:17:25", true);
      filter->addValue("2007-11-30T16:17:39", false);
    } else if (type == 3) {
      filter->addValue("2007-11-30T16:17:00", true);
      filter->addValue("2007-11-30T16:17:05", false);
      filter->addValue("2007-11-30T16:17:12", true);
    } else if (type == 4) {
      filter->addValue("2007-11-30T16:17:00", false);
      filter->addValue("2007-11-30T16:17:40", true);
      filter->addValue("2007-11-30T16:17:45", false);
      filter->addValue("2007-11-30T16:17:50", true);
      filter->addValue("2007-11-30T16:18:00", false);
    }
    return filter;
  }

  std::time_t createTime_t_FromString(const std::string &str) {
    std::tm time_since_1900;
    time_since_1900.tm_isdst = -1;

    // create tm struct

    time_since_1900.tm_year = std::stoi(str.substr(0, 4)) - 1900;
    time_since_1900.tm_mon = std::stoi(str.substr(5, 2)) - 1;
    time_since_1900.tm_mday = std::stoi(str.substr(8, 2));
    time_since_1900.tm_hour = std::stoi(str.substr(11, 2));
    time_since_1900.tm_min = std::stoi(str.substr(14, 2));
    time_since_1900.tm_sec = std::stoi(str.substr(17, 2));

    return std::mktime(&time_since_1900);
  }
};
