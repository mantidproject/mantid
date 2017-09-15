#ifndef MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidAlgorithms/ConvertToEventWorkspace.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class ConvertToEventWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    ConvertToEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() { do_test_exec(false); }

  void test_exec_GenerateMultipleEvents() { do_test_exec(true); }

  void test_exec_PointData_fails() { do_test_exec(true, true); }

  void do_test_exec(bool GenerateMultipleEvents,
                    bool ConvertToPointData = false) {
    std::string inWSName("ConvertToEventWorkspaceTest_InputWS");
    std::string outWSName("ConvertToEventWorkspaceTest_OutputWS");
    // Create the input
    Workspace2D_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(50, 10,
                                                                     true);
    AnalysisDataService::Instance().addOrReplace(inWSName, inWS);

    inWS->dataY(0)[0] = 1.0;
    inWS->dataE(0)[0] = 1.0;
    inWS->dataY(0)[1] = 3.0;
    inWS->dataE(0)[1] = sqrt(3.0);
    inWS->dataY(0)[2] = 0.0;
    inWS->dataE(0)[2] = 0.0;
    inWS->dataY(0)[3] = 2.0;
    inWS->dataE(0)[3] = M_SQRT2;
    inWS->dataY(0)[4] = 10000.0;
    inWS->dataE(0)[4] = 100.0;

    if (ConvertToPointData) {
      FrameworkManager::Instance().exec("ConvertToPointData", 4,
                                        "InputWorkspace", inWSName.c_str(),
                                        "OutputWorkspace", inWSName.c_str());
    }

    ConvertToEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("GenerateMultipleEvents", GenerateMultipleEvents));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    if (ConvertToPointData) {
      // Should NOT work
      TS_ASSERT(!alg.isExecuted());
      return;
    } else {
      // Should work
      TS_ASSERT(alg.isExecuted());
    }

    // Retrieve the workspace from data service.
    EventWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            outWSName));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // This performs a full comparison (histogram
    CompareWorkspaces matcher;
    matcher.initialize();
    matcher.setProperty("Workspace1",
                        boost::dynamic_pointer_cast<MatrixWorkspace>(inWS));
    matcher.setProperty("Workspace2",
                        boost::dynamic_pointer_cast<MatrixWorkspace>(outWS));
    matcher.setProperty("CheckType", false);
    matcher.setProperty("Tolerance", 1e-6);
    matcher.execute();
    TS_ASSERT(matcher.isExecuted());
    TS_ASSERT(matcher.getProperty("Result"));

    // Event-specific checks
    TS_ASSERT_EQUALS(outWS->getNumberEvents(),
                     GenerateMultipleEvents ? 1006 : 499);
    TS_ASSERT_EQUALS(outWS->getSpectrum(1).getNumberEvents(),
                     GenerateMultipleEvents ? 20 : 10);

    // Check a couple of events
    EventList &el = outWS->getSpectrum(0);
    TS_ASSERT_EQUALS(el.getWeightedEventsNoTime().size(),
                     GenerateMultipleEvents ? 26 : 9);
    WeightedEventNoTime ev;
    ev = el.getWeightedEventsNoTime()[0];
    TS_ASSERT_DELTA(ev.tof(), 0.5, 1e-6);
    TS_ASSERT_DELTA(ev.weight(), 1.0, 1e-6);
    TS_ASSERT_DELTA(ev.errorSquared(), 1.0, 1e-6);

    if (GenerateMultipleEvents) {
      ev = el.getWeightedEventsNoTime()[1];
      TS_ASSERT_DELTA(ev.tof(), 1.1666, 1e-4);
      TS_ASSERT_DELTA(ev.weight(), 1.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 1.0, 1e-6);
      ev = el.getWeightedEventsNoTime()[2];
      TS_ASSERT_DELTA(ev.tof(), 1.5000, 1e-4);
      TS_ASSERT_DELTA(ev.weight(), 1.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 1.0, 1e-6);
      ev = el.getWeightedEventsNoTime()[3];
      TS_ASSERT_DELTA(ev.tof(), 1.8333, 1e-4);
      TS_ASSERT_DELTA(ev.weight(), 1.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 1.0, 1e-6);
      // Skipped a bin
      ev = el.getWeightedEventsNoTime()[4];
      TS_ASSERT_DELTA(ev.tof(), 3.25, 1e-4);
      TS_ASSERT_DELTA(ev.weight(), 1.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 1.0, 1e-6);
      ev = el.getWeightedEventsNoTime()[5];
      TS_ASSERT_DELTA(ev.tof(), 3.75, 1e-4);
      TS_ASSERT_DELTA(ev.weight(), 1.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 1.0, 1e-6);
      // The one with 10000 events, reduced to 10 events with weight 1000 each
      ev = el.getWeightedEventsNoTime()[6];
      TS_ASSERT_DELTA(ev.tof(), 4.05, 1e-4);
      TS_ASSERT_DELTA(ev.weight(), 1000.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 1000.0, 1e-6);
    } else {
      ev = el.getWeightedEventsNoTime()[1];
      TS_ASSERT_DELTA(ev.tof(), 1.5, 1e-6);
      TS_ASSERT_DELTA(ev.weight(), 3.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 3.0, 1e-6);
      // Skipped an event because the bin was 0.0 weight
      ev = el.getWeightedEventsNoTime()[2];
      TS_ASSERT_DELTA(ev.tof(), 3.5, 1e-6);
      TS_ASSERT_DELTA(ev.weight(), 2.0, 1e-6);
      TS_ASSERT_DELTA(ev.errorSquared(), 2.0, 1e-6);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(inWSName);
  }

  /// Workspace with infinity or NAN = don't create events there.
  void test_with_nan_and_inf() {
    // Create the input
    Workspace2D_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);

    double nan = std::numeric_limits<double>::quiet_NaN();
    double inf = std::numeric_limits<double>::infinity();
    double ninf = -inf;

    inWS->dataY(0)[0] = 1.0;
    inWS->dataE(0)[0] = 1.0;

    // But nan or inf in either Y or error
    inWS->dataY(0)[1] = nan;
    inWS->dataE(0)[2] = nan;
    inWS->dataY(0)[3] = inf;
    inWS->dataE(0)[4] = inf;
    inWS->dataY(0)[5] = ninf;
    inWS->dataE(0)[6] = ninf;

    for (size_t i = 7; i < 10; i++) {
      inWS->dataY(0)[i] = 0;
      inWS->dataE(0)[i] = 0;
    }

    // Name of the output workspace.
    std::string outWSName("ConvertToEventWorkspaceTest_OutputWS");

    ConvertToEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateMultipleEvents", false));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    EventWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            outWSName));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Only 1 bin had a valid weight/error, so it should have 1 event
    TS_ASSERT_EQUALS(outWS->getNumberEvents(), 1);
  }

  /// Create events for zero-weight bins
  void test_GenerateZeros() {
    // Create the input
    Workspace2D_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);

    // Clear the vector
    inWS->dataY(0).assign(10, 0.0);

    // Name of the output workspace.
    std::string outWSName("ConvertToEventWorkspaceTest_OutputWS");

    ConvertToEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateMultipleEvents", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateZeros", true));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    EventWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            outWSName));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Every bin is created, even though they were zeros
    TS_ASSERT_EQUALS(outWS->getNumberEvents(), 10);
  }
};

#endif /* MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACETEST_H_ */
