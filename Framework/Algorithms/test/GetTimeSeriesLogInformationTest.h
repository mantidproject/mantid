#ifndef MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATIONTEST_H_
#define MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATIONTEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/GetTimeSeriesLogInformation.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

namespace {
/*
 * Create an EventWorkspace including
 * (1) proton charge log from
 * (2) test log in sin function with time
 */
DataObjects::EventWorkspace_sptr createEventWorkspace() {
  using namespace WorkspaceCreationHelper;

  // 1. Empty workspace
  DataObjects::EventWorkspace_sptr eventws =
      WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 2,
                                                                      true);
  // eventws->setName("TestWorkspace");

  // 2. Run star time
  int64_t runstarttime_ns = 3000000000;
  int64_t runstoptime_ns = 3001000000;
  int64_t pulsetime_ns = 100000;

  Types::Core::DateAndTime runstarttime(runstarttime_ns);
  Types::Core::DateAndTime runendtime(runstoptime_ns);
  eventws->mutableRun().addProperty("run_start",
                                    runstarttime.toISO8601String());
  eventws->mutableRun().addProperty("run_end", runendtime.toISO8601String());

  // 3. Proton charge log
  Kernel::TimeSeriesProperty<double> *protonchargelog =
      new Kernel::TimeSeriesProperty<double>("proton_charge");
  int64_t curtime_ns = runstarttime_ns;
  while (curtime_ns <= runstoptime_ns) {
    Types::Core::DateAndTime curtime(curtime_ns);
    protonchargelog->addValue(curtime, 1.0);
    curtime_ns += pulsetime_ns;
  }
  eventws->mutableRun().addProperty(protonchargelog, true);

  // 4. Sine value log (value record 1/4 of pulse time.  it is FAST)
  Kernel::TimeSeriesProperty<double> *sinlog =
      new Kernel::TimeSeriesProperty<double>("FastSineLog");
  double period = static_cast<double>(pulsetime_ns);
  curtime_ns = runstarttime_ns;
  while (curtime_ns < runstoptime_ns) {
    Types::Core::DateAndTime curtime(curtime_ns);
    double value = sin(M_PI * static_cast<double>(curtime_ns) / period * 0.25);
    sinlog->addValue(curtime, value);
    curtime_ns += pulsetime_ns / 4;
  }
  eventws->mutableRun().addProperty(sinlog, true);

  // 5. Cosine value log (value record 4 pulse time.  it is SLOW)
  Kernel::TimeSeriesProperty<double> *coslog =
      new Kernel::TimeSeriesProperty<double>("SlowCosineLog");
  period = static_cast<double>(pulsetime_ns * 10);
  curtime_ns = runstarttime_ns;
  while (curtime_ns < runstoptime_ns) {
    Types::Core::DateAndTime curtime(curtime_ns);
    double value = sin(2 * M_PI * static_cast<double>(curtime_ns) / period);
    coslog->addValue(curtime, value);
    curtime_ns += pulsetime_ns * 2;
  }
  eventws->mutableRun().addProperty(coslog, true);

  return eventws;
}
} // namespace
class GetTimeSeriesLogInformationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetTimeSeriesLogInformationTest *createSuite() {
    return new GetTimeSeriesLogInformationTest();
  }
  static void destroySuite(GetTimeSeriesLogInformationTest *suite) {
    delete suite;
  }

  void test_Init() {
    GetTimeSeriesLogInformation getalg;
    TS_ASSERT_THROWS_NOTHING(getalg.initialize());
    TS_ASSERT(getalg.isInitialized());
  }

  void test_OverAllStatic() {
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("EventWorkspace", eventWS);

    GetTimeSeriesLogInformation getalg;
    getalg.initialize();

    getalg.setProperty("InputWorkspace", eventWS);
    getalg.setProperty("OutputWorkspace", "TimeStat");
    getalg.setProperty("LogName", "FastSineLog");
    getalg.setProperty("InformationWorkspace", "LogInfoTable");

    getalg.execute();
    TS_ASSERT(getalg.isExecuted());

    // Cleanup
    AnalysisDataService::Instance().remove("TimeStat");
  }
};

class GetTimeSeriesLogInformationTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetTimeSeriesLogInformationTestPerformance *createSuite() {
    return new GetTimeSeriesLogInformationTestPerformance();
  }
  static void destroySuite(GetTimeSeriesLogInformationTestPerformance *suite) {
    delete suite;
  }

  void setUp() override { inputWS = createEventWorkspace(); }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("TimeStat");
  }

  void testPerformance() {
    GetTimeSeriesLogInformation getalg;
    getalg.initialize();

    getalg.setProperty("InputWorkspace", inputWS);
    getalg.setProperty("OutputWorkspace", "TimeStat");
    getalg.setProperty("LogName", "FastSineLog");
    getalg.setProperty("InformationWorkspace", "LogInfoTable");

    getalg.execute();
    TS_ASSERT(getalg.isExecuted());
  }

private:
  Mantid::DataObjects::EventWorkspace_sptr inputWS;
};
#endif /* MANTID_ALGORITHMS_GETTIMESERIESLOGINFORMATIONTEST_H_ */
