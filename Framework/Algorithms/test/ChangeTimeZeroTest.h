#ifndef CHANGETIMEZEROTEST_H_
#define CHANGETIMEZEROTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAlgorithms/ChangeTimeZero.h"
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/DateTimeValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

namespace {

const std::string doubleSeriesID("proton_charge");
const std::string boolSeriesID("boolTimeSeries");
const std::string intSeriesID("intTimeSeries");
const std::string stringSeriesID("stringTimeSeries");
const std::string stringID("string");
DateAndTime stringPropertyTime("2010-01-01T00:10:00");
enum LogType { STANDARD, NOPROTONCHARGE };

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

// Provide the logs for matrix workspaces
void provideLogs(LogType logType, Mantid::API::MatrixWorkspace_sptr ws,
                 DateAndTime startTime, int length) {
  switch (logType) {
  case (STANDARD):
    // Use one of each
    addTimeSeriesLogToWorkspace<double>(ws, doubleSeriesID, startTime, 1.0,
                                        length);
    addTimeSeriesLogToWorkspace<bool>(ws, boolSeriesID, startTime, true,
                                      length);
    addTimeSeriesLogToWorkspace<double>(ws, intSeriesID, startTime, 1, length);
    addTimeSeriesLogToWorkspace<std::string>(ws, stringSeriesID, startTime,
                                             "default", length);
    addProperyWithValueToWorkspace<std::string>(
        ws, stringID, stringPropertyTime.toISO8601String());
    break;
  case (NOPROTONCHARGE):
    addTimeSeriesLogToWorkspace<bool>(ws, boolSeriesID, startTime, true,
                                      length);
    addTimeSeriesLogToWorkspace<double>(ws, intSeriesID, startTime, 1, length);
    addTimeSeriesLogToWorkspace<std::string>(ws, stringSeriesID, startTime,
                                             "default", length);
    addProperyWithValueToWorkspace<std::string>(
        ws, stringID, stringPropertyTime.toISO8601String());
    break;
  default:
    addTimeSeriesLogToWorkspace<double>(ws, doubleSeriesID, startTime, 1.0,
                                        length);
    addTimeSeriesLogToWorkspace<bool>(ws, boolSeriesID, startTime, true,
                                      length);
    addTimeSeriesLogToWorkspace<double>(ws, intSeriesID, startTime, 1, length);
    addTimeSeriesLogToWorkspace<std::string>(ws, stringSeriesID, startTime,
                                             "default", length);
    addProperyWithValueToWorkspace<std::string>(
        ws, stringID, stringPropertyTime.toISO8601String());
    break;
  }
}

// Provides a 2D workspace with a log
Mantid::API::MatrixWorkspace_sptr provideWorkspace2D(LogType logType,
                                                     std::string wsName,
                                                     DateAndTime startTime,
                                                     int length) {
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
  provideLogs(logType, ws, startTime, length);
  return ws;
}

// Provides a WorkspaceSingleValue with a log
Mantid::API::MatrixWorkspace_sptr
provideWorkspaceSingleValue(LogType logType, DateAndTime startTime,
                            int length) {
  auto ws = WorkspaceCreationHelper::createWorkspaceSingleValue(10);
  // Add the logs
  provideLogs(logType, ws, startTime, length);
  return ws;
}

// Provides an EventWorkspace with a log
Mantid::API::MatrixWorkspace_sptr
provideEventWorkspaceCustom(LogType logType, DateAndTime startTime, int length,
                            int pixels, int bins, int events) {
  auto ws = WorkspaceCreationHelper::createEventWorkspaceWithStartTime(
      pixels, bins, events, 0.0, 1.0, 2, 0, startTime);
  // Add the logs
  provideLogs(logType, ws, startTime, length);
  return ws;
}

// Provides an EventWorkspace with a log
Mantid::API::MatrixWorkspace_sptr
provideEventWorkspace(LogType logType, DateAndTime startTime, int length) {
  return provideEventWorkspaceCustom(logType, startTime, length, 100, 100, 100);
}

MatrixWorkspace_sptr execute_change_time(MatrixWorkspace_sptr in_ws,
                                         double relativeTimeOffset,
                                         std::string absolutTimeOffset) {
  // Create and run the algorithm
  ChangeTimeZero alg;
  alg.initialize();
  alg.setChild(true);
  alg.setProperty("InputWorkspace", in_ws);
  alg.setPropertyValue("OutputWorkspace", "out_ws");
  alg.setProperty("RelativeTimeOffset", relativeTimeOffset);
  alg.setProperty("AbsoluteTimeOffset", absolutTimeOffset);
  alg.execute();

  // Get the result and return it
  auto out_ws = alg.getProperty("OutputWorkspace");
  return out_ws;
}
}

class ChangeTimeZeroTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChangeTimeZeroTest *createSuite() { return new ChangeTimeZeroTest; }
  static void destroySuite(ChangeTimeZeroTest *suite) { delete suite; }

  ChangeTimeZeroTest()
      : m_startTime("2010-01-01T00:00:00"),
        m_stringPropertyTime(stringPropertyTime),
        m_dateTimeValidator(boost::make_shared<DateTimeValidator>()),
        m_length(10) {}

  void test_Init() {
    ChangeTimeZero alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  // Workspace2D tests
  void
  test_changed_time_for_standard_setting_and_relative_time_and_differnt_inOutWS_and_Workspace2D() {
    // Arrange
    bool inputEqualsOutputWorkspace = false;
    const std::string absoluteTimeShift = "";
    const double relativeTimeShift = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspace2D(LogType::STANDARD, "in_ws", m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift, ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_absolute_time_and_differnt_inOutWS_and_Workspace2D() {
    // Arrange
    bool inputEqualsOutputWorkspace = false;
    DateAndTime absoluteTimeShift(m_startTime);
    absoluteTimeShift += 1000.0;
    const double relativeTimeShift = 0.0;

    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspace2D(LogType::STANDARD, "in_ws", m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift.toISO8601String(), ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_relative_time_and_same_inOutWS_and_Workspace2D() {
    // Arrange
    bool inputEqualsOutputWorkspace = true;
    const std::string absoluteTimeShift = "";
    const double relativeTimeShift = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspace2D(LogType::STANDARD, "in_ws", m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift, ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_absolute_time_and_same_inOutWS_and_Workspace2D() {
    // Arrange
    bool inputEqualsOutputWorkspace = true;
    DateAndTime absoluteTimeShift(m_startTime);
    absoluteTimeShift += 1000.0;
    const double relativeTimeShift = 0.0;
    auto ws =
        provideWorkspace2D(LogType::STANDARD, "in_ws", m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift.toISO8601String(), ws,
                   inputEqualsOutputWorkspace);
  }

  // Absolute times and no proton charges
  void test_exception_is_thrown_for_missing_proton_charge_and_absolute_time() {
    // Arrange
    const double timeShiftDouble = 1000;
    DateAndTime absoluteTimeShift = m_startTime + timeShiftDouble;
    auto ws = provideWorkspace2D(LogType::NOPROTONCHARGE, "in_ws", m_startTime,
                                 m_length);

    // Act
    ChangeTimeZero alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", ws);
    ScopedWorkspace scopedWorkspace(ws);
    alg.setPropertyValue("OutputWorkspace", scopedWorkspace.name());
    alg.setPropertyValue("AbsoluteTimeOffset",
                         absoluteTimeShift.toISO8601String());

    // We expect to see an exception because we are using absolute times and
    // there is no proton charge log
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void
  test_no_exception_is_thrown_for_missing_proton_charge_and_relative_time() {
    // Arrange
    double timeShift = 1000;
    auto ws = provideWorkspace2D(LogType::NOPROTONCHARGE, "in_ws", m_startTime,
                                 m_length);

    // Act
    ChangeTimeZero alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", ws);
    ScopedWorkspace scopedWorkspace(ws);
    alg.setPropertyValue("OutputWorkspace", scopedWorkspace.name());
    alg.setProperty("RelativeTimeOffset", timeShift);

    // We expect to see no exception because we are using realative times
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  // EventWorkspaces tests
  void
  test_changed_time_for_standard_setting_and_relative_time_and_differnt_inOutWS_and_EventWorkspace() {
    // Arrange
    bool inputEqualsOutputWorkspace = false;
    const std::string absoluteTimeShift = "";
    const double relativeTimeShift = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift, ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_absolute_time_and_differnt_inOutWS_and_EventWorkspace() {
    // Arrange
    bool inputEqualsOutputWorkspace = false;
    DateAndTime absoluteTimeShift(m_startTime);
    absoluteTimeShift += 1000.0;
    const double relativeTimeShift = 0.0;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift.toISO8601String(), ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_relative_time_and_same_inOutWS_and_EventWorkspace() {
    // Arrange
    bool inputEqualsOutputWorkspace = true;
    const std::string absoluteTimeShift = "";
    const double relativeTimeShift = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift, ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_absolute_time_and_same_inOutWS_and_EventWorkspace() {
    // Arrange
    bool inputEqualsOutputWorkspace = true;
    DateAndTime absoluteTimeShift(m_startTime);
    absoluteTimeShift += 1000.0;
    const double relativeTimeShift = 0.0;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift.toISO8601String(), ws,
                   inputEqualsOutputWorkspace);
  }

  // Negative and fractional relative times
  void
  test_changed_time_for_standard_setting_and_relative_negative_time_and_same_inOutWS() {
    // Arrange
    bool inputEqualsOutputWorkspace = true;
    const std::string absoluteTimeShift = "";
    const double relativeTimeShift = -1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift, ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_relative_fractional_time_and_same_inOutWS() {
    // Arrange
    bool inputEqualsOutputWorkspace = true;
    const std::string absoluteTimeShift = "";
    const double relativeTimeShift = 1020.5;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideEventWorkspace(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift, ws,
                   inputEqualsOutputWorkspace);
  }

  // WorkspaceSingleValue tests
  void
  test_changed_time_for_standard_setting_and_relative_time_and_differnt_inOutWS_and_WorkspaceSingleValue() {
    // Arrange
    bool inputEqualsOutputWorkspace = false;
    const std::string absoluteTimeShift = "";
    const double relativeTimeShift = 1000;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspaceSingleValue(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift, ws,
                   inputEqualsOutputWorkspace);
  }

  void
  test_changed_time_for_standard_setting_and_absolute_time_and_differnt_inOutWS_and_WorkspaceSingleValue() {
    // Arrange
    bool inputEqualsOutputWorkspace = false;
    DateAndTime absoluteTimeShift(m_startTime);
    absoluteTimeShift += 1000.0;
    const double relativeTimeShift = 0.0;
    Mantid::API::MatrixWorkspace_sptr ws =
        provideWorkspaceSingleValue(LogType::STANDARD, m_startTime, m_length);

    // Act and assert
    act_and_assert(relativeTimeShift, absoluteTimeShift.toISO8601String(), ws,
                   inputEqualsOutputWorkspace);
  }

  /**
 * Test that the algorithm can handle a WorkspaceGroup as input without
 * crashing
 * We have to use the ADS to test WorkspaceGroups
 *
 * Need to use absolute time to test this part of validateInputs
 */
  void testValidateInputsWithWSGroup() {
    const double timeShiftDouble = 1000;
    DateAndTime absoluteTimeShift = m_startTime + timeShiftDouble;
    auto ws1 = provideWorkspace2D(LogType::NOPROTONCHARGE, "in_ws", m_startTime,
                                  m_length);
    auto ws2 = provideWorkspace2D(LogType::NOPROTONCHARGE, "in_ws", m_startTime,
                                  m_length);
    AnalysisDataService::Instance().add("workspace1", ws1);
    AnalysisDataService::Instance().add("workspace2", ws2);
    auto group = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().add("group", group);
    group->add("workspace1");
    group->add("workspace2");
    ChangeTimeZero alg;
    alg.initialize();
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "group"));
    alg.setPropertyValue("OutputWorkspace", "__NoName");
    alg.setPropertyValue("AbsoluteTimeOffset",
                         absoluteTimeShift.toISO8601String());
    TS_ASSERT_THROWS_NOTHING(alg.validateInputs());
    AnalysisDataService::Instance().clear();
  }

private:
  DateAndTime m_startTime;
  DateAndTime m_stringPropertyTime;
  boost::shared_ptr<Mantid::Kernel::DateTimeValidator> m_dateTimeValidator;
  int m_length;

  // act and assert
  void act_and_assert(double relativeTimeShift, std::string absoluteTimeShift,
                      MatrixWorkspace_sptr in_ws,
                      bool inputEqualsOutputWorkspace) {
    // Create a duplicate workspace
    EventWorkspace_sptr duplicate_ws;
    if (auto in_event = boost::dynamic_pointer_cast<EventWorkspace>(in_ws)) {
      duplicate_ws = createComparisonWorkspace(in_event);
    }

    // Act
    ChangeTimeZero alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", in_ws);

    // Check if we perform our operation in place or use an new output workspace
    ScopedWorkspace scopedWorkspace;
    if (inputEqualsOutputWorkspace) {
      scopedWorkspace.set(in_ws);
      alg.setPropertyValue("OutputWorkspace", scopedWorkspace.name());
    } else {
      alg.setPropertyValue("OutputWorkspace", "out_ws");
    }

    alg.setProperty("RelativeTimeOffset", relativeTimeShift);
    alg.setProperty("AbsoluteTimeOffset", absoluteTimeShift);

    // Assert
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    double timeShift = relativeTimeShift;
    if (relativeTimeShift == 0.0) {
      timeShift = DateAndTime::secondsFromDuration(
          DateAndTime(absoluteTimeShift) - m_startTime);
    }

    MatrixWorkspace_sptr out_ws = alg.getProperty("OutputWorkspace");
    do_test_shift(out_ws, timeShift, duplicate_ws);
  }

  // perform the verification
  void do_test_shift(MatrixWorkspace_sptr ws, const double timeShift,
                     MatrixWorkspace_sptr duplicate) const {
    // Check the logs
    TS_ASSERT(ws);

    auto logs = ws->run().getLogData();
    // Go over each log and check the times
    for (auto &log : logs) {
      if (dynamic_cast<Mantid::Kernel::ITimeSeriesProperty *>(log)) {
        do_check_time_series(log, timeShift);
      } else if (dynamic_cast<PropertyWithValue<std::string> *>(log)) {
        do_check_property_with_string_value(log, timeShift);
      }
    }

    // Check the neutrons
    if (auto outWs = boost::dynamic_pointer_cast<EventWorkspace>(ws)) {
      do_check_workspace(outWs, timeShift, duplicate);
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
    for (auto &time : times) {
      double secs = DateAndTime::secondsFromDuration(time - m_startTime);
      TSM_ASSERT_DELTA("Time series logs should have shifted times.", secs,
                       secondCounter, 1e-5);
      ++secondCounter;
    }
  }
  // Check contents of a string value property.
  void do_check_property_with_string_value(Property *prop,
                                           const double timeShift) const {
    auto propertyWithValue =
        dynamic_cast<PropertyWithValue<std::string> *>(prop);
    auto value = propertyWithValue->value();
    if (checkDateTime(value)) {
      DateAndTime newTime(value);
      double secs =
          DateAndTime::secondsFromDuration(newTime - m_stringPropertyTime);
      TSM_ASSERT_DELTA("String property should have shifted time", secs,
                       timeShift, 1e-5);
    }
  }
  // Check contents of an event workspace. We compare the time stamps to the
  // time stamps of the duplicate workspace
  void do_check_workspace(EventWorkspace_sptr ws, double timeShift,
                          MatrixWorkspace_sptr duplicate) const {
    // Get the duplicate input workspace for comparison reasons
    auto duplicateWs = boost::dynamic_pointer_cast<EventWorkspace>(duplicate);

    // For each workspace index
    for (size_t workspaceIndex = 0; workspaceIndex < ws->getNumberHistograms();
         ++workspaceIndex) {

      auto &eventList = ws->getSpectrum(workspaceIndex);
      auto &eventListDuplicate = duplicateWs->getSpectrum(workspaceIndex);

      auto &events = eventList.getEvents();
      auto &eventsDuplicate = eventListDuplicate.getEvents();

      for (unsigned int i = 0; i < events.size(); ++i) {
        double secs = DateAndTime::secondsFromDuration(
            events[i].pulseTime() - eventsDuplicate[i].pulseTime());
        // Don't print a message here, as we iterate over all events
        TS_ASSERT_DELTA(secs, timeShift, 1e-5);
      }
    }
  }

  // Create comparison workspace
  EventWorkspace_sptr
  createComparisonWorkspace(EventWorkspace_sptr inputWorkspace) {
    CloneWorkspace alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty<Workspace_sptr>(
        "InputWorkspace",
        boost::dynamic_pointer_cast<Workspace>(inputWorkspace));
    alg.setProperty("OutputWorkspace", "outWs");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    Workspace_sptr temp = alg.getProperty("OutputWorkspace");
    auto output = boost::dynamic_pointer_cast<EventWorkspace>(temp);
    return output;
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

//-------------------------------------------------------------------
// PERFORMANCE TESTS
//-------------------------------------------------------------------
class ChangeTimeZeroTestPerformance : public CxxTest::TestSuite {
private:
  Mantid::API::MatrixWorkspace_sptr m_workspace2D;
  Mantid::API::MatrixWorkspace_sptr m_workspaceEvent;
  Mantid::API::MatrixWorkspace_sptr m_workspaceEvent_USEONCE;

public:
  void setUp() override {

    DateAndTime date("2010-01-01T00:00:00");

    // Set up the Workspace 2D
    const int length2D = 3000;
    const std::string ws2DName = "WS2D_REUSE";
    m_workspace2D =
        provideWorkspace2D(LogType::STANDARD, ws2DName, date, length2D);

    // Set up the Event Workspace
    const int lengthEvent = 1000;
    const int pixels = 3000;
    const int bins = 3000;
    const int events = 1000;
    m_workspaceEvent = provideEventWorkspaceCustom(
        LogType::STANDARD, date, lengthEvent, pixels, bins, events);
  }

  void test_change_zero_time_for_Workspace2D() {
    execute_change_time(m_workspace2D, 1000, "");
  }

  void test_change_zero_time_for_EventWorkspace() {
    execute_change_time(m_workspaceEvent, 1000, "");
  }
};

#endif /* MANTID_ALGORITHMS_CHANGETIMEZEROTEST_H_ */
