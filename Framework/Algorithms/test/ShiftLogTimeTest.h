#ifndef MANTID_ALGORITHMS_SHIFTLOGTIMETEST_H_
#define MANTID_ALGORITHMS_SHIFTLOGTIMETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/ShiftLogTime.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

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

  void testCopyHist() {
    this->verify("ShiftLogTime_in", "ShiftLogTime_out", 5);
  }

  void testInplace() { this->verify("ShiftLogTime", "ShiftLogTime", 5); }

  void testCopyHistNeg() {
    this->verify("ShiftLogTime_in", "ShiftLogTime_out", -5);
  }

  void testInplaceNeg() { this->verify("ShiftLogTime", "ShiftLogTime", -5); }

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
  void verify(const std::string in_name, const std::string out_name,
              const int shift) {
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
    alg.setProperty("IndexShift", shift);

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
    TS_ASSERT_EQUALS(length - std::abs(shift), newlog->size());
    if (shift > 0) {
      TS_ASSERT_EQUALS(start + static_cast<double>(shift), newlog->firstTime());
      TS_ASSERT_EQUALS(0., newlog->firstValue());
      TS_ASSERT_EQUALS(start + static_cast<double>(length - 1),
                       newlog->lastTime());
      TS_ASSERT_EQUALS(static_cast<double>(shift - 1), newlog->lastValue());
    }
    if (shift < 0) {
      TS_ASSERT_EQUALS(start, newlog->firstTime());
      TS_ASSERT_EQUALS(static_cast<double>(-shift), newlog->firstValue());
      TS_ASSERT_EQUALS(start + static_cast<double>(-shift - 1),
                       newlog->lastTime());
      TS_ASSERT_EQUALS(9., newlog->lastValue());
    }

    // cleanup
    AnalysisDataService::Instance().remove(in_name);
    if (in_name != out_name)
      AnalysisDataService::Instance().remove(out_name);
  }
};

#endif /* MANTID_ALGORITHMS_SHIFTLOGTIMETEST_H_ */
