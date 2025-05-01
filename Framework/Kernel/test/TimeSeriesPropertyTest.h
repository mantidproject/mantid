// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <cxxtest/TestSuite.h>

#include <boost/scoped_ptr.hpp>
#include <cmath>
#include <json/value.h>
#include <memory>
#include <numeric>
#include <vector>

using namespace Mantid::Kernel;
using Mantid::Kernel::Logger;
using Mantid::Types::Core::DateAndTime;

namespace {
/// static Logger definition
Logger g_tspt_log("TimeSeriesPropertyTest");
} // namespace

class TimeSeriesPropertyStatisticsTest : public CxxTest::TestSuite {

public:
  // Instantiate from a Kernel::Statistics object
  void test_fromKernelStatistics() {
    Statistics raw_stats;
    raw_stats.minimum = 1.0;
    raw_stats.maximum = 2.0;
    raw_stats.mean = 3.0;
    raw_stats.median = 4.0;
    raw_stats.standard_deviation = 5.0;
    auto stats = TimeSeriesPropertyStatistics(raw_stats);
    TS_ASSERT_DELTA(stats.minimum, 1.0, 0.1);
    TS_ASSERT_DELTA(stats.maximum, 2.0, 0.1);
    TS_ASSERT_DELTA(stats.mean, 3.0, 0.1);
    TS_ASSERT_DELTA(stats.median, 4.0, 0.1);
    TS_ASSERT_DELTA(stats.standard_deviation, 5.0, 0.1);
  }

  // Instantiate from a single value, constant in time
  void test_fromSingleValue() {
    auto stats = TimeSeriesPropertyStatistics(42.0);
    TS_ASSERT_DELTA(stats.minimum, 42.0, 1.0);
    TS_ASSERT_DELTA(stats.maximum, 42.0, 1.0);
    TS_ASSERT_DELTA(stats.mean, 42.0, 1.0);
    TS_ASSERT_DELTA(stats.median, 42.0, 1.0);
    TS_ASSERT_DELTA(stats.standard_deviation, 0.0, 0.001);
    TS_ASSERT_DELTA(stats.time_mean, 42.0, 1.0);
    TS_ASSERT_DELTA(stats.time_standard_deviation, 0.0, 0.001);
    TS_ASSERT(std::isnan(stats.duration));
  }
};

class TimeSeriesPropertyTest : public CxxTest::TestSuite {
  // Create a small TSP<double>. Callee owns the returned object.
  TimeSeriesProperty<double> *createDoubleTSP() {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 9.99));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 7.55));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 5.55));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 10.55));
    return p;
  }

  // Create a small TSP<int>. Callee owns the returned object.
  TimeSeriesProperty<int> *createIntegerTSP(int numberOfValues) {
    TimeSeriesProperty<int> *log = new TimeSeriesProperty<int>("intProp");
    DateAndTime startTime("2007-11-30T16:17:00");
    for (int value = 0; value < numberOfValues; ++value) {
      DateAndTime time = startTime + value * 10.0;
      TS_ASSERT_THROWS_NOTHING(log->addValue(time, value + 1));
    }
    return log;
  }

  // create a TimeROI object with two ROIS. Overlaps with the TimeSeriesProperty
  // returned by createDoubleTSP()
  TimeROI *createTimeRoi() {
    TimeROI *rois = new TimeROI;
    rois->addROI("2007-11-30T16:17:05", "2007-11-30T16:17:15");
    rois->addROI("2007-11-30T16:17:25", "2007-11-30T16:17:35");
    return rois;
  }

  // compare two vectors element-wise, allowing for a small difference between corresponding values.
  void assert_two_vectors(const std::vector<double> &left, const std::vector<double> &right, double delta) {
    TS_ASSERT_EQUALS(left.size(), right.size());
    for (size_t i = 0; i < left.size(); i++)
      TS_ASSERT_DELTA(left[i], right[i], delta);
  }

  // compare two vectors element-wise for exact match
  template <typename T> void assert_two_vectors(const std::vector<T> &left, const std::vector<T> &right) {
    TS_ASSERT_EQUALS(left.size(), right.size());
    for (size_t i = 0; i < left.size(); i++)
      TS_ASSERT_EQUALS(left[i], right[i]);
  }

