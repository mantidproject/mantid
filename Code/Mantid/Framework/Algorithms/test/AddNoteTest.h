#ifndef MANTID_ALGORITHMS_ADDNOTETEST_H_
#define MANTID_ALGORITHMS_ADDNOTETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/AddNote.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class AddNoteTest : public CxxTest::TestSuite {

private:
  enum UpdateType { Update, Delete };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddNoteTest *createSuite() { return new AddNoteTest(); }
  static void destroySuite(AddNoteTest *suite) { delete suite; }

  void test_delete_existing_removes_complete_log_first() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(
        ws, "Test Name", "2010-09-14T04:20:12", "First Test String"));
    checkLogWithEntryExists<std::string>(ws, "Test Name", "2010-09-14T04:20:12",
                                         "First Test String", 0);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(
        ws, "Test Name", "2010-09-14T04:20:19", "Second Test String", Delete));
    checkLogWithEntryExists<std::string>(ws, "Test Name", "2010-09-14T04:20:19",
                                         "Second Test String", 0);
  }

  //-------------------------- Failure cases----------------------------
  void test_empty_log_name_not_allowed() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Name", ""), std::invalid_argument);
  }

  void test_empty_time_not_allowed() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Time", ""), std::invalid_argument);
  }

  void test_empty_value_not_allowed() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Value", ""), std::invalid_argument);
  }

  void test_time_as_non_iso_formatted_string_throws_invalid_argument() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Time", "NotATime"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("Time", "2014 03 31 09 30"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("Time", "09:30:00"),
                     std::invalid_argument);
  }

  void test_algorithm_fails_if_log_exists_but_is_not_a_time_series() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    auto &run = ws->mutableRun();
    run.addProperty<std::string>("Test Name", "Test");
    TS_ASSERT_THROWS(
        executeAlgorithm(ws, "Test Name", "2010-09-14T04:20:12", "Test String"),
        std::invalid_argument);
  }

  void test_Init() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

private:
  void executeAlgorithm(Mantid::API::MatrixWorkspace_sptr testWS,
                        const std::string &logName, const std::string &logTime,
                        const std::string logValue,
                        const UpdateType update = Update) {

    Mantid::Algorithms::AddNote alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("Workspace", testWS);
    alg.setPropertyValue("Name", logName);
    alg.setPropertyValue("Time", logTime);
    alg.setProperty("Value", logValue);
    if (update == Delete) {
      alg.setProperty("DeleteExisting", true);
    }
    alg.setRethrows(true);
    alg.execute();
  }

  template <typename T>
  void checkLogWithEntryExists(Mantid::API::MatrixWorkspace_sptr testWS,
                               const std::string &logName,
                               const std::string &logTime,
                               const std::string logValue,
                               const size_t position) {
    using Mantid::Kernel::DateAndTime;
    using Mantid::Kernel::TimeSeriesProperty;

    const auto &run = testWS->run();
    TSM_ASSERT("Run does not contain the expected log entry",
               run.hasProperty(logName));

    auto *prop = run.getLogData(logName);
    auto *timeSeries = dynamic_cast<TimeSeriesProperty<T> *>(prop);
    TSM_ASSERT(
        "A log entry with the given name exists but it is not a time series",
        timeSeries);
    auto times = timeSeries->timesAsVector();
    TS_ASSERT(times.size() >= position + 1);
    auto values = timeSeries->valuesAsVector();
    TS_ASSERT_EQUALS(DateAndTime(logTime), times[position]);
    TS_ASSERT(values.size() >= position + 1);
    TS_ASSERT_EQUALS(logValue, values[position]);
  }
};

#endif /* MANTID_ALGORITHMS_ADDNOTETEST_H_ */