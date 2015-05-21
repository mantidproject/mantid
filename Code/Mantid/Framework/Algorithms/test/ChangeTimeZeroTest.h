#ifndef CHANGETIMEZEROTEST_H_
#define CHANGETIMEZEROTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidAlgorithms/ChangeTimeZero.h"
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/DateTimeValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

template <typename T>
void addTimeSeriesLogToWorkspace(Mantid::API::MatrixWorkspace_sptr ws,
                                 std::string id, DateAndTime startTime,
                                 T defaultValue, int length) {
  auto timeSeries = new TimeSeriesProperty<T>(id);
  timeSeries->setUnits("mm");
  for (int i = 0; i < length; i++) {
    timeSeries->addValue(startTime + static_cast<double>(i), defaultValue);
  }
  ws->mutableRun().addProperty(timeSeries, true);
}

template <typename T>
void addProperyWithValueToWorkspace(Mantid::API::MatrixWorkspace_sptr ws,
                                    std::string id, T value) {
  auto propWithVal = new PropertyWithValue<T>(id, value);
  propWithVal->setUnits("mm");
  ws->mutableRun().addProperty(propWithVal, true);
}

class ChangeTimeZeroTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChangeTimeZeroTest *createSuite() { return new ChangeTimeZeroTest; }
  static void destroySuite(ChangeTimeZeroTest *suite) { delete suite; }

  ChangeTimeZeroTest()
      : m_startTime("2010-01-01T00:00:00"),
        m_stringPropertyTime("2010-01-01T00:10:00"),
        m_dateTimeValidator(boost::make_shared<DateTimeValidator>()),
        m_length(10), m_doubleSeriesID("proton_charge"),
        m_boolSeriesID("boolTimeSeries"), m_intSeriesID("intTimeSeries"),
        m_stringSeriesID("stringTimeSeries"), m_stringID("string"),
        m_comparisonWorkspaceName("duplicateWs") {}

  void test_Init() {
    ChangeTimeZero alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  // Workspace2D tests
  void test_changed_time_for_standard_setting_and_relative_time_and_differnt_inOutWS_and_Workspace2D() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = "outWS";
    const std::string timeShift = "1000";
    const double timeShiftDouble = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspace2D(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_absolute_time_and_differnt_inOutWS_and_Workspace2D() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = "outWS";
    const double timeShiftDouble = 1000;
    DateAndTime abosluteTimeShift = m_startTime + timeShiftDouble;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspace2D(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, abosluteTimeShift.toISO8601String(),
                      inputWorkspaceName, outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_relative_time_and_same_inOutWS_and_Workspace2D() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = inputWorkspaceName;
    const std::string timeShift = "1000";
    const double timeShiftDouble = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspace2D(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_absolute_time_and_same_inOutWS_and_Workspace2D() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = inputWorkspaceName;
    const double timeShiftDouble = 1000;
    DateAndTime abosluteTimeShift = m_startTime + timeShiftDouble;
    provideWorkspace2D(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, abosluteTimeShift.toISO8601String(), inputWorkspaceName, outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }


  // Absolute times and no proton charges
  void test_exception_is_thrown_for_missing_proton_charge_and_absolute_time() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = inputWorkspaceName;
    const double timeShiftDouble = 1000;
    DateAndTime abosluteTimeShift = m_startTime + timeShiftDouble;
    provideWorkspace2D(LogType::NOPROTONCHARGE, inputWorkspaceName);

    // Act
    ChangeTimeZero alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("InputWorkspace", inputWorkspaceName);
    alg.setPropertyValue("OutputWorkspace", outputWorkspaceName);
    alg.setPropertyValue("TimeOffset", abosluteTimeShift.toISO8601String());

    // We expect to see an exception because we are using absolute times and there is no proton charge log
    TS_ASSERT_THROWS_ANYTHING(alg.execute());

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_no_exception_is_thrown_for_missing_proton_charge_and_relative_time() {
  // Arrange
  const std::string inputWorkspaceName = "inWS";
  const std::string outputWorkspaceName = inputWorkspaceName;
  const std::string timeShift = "1000";
  provideWorkspace2D(LogType::NOPROTONCHARGE, inputWorkspaceName);

  // Act
  ChangeTimeZero alg;
  alg.setRethrows(true);
  TS_ASSERT_THROWS_NOTHING(alg.initialize())
  TS_ASSERT(alg.isInitialized())
  alg.setPropertyValue("InputWorkspace", inputWorkspaceName);
  alg.setPropertyValue("OutputWorkspace", outputWorkspaceName);
  alg.setPropertyValue("TimeOffset", timeShift);

  // We expect to see an exception because we are using absolute times and there is no proton charge log
  TS_ASSERT_THROWS_NOTHING(alg.execute());

  // Clean up
  cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
}


  // EventWorkspaces tests
  void test_changed_time_for_standard_setting_and_relative_time_and_differnt_inOutWS_and_EventWorkspace() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = "outWS";
    const std::string timeShift = "1000";
    const double timeShiftDouble = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_absolute_time_and_differnt_inOutWS_and_EventWorkspace() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = "outWS";
    const double timeShiftDouble = 1000;
    DateAndTime abosluteTimeShift = m_startTime + timeShiftDouble;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, abosluteTimeShift.toISO8601String(),
                      inputWorkspaceName, outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_relative_time_and_same_inOutWS_and_EventWorkspace() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = inputWorkspaceName;
    const std::string timeShift = "1000";
    const double timeShiftDouble = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_absolute_time_and_same_inOutWS_and_EventWorkspace() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = inputWorkspaceName;
    const std::string timeShift = "1000";
    const double timeShiftDouble = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }


  // Negative and fractional relative times
  void test_changed_time_for_standard_setting_and_relative_negative_time_and_same_inOutWS() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = inputWorkspaceName;
    const std::string timeShift = "-1000";
    const double timeShiftDouble = -1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_relative_fractional_time_and_same_inOutWS() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = inputWorkspaceName;
    const std::string timeShift = "1000.5";
    const double timeShiftDouble = 1000.5;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }


  // WorkspaceSingleValue tests
  void test_changed_time_for_standard_setting_and_relative_time_and_differnt_inOutWS_and_WorkspaceSingleValue() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = "outWS";
    const std::string timeShift = "1000";
    const double timeShiftDouble = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
      provideWorkspaceSingleValue(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, timeShift, inputWorkspaceName,
                      outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
  }

  void test_changed_time_for_standard_setting_and_absolute_time_and_differnt_inOutWS_and_WorkspaceSingleValue() {
    // Arrange
    const std::string inputWorkspaceName = "inWS";
    const std::string outputWorkspaceName = "outWS";
    const double timeShiftDouble = 1000;
    DateAndTime abosluteTimeShift = m_startTime + timeShiftDouble;
    Mantid::API::MatrixWorkspace_sptr ws =
      provideWorkspaceSingleValue(LogType::STANDARD, inputWorkspaceName);

    // Act and assert
    do_act_and_assert(timeShiftDouble, abosluteTimeShift.toISO8601String(),
                      inputWorkspaceName, outputWorkspaceName);

    // Clean up
    cleanUpWorkspaces(inputWorkspaceName, outputWorkspaceName);
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
  std::string m_comparisonWorkspaceName;

  enum LogType { STANDARD, NOPROTONCHARGE };

  // act and assert
  void do_act_and_assert(double timeShiftDouble, std::string timeShift,
                         std::string inWsName, std::string outWsName) {
      // Act
      ChangeTimeZero alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      alg.setPropertyValue("InputWorkspace", inWsName);
      alg.setPropertyValue("OutputWorkspace", outWsName);
      alg.setPropertyValue("TimeOffset", timeShift);

      // Assert
      TS_ASSERT_THROWS_NOTHING(alg.execute());
      TS_ASSERT(alg.isExecuted());
      do_test_shift(outWsName, timeShiftDouble);
    }

  // perform the verification
  void do_test_shift(const std::string &outputWorkspaceName,
                     const double timeShift) const {
    // Check the logs
    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(outputWorkspaceName));
    TS_ASSERT(ws);

    auto logs = ws->run().getLogData();
    // Go over each log and check the times
    for (auto iter = logs.begin(); iter != logs.end(); ++iter) {
      auto prop = ws->run().getLogData((*iter)->name());

      if (isTimeSeries(prop)) {
        do_check_time_series(prop, timeShift);
      } else if (dynamic_cast<PropertyWithValue<std::string> *>(prop)) {
        do_check_property_with_string_value(prop, timeShift);
      }
    }

    // Check the neutrons
    if (auto outWs = boost::dynamic_pointer_cast<EventWorkspace>(ws)) {
      do_check_workspace(outWs, timeShift);
    }
  }
  // Check contents of time series.
  void do_check_time_series(Mantid::Kernel::Property *prop,
                            const double timeShift) const {
    auto timeSeriesProperty = dynamic_cast<ITimeSeriesProperty *>(prop);
    auto times = timeSeriesProperty->timesAsVector();
    // Iterator over all entries of the time series and check if they are
    // altered
    double secondCounter = timeShift;
    for (auto it = times.begin(); it != times.end(); ++it) {
      double secs;
      secs = DateAndTime::secondsFromDuration(*it - m_startTime);
      TSM_ASSERT_DELTA("Time series logs should have shifted times.", secs, secondCounter, 1e-5);
      ++secondCounter;
    }
  }
  // Check contents of a string value property.
  void do_check_property_with_string_value(Property *prop,
                                           const double timeShift) const {
    auto propertyWithValue =
        dynamic_cast<PropertyWithValue<std::string> *>(prop);
    auto value = propertyWithValue->value();
    auto isDateTime = checkDateTime(value);
    if (isDateTime) {
      double secs;
      DateAndTime newTime(value);
      secs = DateAndTime::secondsFromDuration(newTime - m_stringPropertyTime);
      TSM_ASSERT_DELTA("String property should have shifted time", secs, timeShift, 1e-5);
    }
  }
  // Check contents of an event workspace. We compare the time stamps to the
  // time stamps of the duplicate workspace
  void do_check_workspace(EventWorkspace_sptr ws, double timeShift) const {
    // Get the duplicate input workspace for comparison reasons
    auto duplicateWs = boost::dynamic_pointer_cast<EventWorkspace>(
        AnalysisDataService::Instance().retrieve(m_comparisonWorkspaceName));

    // For each workspace index
    for (size_t workspaceIndex = 0; workspaceIndex < ws->getNumberHistograms();
         ++workspaceIndex) {

      auto eventList = ws->getEventListPtr(workspaceIndex);
      auto eventListDuplicate = duplicateWs->getEventListPtr(workspaceIndex);

      auto events = eventList->getEvents();
      auto eventsDuplicate = eventListDuplicate->getEvents();

      for (unsigned int i = 0; i < events.size(); ++i) {
        double secs =
            DateAndTime::secondsFromDuration(events[i].pulseTime() - eventsDuplicate[i].pulseTime());
        // Don't print a message here, as we iterate over all events
        TS_ASSERT_DELTA(secs, timeShift, 1e-5);
      }
    }
  }

  // Provides a 2D workspace with a log
  Mantid::API::MatrixWorkspace_sptr provideWorkspace2D(LogType logType,
                                                       std::string wsName) {
    Workspace2D_sptr ws(new Workspace2D);
    ws->setTitle(wsName);
    ws->initialize(5, 2, 2);
    int jj = 0;
    for (int i = 0; i < 2; ++i) {
      for (jj = 0; jj < 4; ++jj)
        ws->dataX(jj)[i] = 1.0 * i;
      ws->dataY(jj)[i] = 2.0 * i;
    }
    // Add the logs
    provideLogs(logType, ws);

    AnalysisDataService::Instance().add(wsName, ws);
    return ws;
  }

  // Create comparison workspace
  void createComparisonWorkspace(std::string inputWorkspaceName) {
      CloneWorkspace alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      alg.setPropertyValue("InputWorkspace", inputWorkspaceName);
      alg.setPropertyValue("OutputWorkspace", m_comparisonWorkspaceName);

      // Assert
      TS_ASSERT_THROWS_NOTHING(alg.execute());
      TS_ASSERT(alg.isExecuted());
  }

  // Provides an EventWorkspace with a log
  Mantid::API::MatrixWorkspace_sptr provideEventWorkspace(LogType logType,
                                                       std::string wsName) {
   auto ws = WorkspaceCreationHelper::CreateEventWorkspaceWithStartTime(100, 100, 100,
                                                                       0.0, 1.0, 2,
                                                                       0, m_startTime);
    // Add the logs
    provideLogs(logType, ws);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
    createComparisonWorkspace(wsName);
    return ws;
  }

  // Provides a WorkspaceSingleValue with a log
  Mantid::API::MatrixWorkspace_sptr provideWorkspaceSingleValue(LogType logType,
                                                       std::string wsName) {
    auto ws = WorkspaceCreationHelper::CreateWorkspaceSingleValue(10);
    // Add the logs
    provideLogs(logType, ws);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
    return ws;
  }

  // Provide the logs for matrix workspaces 
  void provideLogs(LogType logType, Mantid::API::MatrixWorkspace_sptr ws) {
    switch (logType) {
    case (STANDARD):
      // Use one of each
      addTimeSeriesLogToWorkspace<double>(ws, m_doubleSeriesID, m_startTime,
                                          1.0, m_length);
      addTimeSeriesLogToWorkspace<bool>(ws, m_boolSeriesID, m_startTime, true,
                                        m_length);
      addTimeSeriesLogToWorkspace<double>(ws, m_intSeriesID, m_startTime, 1,
                                          m_length);
      addTimeSeriesLogToWorkspace<std::string>(
          ws, m_stringSeriesID, m_startTime, "default", m_length);
      addProperyWithValueToWorkspace<std::string>(
          ws, m_stringID, m_stringPropertyTime.toISO8601String());
      break;
    case (NOPROTONCHARGE):
      addTimeSeriesLogToWorkspace<bool>(ws, m_boolSeriesID, m_startTime, true,
                                        m_length);
      addTimeSeriesLogToWorkspace<double>(ws, m_intSeriesID, m_startTime, 1,
                                          m_length);
      addTimeSeriesLogToWorkspace<std::string>(
          ws, m_stringSeriesID, m_startTime, "default", m_length);
      addProperyWithValueToWorkspace<std::string>(
          ws, m_stringID, m_stringPropertyTime.toISO8601String());
      break;
    default:
      addTimeSeriesLogToWorkspace<double>(ws, m_doubleSeriesID, m_startTime,
                                          1.0, m_length);
      addTimeSeriesLogToWorkspace<bool>(ws, m_boolSeriesID, m_startTime, true,
                                        m_length);
      addTimeSeriesLogToWorkspace<double>(ws, m_intSeriesID, m_startTime, 1,
                                          m_length);
      addTimeSeriesLogToWorkspace<std::string>(
          ws, m_stringSeriesID, m_startTime, "default", m_length);
      addProperyWithValueToWorkspace<std::string>(
          ws, m_stringID, m_stringPropertyTime.toISO8601String());
      break;
    }
  }

  void cleanUpWorkspaces(std::string inputWorkspaceName, std::string outputWorkspaceName) {
    // Remove the duplicate workspace in case of an event workspace
    auto inputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(inputWorkspaceName));
    if (boost::dynamic_pointer_cast<EventWorkspace>(inputWs)) {
      AnalysisDataService::Instance().remove(m_comparisonWorkspaceName);
    }

    AnalysisDataService::Instance().remove(inputWorkspaceName);
    if (inputWorkspaceName != outputWorkspaceName) {
      AnalysisDataService::Instance().remove(outputWorkspaceName);
    }
  }

  // Check if we are dealing with a dateTime
  bool checkDateTime(const std::string &dateTime) const {
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