public:
  void setUp() override {
    iProp = new TimeSeriesProperty<int>("intProp");
    dProp = new TimeSeriesProperty<double>("doubleProp");
    sProp = new TimeSeriesProperty<std::string>("stringProp");
  }

  void tearDown() override {
    delete iProp;
    delete dProp;
    delete sProp;
  }

  void test_Constructor() {
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT(!iProp->name().compare("intProp"));
    TS_ASSERT(!iProp->documentation().compare(""));
    TS_ASSERT(typeid(std::vector<TimeValueUnit<int>>) == *iProp->type_info());
    TS_ASSERT(!iProp->isDefault())

    TS_ASSERT(!dProp->name().compare("doubleProp"));
    TS_ASSERT(!dProp->documentation().compare(""));
    TS_ASSERT(typeid(std::vector<TimeValueUnit<double>>) == *dProp->type_info());
    TS_ASSERT(!dProp->isDefault())

    TS_ASSERT(!sProp->name().compare("stringProp"));
    TS_ASSERT(!sProp->documentation().compare(""));
    TS_ASSERT(typeid(std::vector<TimeValueUnit<std::string>>) == *sProp->type_info());
    TS_ASSERT(!sProp->isDefault())

    TS_ASSERT_EQUALS(sProp->isValid(), "");
  }

  void test_Constructor_with_values() {
    std::vector<DateAndTime> times = {DateAndTime("2019-01-01T00:00:00"), DateAndTime("2019-01-01T00:01:00")};

    // Test int TimeSeriesProperty
    std::vector<int> iValues = {0, 1};
    auto iPropWithValues = std::make_unique<TimeSeriesProperty<int>>("intProp", times, iValues);
    TS_ASSERT(!iPropWithValues->name().compare("intProp"));
    TS_ASSERT(!iPropWithValues->documentation().compare(""));
    TS_ASSERT(typeid(std::vector<TimeValueUnit<int>>) == *iPropWithValues->type_info());
    TS_ASSERT(!iPropWithValues->isDefault());

    auto iPropTimes = iPropWithValues->timesAsVector();
    TS_ASSERT_EQUALS(iPropTimes[0], DateAndTime("2019-01-01T00:00:00"));
    TS_ASSERT_EQUALS(iPropTimes[1], DateAndTime("2019-01-01T00:01:00"));

    auto iPropValues = iPropWithValues->valuesAsVector();
    TS_ASSERT_EQUALS(iPropValues[0], 0);
    TS_ASSERT_EQUALS(iPropValues[1], 1);

    // Test double TimeSeriesProperty
    std::vector<double> dValues = {0.1, 1.2};
    auto dPropWithValues = std::make_unique<TimeSeriesProperty<double>>("doubleProp", times, dValues);
    TS_ASSERT(!dPropWithValues->name().compare("doubleProp"));
    TS_ASSERT(!dPropWithValues->documentation().compare(""));
    TS_ASSERT(typeid(std::vector<TimeValueUnit<double>>) == *dPropWithValues->type_info());
    TS_ASSERT(!dPropWithValues->isDefault());

    auto dPropTimes = dPropWithValues->timesAsVector();
    TS_ASSERT_EQUALS(dPropTimes[0], DateAndTime("2019-01-01T00:00:00"));
    TS_ASSERT_EQUALS(dPropTimes[1], DateAndTime("2019-01-01T00:01:00"));

    auto dPropValues = dPropWithValues->valuesAsVector();
    TS_ASSERT_EQUALS(dPropValues[0], 0.1);
    TS_ASSERT_EQUALS(dPropValues[1], 1.2);

    // Test string TimeSeriesProperty
    std::vector<std::string> sValues = {"test", "test2"};
    auto sPropWithValues = std::make_unique<TimeSeriesProperty<std::string>>("stringProp", times, sValues);
    TS_ASSERT(!sPropWithValues->name().compare("stringProp"));
    TS_ASSERT(!sPropWithValues->documentation().compare(""));
    TS_ASSERT(typeid(std::vector<TimeValueUnit<std::string>>) == *sPropWithValues->type_info());
    TS_ASSERT(!sPropWithValues->isDefault());

    auto sPropTimes = sPropWithValues->timesAsVector();
    TS_ASSERT_EQUALS(sPropTimes[0], DateAndTime("2019-01-01T00:00:00"));
    TS_ASSERT_EQUALS(sPropTimes[1], DateAndTime("2019-01-01T00:01:00"));

    auto sPropValues = sPropWithValues->valuesAsVector();
    TS_ASSERT_EQUALS(sPropValues[0], "test");
    TS_ASSERT_EQUALS(sPropValues[1], "test2");
  }

  void test_SetValueFromString() {
    TS_ASSERT_THROWS(iProp->setValue("1"), const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(dProp->setValue("5.5"), const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(sProp->setValue("aValue"), const Exception::NotImplementedError &);
  }

  void test_SetValueFromJson() {
    TS_ASSERT_THROWS(iProp->setValueFromJson(Json::Value(1)), const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(dProp->setValueFromJson(Json::Value(5.5)), const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(sProp->setValueFromJson(Json::Value("aValue")), const Exception::NotImplementedError &);
  }

  void test_AddValue() {
    const std::string tester("2007-11-30T16:17:00");
    int sizepre = iProp->size();
    TS_ASSERT_THROWS_NOTHING(iProp->addValue(tester, 1));
    TS_ASSERT_THROWS_NOTHING(iProp->addValue("2007-11-30T16:17:10", 1));
    TS_ASSERT_EQUALS(iProp->size(), sizepre + 2);

    sizepre = dProp->size();
    TS_ASSERT_THROWS_NOTHING(dProp->addValue("2007-11-30T16:17:00", 9.99));
    TS_ASSERT_THROWS_NOTHING(dProp->addValue("2007-11-30T16:17:10", 5.55));
    TS_ASSERT_EQUALS(dProp->size(), sizepre + 2);

    sizepre = sProp->size();
    TS_ASSERT_THROWS_NOTHING(sProp->addValue("2007-11-30T16:17:00", "test"));
    TS_ASSERT_THROWS_NOTHING(sProp->addValue("2007-11-30T16:17:10", "test2"));
    TS_ASSERT_EQUALS(sProp->size(), sizepre + 2);

    // Now try the other overloads
    TimeSeriesProperty<int> otherProp("otherProp");
    TS_ASSERT_THROWS_NOTHING(otherProp.addValue(static_cast<std::time_t>(123), 1));
    TS_ASSERT_THROWS_NOTHING(otherProp.addValue(boost::posix_time::second_clock::local_time(), 1));

    const std::string dString = dProp->value();
    TS_ASSERT_EQUALS(dString.substr(0, 27), "2007-Nov-30 16:17:00  9.99\n");
    const std::string iString = iProp->value();
    TS_ASSERT_EQUALS(iString.substr(0, 24), "2007-Nov-30 16:17:00  1\n");
    const std::string sString = sProp->value();
    TS_ASSERT_EQUALS(sString.substr(0, 27), "2007-Nov-30 16:17:00  test\n");

    // Test the internal toggling of the 'sorted' flag works
    auto twoVals = dProp->valuesAsVector();
    double newVal = 2.22;
    dProp->addValue("2007-11-30T16:17:05", newVal);
    // Calling this method sorts the vector by time, so long as the internal
    // flag says it isn't sorted
    auto threeVals = dProp->valuesAsVector();
    TS_ASSERT_EQUALS(threeVals.size(), 3);
    TS_ASSERT_EQUALS(twoVals[0], threeVals[0]);
    TS_ASSERT_EQUALS(twoVals[1], threeVals[2]);
    TS_ASSERT_EQUALS(newVal, threeVals[1]);
  }
  void test_GetDerivative() {
    dProp->addValue("2007-11-30T16:17:10", 10);
    dProp->addValue("2007-11-30T16:17:12", 12);
    dProp->addValue("2007-11-30T16:17:01", 01);
    dProp->addValue("2007-11-30T16:17:05", 05);

    auto derProp = dProp->getDerivative();
    TS_ASSERT(dynamic_cast<TimeSeriesProperty<double> *>(derProp.get()))

    TS_ASSERT_EQUALS(derProp->size(), 3);
    auto derValues = derProp->valuesAsVector();

    TS_ASSERT_EQUALS(derValues[0], 1);
    TS_ASSERT_EQUALS(derValues[1], 1);
    TS_ASSERT_EQUALS(derValues[2], 1);

    TSM_ASSERT_THROWS("derivative undefined for string property", sProp->getDerivative(), const std::runtime_error &);

    iProp->addValue("2007-11-30T16:17:10", 10);
    TSM_ASSERT_THROWS("derivative undefined for property with less then 2 values", iProp->getDerivative(),
                      const std::runtime_error &);
    iProp->addValue("2007-11-30T16:17:12", 12);

    derProp = iProp->getDerivative();
    TS_ASSERT_EQUALS(derProp->size(), 1);
    derValues = derProp->valuesAsVector();
    TS_ASSERT_EQUALS(derValues[0], 1);
  }
  void test_timesAsVector() {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 5.55));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 9.99));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 5.55));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 5.55));
    std::vector<double> timeSec;
    timeSec = p->timesAsVectorSeconds();
    TS_ASSERT_DELTA(timeSec[0], 0.0, 1e-6);
    TS_ASSERT_DELTA(timeSec[1], 10.0, 1e-6);
    TS_ASSERT_DELTA(timeSec[2], 20.0, 1e-6);
    TS_ASSERT_DELTA(timeSec[3], 30.0, 1e-6);
    std::vector<DateAndTime> time;
    time = p->timesAsVector();
    TS_ASSERT_EQUALS(time[0], DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(time[1], DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(time[2], DateAndTime("2007-11-30T16:17:20"));
    TS_ASSERT_EQUALS(time[3], DateAndTime("2007-11-30T16:17:30"));

    delete p;
  }

  void test_replaceValues() {
    // Arrange
    size_t num = 1000;
    DateAndTime first("2007-11-30T16:17:10");
    std::vector<DateAndTime> times;

    std::vector<double> values;
    std::vector<double> replacementValues;
    double offset = 100.0;
    for (size_t i = 0; i < num; i++) {
      times.emplace_back(first + double(i));
      values.emplace_back(double(i));
      replacementValues.emplace_back(double(i) + offset);
    }
    TimeSeriesProperty<double> tsp("test");
    tsp.addValues(times, values);
    TS_ASSERT_EQUALS(tsp.size(), 1000);
    TS_ASSERT_EQUALS(tsp.nthValue(3), 3.0);

    // Act
    tsp.replaceValues(times, replacementValues);

    // Assert
    TSM_ASSERT_EQUALS("Should have 1000 entries", tsp.size(), 1000);
    TSM_ASSERT_EQUALS("Should be 3 plus the offset of 100", tsp.nthValue(3), 103.0);
  }

  void test_addValues() {
    size_t num = 1000;
    DateAndTime first("2007-11-30T16:17:10");
    std::vector<DateAndTime> times;

    std::vector<double> values;
    for (size_t i = 0; i < num; i++) {
      times.emplace_back(first + double(i));
      values.emplace_back(double(i));
    }
    TimeSeriesProperty<double> tsp("test");
    tsp.addValues(times, values);
    TS_ASSERT_EQUALS(tsp.size(), 1000);
    TS_ASSERT_EQUALS(tsp.nthValue(3), 3.0);
  }

  void test_Casting() {
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(iProp), static_cast<Property *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(dProp), static_cast<Property *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(sProp), static_cast<Property *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<ITimeSeriesProperty *>(iProp), static_cast<ITimeSeriesProperty *>(nullptr));
  }

  //----------------------------------------------------------------------------
  void test_AdditionOperator() {
    TimeSeriesProperty<int> *log = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:19:10", 2));

    TimeSeriesProperty<int> *log2 = new TimeSeriesProperty<int>("MyIntLog2");
    TS_ASSERT_THROWS_NOTHING(log2->addValue("2007-11-30T16:18:00", 3));
    TS_ASSERT_THROWS_NOTHING(log2->addValue("2007-11-30T16:18:10", 4));
    TS_ASSERT_THROWS_NOTHING(log2->addValue("2007-11-30T16:18:11", 5));

    TS_ASSERT_EQUALS(log->size(), 2);

    // Concatenate the lists
    (*log) += log2;

    TS_ASSERT_EQUALS(log->size(), 5);

    DateAndTime t0 = log->firstTime();
    DateAndTime tf = log->lastTime();

    TS_ASSERT_EQUALS(t0, DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(tf, DateAndTime("2007-11-30T16:19:10"));

    delete log;
    delete log2;
  }

  //----------------------------------------------------------------------------
  /// Ticket 2097: This caused an infinite loop
  void test_AdditionOperatorOnYourself() {
    TimeSeriesProperty<int> *log = createIntegerTSP(2);

    (*log) += log;
    // There is now a check and trying to do this does nothing.
    TS_ASSERT_EQUALS(log->size(), 2);

    delete log;
  }

  //----------------------------------------------------------------------------
  void test_filteredValuesAsVector() {
    TimeSeriesProperty<double> *log = createDoubleTSP();
    // no filter
    this->assert_two_vectors(log->filteredValuesAsVector(), log->valuesAsVector(), 0.01);
    // filter encompassing all the time domain
    TimeROI rois;
    rois.addROI("2007-11-30T16:17:00", "2007-11-30T16:17:31");
    this->assert_two_vectors(log->filteredValuesAsVector(&rois), log->valuesAsVector(), 0.01);
    this->assert_two_vectors(log->filteredTimesAsVector(&rois), log->timesAsVector());
    TS_ASSERT_EQUALS(log->valuesAsVector().size(), log->timesAsVector().size());

    // times are outside the ROI's. Some times are at the upper boundaries of the ROI's, thus are excluded
    rois.clear();
    rois.addROI("2007-11-30T16:16:00", "2007-11-30T16:17:00");  // before the first time, including the first time
    rois.addROI("2007-11-30T16:17:01", "2007-11-30T16:17:09");  // between times 1st and 2nd
    rois.addROI("2007-11-30T16:17:15", "2007-11-30T16:17:20");  // between times 2nd and 3rd, including time 3rd
    rois.addROI("2007-11-30T16:17:45", "2007-11-30T16:18:00");  // after last time
    std::vector<double> expected_values_one{9.99, 7.55, 10.55}; // 3rd value is notched out
    std::vector<DateAndTime> expected_times_one{DateAndTime("2007-11-30T16:17:01"), DateAndTime("2007-11-30T16:17:15"),
                                                DateAndTime("2007-11-30T16:17:45")};
    this->assert_two_vectors(log->filteredValuesAsVector(&rois), expected_values_one, 0.01);
    this->assert_two_vectors(log->filteredTimesAsVector(&rois), expected_times_one);

    rois.clear();
    rois.addROI("2007-11-30T16:16:30", "2007-11-30T16:17:05"); // capture the first time
    rois.addROI("2007-11-30T16:17:10", "2007-11-30T16:17:20"); // capture second time, exclude the third
    rois.addROI("2007-11-30T16:17:30", "2007-11-30T16:18:00"); // ROI after last time, including last time
    std::vector<double> expected_values_two{9.99, 7.55, 10.55};
    std::vector<DateAndTime> expected_times_two{DateAndTime("2007-11-30T16:17:00"), DateAndTime("2007-11-30T16:17:10"),
                                                DateAndTime("2007-11-30T16:17:30")};
    this->assert_two_vectors(log->filteredValuesAsVector(&rois), expected_values_two, 0.01);
    this->assert_two_vectors(log->filteredTimesAsVector(&rois), expected_times_two);

    delete log;
  }

  //----------------------------------------------------------------------------
  void test_filterByTime() {
    TimeSeriesProperty<int> *log = createIntegerTSP(6);
    TS_ASSERT_EQUALS(log->realSize(), 6);
    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");

    // Since the filter is < stop, the last one is not counted, so there are  3
    // taken out.

    TimeROI roi(start, stop);
    TS_ASSERT_EQUALS(log->filteredValuesAsVector(&roi).size(), 3);

    delete log;
  }

  //-------------------------------------------------------------------------------
  void test_filterByTimes1() {
    TimeSeriesProperty<int> *log = createIntegerTSP(6);
    TS_ASSERT_EQUALS(log->realSize(), 6);

    TimeROI roi(DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:40"));

    // Since the filter is < stop, the last one is not counted, so there are  3
    // taken out.
    // values are 2, 3, 4
    TS_ASSERT_EQUALS(log->filteredValuesAsVector(&roi).size(), 3);

    delete log;
  }

  void test_removeDataOutsideTimeROI() {
    TimeROI roi;
    roi.addROI(DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:40"));
    roi.addROI(DateAndTime("2007-11-30T16:18:05"), DateAndTime("2007-11-30T16:18:25"));

    std::vector<DateAndTime> times;
    std::vector<DateAndTime> times_expected;
    std::vector<double> values;
    std::vector<double> values_expected;

    // 1. TimeSeriesProperty with a single value should have no changes after filtering
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_1...");
    times = {DateAndTime("2007-11-30T16:19:00")};
    times_expected = times;
    values = {1.};
    values_expected = values;
    auto tsp_input = std::make_unique<TimeSeriesProperty<double>>("one_value", times, values);
    auto tsp_expected = std::make_unique<TimeSeriesProperty<double>>("one_value", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // 2. TimeSeriesProperty with two values
    values = {1., 2.};
    values_expected = values;
    // a. TimeROI entirely between those values - no changes
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2a...");
    times = {DateAndTime("2007-11-30T16:00:00"), DateAndTime("2007-11-30T20:00:00")};
    times_expected = times;
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_a", times, values);
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_a", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // b. TimeROI entirely includes the values - no changes

    // b1. First roi entirely includes the values
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2b1...");
    times = {DateAndTime("2007-11-30T16:17:15"), DateAndTime("2007-11-30T16:17:35")};
    times_expected = times;
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_b1", times, values);
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_b1", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // b2. First roi includes first value, second roi includes second value
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2b2...");
    times = {DateAndTime("2007-11-30T16:17:15"), DateAndTime("2007-11-30T18:15:00")};
    times_expected = times;
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_b2", times, values);
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_b2", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // c. TimeROI includes first value and not second - no changes
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2c...");
    times = {DateAndTime("2007-11-30T16:17:15"), DateAndTime("2007-11-30T16:18:25")};
    times_expected = times;
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_c", times, values);
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_c", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // d. TimeROI includes second value and not first - no changes
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2d...");
    times = {DateAndTime("2007-11-30T16:17:00"), DateAndTime("2007-11-30T16:18:10")};
    times_expected = times;
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_d", times, values);
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_d", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // e. TimeROI is before both values - keep first
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2e...");
    times = {DateAndTime("2007-11-30T16:18:35"), DateAndTime("2007-11-30T16:18:45")};
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_e", times, values);
    times_expected = {times[0]};
    values_expected = {values[0]};
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_e", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // e1. TimeROI right boundary is equal to the first value - keep both, b/c for log value copying purposes we treat
    // TimeROI region as [inclusive,inclusive]
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2e1...");
    times = {DateAndTime("2007-11-30T16:17:40"), DateAndTime("2007-11-30T16:18:45")};
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_e1", times, values);
    times_expected = {times[0], times[1]};
    values_expected = {values[0], values[1]};
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_e1", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // f. TimeROI is after both values - keep second
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_2f...");
    times = {DateAndTime("2007-11-30T16:16:10"), DateAndTime("2007-11-30T16:16:45")};
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("two_values_f", times, values);
    times_expected = {times[1]};   // second time
    values_expected = {values[1]}; // second value
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("two_values_f", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // 3. TimeSeriesProperty with three values
    values = {1., 2., 3.};

    // a. TimeROI entirely between the values - no changes
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_3a...");
    times = {DateAndTime("2007-11-30T16:17:05"), DateAndTime("2007-11-30T16:18:00"),
             DateAndTime("2007-11-30T16:18:45")};
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("three_values_a0", times, values);
    times_expected = times;
    values_expected = values;
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("three_values_a0", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // b. TimeROI includes first value only - keep the first two
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_3b...");
    times = {DateAndTime("2007-11-30T16:17:15"), DateAndTime("2007-11-30T16:18:30"),
             DateAndTime("2007-11-30T16:18:45")};
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("three_values_a", times, values);
    times_expected = {times[0], times[1]};
    values_expected = {values[0], values[1]};
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("three_values_a", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // c. TimeROI includes second value only - no changes
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_3c...");
    times = {DateAndTime("2007-11-30T16:17:00"), DateAndTime("2007-11-30T16:17:15"),
             DateAndTime("2007-11-30T16:18:30")};
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("three_values_b", times, values);
    times_expected = times;
    values_expected = values;
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("three_values_b", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);

    // d. TimeROI includes third value only - keep the last two
    g_tspt_log.notice("\ntest_removeDataOutsideTimeROI_case_3d...");
    times = {DateAndTime("2007-11-30T16:17:00"), DateAndTime("2007-11-30T16:17:05"),
             DateAndTime("2007-11-30T16:18:20")};
    tsp_input = std::make_unique<TimeSeriesProperty<double>>("three_values_c", times, values);
    times_expected = {times[1], times[2]}; // last two
    values_expected = {values[1], values[2]};
    tsp_expected = std::make_unique<TimeSeriesProperty<double>>("three_values_c", times_expected, values_expected);
    tsp_input->removeDataOutsideTimeROI(roi);
    TS_ASSERT_EQUALS(*tsp_input, *tsp_expected);
  }

  void test_cloneInTimeROI() {
    TimeROI roi;
    roi.addROI(DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:40"));
    roi.addROI(DateAndTime("2007-11-30T16:18:05"), DateAndTime("2007-11-30T16:18:25"));

    // Test a case where TimeROI includes the first time value only. The filtered data should be the first two
    // datapoints.
    std::vector<DateAndTime> times{DateAndTime("2007-11-30T16:17:15"), DateAndTime("2007-11-30T16:18:30"),
                                   DateAndTime("2007-11-30T16:18:45")};
    std::vector<DateAndTime> times_expected{times[0], times[1]};
    std::vector<double> values{1., 2., 3.};
    std::vector<double> values_expected{values[0], values[1]};

    auto tsp_input = std::make_unique<TimeSeriesProperty<double>>("three_values", times, values);
    auto tsp_expected = std::make_unique<TimeSeriesProperty<double>>("three_values", times_expected, values_expected);
    auto tsp_result_base = std::shared_ptr<Property>(tsp_input->cloneInTimeROI(roi));
    auto tsp_result = std::static_pointer_cast<TimeSeriesProperty<double>>(tsp_result_base);

    // Make sure the cloned-in-roi time series copy is different from the original, i.e. the roi really filters out some
    // data
    TS_ASSERT(*tsp_result != *tsp_input);
    // Test that the cloned copy is the same as expected
    TS_ASSERT_EQUALS(*tsp_result, *tsp_expected);
  }

  void test_single_value_roi_mean() {
    DateAndTime firstLogTime("2007-11-30T16:17:00");
    TimeSeriesProperty<double> *prop = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(prop->addValue(firstLogTime, 1.)); // value that isn't zero

    // single value no TimeROI
    const auto statsEmptySingleValue = prop->getStatistics();
    TS_ASSERT_EQUALS(statsEmptySingleValue.minimum, 1.);
    TS_ASSERT_EQUALS(statsEmptySingleValue.maximum, 1.);
    TS_ASSERT_EQUALS(statsEmptySingleValue.median, 1.);
    TS_ASSERT_EQUALS(statsEmptySingleValue.mean, 1.);
    TS_ASSERT_EQUALS(statsEmptySingleValue.standard_deviation, 0.);
    TS_ASSERT_EQUALS(statsEmptySingleValue.time_mean, 1.);
    TS_ASSERT_EQUALS(statsEmptySingleValue.time_standard_deviation, 0.);
    TS_ASSERT_EQUALS(statsEmptySingleValue.duration, 0.);

    // with TimeROI
    TimeROI left(firstLogTime, firstLogTime + 10.);
    const auto statsSingleValue = prop->getStatistics(&left);
    TS_ASSERT_EQUALS(statsSingleValue.minimum, 1.);
    TS_ASSERT_EQUALS(statsSingleValue.maximum, 1.);
    TS_ASSERT_EQUALS(statsSingleValue.median, 1.);
    TS_ASSERT_EQUALS(statsSingleValue.mean, 1.);
    TS_ASSERT_EQUALS(statsSingleValue.standard_deviation, 0.);
    TS_ASSERT_EQUALS(statsSingleValue.time_mean, 1.);
    TS_ASSERT_EQUALS(statsSingleValue.time_standard_deviation, 0.);
    TS_ASSERT_EQUALS(statsSingleValue.duration, 10.);

    delete prop;
  }

  void test_multi_value_roi_mean() {
    DateAndTime EPOCH("2007-11-30T16:17:00");
    TimeSeriesProperty<double> *prop = new TimeSeriesProperty<double>("doubleProp");
    for (size_t i = 1; i < 10; ++i) { // values are 1-9, times are first at 16:17:00, last at 16:17:16
      TS_ASSERT_THROWS_NOTHING(prop->addValue(EPOCH + double(2 * (i - 1)), double(i))); // value that isn't zero
    }

    // these values are calculated by hand
    const double MEAN_SIMPLE(5);
    const double STDDEV_SIMPLE(2.581988897471611);
    // single value no TimeROI
    const auto statsEmptyRoi = prop->getStatistics();
    TS_ASSERT_EQUALS(statsEmptyRoi.minimum, 1.);
    TS_ASSERT_EQUALS(statsEmptyRoi.maximum, 9.);
    TS_ASSERT_EQUALS(statsEmptyRoi.median, MEAN_SIMPLE);
    TS_ASSERT_EQUALS(statsEmptyRoi.mean, MEAN_SIMPLE);
    TS_ASSERT_EQUALS(statsEmptyRoi.standard_deviation, STDDEV_SIMPLE);
    TS_ASSERT_EQUALS(statsEmptyRoi.time_mean, MEAN_SIMPLE);
    TS_ASSERT_EQUALS(statsEmptyRoi.time_standard_deviation, STDDEV_SIMPLE);
    TS_ASSERT_EQUALS(statsEmptyRoi.duration, 18.);

    //    std::cout << "\n" << prop->value() << "\n"; // TODO REMOVE

    // with TimeROI including everything
    TimeROI roi(EPOCH, EPOCH + 18.);
    const auto statsRoiAll = prop->getStatistics(&roi);
    TS_ASSERT_EQUALS(statsRoiAll.minimum, 1.);
    TS_ASSERT_EQUALS(statsRoiAll.maximum, 9.);
    TS_ASSERT_EQUALS(statsRoiAll.median, MEAN_SIMPLE);
    TS_ASSERT_EQUALS(statsRoiAll.mean, MEAN_SIMPLE);
    TS_ASSERT_EQUALS(statsRoiAll.standard_deviation, STDDEV_SIMPLE);
    TS_ASSERT_EQUALS(statsRoiAll.time_mean, MEAN_SIMPLE);
    TS_ASSERT_EQUALS(statsRoiAll.time_standard_deviation, STDDEV_SIMPLE);
    TS_ASSERT_EQUALS(statsRoiAll.duration, 18.);

    // single TimeROI to include values [3,4] with the preceeding 2 being implicit
    roi.clear();
    roi.addROI(EPOCH + 3., EPOCH + 7.);
    const auto statsROIOne = prop->getStatistics(&roi);
    // not bothering with stddev
    TS_ASSERT_EQUALS(statsROIOne.minimum, 2.);
    TS_ASSERT_EQUALS(statsROIOne.maximum, 4.);
    TS_ASSERT_EQUALS(statsROIOne.median, 3);
    TS_ASSERT_EQUALS(statsROIOne.mean, 3);
    TS_ASSERT_EQUALS(statsROIOne.time_mean, 3);
    TS_ASSERT_EQUALS(statsROIOne.duration, 4.);

    delete prop;
  }

  void test_extractStatistic() {
    // same as
    DateAndTime firstLogTime("2007-11-30T16:17:00");
    TimeSeriesProperty<double> *prop = new TimeSeriesProperty<double>("doubleProp");
    for (size_t i = 1; i < 10; ++i) { // values are 1-9, times are first at 16:17:00, last at 16:17:16
      TS_ASSERT_THROWS_NOTHING(prop->addValue(firstLogTime + double(2 * (i - 1)), double(i))); // value that isn't zero
    }

    // no TimeROI
    TS_ASSERT_EQUALS(prop->firstValue(), 1.);
    TS_ASSERT_EQUALS(prop->extractStatistic(Math::StatisticType::FirstValue), 1.);
    TS_ASSERT_EQUALS(prop->lastValue(), 9.);
    TS_ASSERT_EQUALS(prop->extractStatistic(Math::StatisticType::LastValue), 9.);

    // notch around the second value at EPOCH+2s which is 2
    TimeROI roi(firstLogTime + 2., firstLogTime + 3.); // only a single value
    TS_ASSERT_EQUALS(prop->firstValue(roi), 2.);
    TS_ASSERT_EQUALS(prop->extractStatistic(Math::StatisticType::FirstValue, &roi), 2.);
    TS_ASSERT_EQUALS(prop->lastValue(roi), 2.);
    TS_ASSERT_EQUALS(prop->extractStatistic(Math::StatisticType::LastValue, &roi), 2.);

    // add a second to include time=166:17:04, 3
    roi.addROI(firstLogTime + 2., firstLogTime + 4.);
    TS_ASSERT_EQUALS(prop->firstValue(roi), 2.);
    TS_ASSERT_EQUALS(prop->extractStatistic(Math::StatisticType::FirstValue, &roi), 2.);
    TS_ASSERT_EQUALS(prop->lastValue(roi), 3.);
    TS_ASSERT_EQUALS(prop->extractStatistic(Math::StatisticType::LastValue, &roi), 3.);

    delete prop;
  }

  void test_filterByTimesN() {
    TimeSeriesProperty<int> *log = createIntegerTSP(10);
    TS_ASSERT_EQUALS(log->realSize(), 10);

    TimeROI roi;
    roi.addROI(DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:40"));
    roi.addROI(DateAndTime("2007-11-30T16:18:05"), DateAndTime("2007-11-30T16:18:25"));

    std::vector<DateAndTime> expTimes{DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:20"),
                                      DateAndTime("2007-11-30T16:17:30"), DateAndTime("2007-11-30T16:18:05"),
                                      DateAndTime("2007-11-30T16:18:10"), DateAndTime("2007-11-30T16:18:20")};
    this->assert_two_vectors(log->filteredValuesAsVector(&roi), {2, 3, 4, 7, 8, 9});
    this->assert_two_vectors(log->filteredTimesAsVector(&roi), expTimes);
    delete log;
  }

  //----------------------------------------------------------------------------
  /**
   * Ticket #2591 - since the values are unchanged, specifying a TimeROI after all
   * the actual values, will return the last value
   */
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead() {
    TimeSeriesProperty<int> *log = createIntegerTSP(1);
    TS_ASSERT_EQUALS(log->realSize(), 1);

    // original time is "2007-11-30T16:17:00"
    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    TimeROI roi(start, stop);

    // Still there!
    TS_ASSERT_EQUALS(log->filteredValuesAsVector(&roi).size(), 1);
    TS_ASSERT_EQUALS(log->filteredValuesAsVector(&roi).front(), 1);
    TS_ASSERT_EQUALS(log->filteredTimesAsVector(&roi).size(), 1);
    TS_ASSERT_EQUALS(log->filteredTimesAsVector(&roi).front(), start);

    delete log;
  }

  //----------------------------------------------------------------------------
  /**
   * Ticket #2591 - since the values are unchanged, specifying a TimeROI after all
   * the actual values, will return the last value
   */
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead_2() {
    TimeSeriesProperty<int> *log = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("1990-01-01T00:00:00", 1));
    TS_ASSERT_EQUALS(log->realSize(), 1);

    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    TimeROI roi(start, stop);

    // Still there!
    TS_ASSERT_EQUALS(log->filteredValuesAsVector(&roi).size(), 1);
    TS_ASSERT_EQUALS(log->filteredValuesAsVector(&roi).front(), 1);
    TS_ASSERT_EQUALS(log->filteredTimesAsVector(&roi).size(), 1);
    TS_ASSERT_EQUALS(log->filteredTimesAsVector(&roi).front(), start);

    delete log;
  }

  //----------------------------------------------------------------------------
  void test_makeFilterByValue() {
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>("MyIntLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:30", 2.0));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:40", 2.01));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:50", 6));

    TS_ASSERT_EQUALS(log->realSize(), 6);

    // Test centred log value boundaries
    SplittingIntervalVec splitter;
    log->makeFilterByValue(splitter, 1.8, 2.2, 1.0, true);

    TS_ASSERT_EQUALS(splitter.size(), 2);
    SplittingInterval s;
    DateAndTime t;

    s = splitter[0];
    t = DateAndTime("2007-11-30T16:17:09");
    TS_ASSERT_DELTA(s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:11");
    TS_ASSERT_DELTA(s.stop(), t, 1e-3);

    s = splitter[1];
    t = DateAndTime("2007-11-30T16:17:29");
    TS_ASSERT_DELTA(s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:41");
    TS_ASSERT_DELTA(s.stop(), t, 1e-3);

    // Now test with left-aligned log value boundaries
    log->makeFilterByValue(splitter, 1.8, 2.2, 1.0);

    TS_ASSERT_EQUALS(splitter.size(), 2);

    s = splitter[0];
    t = DateAndTime("2007-11-30T16:17:10");
    TS_ASSERT_DELTA(s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:20");
    TS_ASSERT_DELTA(s.stop(), t, 1e-3);

    s = splitter[1];
    t = DateAndTime("2007-11-30T16:17:30");
    TS_ASSERT_DELTA(s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:50");
    TS_ASSERT_DELTA(s.stop(), t, 1e-3);

    // Check throws if min > max
    TS_ASSERT_THROWS(log->makeFilterByValue(splitter, 2.0, 1.0, 0.0, true), const std::invalid_argument &);

    delete log;
  }

  //----------------------------------------------------------------------------
  void test_makeFilterByValueWithROI() {
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>("doubleTestLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:30", 2.0));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:40", 2.01));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:50", 6));

    TS_ASSERT_EQUALS(log->realSize(), 6);
    TimeInterval expandedTime(DateAndTime(0), DateAndTime(1));

    // Test centred log value boundaries
    TimeROI roi = log->makeFilterByValue(1.8, 2.2, false, expandedTime, 1.0, true);

    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);

    TS_ASSERT_DELTA(roi.timeAtIndex(0), DateAndTime("2007-11-30T16:17:09"), 1e-3);
    TS_ASSERT_DELTA(roi.timeAtIndex(1), DateAndTime("2007-11-30T16:17:11"), 1e-3);

    TS_ASSERT_DELTA(roi.timeAtIndex(2), DateAndTime("2007-11-30T16:17:29"), 1e-3);
    TS_ASSERT_DELTA(roi.timeAtIndex(3), DateAndTime("2007-11-30T16:17:41"), 1e-3);

    // Now test with left-aligned log value boundaries
    roi = log->makeFilterByValue(1.8, 2.2, false, expandedTime, 1.0);
    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);

    TS_ASSERT_DELTA(roi.timeAtIndex(0), DateAndTime("2007-11-30T16:17:10"), 1e-3);
    TS_ASSERT_DELTA(roi.timeAtIndex(1), DateAndTime("2007-11-30T16:17:20"), 1e-3);

    TS_ASSERT_DELTA(roi.timeAtIndex(2), DateAndTime("2007-11-30T16:17:30"), 1e-3);
    TS_ASSERT_DELTA(roi.timeAtIndex(3), DateAndTime("2007-11-30T16:17:50"), 1e-3);

    TimeROI *existing = new TimeROI(DateAndTime("2007-11-30T16:17:40"), DateAndTime("2007-11-30T16:18:00"));

    roi = log->makeFilterByValue(0.8, 2.2, false, expandedTime, 0.0, false, existing);
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);

    TS_ASSERT_DELTA(roi.timeAtIndex(0), DateAndTime("2007-11-30T16:17:40"), 1e-3);
    TS_ASSERT_DELTA(roi.timeAtIndex(1), DateAndTime("2007-11-30T16:17:50"), 1e-3);

    expandedTime = TimeInterval(DateAndTime("2007-11-30T16:16:00"), DateAndTime("2007-11-30T16:18:30"));

    existing->clear();
    existing->addROI(DateAndTime("2007-11-30T16:16:50"), DateAndTime("2007-11-30T16:17:40"));

    roi = log->makeFilterByValue(0.8, 2.2, true, expandedTime, 1.0, true, existing);
    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);

    TS_ASSERT_DELTA(roi.timeAtIndex(0), DateAndTime("2007-11-30T16:16:50"), 1e-3);
    TS_ASSERT_DELTA(roi.timeAtIndex(1), DateAndTime("2007-11-30T16:17:11"), 1e-3);

    TS_ASSERT_DELTA(roi.timeAtIndex(2), DateAndTime("2007-11-30T16:17:29"), 1e-3);
    TS_ASSERT_DELTA(roi.timeAtIndex(3), DateAndTime("2007-11-30T16:17:40"), 1e-3);

    // Check throws if min > max
    TS_ASSERT_THROWS(log->makeFilterByValue(2.0, 1.0, true, expandedTime, 0.0, true), const std::invalid_argument &);

    delete log;
  }

  void test_makeFilterByValue_throws_for_string_property() {
    TimeSeriesProperty<std::string> log("StringTSP");
    SplittingIntervalVec splitter;
    TS_ASSERT_THROWS(log.makeFilterByValue(splitter, 0.0, 0.0, 0.0, true), const Exception::NotImplementedError &);
  }

  void test_expandFilterToRange() {
    TimeSeriesProperty<int> log("MyIntLog");
    TS_ASSERT_THROWS_NOTHING(log.addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(log.addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(log.addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(log.addValue("2007-11-30T16:17:30", 4));
    TS_ASSERT_THROWS_NOTHING(log.addValue("2007-11-30T16:17:40", 6));
    TS_ASSERT_THROWS_NOTHING(log.addValue("2007-11-30T16:17:50", 2));

    // Create a TimeInterval that's wider than this log
    TimeInterval interval(DateAndTime("2007-11-30T16:16:00"), DateAndTime("2007-11-30T16:18:50"));

    SplittingIntervalVec splitter;
    // Test good at both ends
    log.makeFilterByValue(splitter, 1.0, 2.2, 1.0, false);
    log.expandFilterToRange(splitter, 1.0, 2.2, interval);
    TS_ASSERT_EQUALS(splitter.size(), 2);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:16:00"), 1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:20"), 1e-3);
    TS_ASSERT_DELTA(splitter[1].start(), DateAndTime("2007-11-30T16:17:50"), 1e-3);
    TS_ASSERT_DELTA(splitter[1].stop(), DateAndTime("2007-11-30T16:18:50"), 1e-3);

    // Test bad at both ends
    log.makeFilterByValue(splitter, 2.5, 10.0, 0.0, false);
    log.expandFilterToRange(splitter, 2.5, 10.0, interval);
    TS_ASSERT_EQUALS(splitter.size(), 1);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:17:20"), 1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:50"), 1e-3);

    // Test good at start, bad at end
    log.makeFilterByValue(splitter, -1.0, 1.5, 0.0, false);
    log.expandFilterToRange(splitter, -1.0, 1.5, interval);
    TS_ASSERT_EQUALS(splitter.size(), 1);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:16:00"), 1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:10"), 1e-3);

    // Test good at end, bad at start
    log.makeFilterByValue(splitter, 1.99, 2.5, 1.0, false);
    log.expandFilterToRange(splitter, 1.99, 2.5, interval);
    TS_ASSERT_EQUALS(splitter.size(), 2);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:17:10"), 1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:20"), 1e-3);
    TS_ASSERT_DELTA(splitter[1].start(), DateAndTime("2007-11-30T16:17:50"), 1e-3);
    TS_ASSERT_DELTA(splitter[1].stop(), DateAndTime("2007-11-30T16:18:50"), 1e-3);

    // Check throws if min > max
    TS_ASSERT_THROWS(log.expandFilterToRange(splitter, 2.0, 1.0, interval), const std::invalid_argument &);

    // Test good at both ends, but interval narrower than log range
    TimeInterval narrowinterval(DateAndTime("2007-11-30T16:17:15"), DateAndTime("2007-11-30T16:17:41"));
    log.makeFilterByValue(splitter, 0.0, 10.0, 0.0, false);
    log.expandFilterToRange(splitter, 0.0, 10.0, narrowinterval);
    TS_ASSERT_EQUALS(splitter.size(), 1);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:17:00"), 1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:50"), 1e-3);
  }

  void test_expandFilterToRange_throws_for_string_property() {
    TimeSeriesProperty<std::string> log("StringTSP");
    SplittingIntervalVec splitter;
    TS_ASSERT_THROWS(log.expandFilterToRange(splitter, 0.0, 0.0, TimeInterval()),
                     const Exception::NotImplementedError &);
  }

  void test_averageValueInFilter() {
    auto dblLog = createDoubleTSP();
    auto intLog = createIntegerTSP(5);

    // Test a filter that's fully within the range of both properties
    TimeROI filter(DateAndTime("2007-11-30T16:17:05"), DateAndTime("2007-11-30T16:17:29"));
    TS_ASSERT_DELTA(dblLog->timeAverageValue(&filter), 7.308, 0.001);
    TS_ASSERT_DELTA(intLog->timeAverageValue(&filter), 2.167, 0.001);

    // Test a filter that starts before the log start time
    filter.clear();
    filter.addROI(DateAndTime("2007-11-30T16:16:30"), DateAndTime("2007-11-30T16:17:13"));
    TS_ASSERT_DELTA(dblLog->timeAverageValue(&filter), 9.820, 0.001);
    TS_ASSERT_DELTA(intLog->timeAverageValue(&filter), 1.070, 0.001);

    // How about one that's entirely outside the log range (should just take the
    // last value)
    filter.clear();
    filter.addROI(DateAndTime("2013-01-01T00:00:00"), DateAndTime("2013-01-01T01:00:00"));
    TS_ASSERT_DELTA(dblLog->timeAverageValue(&filter), 10.55, 0.001);
    TS_ASSERT_DELTA(intLog->timeAverageValue(&filter), 5.0, 0.001);

    // Test a filter with two separate ranges, one of which goes past the end of
    // the log
    filter.clear();
    filter.addROI(DateAndTime("2007-11-30T16:17:05"), DateAndTime("2007-11-30T16:17:15"));
    filter.addROI(DateAndTime("2007-11-30T16:17:25"), DateAndTime("2007-11-30T16:17:45"));
    TS_ASSERT_DELTA(dblLog->timeAverageValue(&filter), 9.123, 0.001);
    TS_ASSERT_DELTA(intLog->timeAverageValue(&filter), 3.167, 0.001);

    // Test a filter with two out of order ranges (the second one coming before
    // the first)
    // It should work fine.
    filter.clear();
    filter.addROI(DateAndTime("2007-11-30T16:17:25"), DateAndTime("2007-11-30T16:17:45"));
    filter.addROI(DateAndTime("2007-11-30T16:17:05"), DateAndTime("2007-11-30T16:17:15"));
    TS_ASSERT_DELTA(dblLog->timeAverageValue(&filter), 9.123, 0.001);
    TS_ASSERT_DELTA(intLog->timeAverageValue(&filter), 3.167, 0.001);

    // What about an overlap between the filters? It's odd, but it's allowed.
    filter.clear();
    filter.addROI(DateAndTime("2007-11-30T16:17:05"), DateAndTime("2007-11-30T16:17:15"));
    filter.addROI(DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:20"));
    TS_ASSERT_DELTA(dblLog->timeAverageValue(&filter), (9.99 * 5. + 7.55 * 10.) / 15., 0.001);
    TS_ASSERT_DELTA(intLog->timeAverageValue(&filter), (1. * 5. + 2. * 10.) / 15., 0.001);

    // Check the correct behaviour of empty of single value logs.
    TS_ASSERT(std::isnan(dProp->timeAverageValue(&filter)));
    iProp->addValue(DateAndTime("2010-11-30T16:17:25"), 99);
    TS_ASSERT_EQUALS(iProp->timeAverageValue(&filter), 99.0);

    // Clean up
    delete dblLog;
    delete intLog;
  }

  void test_timeAverageValue() {
    // values are equally spaced in time
    auto dblLog = createDoubleTSP();
    auto intLog = createIntegerTSP(5);

    // average values
    TS_ASSERT_DELTA(dblLog->timeAverageValue(), dblLog->mean(), .0001);
    TS_ASSERT_DELTA(intLog->timeAverageValue(), intLog->mean(), .0001);

    // Clean up
    delete dblLog;
    delete intLog;
  }

  void test_timeAverageValueWithROI() {
    auto dblLog = createDoubleTSP();
    TimeROI *rois = createTimeRoi();
    const double dblMean = dblLog->timeAverageValue(rois);
    delete dblLog; // clean up
    delete rois;
    const double expected = (5.0 * 9.99 + 5.0 * 7.55 + 5.0 * 5.55 + 5.0 * 10.55) / (5.0 + 5.0 + 5.0 + 5.0);
    TS_ASSERT_DELTA(dblMean, expected, .0001);
  }

  void test_averageValueInFilter_throws_for_string_property() {
    TS_ASSERT_THROWS(sProp->timeAverageValue(), const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(sProp->timeAverageValueAndStdDev(), const Exception::NotImplementedError &);
  }

  //----------------------------------------------------------------------------
  void test_statistics() {
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>("MydoubleLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:30", 4));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:40", 5));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:50", 6));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:00", 7));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:10", 8));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:20", 9));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:30", 10));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:18:40", 11));
    TS_ASSERT_EQUALS(log->realSize(), 11);

    TimeSeriesPropertyStatistics stats = log->getStatistics();

    TS_ASSERT_DELTA(stats.minimum, 1.0, 1e-3);
    TS_ASSERT_DELTA(stats.maximum, 11.0, 1e-3);
    TS_ASSERT_DELTA(stats.median, 6.0, 1e-3);
    TS_ASSERT_DELTA(stats.mean, 6.0, 1e-3);
    TS_ASSERT_DELTA(stats.duration, 110.0, 1e-3);
    TS_ASSERT_DELTA(stats.standard_deviation, 3.1622, 1e-3);
    TS_ASSERT_DELTA(log->timeAverageValue(), stats.mean, 1e-3);
    TS_ASSERT_DELTA(stats.time_mean, stats.mean, 1e-3);
    TS_ASSERT_DELTA(stats.time_standard_deviation, stats.standard_deviation, 1e-3);

    delete log;
  }

  // this test is taken from PlotAsymmetryByLogValueTest::test_LogValueFunction
  void test_statistics_excessiveROI() {
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>("MydoubleLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T17:12:34", 178.3));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T17:13:08", 179.4));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T17:13:42", 180.2));

    constexpr double MIN{178.3};
    constexpr double MAX{180.2};
    constexpr double MEDIAN{179.4};
    constexpr double MEAN{(178.3 + 179.4 + 180.2) / 3.};

    // bare stats
    const auto stats_no_roi = log->getStatistics();
    TS_ASSERT_DELTA(stats_no_roi.minimum, MIN, 1e-3);
    TS_ASSERT_DELTA(stats_no_roi.maximum, MAX, 1e-3);
    TS_ASSERT_DELTA(stats_no_roi.median, MEDIAN, 1e-3);
    TS_ASSERT_DELTA(stats_no_roi.mean, MEAN, 1e-3);
    TS_ASSERT_DELTA(stats_no_roi.duration, (136 - 34),
                    1e-3); // last interval is guessed to be same as penultimate interval
    TS_ASSERT_DELTA(log->timeAverageValue(), stats_no_roi.mean, 1e-3);
    TS_ASSERT_DELTA(stats_no_roi.time_mean, 179.3, 1e-3); // calculated by hand

    // this starts 4 seconds before the log does to force checking the durations are done correctly
    // roi duration is 100, but the first 4 seconds should be skipped
    TimeROI roi(DateAndTime("2007-11-30T17:12:30"), DateAndTime("2007-11-30T17:14:10"));

    // bare stats with the wacky TimROI
    const auto stats_roi = log->getStatistics(&roi);
    TS_ASSERT_DELTA(stats_roi.minimum, MIN, 1e-3);
    TS_ASSERT_DELTA(stats_roi.maximum, MAX, 1e-3);
    TS_ASSERT_DELTA(stats_roi.median, MEDIAN, 1e-3);
    TS_ASSERT_DELTA(stats_roi.mean, MEAN, 1e-3);
    TS_ASSERT_DELTA(stats_roi.duration, (130 - 34), 1e-3);
    TS_ASSERT_DELTA(log->timeAverageValue(), stats_roi.mean, 1e-3);
    TS_ASSERT_DELTA(stats_roi.time_mean, 179.24375, 1e-3); // calculated by hand
  }

  void test_empty_statistics() {
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>("MydoubleLog");
    TimeSeriesPropertyStatistics stats = log->getStatistics();
    TS_ASSERT(std::isnan(stats.minimum));
    TS_ASSERT(std::isnan(stats.maximum));
    TS_ASSERT(std::isnan(stats.median));
    TS_ASSERT(std::isnan(stats.mean));
    TS_ASSERT(std::isnan(stats.standard_deviation));
    TS_ASSERT(std::isnan(stats.time_mean));
    TS_ASSERT(std::isnan(stats.time_standard_deviation));
    TS_ASSERT(std::isnan(stats.duration));

    delete log;
  }

  void test_EMU00081100() {
    std::cout << "==============================>\n";
    // this is taken from a log that was showing incorrect behavior
    TimeSeriesProperty<double> log("field_danfysik");
    log.addValue("2018-06-12T23:18:37.000000000", 2289.4013671875);
    log.addValue("2018-06-12T23:18:37.000000000", 2289.4013671875);
    log.addValue("2018-06-12T23:19:07.000000000", 2289.456298828125);
    log.addValue("2018-06-12T23:19:37.000000000", 2289.4013671875);
    log.addValue("2018-06-12T23:20:07.000000000", 2289.4013671875);
    log.addValue("2018-06-12T23:20:38.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:21:08.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:21:39.000000000", 2289.4013671875);
    log.addValue("2018-06-12T23:22:09.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:22:39.000000000", 2289.4013671875);
    log.addValue("2018-06-12T23:23:10.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:23:40.000000000", 2289.456298828125);
    log.addValue("2018-06-12T23:24:11.000000000", 2289.456298828125);
    log.addValue("2018-06-12T23:24:42.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:25:12.000000000", 2289.346435546875);
    log.addValue("2018-06-12T23:25:43.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:26:13.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:26:43.000000000", 2289.456298828125);
    log.addValue("2018-06-12T23:27:14.000000000", 2289.51123046875);
    log.addValue("2018-06-12T23:27:47.000000000", 2289.456298828125);
    constexpr std::size_t NUM_VALS{20};
    TS_ASSERT_EQUALS(log.size(), NUM_VALS);

    // expected value calculated with a parallel implementation in python
    constexpr double TIME_MEAN_EXP{2289.459125}; // previous was 2289.459295};
    // the last duration is faked to be the same as the previous one (46-14)
    constexpr double DURATION_EXP{(27 - 18) * 60. + (47 - 37) + (47 - 14)};
    const auto &values = log.valuesAsVector();
    TS_ASSERT_EQUALS(values.size(), NUM_VALS);
    const double mean = std::accumulate(values.cbegin(), values.cend(), 0.) / double(NUM_VALS);

    // test the statistics against what is expected
    const auto statistics = log.getStatistics();
    TS_ASSERT_DELTA(statistics.mean, mean, 1e-5);
    TS_ASSERT_DELTA(statistics.time_mean, TIME_MEAN_EXP, 1e-5);
    TS_ASSERT_EQUALS(statistics.duration, DURATION_EXP);
    TS_ASSERT_DELTA(log.timeAverageValue(), TIME_MEAN_EXP, 1e-5);
  }

  void test_PlusEqualsOperator_Incompatible_Types_dontThrow() {
    // Adding incompatible types together should not throw, but issue a warning
    // in the log

    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>("MydoubleLog");
    TimeSeriesProperty<int> *logi = new TimeSeriesProperty<int>("MyIntLog");
    PropertyWithValue<double> *val = new PropertyWithValue<double>("MySimpleDouble", 1.23);

    log->operator+=(val);
    log->operator+=(logi);
    logi->operator+=(log);
    val->operator+=(log);
    val->operator+=(logi);

    delete log;
    delete logi;
    delete val;
  }

  void test_LogAtStartOfTime() {
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>("doubleLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("1990-Jan-01 00:00:00", 1));
    TS_ASSERT_THROWS_NOTHING(log->addValue("1990-Jan-01 00:00:10", 2));

    const auto rawstats = log->getStatistics();
    TS_ASSERT_DELTA(rawstats.minimum, 1.0, 1e-3);
    TS_ASSERT_DELTA(rawstats.maximum, 2.0, 1e-3);
    TS_ASSERT_DELTA(rawstats.median, 1.5, 1e-3);
    TS_ASSERT_DELTA(rawstats.mean, 1.5, 1e-3);
    TS_ASSERT_DELTA(rawstats.duration, 20.0, 1e-3);
    TS_ASSERT_DELTA(rawstats.time_mean, 1.5, 1e-3);

    TimeROI roi(DateAndTime("1990-Jan-01 00:00:00"), DateAndTime("1990-Jan-01 00:00:20"));
    const auto filteredstats = log->getStatistics(&roi);
    TS_ASSERT_DELTA(filteredstats.minimum, 1.0, 1e-3);
    TS_ASSERT_DELTA(filteredstats.maximum, 2.0, 1e-3);
    TS_ASSERT_DELTA(filteredstats.median, 1.5, 1e-3);
    TS_ASSERT_DELTA(filteredstats.mean, 1.5, 1e-3);
    TS_ASSERT_DELTA(filteredstats.duration, 20.0, 1e-3);
    TS_ASSERT_DELTA(filteredstats.time_mean, 1.5, 1e-3);
  }

  void test_PlusEqualsOperator_() {
    TimeSeriesProperty<double> *lhs = new TimeSeriesProperty<double>("doubleLog");
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:30", 4));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:40", 5));
    TimeSeriesProperty<double> *rhs = new TimeSeriesProperty<double>("doubleLog");
    TS_ASSERT_THROWS_NOTHING(rhs->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(rhs->addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(rhs->addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(rhs->addValue("2007-11-30T16:17:30", 4));
    TS_ASSERT_THROWS_NOTHING(rhs->addValue("2007-11-30T16:17:40", 5));

    lhs->operator+=(rhs);

    TS_ASSERT_EQUALS(lhs->size(), rhs->size());

    delete lhs;
    delete rhs;
  }

  /*
   * Test include (1) normal interval (2) normal on grid point (3) outside upper
   * boundary
   * (4) outside lower bound
   */
  void test_getSingleValue() {
    TimeSeriesProperty<double> *p = createDoubleTSP();

    DateAndTime time1("2007-11-30T16:17:23");
    double v1 = p->getSingleValue(time1);
    TS_ASSERT_DELTA(v1, 5.55, 1e-6);

    DateAndTime time2("2007-11-30T16:17:03");
    double v2 = p->getSingleValue(time2);
    TS_ASSERT_DELTA(v2, 9.99, 1e-6);

    DateAndTime time3("2007-11-30T16:17:31");
    double v3 = p->getSingleValue(time3);
    TS_ASSERT_DELTA(v3, 10.55, 1e-6);

    DateAndTime time4("2007-11-30T16:17:00");
    double v4 = p->getSingleValue(time4);
    TS_ASSERT_DELTA(v4, 9.99, 1e-6);

    DateAndTime time5("2007-11-30T16:16:59");
    double v5 = p->getSingleValue(time5);
    TS_ASSERT_DELTA(v5, 9.99, 1e-6);

    delete p;
  }

  void test_getSingleValue_emptyPropertyThrows() {
    const TimeSeriesProperty<int> empty("Empty");

    DateAndTime time("2013-01-30T16:17:23");
    TS_ASSERT_THROWS(empty.getSingleValue(time), const std::runtime_error &);
    int i;
    TS_ASSERT_THROWS(empty.getSingleValue(time, i), const std::runtime_error &);
  }

  void test_firstLastTimeValue() {
    TimeSeriesProperty<double> *p = createDoubleTSP();

    Mantid::Types::Core::DateAndTime t0 = p->firstTime();
    Mantid::Types::Core::DateAndTime tf = p->lastTime();

    Mantid::Types::Core::DateAndTime t0c("2007-11-30T16:17:00");
    Mantid::Types::Core::DateAndTime tfc("2007-11-30T16:17:30");

    double v0 = p->firstValue();
    double vf = p->lastValue();

    TS_ASSERT_EQUALS(t0, t0c);
    TS_ASSERT_EQUALS(tf, tfc);

    TS_ASSERT_DELTA(v0, 9.99, 1.0E-8);
    TS_ASSERT_DELTA(vf, 10.55, 1.0E-8);

    delete p;

    return;
  }

  void test_durationInSeconds() {
    TimeSeriesProperty<double> *log = createDoubleTSP();
    TS_ASSERT_DELTA(log->durationInSeconds(), 40.0, 0.1);
    TimeROI *rois = new TimeROI;
    rois->addROI("2007-11-30T16:17:05", "2007-11-30T16:17:15");
    rois->addROI("2007-11-30T16:17:25", "2007-11-30T16:17:35");
    TS_ASSERT_DELTA(log->durationInSeconds(rois), 20.0, 0.1);
  }

  void test_firstLastTimeValue_emptyPropertyThrows() {
    const TimeSeriesProperty<int> empty("Empty");

    TS_ASSERT_THROWS(empty.firstTime(), const std::runtime_error &);
    TS_ASSERT_THROWS(empty.lastTime(), const std::runtime_error &);
    TS_ASSERT_THROWS(empty.firstValue(), const std::runtime_error &);
    TS_ASSERT_THROWS(empty.lastValue(), const std::runtime_error &);
  }

  void test_min_max_value() {
    // Test a double property
    const TimeSeriesProperty<double> *p = createDoubleTSP();
    TS_ASSERT_EQUALS(p->minValue(), 5.55);
    TS_ASSERT_EQUALS(p->maxValue(), 10.55);
    delete p;

    // Test an integer property
    const TimeSeriesProperty<int> *i = createIntegerTSP(8);
    TS_ASSERT_EQUALS(i->minValue(), 1);
    TS_ASSERT_EQUALS(i->maxValue(), 8);
    delete i;

    // Test a string property
    sProp->addValue("2007-11-30T16:17:05", "White");
    sProp->addValue("2007-12-30T16:17:15", "Black");
    sProp->addValue("2008-11-30T16:18:05", "Grey");
    TS_ASSERT_EQUALS(sProp->minValue(), "Black");
    TS_ASSERT_EQUALS(sProp->maxValue(), "White");
  }

  /*
   * Test merge()
   */
  void test_Merge() {
    // 1. Construct p1 and p2
    TimeSeriesProperty<double> *p1 = createDoubleTSP();
    TimeSeriesProperty<double> *p2 = new TimeSeriesProperty<double>("doubleProp2");

    TS_ASSERT_THROWS_NOTHING(p2->addValue("2007-11-30T16:17:05", 19.99));
    TS_ASSERT_THROWS_NOTHING(p2->addValue("2007-11-30T16:17:15", 17.55));
    TS_ASSERT_THROWS_NOTHING(p2->addValue("2007-11-30T16:17:17", 15.55));
    TS_ASSERT_THROWS_NOTHING(p2->addValue("2007-11-30T16:17:35", 110.55));

    // 2. Test
    p1->merge(p2);

    // 3. Verify
    Mantid::Types::Core::DateAndTime t0("2007-11-30T16:17:00");
    Mantid::Types::Core::DateAndTime tf("2007-11-30T16:17:35");
    Mantid::Types::Core::DateAndTime t1("2007-11-30T16:17:05");

    TS_ASSERT_EQUALS(p1->firstTime(), t0);
    TS_ASSERT_EQUALS(p1->lastTime(), tf);

    TS_ASSERT_DELTA(p1->getSingleValue(t0), 9.99, 1.0E-8);
    TS_ASSERT_DELTA(p1->getSingleValue(tf), 110.55, 1.0E-8);
    TS_ASSERT_DELTA(p1->getSingleValue(t1), 19.99, 1.0E-8);

    // -1. Clean
    delete p1;
    delete p2;

    return;
  }

  /*
   * Test setName and getName
   */
  void test_Name() {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");

    std::string propertyname("UnitTest");

    p->setName(propertyname);

    TS_ASSERT_EQUALS(p->name(), propertyname);

    delete p;
  }

  /*
   * Test value()
   */
  void test_Value() {
    TimeSeriesProperty<double> *p = createDoubleTSP();

    std::string pvalue = p->value();
    std::string svalue("2007-Nov-30 16:17:00  9.99\n2007-Nov-30 16:17:10  "
                       "7.55\n2007-Nov-30 16:17:20  5.55\n2007-Nov-30 16:17:30 "
                       " 10.55\n");

    TS_ASSERT_EQUALS(pvalue, svalue);

    delete p;
  }

  /*
   * Test valueAsVector()
   */
  void test_ValueAsVector() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Value as vector
    std::vector<double> values = p->valuesAsVector();

    TS_ASSERT_EQUALS(values.size(), 4);
    for (size_t i = 0; i < 4; i++) {
      TS_ASSERT_DELTA(values[i], static_cast<double>(i) + 1.0, 1.0E-9);
    }

    delete p;

    return;
  }

  /*
   * Test clone
   */
  void test_Clone() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Clone
    TimeSeriesProperty<double> *newp = dynamic_cast<TimeSeriesProperty<double> *>(p->clone());

    // 3. Check
    std::vector<Mantid::Types::Core::DateAndTime> times1 = p->timesAsVector();
    std::vector<double> values1 = p->valuesAsVector();

    std::vector<Mantid::Types::Core::DateAndTime> times2 = newp->timesAsVector();
    std::vector<double> values2 = newp->valuesAsVector();

    TS_ASSERT_EQUALS(times1, times2);

    if (times1.size() == times2.size()) {
      for (size_t i = 0; i < times1.size(); i++) {
        TS_ASSERT_EQUALS(times1[i], times2[i]);
        TS_ASSERT_DELTA(values1[i], values2[i], 1.0E-10);
      }
    }

    // 4. Clean
    delete p;
    delete newp;

    return;
  }

  /*
   * Test cloneWithTimeShift
   */
  void test_cloneWithTimeShift() {
    auto time_series = std::make_unique<TimeSeriesProperty<int>>("IntUnixTest");
    time_series->addValue("2019-02-10T16:17:00", 1);

    auto time_series_small_shift = dynamic_cast<TimeSeriesProperty<int> *>(time_series->cloneWithTimeShift(100.));
    const auto &small_shift_times = time_series_small_shift->timesAsVector();
    TS_ASSERT_EQUALS(small_shift_times[0], DateAndTime("2019-02-10T16:18:40"));

    auto time_series_large_shift = dynamic_cast<TimeSeriesProperty<int> *>(time_series->cloneWithTimeShift(1234.));
    const auto &large_shift_times = time_series_large_shift->timesAsVector();
    TS_ASSERT_EQUALS(large_shift_times[0], DateAndTime("2019-02-10T16:37:34"));

    auto time_series_negative_shift = dynamic_cast<TimeSeriesProperty<int> *>(time_series->cloneWithTimeShift(-1234.));
    const auto &negative_shift_times = time_series_negative_shift->timesAsVector();
    TS_ASSERT_EQUALS(negative_shift_times[0], DateAndTime("2019-02-10T15:56:26"));

    // There is a known issue which can cause cloneWithTimeShift to be called
    // with a large (~9e+9 s) shift. Actual shifting is capped to be ~4.6e+19
    // seconds in DateAndTime::operator+= Typical usage of this method is with
    // shifts in the range tested here.

    delete time_series_small_shift;
    delete time_series_large_shift;
    delete time_series_negative_shift;
  }

  /*
   * Test countSize()
   */
  void test_CountSize() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Check no double entry
    p->countSize();
    TS_ASSERT_EQUALS(p->size(), 4);

    // -1. Clean
    delete p;

    return;
  }

  /*
   * Test isTimeString()
   */
  void test_IsTimeString() {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("Test");

    std::string timestring1("2007-11-30T16:17:00");
    TS_ASSERT(p->isTimeString(timestring1));

    std::string timestring2("2007-11-30 T16:17:00");
    TS_ASSERT(!TimeSeriesProperty<double>::isTimeString(timestring2));

    std::string timestring3("2007U11X30T16a17a00");
    TS_ASSERT(TimeSeriesProperty<double>::isTimeString(timestring3));

    std::string timestring4("2007-11-30T16:I7:00");
    TS_ASSERT(!TimeSeriesProperty<double>::isTimeString(timestring4));

    delete p;

    return;
  }

  void test_clear() {
    boost::scoped_ptr<TimeSeriesProperty<int>> p(new TimeSeriesProperty<int>("aProp"));
    p->addValue("2007-11-30T16:17:00", 1);

    TS_ASSERT_EQUALS(p->size(), 1);
    TS_ASSERT_EQUALS(p->realSize(), 1);

    ITimeSeriesProperty *pi = p.get();
    TS_ASSERT_THROWS_NOTHING(pi->clear());

    TS_ASSERT_EQUALS(p->size(), 0);
    TS_ASSERT_EQUALS(p->realSize(), 0);
  }

  void test_clearOutdated() {
    boost::scoped_ptr<TimeSeriesProperty<int>> p(new TimeSeriesProperty<int>("aProp"));
    p->addValue("2007-11-30T16:17:00", 99);

    ITimeSeriesProperty *pi = p.get();
    TS_ASSERT_THROWS_NOTHING(pi->clearOutdated());
    // No change
    TS_ASSERT_EQUALS(p->size(), 1);
    TS_ASSERT_EQUALS(p->realSize(), 1);
    TS_ASSERT_EQUALS(p->lastValue(), 99);

    DateAndTime t("2007-11-30T15:17:00");
    p->addValue(t, 88);
    TS_ASSERT_EQUALS(p->size(), 2);

    TS_ASSERT_THROWS_NOTHING(pi->clearOutdated());
    TS_ASSERT_EQUALS(p->size(), 1);
    TS_ASSERT_EQUALS(p->realSize(), 1);
    // Note that it kept the last-added entry even though its time is earlier
    TS_ASSERT_EQUALS(p->lastTime(), t);
    TS_ASSERT_EQUALS(p->firstValue(), 88);

    TimeSeriesProperty<double> pp("empty");
    TS_ASSERT_THROWS_NOTHING(pp.clearOutdated());
    // No change
    TS_ASSERT_EQUALS(pp.size(), 0);
    TS_ASSERT_EQUALS(pp.realSize(), 0);
  }

  //----------------------------------------------------------------------------------------------
  /** Test 2 create() functions by creating 3 properties in different
   * approaches.
   */
  void test_Create() {
    // Create property by add 4 entries
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Create method 1
    std::vector<Mantid::Types::Core::DateAndTime> times;
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.emplace_back(1.00);
    values.emplace_back(3.00);
    values.emplace_back(2.00);
    values.emplace_back(4.00);

    TimeSeriesProperty<double> *p1 = new TimeSeriesProperty<double>("Property2");
    p1->create(times, values);

    TS_ASSERT_EQUALS(p->size(), p1->size());
    if (p->size() == p1->size()) {
      std::vector<Mantid::Types::Core::DateAndTime> times0 = p->timesAsVector();
      std::vector<Mantid::Types::Core::DateAndTime> times1 = p1->timesAsVector();
      for (size_t i = 0; i < static_cast<size_t>(p->size()); i++) {
        TS_ASSERT_EQUALS(times0[i], times1[i]);
        TS_ASSERT_DELTA(p->getSingleValue(times0[i]), p1->getSingleValue(times1[i]), 1.0E-9);
      }
    }

    // 3 Create method 2
    Mantid::Types::Core::DateAndTime tStart("2007-11-30T16:17:00");
    std::vector<double> deltaTs;
    std::vector<double> valueXs;

    for (int i = 0; i < 4; i++) {
      deltaTs.emplace_back(static_cast<double>(i) * 10.0);
      valueXs.emplace_back(static_cast<double>(i) + 1.0);
    }

    TimeSeriesProperty<double> *p2 = new TimeSeriesProperty<double>("Property4");
    p2->create(tStart, deltaTs, valueXs);

    TS_ASSERT_EQUALS(p->size(), p2->size());
    if (p->size() == p2->size()) {
      std::vector<Mantid::Types::Core::DateAndTime> times0 = p->timesAsVector();
      std::vector<Mantid::Types::Core::DateAndTime> times1 = p2->timesAsVector();
      for (size_t i = 0; i < static_cast<size_t>(p->size()); i++) {
        TS_ASSERT_EQUALS(times0[i], times1[i]);
        TS_ASSERT_DELTA(p->getSingleValue(times0[i]), p2->getSingleValue(times1[i]), 1.0E-9);
      }
    }

    // -1. Clean
    delete p1;
    delete p;
    delete p2;

    return;
  }

  /*
   * Test time_tValue()
   */
  void test_timeTValue() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. What is correct
    std::vector<std::string> correctS{"2007-Nov-30 16:17:00 1", "2007-Nov-30 16:17:10 2", "2007-Nov-30 16:17:20 3",
                                      "2007-Nov-30 16:17:30 4"};

    // 3. Check
    std::vector<std::string> tvalues = p->time_tValue();
    TS_ASSERT_EQUALS(tvalues.size(), 4);

    for (size_t i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(correctS[i], tvalues[i]);
    }

    // -1. Clean
    delete p;

    return;
  }

  /*
   * Test valueAsCorrectMap()
   */
  void test_valueAsCorrectMap() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 1.99)); // this one is ignored
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Get map
    std::map<Mantid::Types::Core::DateAndTime, double> tmap = p->valueAsCorrectMap();

    // 3. Check
    std::vector<Mantid::Types::Core::DateAndTime> times;
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.emplace_back(1.00);
    values.emplace_back(2.00);
    values.emplace_back(3.00);
    values.emplace_back(4.00);

    std::map<Mantid::Types::Core::DateAndTime, double>::iterator tit;
    size_t index = 0;
    for (tit = tmap.begin(); tit != tmap.end(); ++tit) {
      TS_ASSERT_EQUALS(tit->first, times[index]);
      TS_ASSERT_DELTA(tit->second, values[index], 1.0E-9);
      index++;
    }

    // -1 Clean
    delete p;

    return;
  }

  void test_valueAsMultiMap() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 1.99));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Get multimap
    std::multimap<Mantid::Types::Core::DateAndTime, double> tmap = p->valueAsMultiMap();

    // 3. Check
    std::vector<Mantid::Types::Core::DateAndTime> times;
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.emplace_back(1.00);
    values.emplace_back(1.99);
    values.emplace_back(2.00);
    values.emplace_back(3.00);
    values.emplace_back(4.00);

    size_t index = 0;
    for (auto it = tmap.begin(); it != tmap.end(); ++it) {
      TS_ASSERT_EQUALS(it->first, times[index]);
      TS_ASSERT_DELTA(it->second, values[index], 1.0E-9);
      index++;
    }

    // -1 Clean
    delete p;
  }

  /* Test method valueAsVector
   *
   */
  void test_valueAsVector() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:15", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:25", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Get map
    std::map<Mantid::Types::Core::DateAndTime, double> tmap = p->valueAsMap();

    // 3. Check
    std::vector<Mantid::Types::Core::DateAndTime> times;
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:15"));
    times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.emplace_back(1.00);
    values.emplace_back(2.00);
    values.emplace_back(3.00);
    values.emplace_back(4.00);

    std::map<Mantid::Types::Core::DateAndTime, double>::iterator tit;
    size_t index = 0;
    for (tit = tmap.begin(); tit != tmap.end(); ++tit) {
      TS_ASSERT_EQUALS(tit->first, times[index]);
      TS_ASSERT_DELTA(tit->second, values[index], 1.0E-9);
      index++;
    }

    // -1 Clean
    delete p;

    return;
  }

  /*
   * Test valueAsMap()
   */
  void test_valueAsMap() {
    // 1. Create property
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:25", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:18", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Get map
    std::map<Mantid::Types::Core::DateAndTime, double> tmap = p->valueAsMap();

    // 3. Check
    TS_ASSERT_EQUALS(tmap.size(), 4);

    if (tmap.size() == 4) {
      std::vector<Mantid::Types::Core::DateAndTime> times;
      times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
      times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
      times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
      times.emplace_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
      std::vector<double> values;
      values.emplace_back(1.00);
      values.emplace_back(2.00);
      values.emplace_back(3.00);
      values.emplace_back(4.00);

      std::map<Mantid::Types::Core::DateAndTime, double>::iterator tit;
      size_t index = 0;
      for (tit = tmap.begin(); tit != tmap.end(); ++tit) {
        TS_ASSERT_EQUALS(tit->first, times[index]);
        TS_ASSERT_DELTA(tit->second, values[index], 1.0E-9);
        index++;
      }
    }

    // -1 Clean
    delete p;

    return;
  }

  /*
   * Test nth Time
   */
  void test_nthTime() {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");

    // 1. Test Throws
    TS_ASSERT_THROWS(p->nthTime(1), const std::runtime_error &);

    // 2. Add entries
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 3. Test with term
    Mantid::Types::Core::DateAndTime t0 = p->nthTime(0);
    Mantid::Types::Core::DateAndTime t0c("2007-11-30T16:17:00");
    TS_ASSERT_EQUALS(t0, t0c);

    Mantid::Types::Core::DateAndTime t2 = p->nthTime(2);
    Mantid::Types::Core::DateAndTime t2c("2007-11-30T16:17:20");
    TS_ASSERT_EQUALS(t2, t2c);

    Mantid::Types::Core::DateAndTime t3 = p->nthTime(3);
    Mantid::Types::Core::DateAndTime t3c("2007-11-30T16:17:30");
    TS_ASSERT_EQUALS(t3, t3c);

    Mantid::Types::Core::DateAndTime t100 = p->nthTime(100);
    Mantid::Types::Core::DateAndTime t100c("2007-11-30T16:17:30");
    TS_ASSERT_EQUALS(t100, t100c);

    // 4. Double time
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    t3 = p->nthTime(3);
    TS_ASSERT_EQUALS(t3, t2c);

    // -1. Clean
    delete p;

    return;
  }

  /*
   * Test nthInterval()
   */
  void test_nthInterval() {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");

    // 1. Test Throws
    TS_ASSERT_THROWS(p->nthInterval(0), const std::runtime_error &);

    // 2. Add entries
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:05", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:15", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:55", 5.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:35", 4.00));

    // 3. Test
    Mantid::Kernel::TimeInterval dt0 = p->nthInterval(0);
    TS_ASSERT_EQUALS(dt0.start(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(dt0.stop(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:05"));

    Mantid::Kernel::TimeInterval dt1 = p->nthInterval(1);
    TS_ASSERT_EQUALS(dt1.start(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:05"));
    TS_ASSERT_EQUALS(dt1.stop(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:15"));

    Mantid::Kernel::TimeInterval dt2 = p->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.start(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:15"));
    TS_ASSERT_EQUALS(dt2.stop(), Mantid::Types::Core::DateAndTime("2007-11-30T16:17:35"));

    // -1 Clean
    delete p;

    return;
  }

  void test_duplicateTimes() {
    // emulate duplicates from the MDNorm test(s) with HYS_13656-13658
    const DateAndTime time_first("2013-Jan-13 19:36:16.837000000");
    const DateAndTime time_last("2013-Jan-13 19:36:21.900000202");
    TimeSeriesProperty<double> prop("s1");
    prop.addValue(time_first, -0.001722);
    prop.addValue("2013-Jan-13 19:36:17.290000009", 0.004401);
    prop.addValue("2013-Jan-13 19:36:17.400000023", 0.010716);
    prop.addValue("2013-Jan-13 19:36:17.508999990", 0.016265);
    prop.addValue("2013-Jan-13 19:36:17.727999972", 0.021814);
    prop.addValue("2013-Jan-13 19:36:17.727999972", 0.027172);
    prop.addValue("2013-Jan-13 19:36:17.947000014", 0.032721);
    prop.addValue("2013-Jan-13 19:36:17.947000014", 0.037887);
    prop.addValue("2013-Jan-13 19:36:18.180999981", 0.043437);
    prop.addValue("2013-Jan-13 19:36:18.353000032", 0.052813);
    prop.addValue("2013-Jan-13 19:36:18.353000032", 0.058171);
    prop.addValue("2013-Jan-13 19:36:18.509000050", 0.063911);
    prop.addValue("2013-Jan-13 19:36:18.680999981", 0.069269);
    prop.addValue("2013-Jan-13 19:36:18.680999981", 0.074627);
    prop.addValue("2013-Jan-13 19:36:18.915000068", 0.080367);
    prop.addValue("2013-Jan-13 19:36:18.915000068", 0.085917);
    prop.addValue("2013-Jan-13 19:36:19.165000068", 0.092231);
    prop.addValue("2013-Jan-13 19:36:19.306000101", 0.100459);
    prop.addValue("2013-Jan-13 19:36:19.306000101", 0.1062);
    prop.addValue("2013-Jan-13 19:36:19.524999963", 0.111749);
    prop.addValue("2013-Jan-13 19:36:19.524999963", 0.117107);
    prop.addValue("2013-Jan-13 19:36:19.744000064", 0.122847);
    prop.addValue("2013-Jan-13 19:36:19.744000064", 0.128205);
    prop.addValue("2013-Jan-13 19:36:19.962000000", 0.133754);
    prop.addValue("2013-Jan-13 19:36:19.962000000", 0.139112);
    prop.addValue("2013-Jan-13 19:36:20.149999963", 0.144661);
    prop.addValue("2013-Jan-13 19:36:20.353000032", 0.153081);
    prop.addValue("2013-Jan-13 19:36:20.353000032", 0.158821);
    prop.addValue("2013-Jan-13 19:36:20.571999895", 0.16437);
    prop.addValue("2013-Jan-13 19:36:20.571999895", 0.169537);
    prop.addValue("2013-Jan-13 19:36:20.696999895", 0.175086);
    prop.addValue("2013-Jan-13 19:36:20.900000202", 0.180827);
    prop.addValue("2013-Jan-13 19:36:20.900000202", 0.186376);
    prop.addValue("2013-Jan-13 19:36:21.197000133", 0.191925);
    prop.addValue("2013-Jan-13 19:36:21.197000133", 0.20111);
    prop.addValue("2013-Jan-13 19:36:21.430999862", 0.20685);
    prop.addValue("2013-Jan-13 19:36:21.430999862", 0.213165);
    prop.addValue("2013-Jan-13 19:36:21.572000133", 0.218714);
    prop.addValue("2013-Jan-13 19:36:21.775000202", 0.224072);
    prop.addValue("2013-Jan-13 19:36:21.775000202", 0.229621);
    prop.addValue(time_last, 0.235936);

    // the log is built as a constant ramp up
    const double MIN_VALUE = prop.valuesAsVector().front();
    const double MAX_VALUE = prop.valuesAsVector().back();
    constexpr std::size_t SIZE_ORIG = 1928 - 1888 + 1;   // difference in line numbers above
    constexpr std::size_t SIZE_REDUCED = SIZE_ORIG - 15; // repeats counted by hand
    // there is a fake duration tacked on which is the last non-zero duration beore it
    const double DURATION = DateAndTime::secondsFromDuration(time_last - time_first) + (21.900000202 - 21.775000202);

    const auto statsOrig = prop.getStatistics();
    TS_ASSERT_EQUALS(statsOrig.minimum, MIN_VALUE);
    TS_ASSERT_EQUALS(statsOrig.maximum, MAX_VALUE);
    TS_ASSERT_EQUALS(statsOrig.duration, DURATION);
    TS_ASSERT_EQUALS(prop.size(), SIZE_ORIG);
    TS_ASSERT_EQUALS(prop.valuesAsVector().size(), SIZE_ORIG);

    // remove duplicates
    prop.eliminateDuplicates();
    const auto statsReduced = prop.getStatistics();
    TS_ASSERT_EQUALS(statsReduced.minimum, MIN_VALUE);
    TS_ASSERT_EQUALS(statsReduced.maximum, MAX_VALUE);
    TS_ASSERT_EQUALS(statsReduced.duration, DURATION);
    TS_ASSERT_EQUALS(prop.size(), SIZE_REDUCED);
    TS_ASSERT_EQUALS(prop.valuesAsVector().size(), SIZE_REDUCED);

    // time average mean should be unchanged
    TS_ASSERT_EQUALS(statsReduced.time_mean, statsOrig.time_mean);
  }

  /*
   * Test getMemorySize()
   * Note that this will be same with new container
   */
  void test_getMemorySize() {
    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("doubleProp");

    size_t memsize = p->getMemorySize();
    TS_ASSERT_EQUALS(memsize, 0);

    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    memsize = p->getMemorySize();
    TS_ASSERT_EQUALS(memsize, 64);

    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:27:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:27:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:27:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:27:30", 4.00));

    memsize = p->getMemorySize();
    TS_ASSERT_EQUALS(memsize, 128);

    delete p;

    return;
  }

  void test_filter_by_first_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", expectedFilteredValue);
    series.addValue("2000-11-30T01:01:02", 2);

    const double actualFilteredValue = series.extractStatistic(Mantid::Kernel::Math::FirstValue);
    TSM_ASSERT_EQUALS("Filtering by FirstValue is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_last_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue);

    const double actualFilteredValue = series.extractStatistic(Mantid::Kernel::Math::LastValue);
    TSM_ASSERT_EQUALS("Filtering by LastValue is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_minimum_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 3);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // minimum. 1 < 3 < 4
    series.addValue("2000-11-30T01:01:03", 4);

    const double actualFilteredValue = series.extractStatistic(Mantid::Kernel::Math::Minimum);
    TSM_ASSERT_EQUALS("Filtering by Minimum is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_maximum_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0.1);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // maximum. 1 > 0.9 > 0.1
    series.addValue("2000-11-30T01:01:03", 0.9);

    const double actualFilteredValue = series.extractStatistic(Mantid::Kernel::Math::Maximum);
    TSM_ASSERT_EQUALS("Filtering by Maximum is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_mean_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // time series mean = value at T =
                                            // (T1 + T2 + T3) / 3
    series.addValue("2000-11-30T01:01:03", 2);

    const double actualFilteredValue = series.extractStatistic(Mantid::Kernel::Math::Mean);
    TSM_ASSERT_EQUALS("Filtering by Mean Time is not working.", expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_median() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 2;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", 1);
    series.addValue("2000-11-30T01:01:03",
                    expectedFilteredValue); // Median time.
    series.addValue("2000-11-30T01:01:04", 4);
    series.addValue("2000-11-30T01:02:00", 5);

    const double actualFilteredValue = series.extractStatistic(Mantid::Kernel::Math::Median);
    TSM_ASSERT_EQUALS("Filtering by Median Time is not working.", expectedFilteredValue, actualFilteredValue);
  }

  //----------------------------------------------------------------------------

  /** A test for filter nothing
   */
  void test_filterByTime_out_of_range_filters_nothing() {
    TimeSeriesProperty<int> *log = createIntegerTSP(6);

    size_t original_size = std::size_t(log->realSize());

    TS_ASSERT_EQUALS(original_size, 6);

    DateAndTime start = DateAndTime("2007-11-30T15:00:00"); // Much earlier than first time series value
    DateAndTime stop = DateAndTime("2007-11-30T17:00:00");  // Much later than last time series value

    TimeROI roi(start, stop);

    TSM_ASSERT_EQUALS("Shouldn't be filtering anything!", original_size, log->filteredValuesAsVector(&roi).size());

    delete log;
  }

  void test_getSplittingIntervals_noFilter() {
    const auto &log = getTestLog(); // no filter
    const auto &intervals = log->getTimeIntervals();
    TS_ASSERT_EQUALS(intervals.size(), 1);
    const auto &range = intervals.front();
    TS_ASSERT_EQUALS(range.start(), log->firstTime());

    // the range is extended by the last difference in times
    // this is to make the last value count as much as the penultimate
    const auto lastDuration = log->nthInterval(log->size() - 1).length();
    const auto stop = log->lastTime() + lastDuration;
    TS_ASSERT_EQUALS(range.stop(), stop);
  }

  void test_negativeTimes() {
    using namespace Mantid::Types::Core;
    TimeSeriesProperty<double> series("doubleProperty");
    const DateAndTime startTime(100000, 0);
    const std::vector<double> times{-5000, -1, 0, 1, 5};
    const std::vector<double> values{1, 1, 1, 1, 1};
    series.create(startTime, times, values);
    TS_ASSERT_EQUALS(times.size(), series.size());
    TS_ASSERT_EQUALS(values.size(), series.valuesAsVector().size());
    const std::vector<DateAndTime> timesAsVector = series.timesAsVector();
    for (size_t i = 0; i < times.size(); i++) {
      TS_ASSERT_EQUALS(startTime + times[i], timesAsVector[i]);
    }
  }

private:
  /// Generate a test log
  std::unique_ptr<TimeSeriesProperty<double>> getTestLog() {
    // Build the log
    auto log = std::make_unique<TimeSeriesProperty<double>>("DoubleLog");
    Mantid::Types::Core::DateAndTime logTime("2007-11-30T16:17:00");
    const double incrementSecs(10.0);
    for (int i = 1; i < 12; ++i) {
      const double val = static_cast<double>(i);
      log->addValue(logTime.toISO8601String(), val);
      logTime += incrementSecs;
    }
    return log;
  }

  TimeSeriesProperty<int> *iProp;
  TimeSeriesProperty<double> *dProp;
  TimeSeriesProperty<std::string> *sProp;
};
