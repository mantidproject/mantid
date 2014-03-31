#ifndef MANTID_ALGORITHMS_ADDTIMESERIESLOGTEST_H_
#define MANTID_ALGORITHMS_ADDTIMESERIESLOGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/AddTimeSeriesLog.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class AddTimeSeriesLogTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddTimeSeriesLogTest *createSuite() { return new AddTimeSeriesLogTest(); }
  static void destroySuite( AddTimeSeriesLogTest *suite ) { delete suite; }

  void test_Workspace2D()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(ws, "Test Name", "2010-09-14T04:20:12", 20.0));
    checkLogWithEntryExists(ws, "Test Name", "2010-09-14T04:20:12", 20.0, 0);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(ws, "Test Name", "2010-09-14T04:20:19", 40.0));
    checkLogWithEntryExists(ws, "Test Name", "2010-09-14T04:20:19", 40.0, 1);

  }

  void test_EventWorkspace()
  {
    auto ws = WorkspaceCreationHelper::CreateEventWorkspace(10,10);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(ws, "Test Name", "2010-09-14T04:20:12", 20.0));
    checkLogWithEntryExists(ws, "Test Name", "2010-09-14T04:20:12", 20.0, 0);
    TS_ASSERT_THROWS_NOTHING(executeAlgorithm(ws, "Test Name", "2010-09-14T04:20:19", 40.0));
    checkLogWithEntryExists(ws, "Test Name", "2010-09-14T04:20:19", 40.0, 1);
  }

  //-------------------------- Failure cases ------------------------------------
  void test_empty_log_name_not_allowed()
  {
    Mantid::Algorithms::AddTimeSeriesLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Name", ""), std::invalid_argument);
  }

  void test_empty_time_not_allowed()
  {
    Mantid::Algorithms::AddTimeSeriesLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Time", ""), std::invalid_argument);
  }

  void test_empty_value_not_allowed()
  {
    Mantid::Algorithms::AddTimeSeriesLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Value", ""), std::invalid_argument);
  }

  void test_time_as_non_iso_formatted_string_throws_invalid_argument()
  {
    Mantid::Algorithms::AddTimeSeriesLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Time", "NotATime"), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("Time", "2014 03 31 09 30"), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("Time", "09:30:00"), std::invalid_argument);
  }

  void test_algorithm_fails_if_log_exists_but_is_not_a_time_series()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(10,10);
    auto & run = ws->mutableRun();
    run.addProperty<double>("Test Name", 1.0);
    TS_ASSERT_THROWS(executeAlgorithm(ws, "Test Name", "2010-09-14T04:20:12", 20.0), std::invalid_argument);
  }

private:

  void executeAlgorithm(Mantid::API::MatrixWorkspace_sptr testWS, const std::string & logName, const std::string & logTime,
                        const double logValue)
  {
    //execute algorithm
    Mantid::Algorithms::AddTimeSeriesLog alg;
    alg.setChild(true); // not in ADS
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() )

    alg.setProperty("Workspace", testWS);
    alg.setPropertyValue("Name", logName);
    alg.setPropertyValue("Time", logTime);
    alg.setProperty("Value", logValue);
    alg.setRethrows(true);
    alg.execute();
  }

  void checkLogWithEntryExists(Mantid::API::MatrixWorkspace_sptr testWS, const std::string & logName, const std::string & logTime,
                               const double logValue, const size_t position)
  {
    using Mantid::Kernel::DateAndTime;
    using Mantid::Kernel::TimeSeriesProperty;

    const auto & run = testWS->run();
    TSM_ASSERT("Run does not contain the expected log entry", run.hasProperty(logName));

    auto *prop = run.getLogData(logName);
    auto *timeSeries = dynamic_cast<TimeSeriesProperty<double>*>(prop);
    TSM_ASSERT("A log entry with the given name exists but it is not a time series", timeSeries);
    auto times = timeSeries->timesAsVector();
    TS_ASSERT(times.size() >= position + 1);
    auto values = timeSeries->valuesAsVector();
    TS_ASSERT_EQUALS(DateAndTime(logTime), times[position]);

    TS_ASSERT(values.size() >= position + 1);
    TS_ASSERT_EQUALS(logValue, values[position]);
  }
};


#endif /* MANTID_ALGORITHMS_ADDTIMESERIESLOGTEST_H_ */
