#ifndef MANTID_ALGORITHMS_CHANGETIMEZEROTEST_H_
#define MANTID_ALGORITHMS_CHANGETIMEZEROTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/ChangeTimeZero.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/DateTimeValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;


template<typename T>
void addTimeSeriesLogToWorkspace(Mantid::API::MatrixWorkspace_sptr ws, std::string id, DateAndTime startTime, T defaultValue, int length) {
  auto timeSeries = new TimeSeriesProperty<T>(id);
  timeSeries->setUnits("mm");
  for (int i = 0; i < length; i++)
  {
    timeSeries->addValue(startTime + static_cast<double>(i), defaultValue);
  }
  ws->mutableRun().addProperty(timeSeries, true);
}

template<typename T>
void addProperyWithValueToWorkspace(Mantid::API::MatrixWorkspace_sptr ws, std::string id, T value) {
  auto propWithVal = new PropertyWithValue<T>(id, value);
  propWithVal->setUnits("mm");
  ws->mutableRun().addProperty(propWithVal, true);
}

class ChangeTimeZeroTest : public CxxTest::TestSuite
{
public:
  enum LogType {STANDARD, NOPROTONCHARGE};

  ChangeTimeZeroTest::ChangeTimeZeroTest() :m_startTime("2010-01-01T00:00:00"), m_stringPropertyTime("2010-01-01T00:10:00"), m_dateTimeValidator(boost::make_shared<DateTimeValidator>()){

    m_length = 10;
    m_doubleSeriesID = "proton_charge";
    m_boolSeriesID ="boolTimeSeries";
    m_intSeriesID = "intTimeSeries";
    m_stringSeriesID = "stringTimeSeries";
    m_stringID = "string";
  }

    void test_Init()
  {
    ChangeTimeZero alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT( alg.isInitialized() )
  }

  void test_changed_time_for_standard_setting_and_relative_time_and_differnt_inOutWS_and_Workspace2D() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = "outWS";
    const std::string timeShift = "1000";
    const double timeShiftDouble = 1000;
    Mantid::API::MatrixWorkspace_sptr ws = provideWorkspace2D(LogType::STANDARD, inputWorkspaceName);

    // Act
    ChangeTimeZero alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("InputWorkspace", inputWorkspaceName);
    alg.setPropertyValue("OutputWorkspace", outputWorkspaceName);
    alg.setPropertyValue("TimeOffset", timeShift);

    // Assert
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    do_test_shift(outputWorkspaceName, timeShiftDouble);

    // Clean up
    AnalysisDataService::Instance().remove(inputWorkspaceName);
    AnalysisDataService::Instance().remove(inputWorkspaceName);
  }


void do_test_shift(const std::string& outputWorkspaceName, const double timeShift) const {
  // Check the logs
  auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWorkspaceName));
  TS_ASSERT(ws);

  auto logs = ws->run().getLogData();

  // Go over each log and check the times
  for (auto iter = logs.begin(); iter != logs.end(); ++iter) {
    auto prop = ws->run().getLogData((*iter)->name());

    if (isTimeSeries(prop)) {
      checkTimeSeries(prop, timeShift);
    } else if (dynamic_cast<PropertyWithValue<std::string> *>(prop)) {
      checkPropertyWithStringValue(prop, timeShift);
    }
  }

  // Check the neutrons
  if (auto eventWs = boost::dynamic_pointer_cast<EventWorkspace>(ws)) {
    checkWorkspace(eventWs, timeShift);
  }

}

