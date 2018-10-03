#ifndef CHANGELOGTIMETEST_H_
#define CHANGELOGTIMETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/ChangeLogTime.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <cxxtest/TestSuite.h>

using std::string;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

class ChangeLogTimeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChangeLogTimeTest *createSuite() { return new ChangeLogTimeTest(); }
  static void destroySuite(ChangeLogTimeTest *suite) { delete suite; }

  /// Set up the parameters for what the tests do.
  ChangeLogTimeTest() {
    length = 10;
    logname = "fakelog";
    start_str = "2011-07-14T12:00Z"; // Noon on Bastille day 2011.
  }

  void testCopyHist() { this->verify("ChangeLogTime_in", "ChangeLogTime_out"); }

  void testInplace() { this->verify("ChangeLogTime", "ChangeLogTime"); }

private:
  /// Name of the log to create/modify.
  std::string logname;
  /// Number of log elements.
  int length;
  /// The string version of the start date/time for the log.
  std::string start_str;

  /**
   * Run the actual test including checking the results and cleanup.
   *
   * @param in_name Name of the input workspace.
   * @param out_name Name of the output workspace.
   */
  void verify(const std::string in_name, const std::string out_name) {
    DateAndTime start(start_str);

    // create a workspace to mess with
    Workspace2D_sptr testWorkspace(new Workspace2D);
    testWorkspace->setTitle("input2D");
    testWorkspace->initialize(5, 2, 2);
    int jj = 0;
    for (int i = 0; i < 2; ++i) {
      for (jj = 0; jj < 4; ++jj)
        testWorkspace->dataX(jj)[i] = 1.0 * i;
      testWorkspace->dataY(jj)[i] = 2.0 * i;
    }
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>(logname);
    log->setUnits("furlongs");
    for (int i = 0; i < length; i++) {
      log->addValue(start + static_cast<double>(i), static_cast<double>(i));
    }
    testWorkspace->mutableRun().addProperty(log, true);
    AnalysisDataService::Instance().add(in_name, testWorkspace);

    // set up the algorithm
    ChangeLogTime alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", in_name);
    alg.setPropertyValue("OutputWorkspace", out_name);
    alg.setPropertyValue("LogName", logname);
    alg.setPropertyValue("TimeOffset", ".1");

    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // verify the results
    Workspace2D_sptr outWorkspace =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(out_name);
    TimeSeriesProperty<double> *newlog =
        dynamic_cast<TimeSeriesProperty<double> *>(
            outWorkspace->run().getLogData(logname));
    TS_ASSERT(newlog);
    TS_ASSERT(!newlog->units().empty());
    TS_ASSERT_EQUALS(length, newlog->size());
    TS_ASSERT_EQUALS(start + .1, newlog->firstTime());

    // cleanup
    AnalysisDataService::Instance().remove(in_name);
    if (in_name != out_name)
      AnalysisDataService::Instance().remove(out_name);
  }
};

#endif // CHANGELOGTIMETEST_H_
