#ifndef MANTID_ALGORITHMS_ADDLOGDERIVATIVETEST_H_
#define MANTID_ALGORITHMS_ADDLOGDERIVATIVETEST_H_

#include "MantidAPI/Run.h"
#include "MantidAlgorithms/AddLogDerivative.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Kernel::TimeSeriesProperty;

class AddLogDerivativeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddLogDerivativeTest *createSuite() {
    return new AddLogDerivativeTest();
  }
  static void destroySuite(AddLogDerivativeTest *suite) { delete suite; }

  void test_Init() {
    AddLogDerivative alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /** Perform test, return result */
  TimeSeriesProperty<double> *do_test(int Derivative, bool willFail = false,
                                      bool addRepeatedTimes = false) {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("Dummy", ws);

    TimeSeriesProperty<double> *p =
        new TimeSeriesProperty<double>("doubleProp");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 1.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:10", 2.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:20", 0.00));
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 5.00));
    if (addRepeatedTimes) {
      TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:30", 10.00));
      TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:40", 15.00));
      TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:50", 20.00));
    }
    ws->mutableRun().addProperty(p, true);

    std::string NewLogName = "doubleProp_deriv";

    AddLogDerivative alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "Dummy"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("LogName", "doubleProp"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NewLogName", NewLogName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Derivative", Derivative));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    if (willFail) {
      TS_ASSERT(!alg.isExecuted());
      return nullptr;
    } else
      TS_ASSERT(alg.isExecuted());

    Run &run = ws->mutableRun();
    TS_ASSERT(run.hasProperty(NewLogName));
    if (!run.hasProperty(NewLogName))
      return nullptr;
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(NewLogName));
    TS_ASSERT(p);
    return p;
  }

  void test_exec_1stDerivative() {
    TimeSeriesProperty<double> *p = do_test(1);
    if (!p)
      return;

    TS_ASSERT_EQUALS(p->size(), 3);
    TS_ASSERT_EQUALS(p->nthTime(0).toISO8601String(), "2007-11-30T16:17:05");
    TS_ASSERT_DELTA(p->nthValue(0), 0.1, 1e-5);
    TS_ASSERT_EQUALS(p->nthTime(1).toISO8601String(), "2007-11-30T16:17:15");
    TS_ASSERT_DELTA(p->nthValue(1), -0.2, 1e-5);
    TS_ASSERT_EQUALS(p->nthTime(2).toISO8601String(), "2007-11-30T16:17:25");
    TS_ASSERT_DELTA(p->nthValue(2), 0.5, 1e-5);
  }

  void test_exec_2ndDerivative() {
    TimeSeriesProperty<double> *p = do_test(2);
    if (!p)
      return;

    TS_ASSERT_EQUALS(p->size(), 2);
    TS_ASSERT_EQUALS(p->nthTime(0).toISO8601String(), "2007-11-30T16:17:10");
    TS_ASSERT_DELTA(p->nthValue(0), -0.03, 1e-5);
    TS_ASSERT_EQUALS(p->nthTime(1).toISO8601String(), "2007-11-30T16:17:20");
    TS_ASSERT_DELTA(p->nthValue(1), 0.07, 1e-5);
  }

  void test_exec_3rdDerivative() {
    TimeSeriesProperty<double> *p = do_test(3);
    if (!p)
      return;

    TS_ASSERT_EQUALS(p->size(), 1);
    TS_ASSERT_EQUALS(p->nthTime(0).toISO8601String(), "2007-11-30T16:17:15");
    TS_ASSERT_DELTA(p->nthValue(0), 0.01, 1e-5);
  }

  /** Ticket #4313: Handled repeated time values in logs */
  void test_exec_1stDerivative_repeatedValues() {
    TimeSeriesProperty<double> *p = do_test(1, false, true);
    if (!p)
      return;
    TS_ASSERT_EQUALS(p->size(), 5);
    TS_ASSERT_EQUALS(p->nthTime(3).toISO8601String(), "2007-11-30T16:17:35");
    TS_ASSERT_DELTA(p->nthValue(3), 1.0, 1e-5);
    TS_ASSERT_EQUALS(p->nthTime(4).toISO8601String(), "2007-11-30T16:17:45");
    TS_ASSERT_DELTA(p->nthValue(4), 0.5, 1e-5);
  }

  void test_exec_failures() {
    do_test(4, true);
    do_test(5, true);
  }
};

#endif /* MANTID_ALGORITHMS_ADDLOGDERIVATIVETEST_H_ */