// Check contents of time series.
void checkTimeSeries(Mantid::Kernel::Property *prop, const double timeShift) const {
  auto timeSeriesProperty = dynamic_cast<ITimeSeriesProperty*>(prop);
  auto times = timeSeriesProperty->timesAsVector();
  // Iterator over all entries of the time series and check if they are altered
  for (auto it = times.begin(); it != times.end(); ++it) {
    double secs;
    secs = DateAndTime::secondsFromDuration(*it - m_startTime);
    TS_ASSERT_DELTA(secs, timeShift, 1e-5);
  }
}
// Check contents of a string value property.
void checkPropertyWithStringValue(Property * prop, const double timeShift) const{
  auto propertyWithValue = dynamic_cast<PropertyWithValue<std::string> *>(prop);
  auto value = propertyWithValue->value();
  auto isDateTime = checkDateTime(value);
  if (isDateTime) {
    double secs;
    DateAndTime newTime(value);
    secs = DateAndTime::secondsFromDuration(newTime - m_startTime);
    TS_ASSERT_DELTA(secs, timeShift, 1e-5);
  }
}
// Check contents of an event workspace.
void checkWorkspace(EventWorkspace_sptr ws, double timeShift) const {
  // For each workspace index
  for (size_t workspaceIndex = 0;
       workspaceIndex < ws->getNumberHistograms(); ++workspaceIndex) {
    // For each event
    auto eventList = ws->getEventListPtr(workspaceIndex);
    auto events = eventList->getEvents();
    double secondCounter = timeShift;
    for (auto it = events.begin(); it != events.end(); ++it) {
      double secs = DateAndTime::secondsFromDuration(it->pulseTime() - m_startTime);
      TS_ASSERT_DELTA(secs, secondCounter, 1e-5);
      ++secondCounter;
    }
  }
}

private: 
  DateAndTime m_startTime;
  DateAndTime m_stringPropertyTime;
  boost::shared_ptr<Mantid::Kernel::DateTimeValidator> m_dateTimeValidator;
  int m_length;
  std::string m_doubleSeriesID;
  std::string m_boolSeriesID;
  std::string m_intSeriesID;
  std::string m_stringSeriesID;
  std::string m_stringID;

  // Provides a 2D workspace with a log
  Mantid::API::MatrixWorkspace_sptr provideWorkspace2D(LogType logType, std::string wsName) {
    Workspace2D_sptr ws(new Workspace2D);
    ws->setTitle(wsName);
    ws->initialize(5,2,2);
    int jj=0;
    for (int i =0; i < 2; ++i)
    {
      for (jj=0; jj<4; ++jj)
      ws->dataX(jj)[i] = 1.0*i;
      ws->dataY(jj)[i] = 2.0*i;
    }

    switch(logType) {
      case (STANDARD):
        // Use one of each
        addTimeSeriesLogToWorkspace<double>(ws, m_doubleSeriesID, m_startTime, 1.0, m_length);
        addTimeSeriesLogToWorkspace<bool>(ws, m_boolSeriesID, m_startTime, true, m_length);
        addTimeSeriesLogToWorkspace<double>(ws, m_intSeriesID, m_startTime, 1, m_length);
        addTimeSeriesLogToWorkspace<std::string>(ws, m_stringSeriesID, m_startTime, "default", m_length);
        addProperyWithValueToWorkspace<std::string>(ws, m_stringID, m_stringPropertyTime.toISO8601String());
        break;
      case (NOPROTONCHARGE):
        addTimeSeriesLogToWorkspace<bool>(ws, m_boolSeriesID, m_startTime, true, m_length);
        addTimeSeriesLogToWorkspace<double>(ws, m_intSeriesID, m_startTime, 1, m_length);
        addTimeSeriesLogToWorkspace<std::string>(ws, m_stringSeriesID, m_startTime, "default", m_length);
        addProperyWithValueToWorkspace<std::string>(ws, m_stringID, m_stringPropertyTime.toISO8601String());
        break;
      default:
        addTimeSeriesLogToWorkspace<double>(ws, m_doubleSeriesID, m_startTime, 1.0, m_length);
        addTimeSeriesLogToWorkspace<bool>(ws, m_boolSeriesID, m_startTime, true, m_length);
        addTimeSeriesLogToWorkspace<double>(ws, m_intSeriesID, m_startTime, 1, m_length);
        addTimeSeriesLogToWorkspace<std::string>(ws, m_stringSeriesID, m_startTime, "default", m_length);
        addProperyWithValueToWorkspace<std::string>(ws, m_stringID, m_stringPropertyTime.toISO8601String());
        break;
    }
    AnalysisDataService::Instance().add(wsName, ws);

    return ws;
  }

  // Check if we are dealing with a dateTime
  bool checkDateTime(const std::string& dateTime) const{
    auto isDateTime = false;
    // Hedge for bad lexical casts in the DateTimeValidator
    try {
      isDateTime = m_dateTimeValidator->isValid(dateTime) == "";
    } catch (...) {
      isDateTime = false;
    }
    return isDateTime;
  }
};

#endif /* MANTID_ALGORITHMS_CHANGETIMEZEROTEST_H_ */

