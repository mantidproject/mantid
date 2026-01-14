// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/LogManager.h"
#include "MantidFrameworkTestHelpers/NexusTestHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <initializer_list>
#include <json/value.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Types::Core::DateAndTime;

// Helper class
namespace {
class ConcreteProperty : public Property {
public:
  ConcreteProperty() : Property("Test", typeid(int)) {}
  ConcreteProperty(std::string name) : Property(name, typeid(int)) {}
  ConcreteProperty *clone() const override { return new ConcreteProperty(*this); }
  bool isDefault() const override { return true; }
  std::string getDefault() const override { return "getDefault() is not implemented in this class"; }
  std::string value() const override { return m_value; }
  Json::Value valueAsJson() const override { return Json::Value(); }
  std::string setValue(const std::string &value) override {
    m_value = value;
    return m_value;
  }
  std::string setValueFromJson(const Json::Value &) override { return ""; }
  std::string setValueFromProperty(const Property &) override { return ""; }
  std::string setDataItem(const std::shared_ptr<DataItem> &) override { return ""; }
  Property &operator+=(Property const *) override { return *this; }

private:
  std::string m_value = "Nothing";
};

template <typename T> void addTestPropertyWithValue(LogManager &run, const std::string &name, const T &value) {
  auto singleValue = new PropertyWithValue<T>(name, value);
  run.addProperty(singleValue);
}

void addTestTimeSeriesFilter(LogManager &run, const std::string &name) {
  auto timeSeries = new TimeSeriesProperty<bool>(name);
  timeSeries->addValue("2012-07-19T16:17:00", true);
  timeSeries->addValue("2012-07-19T16:17:10", true);
  timeSeries->addValue("2012-07-19T16:17:20", true);
  timeSeries->addValue("2012-07-19T16:17:30", true);
  timeSeries->addValue("2012-07-19T16:17:40", false);
  timeSeries->addValue("2012-07-19T16:17:50", false);
  timeSeries->addValue("2012-07-19T16:18:00", true);
  timeSeries->addValue("2012-07-19T16:18:10", true);
  timeSeries->addValue("2012-07-19T16:19:20", true);
  timeSeries->addValue("2012-07-19T16:19:20", true);
  run.addProperty(timeSeries);
}

template <typename T> void addTestTimeSeries(LogManager &run, const std::string &name) {
  auto timeSeries = new TimeSeriesProperty<T>(name);
  timeSeries->addValue("2012-07-19T16:17:00", 2);
  timeSeries->addValue("2012-07-19T16:17:10", 3);
  timeSeries->addValue("2012-07-19T16:17:20", 4);
  timeSeries->addValue("2012-07-19T16:17:30", 5);
  timeSeries->addValue("2012-07-19T16:17:40", 6);
  timeSeries->addValue("2012-07-19T16:17:50", 20);
  timeSeries->addValue("2012-07-19T16:18:00", 21);
  timeSeries->addValue("2012-07-19T16:18:10", 22);
  timeSeries->addValue("2012-07-19T16:19:20", 23);
  timeSeries->addValue("2012-07-19T16:19:20", 24); // effectively replaces the 23 for time average
  run.addProperty(timeSeries);
}
} // namespace

void addTimeSeriesEntry(LogManager &runInfo, const std::string &name, double val) {
  TimeSeriesProperty<double> *tsp;
  tsp = new TimeSeriesProperty<double>(name);
  tsp->addValue("2011-05-24T00:00:00", val);
  runInfo.addProperty(tsp);
}

class LogManagerTest : public CxxTest::TestSuite {
public:
  void testAddGetData() {
    LogManager runInfo;

    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING(runInfo.addProperty(p));

    Property *pp = nullptr;
    TS_ASSERT_THROWS_NOTHING(pp = runInfo.getProperty("Test"));
    TS_ASSERT_EQUALS(p, pp);
    TS_ASSERT(!pp->name().compare("Test"));
    TS_ASSERT(dynamic_cast<ConcreteProperty *>(pp));
    TS_ASSERT_THROWS(pp = runInfo.getProperty("NotThere"), const Exception::NotFoundError &);

    std::vector<Property *> props = runInfo.getProperties();
    TS_ASSERT(!props.empty());
    TS_ASSERT_EQUALS(props.size(), 1);
    TS_ASSERT(!props[0]->name().compare("Test"));
    TS_ASSERT(dynamic_cast<ConcreteProperty *>(props[0]));
  }

