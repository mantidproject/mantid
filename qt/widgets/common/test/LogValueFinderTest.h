// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDERTEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidQtWidgets/Common/LogValueFinder.h"

#include <QStringList>
#include <cxxtest/TestSuite.h>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::ScopedWorkspace;
using Mantid::API::WorkspaceFactory;
using Mantid::Kernel::Math::StatisticType;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Types::Core::DateAndTime;
using MantidQt::MantidWidgets::LogValueFinder;

class LogValueFinderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LogValueFinderTest *createSuite() { return new LogValueFinderTest(); }
  static void destroySuite(LogValueFinderTest *suite) { delete suite; }

  void test_getLogNames() {
    ScopedWorkspace ws1(createTestWS(1));
    ScopedWorkspace ws2(createTestWS(2));
    QStringList wsNames;
    wsNames << QString::fromStdString(ws1.name())
            << QString::fromStdString(ws2.name());
    LogValueFinder finder(wsNames);
    std::vector<std::string> logNames;
    const std::vector<std::string> expectedNames = {
        "stringProp", "dblProp", "intProp", "boolProp", "timeSeries"};
    TS_ASSERT_THROWS_NOTHING(logNames = finder.getLogNames());
    TS_ASSERT_EQUALS(logNames.size(), expectedNames.size());
    TS_ASSERT_EQUALS(logNames, expectedNames);
  }

  void test_getLogValue_byString_byIndex() {
    ScopedWorkspace ws0(createTestWS(0));
    ScopedWorkspace ws1(createTestWS(1));
    QStringList wsNames;
    wsNames << QString::fromStdString(ws0.name())
            << QString::fromStdString(ws1.name());
    LogValueFinder finder(wsNames);
    double valIndex0 = 0.;
    TS_ASSERT_THROWS_NOTHING(
        valIndex0 = finder.getLogValue("dblProp", StatisticType::Mean, 0));
    double valIndex1 = 0.;
    TS_ASSERT_THROWS_NOTHING(
        valIndex1 = finder.getLogValue("dblProp", StatisticType::Mean, 1));
    double valString0 = 0.;
    TS_ASSERT_THROWS_NOTHING(valString0 = finder.getLogValue(
                                 "dblProp", StatisticType::Mean, wsNames[0]));
    double valString1 = 0.;
    TS_ASSERT_THROWS_NOTHING(valString1 = finder.getLogValue(
                                 "dblProp", StatisticType::Mean, wsNames[1]));
    TS_ASSERT_EQUALS(valIndex0, valString0);
    TS_ASSERT_EQUALS(valIndex1, valString1);
    TS_ASSERT_DELTA(valIndex0, 0.0, 1.e-7);
    TS_ASSERT_DELTA(valIndex1, 1.0, 1.e-7);
  }

  void test_getLogValue_integer() {
    ScopedWorkspace ws0(createTestWS(0));
    ScopedWorkspace ws1(createTestWS(1));
    QStringList wsNames;
    wsNames << QString::fromStdString(ws0.name())
            << QString::fromStdString(ws1.name());
    LogValueFinder finder(wsNames);
    double val0, val1;
    TS_ASSERT_THROWS_NOTHING(
        val0 = finder.getLogValue("intProp", StatisticType::Mean, 0));
    TS_ASSERT_THROWS_NOTHING(
        val1 = finder.getLogValue("intProp", StatisticType::Mean, 1));
    TS_ASSERT_DELTA(val0, 0.0, 1.e-7);
    TS_ASSERT_DELTA(val1, 1.0, 1.e-7);
  }

  void test_getLogValue_timeSeries() {
    ScopedWorkspace ws0(createTestWS(0));
    ScopedWorkspace ws1(createTestWS(1));
    QStringList wsNames;
    wsNames << QString::fromStdString(ws0.name())
            << QString::fromStdString(ws1.name());
    LogValueFinder finder(wsNames);
    double val0, val1;
    TS_ASSERT_THROWS_NOTHING(
        val0 = finder.getLogValue("timeSeries", StatisticType::Mean, 0));
    TS_ASSERT_THROWS_NOTHING(
        val1 = finder.getLogValue("timeSeries", StatisticType::Mean, 1));
    TS_ASSERT_DELTA(val0, 4.5, 1.e-7);
    TS_ASSERT_DELTA(val1, 5.5, 1.e-7);
  }

  void test_getLogValue_bool_throws() {
    ScopedWorkspace ws0(createTestWS(0));
    ScopedWorkspace ws1(createTestWS(1));
    QStringList wsNames;
    wsNames << QString::fromStdString(ws0.name())
            << QString::fromStdString(ws1.name());
    LogValueFinder finder(wsNames);
    TS_ASSERT_THROWS(finder.getLogValue("boolProp", StatisticType::Mean, 0),
                     std::invalid_argument);
    TS_ASSERT_THROWS(finder.getLogValue("boolProp", StatisticType::Mean, 1),
                     std::invalid_argument);
  }

  void test_getLogValue_nonExistentWorkspace_throws() {
    ScopedWorkspace ws0(createTestWS(0));
    ScopedWorkspace ws1(createTestWS(1));
    QStringList wsNames;
    wsNames << QString::fromStdString(ws0.name())
            << QString::fromStdString(ws1.name());
    LogValueFinder finder(wsNames);
    TS_ASSERT_THROWS(
        finder.getLogValue("dblProp", StatisticType::Mean, "no_workspace"),
        std::invalid_argument);
  }

  void test_getLogValue_indexOutOfRange_throws() {
    ScopedWorkspace ws0(createTestWS(0));
    ScopedWorkspace ws1(createTestWS(1));
    QStringList wsNames;
    wsNames << QString::fromStdString(ws0.name())
            << QString::fromStdString(ws1.name());
    LogValueFinder finder(wsNames);
    TS_ASSERT_THROWS(finder.getLogValue("dblProp", StatisticType::Mean, 2),
                     std::invalid_argument);
  }

private:
  /// Create test workspace with logs
  MatrixWorkspace_sptr createTestWS(int logValue) {
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    auto &run = ws->mutableRun();
    run.addProperty<std::string>("stringProp", std::to_string(logValue));
    run.addProperty<double>("dblProp", static_cast<double>(logValue));
    run.addProperty<int>("intProp", logValue);
    run.addProperty<bool>("boolProp", logValue != 0);
    auto tsp =
        Mantid::Kernel::make_unique<TimeSeriesProperty<double>>("timeSeries");
    std::vector<DateAndTime> times;
    std::vector<double> values;
    for (size_t i = 0; i < 10; ++i) {
      DateAndTime t;
      const std::string &time = "2016-08-24T14:26:0" + std::to_string(i);
      t.setFromISO8601(time);
      times.push_back(t);
      values.push_back(static_cast<double>(i + logValue));
    }
    tsp->addValues(times, values);
    run.addLogData(std::move(tsp));
    return ws;
  }
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDERTEST_H_ */
