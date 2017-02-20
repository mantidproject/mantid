#ifndef MANTID_ALGORITHMS_SHIFTLOGTIMETEST_H_
#define MANTID_ALGORITHMS_SHIFTLOGTIMETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ShiftLogTime.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class ShiftLogTimeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ShiftLogTimeTest *createSuite() { return new ShiftLogTimeTest(); }
  static void destroySuite(ShiftLogTimeTest *suite) { delete suite; }

  /// Set up the parameters for what the tests do.
  ShiftLogTimeTest() {
    length = 10;
    logname = "fakelog";
    start_str = "2011-07-14T12:00Z"; // Noon on Bastille day 2011.
  }

  void testCopyHist() { this->verify("ShiftLogTime_in", "ShiftLogTime_out"); }

  void testInplace() { this->verify("ShiftLogTime", "ShiftLogTime"); }

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
    ShiftLogTime alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", in_name);
    alg.setPropertyValue("OutputWorkspace", out_name);
    alg.setPropertyValue("LogName", logname);
    alg.setPropertyValue("IndexShift", "5");

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
    TS_ASSERT_EQUALS(length - 5, newlog->size());
    TS_ASSERT_EQUALS(start + 5., newlog->firstTime());
    TS_ASSERT_EQUALS(0., newlog->firstValue());

    // cleanup
    AnalysisDataService::Instance().remove(in_name);
    if (in_name != out_name)
      AnalysisDataService::Instance().remove(out_name);
  }
};

#endif /* MANTID_ALGORITHMS_SHIFTLOGTIMETEST_H_ */
