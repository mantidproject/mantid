#ifndef MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CreateGroupingWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void doTest(std::string outWSName) {
    // Retrieve the workspace from data service.
    GroupingWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // We expect the same number of histograms as detectors in the instrument
    // (excluding monitors)
    const bool skipMonitors(true);
    const size_t ndets = ws->getInstrument()->getNumberDetectors(skipMonitors);
    TS_ASSERT_EQUALS(ndets, ws->getNumberHistograms());

    TS_ASSERT_EQUALS(ws->blocksize(), 1);
    // All zero.
    TS_ASSERT_EQUALS(ws->dataY(0)[0], 0.0);
    TS_ASSERT_EQUALS(ws->dataY(100)[0], 0.0);
    TS_ASSERT_EQUALS(ws->dataY(10000)[0], 0.0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_withInstrumentName() {
    // Name of the output workspace.
    std::string outWSName("Grouping_2012-11-27");
    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentName", "POWGEN"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    doTest(outWSName);
    TS_ASSERT_EQUALS(
        static_cast<int>(alg.getProperty("NumberGroupedSpectraResult")), 0);
    TS_ASSERT_EQUALS(static_cast<int>(alg.getProperty("NumberGroupsResult")),
                     0);
  }

  void test_exec_withInstrumentFileName() {
    // Name of the output workspace.
    std::string outWSName("CreateGroupingWorkspaceTest_OutputWS");
    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InstrumentFilename", "POWGEN_Definition_2011-02-25.xml"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    doTest(outWSName);
    TS_ASSERT_EQUALS(
        static_cast<int>(alg.getProperty("NumberGroupedSpectraResult")), 0);
    TS_ASSERT_EQUALS(static_cast<int>(alg.getProperty("NumberGroupsResult")),
                     0);
  }

  void test_exec_WithBankNames() {
    // Name of the output workspace.
    std::string outWSName("CreateGroupingWorkspaceTest_OutputWS");

    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InstrumentFilename", "CNCS_Definition.xml"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("GroupNames", "bank1,bank2, bank3,bank4"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    GroupingWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(
        static_cast<int>(alg.getProperty("NumberGroupedSpectraResult")), 4096);
    TS_ASSERT_EQUALS(static_cast<int>(alg.getProperty("NumberGroupsResult")),
                     4);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 51200);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
    for (int i = 1; i <= 4; ++i) {
      TS_ASSERT_EQUALS(ws->dataY((i - 1) * 1024)[0], double(i) * 1.0);
      TS_ASSERT_EQUALS(ws->dataY((i - 1) * 1024 + 1023)[0], double(i) * 1.0);
    }
    // The rest is zero
    TS_ASSERT_EQUALS(ws->dataY(5 * 1024)[0], 0.0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove("CNCS_7860_event");
  }

  void test_exec_WithOldCalFile() {
    // Name of the output workspace.
    std::string outWSName("CreateGroupingWorkspaceTest_OutputWS");

    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InstrumentFilename", "POWGEN_Definition_2010.xml"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OldCalFilename", "pg3_mantid_det.cal"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    GroupingWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(
        static_cast<int>(alg.getProperty("NumberGroupedSpectraResult")), 18750);
    TS_ASSERT_EQUALS(static_cast<int>(alg.getProperty("NumberGroupsResult")),
                     4);

    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_WithFixedGroups() {
    // Name of the output workspace.
    std::string outWSName("CreateGroupingWorkspaceTest_OutputWS");

    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentName", "IRIS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FixedGroupCount", 10));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ComponentName", "graphite"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    GroupingWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(
        static_cast<int>(alg.getProperty("NumberGroupedSpectraResult")), 50);
    TS_ASSERT_EQUALS(static_cast<int>(alg.getProperty("NumberGroupsResult")),
                     10);

    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_WithFixedGroups_FailOnGroupsGreaterThanDet() {
    // Name of the output workspace.
    std::string outWSName("CreateGroupingWorkspaceTest_OutputWS_fail");

    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentName", "IRIS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FixedGroupCount", 52));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ComponentName", "graphite"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Should fail as IRIS graphite component has only 51 spectra
    TS_ASSERT(!alg.isExecuted());

    AnalysisDataService::Instance().remove(outWSName);
  }
};

/* Test the performance when creating groups with very large
 * instruments, i.e. TOPAZ
 */
class CreateGroupingWorkspaceTestPerformance : public CxxTest::TestSuite {
public:
  std::string outWSName;

  void setUp() override {
    outWSName = "CreateGroupingWorkspaceTestPerformance_OutputWS";
    // Load a small test file
    FrameworkManager::Instance().exec("LoadEmptyInstrument", 4, "Filename",
                                      "TOPAZ_Definition_2010.xml",
                                      "OutputWorkspace", "TOPAZ_2010");
  }

  void tearDown() override {
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove("TOPAZ_2010");
  }

  /* Test creating a grouping workspace with bank names */
  void test_TOPAZ_2010() {
    CreateGroupingWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "TOPAZ_2010"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "GroupNames", "bank1,bank2,bank3,bank4,bank5,bank6,bank7,bank8,bank9,"
                      "bank10,bank11,bank12,bank13,bank14,bank15"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    GroupingWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(
        static_cast<int>(alg.getProperty("NumberGroupedSpectraResult")),
        983040);
    TS_ASSERT_EQUALS(static_cast<int>(alg.getProperty("NumberGroupsResult")),
                     15);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 65536 * 15);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
    // Check one entry in each group
    for (int i = 0; i < 15; ++i) {
      TS_ASSERT_EQUALS(ws->dataY(i * 65536)[0],
                       double(i + 1) * 1.0); // Groups start at 1.0
    }
  }
};

#endif /* MANTID_ALGORITHMS_CREATEGROUPINGWORKSPACETEST_H_ */
