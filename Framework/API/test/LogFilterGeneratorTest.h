// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_LOGFILTERGENERATORTEST_H_
#define MANTID_API_LOGFILTERGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/LogFilterGenerator.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidTestHelpers/FakeObjects.h"

#include <numeric>

using Mantid::API::LogFilterGenerator;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Kernel::LogFilter;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Types::Core::DateAndTime;

class LogFilterGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LogFilterGeneratorTest *createSuite() {
    return new LogFilterGeneratorTest();
  }
  static void destroySuite(LogFilterGeneratorTest *suite) { delete suite; }

  void test_logDoesNotExist_throws() {
    auto ws = createTestWorkspace();
    LogFilterGenerator generator(LogFilterGenerator::FilterType::Status, ws);
    TS_ASSERT_THROWS(generator.generateFilter("NonExistentLog"),
                     const std::invalid_argument &);
  }

  void test_logExistsButIsNotNumericTimeSeries_throws() {
    auto ws = createTestWorkspace();
    LogFilterGenerator generator(LogFilterGenerator::FilterType::Status, ws);
    TS_ASSERT_THROWS(generator.generateFilter("BadLog"),
                     const std::invalid_argument &);
  }

  void test_typeIsNone_noFilterReturned() {
    auto ws = createTestWorkspace();
    LogFilterGenerator generator(LogFilterGenerator::FilterType::None, ws);
    std::unique_ptr<LogFilter> filter;
    TS_ASSERT_THROWS_NOTHING(filter = generator.generateFilter("TestLog"));
    TS_ASSERT(!filter->filter());
  }

  void test_typeIsStatus_noRunningLogPresent_thenNoFilterReturned() {
    auto ws = createTestWorkspace(false, false, false);
    LogFilterGenerator generator(LogFilterGenerator::FilterType::Status, ws);
    std::unique_ptr<LogFilter> filter;
    TS_ASSERT_THROWS_NOTHING(filter = generator.generateFilter("TestLog"));
    TS_ASSERT(!filter->filter());
  }

  void test_typeIsStatus() {
    auto ws = createTestWorkspace();
    LogFilterGenerator generator(LogFilterGenerator::FilterType::Status, ws);
    std::unique_ptr<LogFilter> filter;
    TS_ASSERT_THROWS_NOTHING(filter = generator.generateFilter("TestLog"));
    TS_ASSERT(filter->filter());
    const auto &resultMap = filter->filter()->valueAsCorrectMap();
    std::map<DateAndTime, bool> expected;
    expected.emplace(DateAndTime("2007-11-30T16:17:00"), true);
    expected.emplace(DateAndTime("2007-11-30T16:17:30"), false);
    expected.emplace(DateAndTime("2007-11-30T16:18:00"), true);
    TS_ASSERT_EQUALS(resultMap.size(), 3);
    TS_ASSERT_EQUALS(resultMap, expected);
  }

  void test_typeIsPeriod_noPeriodLogPresent_thenNoFilterReturned() {
    auto ws = createTestWorkspace(true, false, false);
    LogFilterGenerator generator(LogFilterGenerator::FilterType::Period, ws);
    std::unique_ptr<LogFilter> filter;
    TS_ASSERT_THROWS_NOTHING(filter = generator.generateFilter("TestLog"));
    TS_ASSERT(!filter->filter());
  }

  void test_typeIsPeriod() {
    auto ws = createTestWorkspace();
    LogFilterGenerator generator(LogFilterGenerator::FilterType::Period, ws);
    std::unique_ptr<LogFilter> filter;
    TS_ASSERT_THROWS_NOTHING(filter = generator.generateFilter("TestLog"));
    TS_ASSERT(filter->filter());
    const auto &values = filter->filter()->valueAsCorrectMap();
    TS_ASSERT(!values.empty());
    std::map<DateAndTime, bool> expected;
    expected.emplace(DateAndTime("2007-11-30T16:18:20"), true);
    expected.emplace(DateAndTime("2007-11-30T16:18:50"), false);
    TS_ASSERT_EQUALS(expected, values);
  }

  void test_typeIsStatusAndPeriod() {
    auto ws = createTestWorkspace();
    LogFilterGenerator generator(
        LogFilterGenerator::FilterType::StatusAndPeriod, ws);
    std::unique_ptr<LogFilter> filter;
    TS_ASSERT_THROWS_NOTHING(filter = generator.generateFilter("TestLog"));
    TS_ASSERT(filter->filter());
    const auto &values = filter->filter()->valueAsCorrectMap();
    TS_ASSERT(!values.empty());
    std::map<DateAndTime, bool> expected;
    // This is an "intersection" (&&):
    //   Time    Status   Period   Result
    // 16:17:00    T        F        F
    // 16:17:30    F        F        F
    // 16:18:00    T        F        F
    // 16:18:20    T        T        T
    // 16:18:50    T        F        F
    expected.emplace(DateAndTime("2007-11-30T16:17:00"), false);
    expected.emplace(DateAndTime("2007-11-30T16:17:30"), false);
    expected.emplace(DateAndTime("2007-11-30T16:18:00"), false);
    expected.emplace(DateAndTime("2007-11-30T16:18:20"), true);
    expected.emplace(DateAndTime("2007-11-30T16:18:50"), false);
    TS_ASSERT_EQUALS(expected, values);
  }

