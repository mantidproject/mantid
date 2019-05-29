// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TIMESERIESPROPERTYTEST_H_
#define TIMESERIESPROPERTYTEST_H_

#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/make_unique.h"
#include <cxxtest/TestSuite.h>

#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <json/value.h>
#include <vector>

using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

class TimeSeriesPropertyTest : public CxxTest::TestSuite {
  // Create a small TSP<double>. Callee owns the returned object.
  TimeSeriesProperty<double> *createDoubleTSP() {
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
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
    TS_ASSERT(typeid(std::vector<TimeValueUnit<double>>) ==
              *dProp->type_info());
    TS_ASSERT(!dProp->isDefault())

    TS_ASSERT(!sProp->name().compare("stringProp"));
    TS_ASSERT(!sProp->documentation().compare(""));
    TS_ASSERT(typeid(std::vector<TimeValueUnit<std::string>>) ==
              *sProp->type_info());
    TS_ASSERT(!sProp->isDefault())

    TS_ASSERT_EQUALS(sProp->isValid(), "");
  }

  void test_SetValueFromString() {
    TS_ASSERT_THROWS(iProp->setValue("1"),
                     const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(dProp->setValue("5.5"),
                     const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(sProp->setValue("aValue"),
                     const Exception::NotImplementedError &);
  }

  void test_SetValueFromJson() {
    TS_ASSERT_THROWS(iProp->setValueFromJson(Json::Value(1)),
                     const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(dProp->setValueFromJson(Json::Value(5.5)),
                     const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(sProp->setValueFromJson(Json::Value("aValue")),
                     const Exception::NotImplementedError &);
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
    TS_ASSERT_THROWS_NOTHING(
        otherProp.addValue(static_cast<std::time_t>(123), 1));
    TS_ASSERT_THROWS_NOTHING(
        otherProp.addValue(boost::posix_time::second_clock::local_time(), 1));

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

    TSM_ASSERT_THROWS("derivative undefined for string property",
                      sProp->getDerivative(), const std::runtime_error &);

    iProp->addValue("2007-11-30T16:17:10", 10);
    TSM_ASSERT_THROWS(
        "derivative undefined for property with less then 2 values",
        iProp->getDerivative(), const std::runtime_error &);
    iProp->addValue("2007-11-30T16:17:12", 12);

    derProp = iProp->getDerivative();
    TS_ASSERT_EQUALS(derProp->size(), 1);
    derValues = derProp->valuesAsVector();
    TS_ASSERT_EQUALS(derValues[0], 1);
  }
  void test_timesAsVector() {
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
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
      times.push_back(first + double(i));
      values.push_back(double(i));
      replacementValues.push_back(double(i) + offset);
    }
    TimeSeriesProperty<double> tsp("test");
    tsp.addValues(times, values);
    TS_ASSERT_EQUALS(tsp.size(), 1000);
    TS_ASSERT_EQUALS(tsp.nthValue(3), 3.0);

    // Act
    tsp.replaceValues(times, replacementValues);

    // Assert
    TSM_ASSERT_EQUALS("Should have 1000 entries", tsp.size(), 1000);
    TSM_ASSERT_EQUALS("Should be 3 plus the offset of 100", tsp.nthValue(3),
                      103.0);
  }

  void test_addValues() {
    size_t num = 1000;
    DateAndTime first("2007-11-30T16:17:10");
    std::vector<DateAndTime> times;

    std::vector<double> values;
    for (size_t i = 0; i < num; i++) {
      times.push_back(first + double(i));
      values.push_back(double(i));
    }
    TimeSeriesProperty<double> tsp("test");
    tsp.addValues(times, values);
    TS_ASSERT_EQUALS(tsp.size(), 1000);
    TS_ASSERT_EQUALS(tsp.nthValue(3), 3.0);
  }

  void test_Casting() {
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(iProp),
                      static_cast<Property *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(dProp),
                      static_cast<Property *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<Property *>(sProp),
                      static_cast<Property *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<ITimeSeriesProperty *>(iProp),
                      static_cast<ITimeSeriesProperty *>(nullptr));
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

  void test_ComparisonOperator() {
    // Setup two logs and two filters so that logs have different sizes but are
    // the same size after applying the filter

    TimeSeriesProperty<int> *log1 = new TimeSeriesProperty<int>("count_rate");
    log1->addValue("2016-03-17T00:00:00", 1);
    log1->addValue("2016-03-17T00:30:00", 2);
    log1->addValue("2016-03-17T01:00:00", 3);
    log1->addValue("2016-03-17T01:30:00", 4);
    log1->addValue("2016-03-17T02:00:00", 5);
    TimeSeriesProperty<bool> *filter1 = new TimeSeriesProperty<bool>("filter");
    filter1->addValue("2016-Mar-17 00:00:00", 1);
    filter1->addValue("2016-Mar-17 01:00:00", 0);
    log1->filterWith(filter1);

    TimeSeriesProperty<int> *log2 = new TimeSeriesProperty<int>("count_rate");
    log2->addValue("2016-03-17T03:00:00", 1);
    log2->addValue("2016-03-17T04:00:00", 2);
    log2->addValue("2016-03-17T05:00:00", 3);
    log2->addValue("2016-03-17T06:00:0", 4);
    TimeSeriesProperty<bool> *filter2 = new TimeSeriesProperty<bool>("filter");
    filter2->addValue("2016-Mar-17 03:00:00", 1);
    filter2->addValue("2016-Mar-17 05:00:00", 0);
    log2->filterWith(filter2);

    TS_ASSERT(!(*log1 == *log2));

    delete log1;
    delete log2;
    delete filter1;
    delete filter2;
  }

  //----------------------------------------------------------------------------
  void test_filterByTime() {
    TimeSeriesProperty<int> *log = createIntegerTSP(6);
    TS_ASSERT_EQUALS(log->realSize(), 6);
    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");

    // Since the filter is < stop, the last one is not counted, so there are  3
    // taken out.

    log->filterByTime(start, stop);

    TS_ASSERT_EQUALS(log->realSize(), 3);

    delete log;
  }

  //-------------------------------------------------------------------------------
  void test_filterByTimes1() {
    TimeSeriesProperty<int> *log = createIntegerTSP(6);
    TS_ASSERT_EQUALS(log->realSize(), 6);

    Mantid::Kernel::SplittingInterval interval0(
        DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:40"),
        0);

    Mantid::Kernel::TimeSplitterType splitters;
    splitters.push_back(interval0);

    // Since the filter is < stop, the last one is not counted, so there are  3
    // taken out.

    log->filterByTimes(splitters);

    TS_ASSERT_EQUALS(log->realSize(), 3);

    delete log;
  }

  void test_filterByTimesN() {
    TimeSeriesProperty<int> *log = createIntegerTSP(10);
    TS_ASSERT_EQUALS(log->realSize(), 10);

    Mantid::Kernel::SplittingInterval interval0(
        DateAndTime("2007-11-30T16:17:10"), DateAndTime("2007-11-30T16:17:40"),
        0);

    Mantid::Kernel::SplittingInterval interval1(
        DateAndTime("2007-11-30T16:18:05"), DateAndTime("2007-11-30T16:18:25"),
        0);

    Mantid::Kernel::TimeSplitterType splitters;
    splitters.push_back(interval0);
    splitters.push_back(interval1);

    // Since the filter is < stop, the last one is not counted, so there are  3
    // taken out.

    log->filterByTimes(splitters);

    TS_ASSERT_EQUALS(log->realSize(), 6);

    delete log;
  }

  //----------------------------------------------------------------------------
  /// Ticket #2591
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead() {
    TimeSeriesProperty<int> *log = createIntegerTSP(1);
    TS_ASSERT_EQUALS(log->realSize(), 1);

    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    log->filterByTime(start, stop);

    // Still there!
    TS_ASSERT_EQUALS(log->realSize(), 1);

    delete log;
  }

  //----------------------------------------------------------------------------
  /// Ticket #2591
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead_2() {
    TimeSeriesProperty<int> *log = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("1990-01-01T00:00:00", 1));
    TS_ASSERT_EQUALS(log->realSize(), 1);

    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    log->filterByTime(start, stop);

    // Still there!
    TS_ASSERT_EQUALS(log->realSize(), 1);

    delete log;
  }

  //----------------------------------------------------------------------------
  void test_makeFilterByValue() {
    TimeSeriesProperty<double> *log =
        new TimeSeriesProperty<double>("MyIntLog");
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:30", 2.0));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:40", 2.01));
    TS_ASSERT_THROWS_NOTHING(log->addValue("2007-11-30T16:17:50", 6));

    TS_ASSERT_EQUALS(log->realSize(), 6);

    // Test centred log value boundaries
    TimeSplitterType splitter;
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
    TS_ASSERT_THROWS(log->makeFilterByValue(splitter, 2.0, 1.0, 0.0, true),
                     const std::invalid_argument &);

    delete log;
  }

