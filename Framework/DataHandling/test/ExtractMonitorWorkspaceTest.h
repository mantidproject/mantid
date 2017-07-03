#ifndef MANTID_DATAHANDLING_EXTRACTMONITORWORKSPACETEST_H_
#define MANTID_DATAHANDLING_EXTRACTMONITORWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ExtractMonitorWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::DataHandling::ExtractMonitorWorkspace;
using namespace Mantid::API;

class ExtractMonitorWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractMonitorWorkspaceTest *createSuite() {
    return new ExtractMonitorWorkspaceTest();
  }
  static void destroySuite(ExtractMonitorWorkspaceTest *suite) { delete suite; }

  ExtractMonitorWorkspaceTest()
      : outWSName("ExtractMonitorWorkspaceTest_OutputWS") {}

  void test_init() {
    ExtractMonitorWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT(alg.getProperty("ClearFromInputWorkspace"))
  }

  void test_fails_if_no_monitor_workspace() {
    auto inws = WorkspaceCreationHelper::create1DWorkspaceRand(1, true);

    ExtractMonitorWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("MonitorWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(!alg.isExecuted());
  }

  void doTest(MatrixWorkspace_sptr inws, MatrixWorkspace_sptr monws) {
    inws->setMonitorWorkspace(monws);
    TS_ASSERT_EQUALS(inws->monitorWorkspace(), monws);

    ExtractMonitorWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("MonitorWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ClearFromInputWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws, monws);
    TS_ASSERT_EQUALS(inws->monitorWorkspace(), monws);

    // Now run it clearing off the monitor from the input workspace
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ClearFromInputWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inws));
    TS_ASSERT(alg.execute());
    TSM_ASSERT(
        "The monitor workspace should have been wiped off the input workspace",
        !inws->monitorWorkspace());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_2D_2D() {
    auto inws = WorkspaceCreationHelper::create1DWorkspaceRand(1, true);
    auto monws = WorkspaceCreationHelper::create1DWorkspaceFib(1, true);
    doTest(inws, monws);
  }

  // These tests demonstrate that the workspaces don't have to be of the same
  // type

  void test_2D_event() {
    auto inws = WorkspaceCreationHelper::create1DWorkspaceRand(1, true);
    auto monws = WorkspaceCreationHelper::createEventWorkspace2(1, 1);
    doTest(inws, monws);
  }

  void test_event_2D() {
    auto inws = WorkspaceCreationHelper::createEventWorkspace2(1, 1);
    auto monws = WorkspaceCreationHelper::create1DWorkspaceRand(1, true);
    doTest(inws, monws);
  }

  void test_event_event() {
    auto inws = WorkspaceCreationHelper::createEventWorkspace2(1, 1);
    auto monws = WorkspaceCreationHelper::createEventWorkspace2(1, 1);
    doTest(inws, monws);
  }

private:
  std::string outWSName;
};

#endif /* MANTID_DATAHANDLING_EXTRACTMONITORWORKSPACETEST_H_ */
