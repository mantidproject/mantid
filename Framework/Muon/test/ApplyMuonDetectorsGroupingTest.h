#ifndef MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_
#define MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidMuon/ApplyMuonDetectorsGrouping.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using Mantid::DataHandling::LoadMuonNexus2;

/// Holds data loaded from file
struct LoadedData {
  Workspace_sptr workspace;
  double timeZero;
  Workspace_sptr grouping;
};

class ApplyMuonDetectorsGroupingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyMuonDetectorsGroupingTest *createSuite() {
    return new ApplyMuonDetectorsGroupingTest();
  };
  static void destroySuite(ApplyMuonDetectorsGroupingTest *suite) {
    delete suite;
  };

  void test_Init() {
    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  };

  void test_output_produced() {
    std::string emptyString("");

    auto data = loadEMU();
    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    auto ws = data->workspace;

    std::string wsName = (data->workspace)->getName();
    std::string wsGroupName = (wsGroup)->getName();

    AnalysisDataService::Instance().addOrReplace("inputData", data->workspace);
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroup->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1,2,3,4,5"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("plotType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeLoadZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SummedPeriods", std::to_string(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }
  }

  void test_single_group_doesnt_change_data() {

    std::string emptyString("");

    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::createGroupedWorkspace2D(1, 10, 0.5);
    AnalysisDataService::Instance().addOrReplace("inputData", ws);

    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    std::string wsName = ws->getName();
    std::string wsGroupName = (wsGroup)->getName();

    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroup->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("plotType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeLoadZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SummedPeriods", std::to_string(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Counts; #1"));
    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));

    auto wsOut = wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw");

    CompareWorkspaces algCompare;
    TS_ASSERT_THROWS_NOTHING(algCompare.initialize());
    TS_ASSERT(algCompare.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        algCompare.setProperty("Workspace1", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        algCompare.setProperty("Workspace2", wsOut->getName()));
    TS_ASSERT_THROWS_NOTHING(algCompare.setProperty("Tolerance", 0.001));
    TS_ASSERT_THROWS_NOTHING(algCompare.execute());
    TS_ASSERT(algCompare.isExecuted());

    TS_ASSERT(algCompare.getProperty("Result"));

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }
  }

  void test_grouping_with_counts() {

    std::string emptyString("");
    // Populate y-values with 2.0
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::createGroupedWorkspace2D(1, 10, 0.5);
    AnalysisDataService::Instance().addOrReplace("inputData", ws);

    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    std::string wsName = ws->getName();
    std::string wsGroupName = (wsGroup)->getName();

    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroup->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1,2,3,4,5"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("plotType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeLoadZero", 0.0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SummedPeriods", std::to_string(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Counts; #1"));
    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 2.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 4.500, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 10.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 10.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 10.000, 0.001);
    // Quadrature errors : Sqrt(10)
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 3.162, 0.001);
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 3.162, 0.001);
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 3.162, 0.001);

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }
  }

  /**
  * Use LoadMuonNexus to load data from file and return it
  * @param filename :: [input] Name of file to load
  * @returns LoadedData struct
  */
  std::unique_ptr<LoadedData> loadData(const std::string &filename) {
    auto data = Mantid::Kernel::make_unique<LoadedData>();
    LoadMuonNexus2 load;
    load.initialize();
    load.setChild(true);
    load.setPropertyValue("Filename", filename);
    load.setPropertyValue("OutputWorkspace", "__notused");
    load.setPropertyValue("DetectorGroupingTable", "__notused");
    load.execute();
    data->workspace = load.getProperty("OutputWorkspace");
    data->timeZero = load.getProperty("TimeZero");
    data->grouping = load.getProperty("DetectorGroupingTable");
    return data;
  }

  /**
  * Use LoadMuonNexus to load data from EMU file
  * @returns LoadedData struct
  */
  std::unique_ptr<LoadedData> loadEMU() { return loadData("emu00006473.nxs"); }

  /**
  * Use LoadMuonNexus to load data from MUSR file
  * @returns LoadedData struct
  */
  std::unique_ptr<LoadedData> loadMUSR() {
    return loadData("MUSR00015189.nxs");
  }

private:
  TableWorkspace_sptr createGroupingTable(const std::vector<int> &group1,
                                          const std::vector<int> &group2) {
    auto t = boost::make_shared<TableWorkspace>();

    t->addColumn("vector_int", "Detectors");

    TableRow row1 = t->appendRow();
    row1 << group1;

    TableRow row2 = t->appendRow();
    row2 << group2;

    return t;
  }
};

#endif /* MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_ */