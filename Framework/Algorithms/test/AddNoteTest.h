// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_ADDNOTETEST_H_
#define MANTID_ALGORITHMS_ADDNOTETEST_H_

#include "MantidAlgorithms/AddNote.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

class AddNoteTest : public CxxTest::TestSuite {

private:
  enum UpdateType { Update, Delete };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddNoteTest *createSuite() { return new AddNoteTest(); }
  static void destroySuite(AddNoteTest *suite) { delete suite; }

  void test_delete_existing_removes_complete_log_first() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(
        ws, "Test Name", "2010-09-14T04:20:12", "First Test String"));
    checkLogWithEntryExists<std::string>(ws, "Test Name", "2010-09-14T04:20:12",
                                         0, "First Test String", 0);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(
        ws, "Test Name", "2010-09-14T04:20:19", "Second Test String", Delete));
    checkLogWithEntryExists<std::string>(ws, "Test Name", "2010-09-14T04:20:19",
                                         0, "Second Test String", 0);
  }

  void test_empty_time_property_produces_current_time_in_log_output() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    // Get Current Date Time
    namespace pt = boost::posix_time;
    auto dateTimeObj =
        Mantid::Types::Core::DateAndTime(pt::second_clock::local_time());
    std::string time = dateTimeObj.toISO8601String();
    std::string timeOffset = time;
    TS_ASSERT_THROWS_NOTHING(
        executeAlgorithm(ws, "Test Time", "", "Test String"));
    checkLogWithEntryExists<std::string>(ws, "Test Time", time, 1,
                                         "Test String", 0);
  }

  //-------------------------- Failure cases----------------------------
  void test_empty_log_name_not_allowed() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Name", ""), const std::invalid_argument &);
  }

  void test_empty_value_not_allowed() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Value", ""), const std::invalid_argument &);
  }

  void test_empty_time_is_allowed() {
    Mantid::Algorithms::AddNote alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Time", ""));
  }

  void test_algorithm_fails_if_log_exists_but_is_not_a_time_series() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto &run = ws->mutableRun();
    run.addProperty<std::string>("Test Name", "Test");
    TS_ASSERT_THROWS(
        executeAlgorithm(ws, "Test Name", "2010-09-14T04:20:12", "Test String"),
        const std::invalid_argument &);
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
                               const std::string &logStartTime,
                               const int &logEndTime,
                               const std::string logValue,
                               const size_t position) {
    using Mantid::Kernel::TimeSeriesProperty;
    using Mantid::Types::Core::DateAndTime;

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
    if (logEndTime == 0) {
      TS_ASSERT_EQUALS(DateAndTime(logStartTime), times[position]);
    } else {
      int logMinTime = 0, logMaxTime = 0;
      TS_ASSERT_THROWS_NOTHING(
          logMinTime = (times[position].toISO8601String().at(15)) - '0');
      TS_ASSERT_THROWS_NOTHING(logMaxTime =
                                   (logStartTime.at(15) + logEndTime) - '0');
      const int remainder = logMaxTime - logMinTime;
      TS_ASSERT_LESS_THAN_EQUALS(0, remainder);
    }
    TS_ASSERT(values.size() >= position + 1);
    TS_ASSERT_EQUALS(logValue, values[position]);
  }
};

#endif /* MANTID_ALGORITHMS_ADDNOTETEST_H_ */