  void test_makeFilterByValue_throws_for_string_property() {
    TimeSeriesProperty<std::string> log("StringTSP");
    TimeSplitterType splitter;
    TS_ASSERT_THROWS(log.makeFilterByValue(splitter, 0.0, 0.0, 0.0, true),
                     const Exception::NotImplementedError &);
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
    TimeInterval interval(DateAndTime("2007-11-30T16:16:00"),
                          DateAndTime("2007-11-30T16:18:50"));

    TimeSplitterType splitter;
    // Test good at both ends
    log.makeFilterByValue(splitter, 1.0, 2.2, 1.0, false);
    log.expandFilterToRange(splitter, 1.0, 2.2, interval);
    TS_ASSERT_EQUALS(splitter.size(), 2);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:16:00"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:20"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[1].start(), DateAndTime("2007-11-30T16:17:50"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[1].stop(), DateAndTime("2007-11-30T16:18:50"),
                    1e-3);

    // Test bad at both ends
    log.makeFilterByValue(splitter, 2.5, 10.0, 0.0, false);
    log.expandFilterToRange(splitter, 2.5, 10.0, interval);
    TS_ASSERT_EQUALS(splitter.size(), 1);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:17:20"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:50"),
                    1e-3);

    // Test good at start, bad at end
    log.makeFilterByValue(splitter, -1.0, 1.5, 0.0, false);
    log.expandFilterToRange(splitter, -1.0, 1.5, interval);
    TS_ASSERT_EQUALS(splitter.size(), 1);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:16:00"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:10"),
                    1e-3);

    // Test good at end, bad at start
    log.makeFilterByValue(splitter, 1.99, 2.5, 1.0, false);
    log.expandFilterToRange(splitter, 1.99, 2.5, interval);
    TS_ASSERT_EQUALS(splitter.size(), 2);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:17:10"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:20"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[1].start(), DateAndTime("2007-11-30T16:17:50"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[1].stop(), DateAndTime("2007-11-30T16:18:50"),
                    1e-3);

    // Check throws if min > max
    TS_ASSERT_THROWS(log.expandFilterToRange(splitter, 2.0, 1.0, interval),
                     const std::invalid_argument &);

    // Test good at both ends, but interval narrower than log range
    TimeInterval narrowinterval(DateAndTime("2007-11-30T16:17:15"),
                                DateAndTime("2007-11-30T16:17:41"));
    log.makeFilterByValue(splitter, 0.0, 10.0, 0.0, false);
    log.expandFilterToRange(splitter, 0.0, 10.0, narrowinterval);
    TS_ASSERT_EQUALS(splitter.size(), 1);
    TS_ASSERT_DELTA(splitter[0].start(), DateAndTime("2007-11-30T16:17:00"),
                    1e-3);
    TS_ASSERT_DELTA(splitter[0].stop(), DateAndTime("2007-11-30T16:17:50"),
                    1e-3);
  }

  void test_expandFilterToRange_throws_for_string_property() {
    TimeSeriesProperty<std::string> log("StringTSP");
    TimeSplitterType splitter;
    TS_ASSERT_THROWS(
        log.expandFilterToRange(splitter, 0.0, 0.0, TimeInterval()),
        const Exception::NotImplementedError &);
  }

  void test_averageValueInFilter() {
    auto dblLog = createDoubleTSP();
    auto intLog = createIntegerTSP(5);

    // Test a filter that's fully within the range of both properties
    TimeSplitterType filter;
    filter.push_back(SplittingInterval(DateAndTime("2007-11-30T16:17:05"),
                                       DateAndTime("2007-11-30T16:17:29")));
    TS_ASSERT_DELTA(dblLog->averageValueInFilter(filter), 7.308, 0.001);
    TS_ASSERT_DELTA(intLog->averageValueInFilter(filter), 2.167, 0.001);

    // Test a filter that starts before the log start time
    filter[0] = SplittingInterval(DateAndTime("2007-11-30T16:16:30"),
                                  DateAndTime("2007-11-30T16:17:13"));
    TS_ASSERT_DELTA(dblLog->averageValueInFilter(filter), 9.820, 0.001);
    TS_ASSERT_DELTA(intLog->averageValueInFilter(filter), 1.070, 0.001);

    // How about one that's entirely outside the log range (should just take the
    // last value)
    filter[0] = SplittingInterval(DateAndTime("2013-01-01T00:00:00"),
                                  DateAndTime("2013-01-01T01:00:00"));
    TS_ASSERT_DELTA(dblLog->averageValueInFilter(filter), 10.55, 0.001);
    TS_ASSERT_DELTA(intLog->averageValueInFilter(filter), 5.0, 0.001);

    // Test a filter with two separate ranges, one of which goes past the end of
    // the log
    filter[0] = SplittingInterval(DateAndTime("2007-11-30T16:17:05"),
                                  DateAndTime("2007-11-30T16:17:15"));
    filter.push_back(SplittingInterval(DateAndTime("2007-11-30T16:17:25"),
                                       DateAndTime("2007-11-30T16:17:45")));
    TS_ASSERT_DELTA(dblLog->averageValueInFilter(filter), 9.123, 0.001);
    TS_ASSERT_DELTA(intLog->averageValueInFilter(filter), 3.167, 0.001);

    // Test a filter with two out of order ranges (the second one coming before
    // the first)
    // It should work fine.
    filter[0] = filter[1];
    filter[0] = SplittingInterval(DateAndTime("2007-11-30T16:17:05"),
                                  DateAndTime("2007-11-30T16:17:15"));
    TS_ASSERT_DELTA(dblLog->averageValueInFilter(filter), 9.123, 0.001);
    TS_ASSERT_DELTA(intLog->averageValueInFilter(filter), 3.167, 0.001);

    // What about an overlap between the filters? It's odd, but it's allowed.
    filter[0] = SplittingInterval(DateAndTime("2007-11-30T16:17:05"),
                                  DateAndTime("2007-11-30T16:17:15"));
    filter[1] = SplittingInterval(DateAndTime("2007-11-30T16:17:10"),
                                  DateAndTime("2007-11-30T16:17:20"));
    TS_ASSERT_DELTA(dblLog->averageValueInFilter(filter), 8.16, 0.001);
    TS_ASSERT_DELTA(intLog->averageValueInFilter(filter), 1.75, 0.001);

    // Check the correct behaviour of empty of single value logs.
    TS_ASSERT(std::isnan(dProp->averageValueInFilter(filter)));
    iProp->addValue(DateAndTime("2010-11-30T16:17:25"), 99);
    TS_ASSERT_EQUALS(iProp->averageValueInFilter(filter), 99.0);

    // Clean up
    delete dblLog;
    delete intLog;
  }

  void test_timeAverageValue() {
    auto dblLog = createDoubleTSP();
    auto intLog = createIntegerTSP(5);

    // average values
    const double dblMean = dblLog->timeAverageValue();
    TS_ASSERT_DELTA(dblMean, 7.6966, .0001);
    const double intMean = intLog->timeAverageValue();
    TS_ASSERT_DELTA(intMean, 2.5, .0001);

    // Clean up
    delete dblLog;
    delete intLog;
  }

  void test_averageValueInFilter_throws_for_string_property() {
    TimeSplitterType splitter;
    TS_ASSERT_THROWS(sProp->averageValueInFilter(splitter),
                     const Exception::NotImplementedError &);
    TS_ASSERT_THROWS(sProp->averageAndStdDevInFilter(splitter),
                     const Exception::NotImplementedError &);
  }

  //----------------------------------------------------------------------------
  void test_splitByTime_and_getTotalValue() {
    TimeSeriesProperty<int> *log = createIntegerTSP(12);
    // Make the outputs
    std::vector<Property *> outputs;
    for (std::size_t i = 0; i < 5; i++) {
      TimeSeriesProperty<int> *newlog = new TimeSeriesProperty<int>("MyIntLog");
      outputs.push_back(newlog);
    }

    // Make a splitter
    DateAndTime start, stop;
    TimeSplitterType splitter;
    start = DateAndTime("2007-11-30T16:17:10");
    stop = DateAndTime("2007-11-30T16:17:40");
    splitter.push_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:55");
    stop = DateAndTime("2007-11-30T16:17:56");
    splitter.push_back(SplittingInterval(start, stop, 1));

    start = DateAndTime("2007-11-30T16:17:56");
    stop = DateAndTime("2007-11-30T16:18:01");
    splitter.push_back(SplittingInterval(start, stop, 2)); // just one entry

    start = DateAndTime("2007-11-30T16:18:09");
    stop = DateAndTime("2007-11-30T16:18:21");
    splitter.push_back(SplittingInterval(start, stop, 3));

    start = DateAndTime("2007-11-30T16:18:45");
    stop = DateAndTime("2007-11-30T16:22:50");
    splitter.push_back(SplittingInterval(start, stop, 4));

    log->splitByTime(splitter, outputs, false);

    TS_ASSERT_EQUALS(
        dynamic_cast<TimeSeriesProperty<int> *>(outputs[0])->realSize(), 3);
    TS_ASSERT_EQUALS(
        dynamic_cast<TimeSeriesProperty<int> *>(outputs[1])->realSize(), 1);
    TS_ASSERT_EQUALS(
        dynamic_cast<TimeSeriesProperty<int> *>(outputs[2])->realSize(), 2);
    TS_ASSERT_EQUALS(
        dynamic_cast<TimeSeriesProperty<int> *>(outputs[3])->realSize(), 3);
    TS_ASSERT_EQUALS(
        dynamic_cast<TimeSeriesProperty<int> *>(outputs[4])->realSize(), 2);

    delete log;
    delete outputs[0];
    delete outputs[1];
    delete outputs[2];
    delete outputs[3];
    delete outputs[4];
  }

  //----------------------------------------------------------------------------
  void test_splitByTime_withOverlap() {
    TimeSeriesProperty<int> *log = createIntegerTSP(12);

    // Make the outputs
    std::vector<Property *> outputs;
    for (std::size_t i = 0; i < 1; i++) {
      TimeSeriesProperty<int> *newlog = new TimeSeriesProperty<int>("MyIntLog");
      outputs.push_back(newlog);
    }

    // Make a splitter
    DateAndTime start, stop;
    TimeSplitterType splitter;
    start = DateAndTime("2007-11-30T16:17:10");
    stop = DateAndTime("2007-11-30T16:17:40");
    splitter.push_back(SplittingInterval(start, stop, 0));

    start = DateAndTime("2007-11-30T16:17:35");
    stop = DateAndTime("2007-11-30T16:17:59");
    splitter.push_back(SplittingInterval(start, stop, 0));

    log->splitByTime(splitter, outputs, false);

    TS_ASSERT_EQUALS(
        dynamic_cast<TimeSeriesProperty<int> *>(outputs[0])->realSize(), 5);

    delete log;
    delete outputs[0];
  }

  //----------------------------------------------------------------------------
  /**
   * otuput 0 has entries: 3
   * otuput 1 has entries: 5
   * otuput 2 has entries: 2
   * otuput 3 has entries: 7
   * @brief test_splitByTimeVector
   */
  void test_splitByTimeVector() {
    // create the splitters
    std::vector<DateAndTime> split_time_vec;
    split_time_vec.push_back(DateAndTime("2007-11-30T16:17:10"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:17:40"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:17:55"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:17:56"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:18:09"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:18:45"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:22:50"));

    std::vector<int> split_target_vec;
    split_target_vec.push_back(1);
    split_target_vec.push_back(0);
    split_target_vec.push_back(2);
    split_target_vec.push_back(0);
    split_target_vec.push_back(1);
    split_target_vec.push_back(3);

    TimeSeriesProperty<int> log("test log");
    log.addValue(DateAndTime("2007-11-30T16:17:00"), 1);
    log.addValue(DateAndTime("2007-11-30T16:17:30"), 2);
    log.addValue(DateAndTime("2007-11-30T16:18:00"), 3);
    log.addValue(DateAndTime("2007-11-30T16:18:30"), 4);
    log.addValue(DateAndTime("2007-11-30T16:19:00"), 5);
    log.addValue(DateAndTime("2007-11-30T16:19:30"), 6);
    log.addValue(DateAndTime("2007-11-30T16:20:00"), 7);
    log.addValue(DateAndTime("2007-11-30T16:20:30"), 8);
    log.addValue(DateAndTime("2007-11-30T16:21:00"), 9);
    log.addValue(DateAndTime("2007-11-30T16:21:30"), 10);

    std::vector<TimeSeriesProperty<int> *> outputs;
    for (int itarget = 0; itarget < 4; ++itarget) {
      TimeSeriesProperty<int> *tsp = new TimeSeriesProperty<int>("target");
      outputs.push_back(tsp);
    }

    log.splitByTimeVector(split_time_vec, split_target_vec, outputs);

    // Exam the split entries
    TimeSeriesProperty<int> *out_0 = outputs[0];
    // FIXME - Check whether out_0 is correct!
    TS_ASSERT_EQUALS(out_0->size(), 3);
    TS_ASSERT_EQUALS(out_0->nthValue(0), 2);
    TS_ASSERT_EQUALS(out_0->nthValue(1), 3);
    TS_ASSERT_EQUALS(out_0->nthValue(2), 4);

    TimeSeriesProperty<int> *out_1 = outputs[1];
    TS_ASSERT_EQUALS(out_1->size(), 5);
    TS_ASSERT_EQUALS(out_1->nthValue(0), 1);
    TS_ASSERT_EQUALS(out_1->nthValue(1), 2);
    TS_ASSERT_EQUALS(out_1->nthValue(2), 3);
    TS_ASSERT_EQUALS(out_1->nthValue(3), 4);
    TS_ASSERT_EQUALS(out_1->nthValue(4), 5);

    TimeSeriesProperty<int> *out_2 = outputs[2];
    TS_ASSERT_EQUALS(out_2->size(), 2);
    TS_ASSERT_EQUALS(out_2->nthValue(0), 2);
    TS_ASSERT_EQUALS(out_2->nthValue(1), 3);

    TimeSeriesProperty<int> *out_3 = outputs[3];
    TS_ASSERT_EQUALS(out_3->size(), 7);
    // out[3] should have entries: 4, 5, 6, 7, 8, 9, 10
    for (int j = 0; j < out_3->size(); ++j) {
      TS_ASSERT_EQUALS(out_3->nthValue(j), j + 4);
    }

    for (auto outputPtr : outputs) {
      delete outputPtr;
    }
  }

  //----------------------------------------------------------------------------
  /** last splitter is before first entry
   * @brief test_splitByTimeVectorEarlySplitter
   */
  void test_splitByTimeVectorEarlySplitter() {
    // create the splitters
    std::vector<DateAndTime> split_time_vec;
    split_time_vec.push_back(DateAndTime("2007-11-30T16:00:10"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:00:40"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:07:55"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:07:56"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:08:09"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:08:45"));
    split_time_vec.push_back(DateAndTime("2007-11-30T16:12:50"));

    std::vector<int> split_target_vec;
    split_target_vec.push_back(1);
    split_target_vec.push_back(0);
    split_target_vec.push_back(2);
    split_target_vec.push_back(0);
    split_target_vec.push_back(1);
    split_target_vec.push_back(3);

    TimeSeriesProperty<int> log("test log");
    log.addValue(DateAndTime("2007-11-30T16:17:00"), 1);
    log.addValue(DateAndTime("2007-11-30T16:17:30"), 2);
    log.addValue(DateAndTime("2007-11-30T16:18:00"), 3);
    log.addValue(DateAndTime("2007-11-30T16:18:30"), 4);
    log.addValue(DateAndTime("2007-11-30T16:19:00"), 5);
    log.addValue(DateAndTime("2007-11-30T16:19:30"), 6);
    log.addValue(DateAndTime("2007-11-30T16:20:00"), 7);
    log.addValue(DateAndTime("2007-11-30T16:20:30"), 8);
    log.addValue(DateAndTime("2007-11-30T16:21:00"), 9);
    log.addValue(DateAndTime("2007-11-30T16:21:30"), 10);

    // Initialze the 4 splitters
    std::vector<TimeSeriesProperty<int> *> outputs;
    for (int itarget = 0; itarget < 4; ++itarget) {
      outputs.push_back(new TimeSeriesProperty<int>("target"));
    }

    log.splitByTimeVector(split_time_vec, split_target_vec, outputs);

    // check
    for (int i = 0; i < 4; ++i) {
      TimeSeriesProperty<int> *out_i = outputs[i];
      TS_ASSERT_EQUALS(out_i->size(), 0);
      delete out_i;
      outputs[i] = nullptr;
    }

    return;
  }

  //----------------------------------------------------------------------------
  /** first splitter is after last entry
   * @brief test_splitByTimeVectorLaterSplitter
   */
  void test_splitByTimeVectorLaterSplitter() {
    // create the splitters
    std::vector<DateAndTime> split_time_vec;
    split_time_vec.push_back(DateAndTime("2007-12-30T16:00:10"));
    split_time_vec.push_back(DateAndTime("2007-12-30T16:00:40"));
    split_time_vec.push_back(DateAndTime("2007-12-30T16:07:55"));
    split_time_vec.push_back(DateAndTime("2007-12-30T16:07:56"));
    split_time_vec.push_back(DateAndTime("2007-12-30T16:08:09"));
    split_time_vec.push_back(DateAndTime("2007-12-30T16:08:45"));
    split_time_vec.push_back(DateAndTime("2007-12-30T16:12:50"));

    std::vector<int> split_target_vec;
    split_target_vec.push_back(1);
    split_target_vec.push_back(0);
    split_target_vec.push_back(2);
    split_target_vec.push_back(0);
    split_target_vec.push_back(1);
    split_target_vec.push_back(3);

    // create test log
    TimeSeriesProperty<int> log("test log");
    log.addValue(DateAndTime("2007-11-30T16:17:00"), 1);
    log.addValue(DateAndTime("2007-11-30T16:17:30"), 2);
    log.addValue(DateAndTime("2007-11-30T16:18:00"), 3);
    log.addValue(DateAndTime("2007-11-30T16:18:30"), 4);
    log.addValue(DateAndTime("2007-11-30T16:19:00"), 5);
    log.addValue(DateAndTime("2007-11-30T16:19:30"), 6);
    log.addValue(DateAndTime("2007-11-30T16:20:00"), 7);
    log.addValue(DateAndTime("2007-11-30T16:20:30"), 8);
    log.addValue(DateAndTime("2007-11-30T16:21:00"), 9);
    log.addValue(DateAndTime("2007-11-30T16:21:30"), 10);

    // Initialze the 4 splitters
    std::vector<TimeSeriesProperty<int> *> outputs;
    for (int itarget = 0; itarget < 4; ++itarget) {
      outputs.emplace_back(new TimeSeriesProperty<int>("target"));
    }

    log.splitByTimeVector(split_time_vec, split_target_vec, outputs);

    // check
    for (int i = 0; i < 4; ++i) {
      TimeSeriesProperty<int> *out_i = outputs[i];
      TS_ASSERT_EQUALS(out_i->size(), 1);
      delete out_i;
      outputs[i] = nullptr;
    }
  }

  //----------------------------------------------------------------------------
  /** high-frequency splitters splits a slow change log
   * @brief test_splitByTimeVectorFastLogSplitter
   */
  void test_splitByTimeVectorFastLogSplitter() {
    // create test log
    TimeSeriesProperty<int> log("test log");
    log.addValue(DateAndTime("2007-11-30T16:17:00"), 1);
    log.addValue(DateAndTime("2007-11-30T16:17:30"), 2);
    log.addValue(DateAndTime("2007-11-30T16:18:00"), 3);
    log.addValue(DateAndTime("2007-11-30T16:18:30"), 4);
    log.addValue(DateAndTime("2007-11-30T16:19:00"), 5);
    log.addValue(DateAndTime("2007-11-30T16:19:30"), 6);
    log.addValue(DateAndTime("2007-11-30T16:20:00"), 7);
    log.addValue(DateAndTime("2007-11-30T16:20:30"), 8);
    log.addValue(DateAndTime("2007-11-30T16:21:00"), 9);
    log.addValue(DateAndTime("2007-11-30T16:21:30"), 10);

    // create a high frequency splitter
    DateAndTime split_time("2007-11-30T16:17:00");
    int64_t dt = 100 * 1000;

    std::vector<DateAndTime> vec_split_times;
    std::vector<int> vec_split_target;

    for (int i = 0; i < 10; ++i) {
      for (int j = 0; j < 10; ++j) {
        vec_split_times.push_back(split_time);
        split_time += dt;
        vec_split_target.push_back(j);
      }
    }

    // push back last split-time (split stop)
    vec_split_times.push_back(split_time);

    // Initialze the 10 splitters
    std::vector<TimeSeriesProperty<int> *> outputs;
    for (int itarget = 0; itarget < 10; ++itarget) {
      outputs.push_back(new TimeSeriesProperty<int>("target"));
    }

    // split time series property
    log.splitByTimeVector(vec_split_times, vec_split_target, outputs);

    // test
    for (auto &it : outputs) {
      TS_ASSERT_EQUALS(it->size(), 2);
      delete it;
      it = nullptr;
    }
  }

  //----------------------------------------------------------------------------
  /** Extreme case 1: the last entry of time series property is before the first
   * splitter.  The test case is extracted from issue #21836, in which
   * the last entry is before the first splitter
   * @brief test_SplitByTimeExtremeCase1.
   */
  void test_SplitByTimeExtremeCase1() {
    // create test log
    TimeSeriesProperty<int> int_log("test int log 21836");
    int_log.addValue(DateAndTime("2017-11-10T03:12:06"), 1);
    int_log.addValue(DateAndTime("2017-11-10T03:12:31"), 3);
    int_log.addValue(DateAndTime("2017-11-10T03:12:40"), 2);

    TimeSeriesProperty<double> dbl_log("test double log 21836");
    dbl_log.addValue(DateAndTime("2017-11-10T03:12:06"), 1.0);
    dbl_log.addValue(DateAndTime("2017-11-10T03:12:31"), 3.0);
    dbl_log.addValue(DateAndTime("2017-11-10T03:12:40"), 2.0);

    // create the splitters
    std::vector<DateAndTime> split_time_vec;
    split_time_vec.push_back(DateAndTime("2017-11-10T03:13:06.814538624"));
    split_time_vec.push_back(DateAndTime("2017-11-10T03:14:07.764311936"));
    split_time_vec.push_back(DateAndTime("2017-11-10T03:15:07.697312000"));
    split_time_vec.push_back(DateAndTime("2017-11-10T03:16:08.827971840"));
    split_time_vec.push_back(DateAndTime("2017-11-10T03:17:08.745746688"));
    split_time_vec.push_back(DateAndTime("2017-11-10T03:20:10.757950208"));

    // create the target vector
    std::vector<int> split_target_vec(5);
    for (size_t i = 0; i < 5; ++i) {
      split_target_vec[i] = (i + 1) % 2;
    }

    // Initialze the 2 splitters
    std::vector<TimeSeriesProperty<int> *> outputs;
    for (int itarget = 0; itarget < 2; ++itarget) {
      outputs.push_back(new TimeSeriesProperty<int>("target"));
    }

    // split
    int_log.splitByTimeVector(split_time_vec, split_target_vec, outputs);

    // check
    for (int i = 0; i < 2; ++i) {
      TimeSeriesProperty<int> *out_i = outputs[i];
      TS_ASSERT_EQUALS(out_i->size(), 1);
      delete out_i;
      outputs[i] = nullptr;
    }

    return;
  }

  //----------------------------------------------------------------------------
  void test_statistics() {
    TimeSeriesProperty<double> *log =
        new TimeSeriesProperty<double>("MydoubleLog");
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
    TS_ASSERT_DELTA(stats.duration, 100.0, 1e-3);
    TS_ASSERT_DELTA(stats.standard_deviation, 3.1622, 1e-3);
    TS_ASSERT_DELTA(log->timeAverageValue(), 5.5, 1e-3);
    TS_ASSERT_DELTA(stats.time_mean, 5.5, 1e-3);
    TS_ASSERT_DELTA(stats.time_standard_deviation, 2.872, 1e-3);

    delete log;
  }

  void test_empty_statistics() {
    TimeSeriesProperty<double> *log =
        new TimeSeriesProperty<double>("MydoubleLog");
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

  void test_PlusEqualsOperator_Incompatible_Types_dontThrow() {
    // Adding incompatible types together should not throw, but issue a warning
    // in the log

    TimeSeriesProperty<double> *log =
        new TimeSeriesProperty<double>("MydoubleLog");
    TimeSeriesProperty<int> *logi = new TimeSeriesProperty<int>("MyIntLog");
    PropertyWithValue<double> *val =
        new PropertyWithValue<double>("MySimpleDouble", 1.23);

    log->operator+=(val);
    log->operator+=(logi);
    logi->operator+=(log);
    val->operator+=(log);
    val->operator+=(logi);

    delete log;
    delete logi;
    delete val;
  }

  void test_PlusEqualsOperator_() {
    TimeSeriesProperty<double> *lhs =
        new TimeSeriesProperty<double>("doubleLog");
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:00", 1));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:10", 2));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:20", 3));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:30", 4));
    TS_ASSERT_THROWS_NOTHING(lhs->addValue("2007-11-30T16:17:40", 5));
    TimeSeriesProperty<double> *rhs =
        new TimeSeriesProperty<double>("doubleLog");
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
    TimeSeriesProperty<double> *p2 =
        new TimeSeriesProperty<double>("doubleProp2");

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Clone
    TimeSeriesProperty<double> *newp =
        dynamic_cast<TimeSeriesProperty<double> *>(p->clone());

    // 3. Check
    std::vector<Mantid::Types::Core::DateAndTime> times1 = p->timesAsVector();
    std::vector<double> values1 = p->valuesAsVector();

    std::vector<Mantid::Types::Core::DateAndTime> times2 =
        newp->timesAsVector();
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

    auto time_series_small_shift = dynamic_cast<TimeSeriesProperty<int> *>(
        time_series->cloneWithTimeShift(100.));
    const auto &small_shift_times = time_series_small_shift->timesAsVector();
    TS_ASSERT_EQUALS(small_shift_times[0], DateAndTime("2019-02-10T16:18:40"));

    auto time_series_large_shift = dynamic_cast<TimeSeriesProperty<int> *>(
        time_series->cloneWithTimeShift(1234.));
    const auto &large_shift_times = time_series_large_shift->timesAsVector();
    TS_ASSERT_EQUALS(large_shift_times[0], DateAndTime("2019-02-10T16:37:34"));

    auto time_series_negative_shift = dynamic_cast<TimeSeriesProperty<int> *>(
        time_series->cloneWithTimeShift(-1234.));
    const auto &negative_shift_times =
        time_series_negative_shift->timesAsVector();
    TS_ASSERT_EQUALS(negative_shift_times[0],
                     DateAndTime("2019-02-10T15:56:26"));

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
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
    boost::scoped_ptr<TimeSeriesProperty<int>> p(
        new TimeSeriesProperty<int>("aProp"));
    p->addValue("2007-11-30T16:17:00", 1);

    TS_ASSERT_EQUALS(p->size(), 1);
    TS_ASSERT_EQUALS(p->realSize(), 1);

    ITimeSeriesProperty *pi = p.get();
    TS_ASSERT_THROWS_NOTHING(pi->clear());

    TS_ASSERT_EQUALS(p->size(), 0);
    TS_ASSERT_EQUALS(p->realSize(), 0);
  }

  void test_clearOutdated() {
    boost::scoped_ptr<TimeSeriesProperty<int>> p(
        new TimeSeriesProperty<int>("aProp"));
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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Create method 1
    std::vector<Mantid::Types::Core::DateAndTime> times;
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.push_back(1.00);
    values.push_back(3.00);
    values.push_back(2.00);
    values.push_back(4.00);

    TimeSeriesProperty<double> *p1 =
        new TimeSeriesProperty<double>("Property2");
    p1->create(times, values);

    TS_ASSERT_EQUALS(p->size(), p1->size());
    if (p->size() == p1->size()) {
      std::vector<Mantid::Types::Core::DateAndTime> times0 = p->timesAsVector();
      std::vector<Mantid::Types::Core::DateAndTime> times1 =
          p1->timesAsVector();
      for (size_t i = 0; i < static_cast<size_t>(p->size()); i++) {
        TS_ASSERT_EQUALS(times0[i], times1[i]);
        TS_ASSERT_DELTA(p->getSingleValue(times0[i]),
                        p1->getSingleValue(times1[i]), 1.0E-9);
      }
    }

    // 3 Create method 2
    Mantid::Types::Core::DateAndTime tStart("2007-11-30T16:17:00");
    std::vector<double> deltaTs;
    std::vector<double> valueXs;

    for (int i = 0; i < 4; i++) {
      deltaTs.push_back(static_cast<double>(i) * 10.0);
      valueXs.push_back(static_cast<double>(i) + 1.0);
    }

    TimeSeriesProperty<double> *p2 =
        new TimeSeriesProperty<double>("Property4");
    p2->create(tStart, deltaTs, valueXs);

    TS_ASSERT_EQUALS(p->size(), p2->size());
    if (p->size() == p2->size()) {
      std::vector<Mantid::Types::Core::DateAndTime> times0 = p->timesAsVector();
      std::vector<Mantid::Types::Core::DateAndTime> times1 =
          p2->timesAsVector();
      for (size_t i = 0; i < static_cast<size_t>(p->size()); i++) {
        TS_ASSERT_EQUALS(times0[i], times1[i]);
        TS_ASSERT_DELTA(p->getSingleValue(times0[i]),
                        p2->getSingleValue(times1[i]), 1.0E-9);
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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. What is correct
    std::vector<std::string> correctS{
        "2007-Nov-30 16:17:00 1", "2007-Nov-30 16:17:10 2",
        "2007-Nov-30 16:17:20 3", "2007-Nov-30 16:17:30 4"};

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(
        p->addValue("2007-11-30T16:17:10", 1.99)); // this one is ignored
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Get map
    std::map<Mantid::Types::Core::DateAndTime, double> tmap =
        p->valueAsCorrectMap();

    // 3. Check
    std::vector<Mantid::Types::Core::DateAndTime> times;
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.push_back(1.00);
    values.push_back(2.00);
    values.push_back(3.00);
    values.push_back(4.00);

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 3.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 1.99));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 4.00));

    // 2. Get multimap
    std::multimap<Mantid::Types::Core::DateAndTime, double> tmap =
        p->valueAsMultiMap();

    // 3. Check
    std::vector<Mantid::Types::Core::DateAndTime> times;
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.push_back(1.00);
    values.push_back(1.99);
    values.push_back(2.00);
    values.push_back(3.00);
    values.push_back(4.00);

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
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
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:15"));
    times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
    std::vector<double> values;
    values.push_back(1.00);
    values.push_back(2.00);
    values.push_back(3.00);
    values.push_back(4.00);

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
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
      times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
      times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
      times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
      times.push_back(Mantid::Types::Core::DateAndTime("2007-11-30T16:17:30"));
      std::vector<double> values;
      values.push_back(1.00);
      values.push_back(2.00);
      values.push_back(3.00);
      values.push_back(4.00);

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");

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
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");

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
    TS_ASSERT_EQUALS(dt0.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(dt0.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:05"));

    Mantid::Kernel::TimeInterval dt1 = p->nthInterval(1);
    TS_ASSERT_EQUALS(dt1.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:05"));
    TS_ASSERT_EQUALS(dt1.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:15"));

    Mantid::Kernel::TimeInterval dt2 = p->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:15"));
    TS_ASSERT_EQUALS(dt2.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:35"));

    // -1 Clean
    delete p;

    return;
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
      deltaTs.push_back(static_cast<double>(i) * 10.0);
      valueXs.push_back(static_cast<double>(i) + 1.0);
    }
    TimeSeriesProperty<double> *p1 =
        new TimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // b) Copy size and interval information in order to verify clearFilter()
    size_t origsize = p1->size();
    std::vector<Mantid::Kernel::TimeInterval> dts;

    for (size_t i = 0; i < origsize; i++) {
      dts.push_back(p1->nthInterval(static_cast<int>(i)));
    }

    // 2. Create a filter
    TimeSeriesProperty<bool> *filter = new TimeSeriesProperty<bool>("Filter");
    filter->addValue("2007-11-30T16:17:06", true);
    filter->addValue("2007-11-30T16:17:16", false);
    filter->addValue("2007-11-30T16:18:40", true);
    filter->addValue("2007-11-30T16:19:30", false);

    p1->filterWith(filter);

    // 4. Formal check (1) Size  (2) Number of Interval
    p1->countSize();
    TS_ASSERT_EQUALS(p1->size(), 7);

    Mantid::Kernel::TimeInterval dt1 = p1->nthInterval(1);
    TS_ASSERT_EQUALS(dt1.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(dt1.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));

    Mantid::Kernel::TimeInterval dt2 = p1->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:18:40"));
    TS_ASSERT_EQUALS(dt2.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:18:50"));

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
    auto p1 = boost::make_shared<TimeSeriesProperty<double>>("SingleValueTSP");
    p1->addValue("2007-11-30T16:17:00", 1.5);

    auto filterEndsBefore =
        boost::make_shared<TimeSeriesProperty<bool>>("EndsBefore");
    filterEndsBefore->addValue("2007-11-30T16:16:30", false);
    filterEndsBefore->addValue("2007-11-30T16:16:58", true);
    p1->filterWith(filterEndsBefore.get());
    TS_ASSERT_EQUALS(1, p1->size());

    p1->clearFilter();
    auto filterEndsAfter =
        boost::make_shared<TimeSeriesProperty<bool>>("EndsAfter");
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
      deltaTs.push_back(static_cast<double>(i) * 10.0);
      valueXs.push_back(static_cast<double>(i) + 1.0);
    }
    TimeSeriesProperty<double> *p1 =
        new TimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    TimeSeriesProperty<bool> *filter = new TimeSeriesProperty<bool>("Filter");
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
    TS_ASSERT_EQUALS(dt0.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:00"));
    TS_ASSERT_EQUALS(dt0.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    double v0 = p1->nthValue(0);
    TS_ASSERT_DELTA(v0, 1, 0.00000001);

    Mantid::Kernel::TimeInterval dt1 = p1->nthInterval(1);
    TS_ASSERT_EQUALS(dt1.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(dt1.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    double v1 = p1->nthValue(1);
    TS_ASSERT_DELTA(v1, 2, 0.00000001);

    Mantid::Kernel::TimeInterval dt2 = p1->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:18:40"));
    TS_ASSERT_EQUALS(dt2.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:18:50"));
    double v2 = p1->nthValue(2);
    TS_ASSERT_DELTA(v2, 11, 0.00000001);

    Mantid::Kernel::TimeInterval dt12 = p1->nthInterval(11);
    TS_ASSERT_EQUALS(dt12.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:20:10"));
    TS_ASSERT_EQUALS(dt12.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T17:19:30"));
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
      deltaTs.push_back(static_cast<double>(i) * 10.0);
      valueXs.push_back(static_cast<double>(i) + 1.0);
    }
    TimeSeriesProperty<double> *p1 =
        new TimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    TimeSeriesProperty<bool> *filter = new TimeSeriesProperty<bool>("Filter");
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
    TS_ASSERT_EQUALS(dt0.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    TS_ASSERT_EQUALS(dt0.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
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
      deltaTs.push_back(static_cast<double>(i) * 10.0);
      valueXs.push_back(static_cast<double>(i) + 1.0);
    }
    TimeSeriesProperty<double> *p1 =
        new TimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    TimeSeriesProperty<bool> *filter = new TimeSeriesProperty<bool>("Filter");
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
    TS_ASSERT_EQUALS(dt1.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:10"));
    TS_ASSERT_EQUALS(dt1.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    double v1 = p1->nthValue(1);
    TS_ASSERT_DELTA(v1, 2, 1.0E-8);

    Mantid::Kernel::TimeInterval dt2 = p1->nthInterval(2);
    TS_ASSERT_EQUALS(dt2.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:18:40"));
    TS_ASSERT_EQUALS(dt2.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:18:50"));
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
      deltaTs.push_back(static_cast<double>(i) * 10.0);
      valueXs.push_back(static_cast<double>(i) + 1.0);
    }
    TimeSeriesProperty<double> *p1 =
        new TimeSeriesProperty<double>("BaseProperty");
    p1->create(tStart, deltaTs, valueXs);

    std::vector<Mantid::Types::Core::DateAndTime> times = p1->timesAsVector();
    std::vector<double> values = p1->valuesAsVector();

    // 2. Create a filter for T. F. T. F...
    TimeSeriesProperty<bool> *filter = new TimeSeriesProperty<bool>("Filter");
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
    TS_ASSERT_EQUALS(dt0.begin(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:16"));
    TS_ASSERT_EQUALS(dt0.end(),
                     Mantid::Types::Core::DateAndTime("2007-11-30T16:17:20"));
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
   * Test getMemorySize()
   * Note that this will be same with new container
   */
  void test_getMemorySize() {
    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");

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

    const double actualFilteredValue =
        filterByStatistic(&series, Mantid::Kernel::Math::FirstValue);
    TSM_ASSERT_EQUALS("Filtering by FirstValue is not working.",
                      expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_last_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02", expectedFilteredValue);

    const double actualFilteredValue =
        filterByStatistic(&series, Mantid::Kernel::Math::LastValue);
    TSM_ASSERT_EQUALS("Filtering by LastValue is not working.",
                      expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_minimum_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 3);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // minimum. 1 < 3 < 4
    series.addValue("2000-11-30T01:01:03", 4);

    const double actualFilteredValue =
        filterByStatistic(&series, Mantid::Kernel::Math::Minimum);
    TSM_ASSERT_EQUALS("Filtering by Minimum is not working.",
                      expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_maximum_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0.1);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // maximum. 1 > 0.9 > 0.1
    series.addValue("2000-11-30T01:01:03", 0.9);

    const double actualFilteredValue =
        filterByStatistic(&series, Mantid::Kernel::Math::Maximum);
    TSM_ASSERT_EQUALS("Filtering by Maximum is not working.",
                      expectedFilteredValue, actualFilteredValue);
  }

  void test_filter_by_mean_value() {
    TimeSeriesProperty<double> series("doubleProperty");

    const double expectedFilteredValue = 1;
    series.addValue("2000-11-30T01:01:01", 0);
    series.addValue("2000-11-30T01:01:02",
                    expectedFilteredValue); // time series mean = value at T =
                                            // (T1 + T2 + T3) / 3
    series.addValue("2000-11-30T01:01:03", 2);

    const double actualFilteredValue =
        filterByStatistic(&series, Mantid::Kernel::Math::Mean);
    TSM_ASSERT_EQUALS("Filtering by Mean Time is not working.",
                      expectedFilteredValue, actualFilteredValue);
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

    const double actualFilteredValue =
        filterByStatistic(&series, Mantid::Kernel::Math::Median);
    TSM_ASSERT_EQUALS("Filtering by Median Time is not working.",
                      expectedFilteredValue, actualFilteredValue);
  }

  //----------------------------------------------------------------------------

  /** A test for filter nothing
   */
  void test_filterByTime_out_of_range_filters_nothing() {
    TimeSeriesProperty<int> *log = createIntegerTSP(6);

    size_t original_size = log->realSize();

    TS_ASSERT_EQUALS(original_size, 6);

    DateAndTime start = DateAndTime(
        "2007-11-30T15:00:00"); // Much earlier than first time series value
    DateAndTime stop = DateAndTime(
        "2007-11-30T17:00:00"); // Much later than last time series value

    log->filterByTime(start, stop);

    TSM_ASSERT_EQUALS("Shouldn't be filtering anything!", original_size,
                      log->realSize());

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

  void test_getSplittingIntervals_noFilter() {
    const auto &log = getTestLog(); // no filter
    const auto &intervals = log->getSplittingIntervals();
    TS_ASSERT_EQUALS(intervals.size(), 1);
    const auto &range = intervals.front();
    TS_ASSERT_EQUALS(range.start(), log->firstTime());
    TS_ASSERT_EQUALS(range.stop(), log->lastTime());
  }

  void test_getSplittingIntervals_repeatedEntries() {
    const auto &log = getTestLog();
    // Add the filter
    auto filter =
        Mantid::Kernel::make_unique<TimeSeriesProperty<bool>>("Filter");
    Mantid::Types::Core::DateAndTime firstStart("2007-11-30T16:17:00"),
        firstEnd("2007-11-30T16:17:15"), secondStart("2007-11-30T16:18:35"),
        secondEnd("2007-11-30T16:18:40");
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
      const auto &firstRange = intervals.front(),
                 &secondRange = intervals.back();
      TS_ASSERT_EQUALS(firstRange.start(), firstStart);
      TS_ASSERT_EQUALS(firstRange.stop(), firstEnd);
      TS_ASSERT_EQUALS(secondRange.start(), secondStart);
      TS_ASSERT_EQUALS(secondRange.stop(), secondEnd);
    }
  }

  void test_getSplittingIntervals_startEndTimes() {
    const auto &log = getTestLog();
    // Add the filter
    auto filter =
        Mantid::Kernel::make_unique<TimeSeriesProperty<bool>>("Filter");
    Mantid::Types::Core::DateAndTime firstEnd("2007-11-30T16:17:05"),
        secondStart("2007-11-30T16:17:10"), secondEnd("2007-11-30T16:17:15"),
        thirdStart("2007-11-30T16:18:35");
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
  /// Generate a test log
  std::unique_ptr<TimeSeriesProperty<double>> getTestLog() {
    // Build the log
    auto log =
        Mantid::Kernel::make_unique<TimeSeriesProperty<double>>("DoubleLog");
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
  std::unique_ptr<TimeSeriesProperty<double>> getFilteredTestLog() {
    // Build the log
    auto log = getTestLog();
    // Add the filter
    auto filter =
        Mantid::Kernel::make_unique<TimeSeriesProperty<bool>>("Filter");
    filter->addValue("2007-11-30T16:17:00", true);
    filter->addValue("2007-11-30T16:17:15", false);
    filter->addValue("2007-11-30T16:17:25", true);
    filter->addValue("2007-11-30T16:18:35", false);
    log->filterWith(filter.get());
    return log;
  }

  TimeSeriesProperty<int> *iProp;
  TimeSeriesProperty<double> *dProp;
  TimeSeriesProperty<std::string> *sProp;
};

#endif /*TIMESERIESPROPERTYTEST_H_*/