private:
  /**
   * Generate a test workspace.
   * @param hasStatusLog :: [input] Whether to include a "running" log
   * @param hasPeriodLog :: [input] Whether to include a "period 1" log
   * @param hasBadLog :: [input] Whether to include a log that is not a numeric
   * TSP
   * @returns :: Test workspace with required logs
   */
  MatrixWorkspace_sptr createTestWorkspace(bool hasStatusLog = true,
                                           bool hasPeriodLog = true,
                                           bool hasBadLog = true) {
    MatrixWorkspace_sptr ws = boost::make_shared<WorkspaceTester>();
    const std::vector<double> xData{0.0, 1.0}, yCounts{25.0}, errors{5.0};
    ws->initialize(1, xData.size(), yCounts.size());
    ws->setBinEdges(0, xData);
    ws->setCounts(0, yCounts);
    ws->setCountStandardDeviations(0, errors);

    // Create the log to be filtered
    auto log =
        std::make_unique<TimeSeriesProperty<double>>("TestLog");
    constexpr size_t logSize(12);
    const DateAndTime initialTime("2007-11-30T16:17:00");
    std::vector<DateAndTime> times;
    std::vector<double> values;
    times.reserve(logSize);
    values.reserve(logSize);
    constexpr double incrementSecs(10.0);
    for (size_t i = 0; i < logSize; ++i) {
      const double val = static_cast<double>(i);
      times.push_back(initialTime + val * incrementSecs);
      values.push_back(val);
    }
    log->addValues(times, values);
    ws->mutableRun().addLogData(std::move(log));

    // Status ("running") log
    if (hasStatusLog) {
      auto status =
          std::make_unique<TimeSeriesProperty<bool>>("running");
      status->addValue(initialTime, true);
      status->addValue(initialTime + 30.0, false);
      status->addValue(initialTime + 60.0, true);
      ws->mutableRun().addLogData(std::move(status));
    }

    // Period log
    if (hasPeriodLog) {
      auto period =
          std::make_unique<TimeSeriesProperty<bool>>("period 1");
      period->addValue(initialTime + 80.0, true);
      period->addValue(initialTime + 110.0, false);
      ws->mutableRun().addLogData(std::move(period));
    }

    // Log that isn't a numeric TSP
    if (hasBadLog) {
      auto bad = std::make_unique<TimeSeriesProperty<std::string>>(
          "BadLog");
      bad->addValue(initialTime + 15.0, "hello");
      bad->addValue(initialTime + 45.0, "string");
      ws->mutableRun().addLogData(std::move(bad));
    }

    return ws;
  }
};

#endif /* MANTID_API_LOGFILTERGENERATORTEST_H_ */
