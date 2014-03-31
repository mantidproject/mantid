#ifndef MANTID_ALGORITHMS_ADDTIMESERIESLOGTEST_H_
#define MANTID_ALGORITHMS_ADDTIMESERIESLOGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/AddTimeSeriesLog.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class AddTimeSeriesLogTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddTimeSeriesLogTest *createSuite() { return new AddTimeSeriesLogTest(); }
  static void destroySuite( AddTimeSeriesLogTest *suite ) { delete suite; }

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

private:

  void executeAlgorithm(Mantid::API::MatrixWorkspace_sptr testWS, const std::string & logName, const std::string & logTime,
                        const double logValue)
  {
    //execute algorithm
    Mantid::Algorithms::AddTimeSeriesLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() )

    alg.setProperty("Workspace", testWS);
    alg.setPropertyValue("Name", logName);
    alg.setPropertyValue("Time", logTime);
    alg.setProperty("Value", logValue);
    alg.setRethrows(true);
    alg.execute();
  }


};


#endif /* MANTID_ALGORITHMS_ADDTIMESERIESLOGTEST_H_ */