  void testRemoveLogData() {
    LogManager runInfo;

    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING(runInfo.addProperty(p));
    TS_ASSERT_THROWS_NOTHING(runInfo.removeProperty("Test"));
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 0);
  }

  void testStartTime() {
    LogManager runInfo;
    // Nothing there yet
    TS_ASSERT_THROWS(runInfo.startTime(), const std::runtime_error &);
    // Add run_start and see that get picked up
    const std::string run_start("2013-12-19T13:38:00");
    auto run_start_prop = new PropertyWithValue<std::string>("run_start", run_start);
    runInfo.addProperty(run_start_prop);
    TS_ASSERT_EQUALS(runInfo.startTime(), DateAndTime(run_start));
    // Add start_time and see that get picked up in preference
    const std::string start_time("2013-12-19T13:40:00");
    auto start_time_prop = new PropertyWithValue<std::string>("start_time", start_time);
    runInfo.addProperty(start_time_prop);
    TS_ASSERT_EQUALS(runInfo.startTime(), DateAndTime(start_time));
    // But get back run_start again if start_time is equal to the epoch
    const std::string epoch("1990-01-01T00:00:00");
    start_time_prop->setValue(epoch);
    TS_ASSERT_EQUALS(runInfo.startTime(), DateAndTime(run_start));
    // And back to failure if they're both that
    run_start_prop->setValue(epoch);
    TS_ASSERT_THROWS(runInfo.startTime(), const std::runtime_error &);

    // Set run_start back to valid value and make start_time contain nonsense
    run_start_prop->setValue(run_start);
    start_time_prop->setValue("__");
    TS_ASSERT_EQUALS(runInfo.startTime(), DateAndTime(run_start));
    // Now make start_time a completely different property type
    runInfo.removeProperty("start_time");
    runInfo.addProperty(new PropertyWithValue<double>("start_time", 3.33));
    TS_ASSERT_EQUALS(runInfo.startTime(), DateAndTime(run_start));
    // Now make run_start something invalid
    run_start_prop->setValue("notADate");
    TS_ASSERT_THROWS(runInfo.startTime(), const std::runtime_error &);
    // And check things if it's the wrong property type
    runInfo.removeProperty("run_start");
    addTimeSeriesEntry(runInfo, "run_start", 4.44);
    TS_ASSERT_THROWS(runInfo.startTime(), const std::runtime_error &);
  }

  void testEndTime() {
    LogManager runInfo;
    // Nothing there yet
    TS_ASSERT_THROWS(runInfo.endTime(), const std::runtime_error &);
    // Proton Charge log with only one entry
    addTimeSeriesEntry(runInfo, "proton_charge", 78.9);
    TS_ASSERT_EQUALS(runInfo.endTime(), DateAndTime("2011-05-24T00:00:00"));
    runInfo.removeProperty("proton_charge");
    // Proton Charge log with multiple entries
    addTestTimeSeries<double>(runInfo, "proton_charge");
    TS_ASSERT_EQUALS(runInfo.endTime(), DateAndTime("2012-07-19T16:19:20"));
    // Add run_end and see that get picked up
    const std::string run_end("2013-12-19T13:38:00");
    auto run_end_prop = new PropertyWithValue<std::string>("run_end", run_end);
    runInfo.addProperty(run_end_prop);
    TS_ASSERT_EQUALS(runInfo.endTime(), DateAndTime(run_end));
    // Add end_time and see that get picked up in preference
    const std::string end_time("2013-12-19T13:40:00");
    auto end_time_prop = new PropertyWithValue<std::string>("end_time", end_time);
    runInfo.addProperty(end_time_prop);
    TS_ASSERT_EQUALS(runInfo.endTime(), DateAndTime(end_time));

    // Remove proton charge log to make sure endTime() throws correct exceptions
    runInfo.removeProperty("proton_charge");
    // Set run_end back to valid value and make end_time contain nonsense
    run_end_prop->setValue(run_end);
    end_time_prop->setValue("__");
    TS_ASSERT_EQUALS(runInfo.endTime(), DateAndTime(run_end));
    // Now make end_time a completely different property type
    runInfo.removeProperty("end_time");
    runInfo.addProperty(new PropertyWithValue<double>("end_time", 3.33));
    TS_ASSERT_EQUALS(runInfo.endTime(), DateAndTime(run_end));
    // Now make run_end something invalid
    run_end_prop->setValue("notADate");
    TS_ASSERT_THROWS(runInfo.endTime(), const std::runtime_error &);
    // And check things if it's the wrong property type
    runInfo.removeProperty("run_end");
    addTimeSeriesEntry(runInfo, "run_end", 4.44);
    TS_ASSERT_THROWS(runInfo.endTime(), const std::runtime_error &);
  }

  void testMemory() {
    LogManager runInfo;
    TS_ASSERT_EQUALS(runInfo.getMemorySize(), 0);

    Property *p = new ConcreteProperty();
    runInfo.addProperty(p);

    TS_ASSERT_EQUALS(runInfo.getMemorySize(), p->getMemorySize() + sizeof(Property *));
  }

  void test_GetTimeSeriesProperty_Returns_TSP_When_Log_Exists() {
    LogManager runInfo;
    const std::string &name = "double_time_series";
    const double value = 10.9;
    addTimeSeriesEntry(runInfo, name, value);

    TimeSeriesProperty<double> *tsp(nullptr);
    TS_ASSERT_THROWS_NOTHING(tsp = runInfo.getTimeSeriesProperty<double>(name));
    TS_ASSERT_DELTA(tsp->firstValue(), value, 1e-12);
  }

  void test_GetTimeSeriesProperty_Throws_When_Log_Does_Not_Exist() {
    LogManager runInfo;
    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>("not_a_log"), const Exception::NotFoundError &);
  }

  void test_GetTimeSeriesProperty_Throws_When_Log_Exists_But_Is_Not_Correct_Type() {
    LogManager runInfo;
    const std::string &name = "double_prop";
    runInfo.addProperty(name, 5.6); // Standard double property

    TS_ASSERT_THROWS(runInfo.getTimeSeriesProperty<double>(name), const std::invalid_argument &);
  }

  void test_GetPropertyAsType_Throws_When_Property_Does_Not_Exist() {
    LogManager runInfo;
    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<double>("not_a_log"), const Exception::NotFoundError &);
  }

  void test_GetPropertyAsType_Returns_Expected_Value_When_Type_Is_Correct() {
    LogManager runInfo;
    const std::string &name = "double_prop";
    const double value = 5.6;
    runInfo.addProperty(name, value); // Standard double property

    double retrieved(0.0);
    TS_ASSERT_THROWS_NOTHING(retrieved = runInfo.getPropertyValueAsType<double>(name));
    TS_ASSERT_DELTA(retrieved, value, 1e-12);
  }

  void test_GetPropertyAsType_Throws_When_Requested_Type_Does_Not_Match() {
    LogManager runInfo;
    runInfo.addProperty("double_prop", 6.7); // Standard double property

    TS_ASSERT_THROWS(runInfo.getPropertyValueAsType<int>("double_prop"), const std::invalid_argument &);
  }

  void test_GetPropertyAsSingleValue_SingleValue_DoubleType() {
    doTest_GetPropertyAsSingleValue_SingleType<double>(1.0);
  }

  void test_GetPropertyAsSingleValue_SingleValue_FloatType() {
    doTest_GetPropertyAsSingleValue_SingleType<float>(1.0F);
  }

  void test_GetPropertyAsSingleValue_SingleValue_Int32Type() { doTest_GetPropertyAsSingleValue_SingleType<int32_t>(1); }

  void test_GetPropertyAsSingleValue_SingleValue_Int64Type() {
    doTest_GetPropertyAsSingleValue_SingleType<int64_t>(1L);
  }

  void test_GetPropertyAsSingleValue_SingleValue_Uint32Type() {
    doTest_GetPropertyAsSingleValue_SingleType<uint32_t>(1U);
  }

  void test_GetPropertyAsSingleValue_SingleValue_Uint64Type() {
    doTest_GetPropertyAsSingleValue_SingleType<uint64_t>(1UL);
  }

  void test_GetPropertyAsSingleValue_SingleValue_StringType() {
    LogManager runInfo;
    const std::string name = "string_prop", value = "1";
    runInfo.addProperty<std::string>(name, value);
    double result = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT_THROWS_NOTHING(result = runInfo.getPropertyAsSingleValue(name));
    TS_ASSERT_DELTA(1.0, result, 1e-12);
  }

  void test_GetPropertyAsIntegerValue_SingleValue_Int32Type() { doTest_GetPropertyAsIntegerValue<int32_t>(1); }

  void test_GetPropertyAsIntegerValue_SingleValue_Int64Type() { doTest_GetPropertyAsIntegerValue<int64_t>(1L); }

  void test_GetPropertyAsIntegerValue_SingleValue_Uint32Type() { doTest_GetPropertyAsIntegerValue<uint32_t>(1U); }

  void test_GetPropertyAsIntegerValue_SingleValue_Uint64Type() { doTest_GetPropertyAsIntegerValue<uint64_t>(1UL); }

  void test_GetPropertyAsSingleInteger_DoubleType_Throws() {
    LogManager runInfo;
    const std::string name = "T_prop";
    runInfo.addProperty<double>(name, 1.0);
    TS_ASSERT_THROWS(runInfo.getPropertyAsIntegerValue(name), const std::invalid_argument &);
  }

  void test_GetPropertyAsSingleInteger_Throws_for_nonexistant_property() {
    LogManager runInfo;
    TS_ASSERT_THROWS(runInfo.getPropertyAsIntegerValue("T_prop"), const Exception::NotFoundError &);
  }

  void test_GetPropertyAsSingleValue_TimeSeries_DoubleType() {
    doTest_GetPropertyAsSingleValue_TimeSeriesType<double>();
  }

  void test_GetPropertyAsSingleValue_TimeSeries_FloatType() { doTest_GetPropertyAsSingleValue_TimeSeriesType<float>(); }

  void test_GetPropertyAsSingleValue_TimeSeries_Int32Type() {
    doTest_GetPropertyAsSingleValue_TimeSeriesType<int32_t>();
  }

  void test_GetPropertyAsSingleValue_TimeSeries_Int64Type() {
    doTest_GetPropertyAsSingleValue_TimeSeriesType<int64_t>();
  }

  void test_GetPropertyAsSingleValue_TimeSeries_Uint32Type() {
    doTest_GetPropertyAsSingleValue_TimeSeriesType<uint32_t>();
  }

  void test_GetPropertyAsSingleValue_TimeSeries_Uint64Type() {
    doTest_GetPropertyAsSingleValue_TimeSeriesType<uint64_t>();
  }

  void test_GetPropertyAsSingleValue_Throws_If_String_Is_Invalid() {
    LogManager runInfo;
    const std::string name = "string_prop";
    runInfo.addProperty<std::string>(name, "hello"); // not a number

    TS_ASSERT_THROWS(runInfo.getPropertyAsSingleValue(name), const std::invalid_argument &);
  }

  void test_GetPropertyAsSingleValue_Throws_If_Type_Is_Not_Numeric_Or_TimeSeries_Numeric_Or_Valid_String() {
    LogManager runInfo;
    const std::string name = "bool_prop";
    const bool value(false);
    runInfo.addProperty<bool>(name, value); // Adds a bool property

    TS_ASSERT_THROWS(runInfo.getPropertyAsSingleValue(name), const std::invalid_argument &);
  }

  void test_GetPropertyAsSingleValue_Returns_Simple_Mean_By_Default_For_Time_Series() {
    LogManager runInfo;
    const std::string name = "series";
    addTestTimeSeries<double>(runInfo, name);

    const double expectedValue(13.0);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name), expectedValue, 1e-12);
  }

  void test_GetPropertyAsSingleValue_Returns_Correct_SingleValue_For_Each_StatisticType() {
    LogManager runInfo;
    const std::string name = "series";
    addTestTimeSeries<double>(runInfo, name);
    // time average values remain unchanged
    // all values were calculated using a independent implementation in python
    const double FIRST_VALUE{2.}; // also the min
    const double LAST_VALUE{24.}; // also the max
    const double MEAN{13};
    const double TIME_AVG_MEAN{18.2380952348};
    // const double TIME_AVG_STDDEV {8.523975789812294};

    TS_ASSERT_EQUALS(runInfo.getProperty(name)->size(), 10);
    TS_ASSERT_LESS_THAN(0, runInfo.getMemorySize()); // memory is non-zero
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Mean), MEAN, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Minimum), FIRST_VALUE, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Maximum), LAST_VALUE, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::FirstValue), FIRST_VALUE, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::LastValue), LAST_VALUE, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Median), 13.0, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::StdDev), 9.1104335791443, 1e-12);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::TimeAveragedMean), TIME_AVG_MEAN, 1e-8);
    // TODO not ready
    // TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::TimeAverageStdDev), TIME_AVG_STDDEV, 1e-12);

    // the old values are cached so we need clear it to get nw math done
    dynamic_cast<TimeSeriesProperty<double> *>(runInfo.getProperty(name))->eliminateDuplicates();
    runInfo.clearSingleValueCache();

    // have the duplicate values (two values for the same time) removed
    // this will remove the second to last value which will change the mean, median, and stddev
    TS_ASSERT_EQUALS(runInfo.getProperty(name)->size(), 9);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Mean), 11.88888888888889, 1e-12);   // TODO
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Minimum), FIRST_VALUE, 1e-12);      // TODO
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Maximum), LAST_VALUE, 1e-12);       // TODO
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::FirstValue), FIRST_VALUE, 1e-12);   // TODO
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::LastValue), LAST_VALUE, 1e-12);     // TODO
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::Median), 6.0, 1e-12);               // TODO
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::StdDev), 8.937367800973425, 1e-12); // TODO
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::TimeAveragedMean), TIME_AVG_MEAN, 1e-8);
    // TODO not ready
    // TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Math::TimeAverageStdDev), TIME_AVG_STDDEV, 1e-12);
  }

  void test_GetPropertyAsSingleValue_Returns_Expected_Single_Value_On_Successive_Calls_With_Different_Stat_Types() {
    LogManager run;
    const std::string name = "series";
    addTestTimeSeries<double>(run, name);

    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Mean), 13.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Minimum), 2.0);
    TS_ASSERT_EQUALS(run.getPropertyAsSingleValue(name, Math::Minimum), 2.0);
  }

  void test_GetPropertyAsSingleValue_Returns_Correct_Value_On_Second_Call_When_Log_Has_Been_Replaced() {
    LogManager runInfo;
    const std::string name = "double";
    double value(5.1);
    runInfo.addProperty(name, value);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);

    // Replace the log with a different value
    value = 10.3;
    runInfo.addProperty(name, value, /*overwrite*/ true);

    TS_ASSERT_EQUALS(runInfo.getPropertyAsSingleValue(name), value);
  }

  void test_getTimeAveragedStd() {
    LogManager run;
    const std::string name = "series";
    addTestTimeSeries<double>(run, name);

    TS_ASSERT_DELTA(run.getTimeAveragedStd(name), 8.0646, 0.001);
  }

  void test_getTimeAveragedValue() {
    LogManager run;
    const std::string name = "series";
    addTestTimeSeries<double>(run, name);

    TS_ASSERT_DELTA(run.getTimeAveragedValue(name), 18.2380, 0.001);
  }

  void test_getStatistics() {
    LogManager run;

    // local helper function
    auto assertSingleValue = [&](const std::string &name, double value) {
      auto stats = run.getStatistics(name);
      TS_ASSERT_DELTA(stats.minimum, value, 0.001);
      TS_ASSERT_DELTA(stats.maximum, value, 0.001);
      TS_ASSERT_DELTA(stats.mean, value, 0.001);
      TS_ASSERT_DELTA(stats.median, value, 0.001);
      TS_ASSERT_DELTA(stats.standard_deviation, 0.0, 0.001);
      TS_ASSERT_DELTA(stats.time_mean, value, 0.001);
      TS_ASSERT_DELTA(stats.time_standard_deviation, 0.0, 0.001);
      TS_ASSERT(std::isnan(stats.duration));
    };

    // test valid single value property
    // addTestPropertyWithValue<size_t>(run, "single-size_t", 42);
    // assertSingleValue("single-size_t", 42.0);
    addTestPropertyWithValue<int>(run, "single-int", 43);
    assertSingleValue("single-int", 43.0);
    addTestPropertyWithValue<float>(run, "single-float", 44.0);
    assertSingleValue("single-float", 44.0);
    addTestPropertyWithValue<double>(run, "single-double", 45.0);
    assertSingleValue("single-double", 45.0);

    // test invalid single value property
    {
      addTestPropertyWithValue<std::string>(run, "single-string", "46");
      auto stats = run.getStatistics("single-string");
      TS_ASSERT_EQUALS(std::isnan(stats.minimum), true);
      TS_ASSERT_EQUALS(std::isnan(stats.maximum), true);
      TS_ASSERT_EQUALS(std::isnan(stats.mean), true);
      TS_ASSERT_EQUALS(std::isnan(stats.standard_deviation), true);
      TS_ASSERT_EQUALS(std::isnan(stats.time_mean), true);
      TS_ASSERT_EQUALS(std::isnan(stats.time_standard_deviation), true);
      TS_ASSERT_EQUALS(std::isnan(stats.duration), true);
    }

    // test time series
    // addTestTimeSeries<size_t>(run, "series-size_t");
    addTestTimeSeries<int>(run, "series-int");
    addTestTimeSeries<float>(run, "series-float");
    addTestTimeSeries<double>(run, "series-double");
    // for (std::string name : {"series-size_t", "series-int", "series-float", "series-double"}) {
    for (std::string name : {"series-int", "series-float", "series-double"}) {
      auto stats = run.getStatistics(name);
      TS_ASSERT_DELTA(stats.minimum, 2.0, 0.001);
      TS_ASSERT_DELTA(stats.maximum, 24.0, 0.001);
      TS_ASSERT_DELTA(stats.mean, 13.0, 0.001);
      TS_ASSERT_DELTA(stats.median, 13.0, 0.001);
      TS_ASSERT_DELTA(stats.standard_deviation, 9.1104, 0.001);
      TS_ASSERT_DELTA(stats.time_mean, 18.2381, 0.001);
      TS_ASSERT_DELTA(stats.time_standard_deviation, 8.06464, 0.001);
      TS_ASSERT_DELTA(stats.duration, 210.0, 0.001);
    }
  }

  void test_clear() {
    // Set up a Run object with 3 properties in it (1 time series, 2 single
    // value)
    LogManager runInfo;
    const std::string stringProp("aStringProp");
    const std::string stringVal("testing");
    runInfo.addProperty(stringProp, stringVal);
    const std::string intProp("anIntProp");
    runInfo.addProperty(intProp, 99);
    const std::string tspProp("tsp");
    addTestTimeSeries<double>(runInfo, "tsp");

    // Check it's set up right
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 3);
    auto tsp = runInfo.getTimeSeriesProperty<double>(tspProp);
    TS_ASSERT_EQUALS(tsp->realSize(), 10);

    // Do the clearing work
    TS_ASSERT_THROWS_NOTHING(runInfo.clearTimeSeriesLogs());

    // Check the time-series property is empty, but not the others
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 3);
    TS_ASSERT_EQUALS(tsp->realSize(), 0);
    TS_ASSERT_EQUALS(runInfo.getPropertyValueAsType<std::string>(stringProp), stringVal);
    TS_ASSERT_EQUALS(runInfo.getPropertyValueAsType<int>(intProp), 99);
  }

  void clearOutdatedTimeSeriesLogValues() {
    // Set up a Run object with 3 properties in it (1 time series, 2 single
    // value)
    LogManager runInfo;
    const std::string stringProp("aStringProp");
    const std::string stringVal("testing");
    runInfo.addProperty(stringProp, stringVal);
    const std::string intProp("anIntProp");
    runInfo.addProperty(intProp, 99);
    const std::string tspProp("tsp");
    addTestTimeSeries<double>(runInfo, "tsp");

    // Check it's set up right
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 3);
    auto tsp = runInfo.getTimeSeriesProperty<double>(tspProp);
    TS_ASSERT_EQUALS(tsp->realSize(), 10);

    auto lastTime = tsp->lastTime();
    auto lastValue = tsp->lastValue();

    // Do the clearing work
    TS_ASSERT_THROWS_NOTHING(runInfo.clearOutdatedTimeSeriesLogValues());

    // Check the time-series property has 1 entry, & the others are unchanged
    TS_ASSERT_EQUALS(runInfo.getProperties().size(), 3);
    TS_ASSERT_EQUALS(tsp->realSize(), 1);
    TS_ASSERT_EQUALS(tsp->firstTime(), lastTime);
    TS_ASSERT_EQUALS(tsp->firstValue(), lastValue);
    TS_ASSERT_EQUALS(runInfo.getPropertyValueAsType<std::string>(stringProp), stringVal);
    TS_ASSERT_EQUALS(runInfo.getPropertyValueAsType<int>(intProp), 99);
  }

  /** Save and load to NXS file */
  void test_nexus() {
    NexusTestHelper th(true);
    th.createFile("LogManagerTest.nxs");

    LogManager run1;
    addTimeSeriesEntry(run1, "double_series", 45.0);
    run1.addProperty(new PropertyWithValue<int>("int_val", 1234));
    run1.addProperty(new PropertyWithValue<std::string>("string_val", "help_im_stuck_in_a_log_file"));
    run1.addProperty(new PropertyWithValue<double>("double_val", 5678.9));
    addTimeSeriesEntry(run1, "phi", 12.3);
    addTimeSeriesEntry(run1, "chi", 45.6);
    addTimeSeriesEntry(run1, "omega", 78.9);
    addTimeSeriesEntry(run1, "proton_charge", 78.9);

    run1.saveNexus(th.file.get(), "logs");
    th.file->openGroup("logs", "NXgroup");
    th.file->makeGroup("junk_to_ignore", "NXmaterial");
    th.file->makeGroup("more_junk_to_ignore", "NXsample");

    // ---- Now re-load the same and compare ------
    th.reopenFile();
    LogManager run2;
    run2.loadNexus(th.file.get(), "logs", false);
    TS_ASSERT(run2.hasProperty("double_series"));
    TS_ASSERT(run2.hasProperty("int_val"));
    TS_ASSERT(run2.hasProperty("string_val"));
    TS_ASSERT(run2.hasProperty("double_val"));
    // This test both uses the goniometer axes AND looks up some values.

    // Reload without opening the group (for backwards-compatible reading of old
    // files)
    LogManager run3;
    th.file.get()->openGroup("logs", "NXgroup");
    run3.loadNexus(th.file.get(), "", false);
    TS_ASSERT(run3.hasProperty("double_series"));
    TS_ASSERT(run3.hasProperty("int_val"));
    TS_ASSERT(run3.hasProperty("string_val"));
    TS_ASSERT(run3.hasProperty("double_val"));
  }

  /** Check for loading the old way of saving proton_charge */
  void test_legacy_nexus() {
    NexusTestHelper th(true);
    th.createFile("LogManagerTest.nxs");
    th.file->makeGroup("sample", "NXsample", 1);
    th.file->writeData("proton_charge", 1.234);
    th.reopenFile();
    th.file->openGroup("sample", "NXsample");
    LogManager run3;
    run3.loadNexus(th.file.get(), "", false);
  }

  void test_operator_equals() {
    LogManager a;
    LogManager b;
    a.addProperty(std::make_unique<ConcreteProperty>());
    b.addProperty(std::make_unique<ConcreteProperty>());
    TS_ASSERT_EQUALS(a, b);
    TS_ASSERT(!(a != b));
  }

  void test_not_equals_when_number_of_entries_differ() {
    LogManager a;
    LogManager b;
    a.addProperty(std::make_unique<ConcreteProperty>("a1"));
    b.addProperty(std::make_unique<ConcreteProperty>("b1"));
    b.addProperty(std::make_unique<ConcreteProperty>("b2"));
    TS_ASSERT_DIFFERS(a, b);
    TS_ASSERT(!(a == b));
  }

  void test_not_equals_when_values_differ() {
    LogManager a;
    LogManager b;
    auto prop1 = std::make_unique<ConcreteProperty>();
    auto prop2 = std::make_unique<ConcreteProperty>();
    prop2->setValue("another_value");
    a.addProperty(std::move(prop1));
    b.addProperty(std::move(prop2));
    TS_ASSERT_DIFFERS(a, b);
    TS_ASSERT(!(a == b));
  }

  void test_not_equals_when_keys_differ() {
    LogManager a;
    LogManager b;
    auto prop1 = std::make_unique<ConcreteProperty>("Temp");
    auto prop2 = std::make_unique<ConcreteProperty>("Pressure");
    a.addProperty(std::move(prop1));
    b.addProperty(std::move(prop2));
    TS_ASSERT_DIFFERS(a, b);
    TS_ASSERT(!(a == b));
  }

  void test_has_invalid_values_filter() {
    LogManager runInfo;
    const std::string name = "test_has_invalid_values_filter";
    const std::string filterName = runInfo.getInvalidValuesFilterLogName(name);
    TSM_ASSERT("The filter name should start with the log name", filterName.rfind(name, 0) == 0);
    addTestTimeSeries<double>(runInfo, name);

    TS_ASSERT_EQUALS(runInfo.hasInvalidValuesFilter(name), false);
    addTestTimeSeriesFilter(runInfo, filterName);

    TS_ASSERT_EQUALS(runInfo.hasInvalidValuesFilter(name), true);
  }

  void test_get_invalid_values_filter() {
    LogManager runInfo;
    const std::string name = "test_get_invalid_values_filter";
    const std::string filterName = runInfo.getInvalidValuesFilterLogName(name);
    addTestTimeSeries<double>(runInfo, name);
    auto *filterfail = runInfo.getInvalidValuesFilter(name);
    TSM_ASSERT("The filter was returned correrctly as NULL", filterfail == nullptr);
    addTestTimeSeriesFilter(runInfo, filterName);

    auto *filter = runInfo.getInvalidValuesFilter(name);
    TSM_ASSERT("The filter was returned incorrectly as NULL", filter);
    TS_ASSERT_THROWS_NOTHING(filter->firstTime());

    // check it can be used to filter the log
    auto *log = runInfo.getProperty(name);
    auto *tsLog = dynamic_cast<TimeSeriesProperty<double> *>(log);
    TS_ASSERT(tsLog);
    if (tsLog) {
      auto filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(tsLog, *filter);
      TS_ASSERT_DELTA(filtered->nthValue(0), 2, 1e-5);
      TS_ASSERT_DELTA(filtered->nthValue(1), 3, 1e-5);
      TS_ASSERT_DELTA(filtered->nthValue(2), 4, 1e-5);
      TS_ASSERT_DELTA(filtered->nthValue(3), 5, 1e-5);
      TS_ASSERT_DELTA(filtered->nthValue(4), 21, 1e-5);
      TS_ASSERT_DELTA(filtered->nthValue(5), 22, 1e-5);
    }
  }

  void test_removeDataOutsideTimeROI() {
    // Build an input run info object with a few properties, including a time series
    LogManager run_input;
    addTestPropertyWithValue<double>(run_input, "single-double", 2023.0);
    addTestPropertyWithValue<std::string>(run_input, "single_string", "2023");
    addTestTimeSeries<double>(run_input, "time_series");

    // Create a TimeROI
    TimeROI roi;
    roi.addROI(DateAndTime("2012-07-19T16:17:20"), DateAndTime("2012-07-19T16:17:35"));
    roi.addROI(DateAndTime("2012-07-19T16:17:45"), DateAndTime("2012-07-19T16:18:10"));

    // Build an expected run info object with the same properties as the input, except for the time series
    LogManager run_expect;
    addTestPropertyWithValue<double>(run_expect, "single-double", 2023.0);
    addTestPropertyWithValue<std::string>(run_expect, "single_string", "2023");
    // Use a filtered time series property instead of original
    auto tsp = run_input.getTimeSeriesProperty<double>("time_series");
    run_expect.addProperty(tsp->cloneInTimeROI(roi), true);
    run_expect.setTimeROI(roi);

    // Test that after the input run info is filtered with the same TimeROI, the result is as expected
    run_input.setTimeROI(roi);
    run_input.removeDataOutsideTimeROI();
    TS_ASSERT_EQUALS(run_input, run_expect);
  }

  void test_cloneInTimeROI() {
    // Build an input run info object with a few properties, including a time series
    LogManager run_input;
    addTestPropertyWithValue<double>(run_input, "single-double", 2023.0);
    addTestPropertyWithValue<std::string>(run_input, "single_string", "2023");
    addTestTimeSeries<double>(run_input, "time_series");

    // Create a TimeROI
    TimeROI roi;
    roi.addROI(DateAndTime("2012-07-19T16:17:20"), DateAndTime("2012-07-19T16:17:35"));
    roi.addROI(DateAndTime("2012-07-19T16:17:45"), DateAndTime("2012-07-19T16:18:10"));

    // Build an expected run info object with the same properties as the input, except for the time series
    LogManager run_expect;
    addTestPropertyWithValue<double>(run_expect, "single-double", 2023.0);
    addTestPropertyWithValue<std::string>(run_expect, "single_string", "2023");
    // Use a filtered time series property instead of original
    auto tsp = run_input.getTimeSeriesProperty<double>("time_series");
    run_expect.addProperty(tsp->cloneInTimeROI(roi), true);
    run_expect.setTimeROI(roi);

    // Create a cloned-in-roi copy of the orginal run info object
    LogManager *run_result_ptr = run_input.cloneInTimeROI(roi);
    // Make sure the copy is different from the original, i.e. the roi really filters out some data
    TS_ASSERT(*run_result_ptr != run_input);
    // Test that the copy is the same as expected
    TS_ASSERT_EQUALS(*run_result_ptr, run_expect);
    delete run_result_ptr;
  }

