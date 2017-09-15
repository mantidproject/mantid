#ifndef MANTID_ALGORITHMS_AverageLogDATATEST_H_
#define MANTID_ALGORITHMS_AverageLogDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/AverageLogData.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::AverageLogData;

class AverageLogDataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AverageLogDataTest *createSuite() { return new AverageLogDataTest(); }
  static void destroySuite(AverageLogDataTest *suite) { delete suite; }

  void test_Init() {
    AverageLogData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_basic() {
    makeWS(0.);
    AverageLogData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("LogName", "p1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check average
    const double av = alg.getProperty("Average"),
                 err = alg.getProperty("Error");
    TS_ASSERT_DELTA(av, 0.1, 1e-8);
    TS_ASSERT_DELTA(err, 0.3, 1e-8);

    // Remove workspace from the data service.*/
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

  void test_shift() {
    makeWS(-200.);
    AverageLogData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("LogName", "p1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check average
    const double av = alg.getProperty("Average"),
                 err = alg.getProperty("Error");
    TS_ASSERT_DELTA(av, 0.1, 1e-8);
    TS_ASSERT_DELTA(err, 0.3, 1e-8);

    // Remove workspace from the data service.*/
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

  void test_noshiftneg() {
    makeWS(-200.);
    AverageLogData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("LogName", "p1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FixZero", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check average
    const double av = alg.getProperty("Average"),
                 err = alg.getProperty("Error");
    TS_ASSERT_DELTA(av, 1., 1e-8);
    TS_ASSERT_DELTA(err, 0., 1e-8);

    // Remove workspace from the data service.*/
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

  void test_noshiftpos() {
    makeWS(200.);
    AverageLogData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("LogName", "p1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FixZero", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check average
    const double av = alg.getProperty("Average"),
                 err = alg.getProperty("Error");
    TS_ASSERT(std::isnan(av));
    TS_ASSERT(std::isnan(err));

    // Remove workspace from the data service.*/
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
  }

private:
  std::string inputWS;
  void makeWS(double shift) {
    inputWS = "AverageLogDataTestWS";
    Mantid::DataObjects::Workspace2D_sptr w =
        WorkspaceCreationHelper::create2DWorkspace(1, 1);
    Mantid::Kernel::DateAndTime run_start("2010-01-01T00:00:00");
    Mantid::Kernel::TimeSeriesProperty<double> *pc, *p1;
    pc = new Mantid::Kernel::TimeSeriesProperty<double>("proton_charge");
    pc->setUnits("picoCoulomb");
    for (double i = 0; i < 100; i++)
      pc->addValue(run_start + i, 1.0);
    w->mutableRun().addProperty(pc);
    // value p1 0 to 89, 1 to 99 average=0.1, error =0.3
    p1 = new Mantid::Kernel::TimeSeriesProperty<double>("p1");
    p1->addValue(run_start + shift, 0.);
    p1->addValue(run_start + shift + 90., 1.);
    w->mutableRun().addProperty(p1);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, w);
  }
};

#endif /* MANTID_ALGORITHMS_AverageLogDATATEST_H_ */