private:
  template <typename T> void doTest_GetPropertyAsSingleValue_SingleType(const T value) {
    LogManager runInfo;
    const std::string name = "T_prop";
    runInfo.addProperty<T>(name, value);
    double result = std::nan("1");
    TS_ASSERT_THROWS_NOTHING(result = runInfo.getPropertyAsSingleValue(name));
    TS_ASSERT_EQUALS(value, static_cast<T>(result));
  }

  template <typename T> void doTest_GetPropertyAsSingleValue_TimeSeriesType() {
    LogManager runInfo;
    const std::string name = "T_series";
    addTestTimeSeries<T>(runInfo, name);
    const double expectedValue(13.0);
    TS_ASSERT_DELTA(runInfo.getPropertyAsSingleValue(name, Mantid::Kernel::Math::Mean), expectedValue, 1e-12);
  }

  template <typename T> void doTest_GetPropertyAsIntegerValue(const T value) {
    LogManager runInfo;
    const std::string name = "T_prop";
    runInfo.addProperty<T>(name, value);
    int result = runInfo.getPropertyAsIntegerValue(name);
    TS_ASSERT_THROWS_NOTHING(result = runInfo.getPropertyAsIntegerValue(name));
    TS_ASSERT_EQUALS(value, static_cast<T>(result));
  }
};

//---------------------------------------------------------------------------------------
// Performance test
//---------------------------------------------------------------------------------------

class LogManagerTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LogManagerTestPerformance *createSuite() { return new LogManagerTestPerformance(); }
  static void destroySuite(LogManagerTestPerformance *suite) { delete suite; }

  LogManagerTestPerformance() : m_testRun(), m_propName("test") { addTestTimeSeries<double>(m_testRun, m_propName); }

  void test_Accessing_Single_Value_From_Times_Series_A_Large_Number_Of_Times() {
    for (size_t i = 0; i < 20000; ++i) {
      // This has an observable side-effect of calling, so we don't need
      // to store its return value
      m_testRun.getPropertyAsSingleValue(m_propName);
    }
  }

  LogManager m_testRun;
  std::string m_propName;
};
