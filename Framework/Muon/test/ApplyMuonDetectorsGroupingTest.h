#ifndef MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_
#define MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidMuon/ApplyMuonDetectorsGrouping.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;

namespace {

// Create fake muon data with exponential decay
struct yDataAsymmetry {
  double operator()(const double time, size_t specNum) {
    double amplitude = (specNum + 1) * 10.;
    double tau = Mantid::PhysicalConstants::MuonLifetime *
                 1e6; // Muon life time in microseconds
    return (20. * (1.0 + amplitude * exp(-time / tau)));
  }
};

struct yDataCounts {
  yDataCounts() : m_count(-1){};
  int m_count;
  double operator()(const double x, size_t) {
    m_count++;
    return static_cast<double>(m_count);
  }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};

/**
 * Create a matrix workspace appropriate for Group Asymmetry. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal.
 * @param nspec :: The number of spectra
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @return Pointer to the workspace.
 */
MatrixWorkspace_sptr createAsymmetryWorkspace(size_t nspec, size_t maxt) {

  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yDataAsymmetry(), static_cast<int>(nspec), 0.0, 1.0,
          (1.0 / static_cast<double>(maxt)), true, eData());

  ws->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(
      static_cast<int>(nspec)));

  for (int g = 0; g < static_cast<int>(nspec); g++) {
    auto &spec = ws->getSpectrum(g);
    spec.addDetectorID(g + 1);
    spec.setSpectrumNo(g + 1);
  }

  // Add number of good frames (required for Asymmetry calculation)
  ws->mutableRun().addProperty("goodfrm", 10);

  return ws;
}

/**
 * Create a matrix workspace appropriate for Group Counts. One detector per
 * spectra, numbers starting from 1. The detector ID and spectrum number are
 * equal. Y values increase from 0 in integer steps.
 * @param nspec :: The number of spectra
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param seed :: Number added to all y-values.
 * @return Pointer to the workspace.
 */
MatrixWorkspace_sptr createCountsWorkspace(size_t nspec, size_t maxt,
                                           double seed) {

  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(
          yDataCounts(), static_cast<int>(nspec), 0.0, 1.0,
          (1.0 / static_cast<double>(maxt)), true, eData());

  ws->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(
      static_cast<int>(nspec)));

  for (int g = 0; g < static_cast<int>(nspec); g++) {
    auto &spec = ws->getSpectrum(g);
    spec.addDetectorID(g + 1);
    spec.setSpectrumNo(g + 1);
    ws->mutableY(g) += seed;
  }

  return ws;
}

/**
 * Create a WorkspaceGroup and add to the ADS, populate with MatrixWorkspaces
 * simulating periods as used in muon analysis. Workspace for period i has a
 * name ending _i.
 * @param nPeriods :: The number of periods (independent workspaces)
 * @param maxt ::  The number of histogram bin edges (between 0.0 and 1.0).
 * Number of bins = maxt - 1 .
 * @param wsGroupName :: Name of the workspace group containing the period
 * workspaces.
 * @return Pointer to the workspace group.
 */
WorkspaceGroup_sptr
createMultiPeriodWorkspaceGroup(const int &nPeriods, size_t nspec, size_t maxt,
                                const std::string &wsGroupName) {

  WorkspaceGroup_sptr wsGroup = boost::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().addOrReplace(wsGroupName, wsGroup);

  std::string wsNameStem = "MuonDataPeriod_";
  std::string wsName;

  for (int period = 1; period < nPeriods + 1; period++) {
    // Period 1 yvalues : 1,2,3,4,5,6,7,8,9,10
    // Period 2 yvalues : 2,3,4,5,6,7,8,9,10,11 etc..
    MatrixWorkspace_sptr ws = createCountsWorkspace(nspec, maxt, period);
    wsGroup->addWorkspace(ws);
    wsName = wsNameStem + std::to_string(period);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
  }

  return wsGroup;
}


ITableWorkspace_sptr createDeadTimeTable(const int& nspec, std::vector<double>& deadTimes) {

	auto deadTimeTable = boost::dynamic_pointer_cast<ITableWorkspace>(
		WorkspaceFactory::Instance().createTable("TableWorkspace"));

	deadTimeTable->addColumn("int", "Spectrum Number");
	deadTimeTable->addColumn("double", "Dead Time");

	if (deadTimes.size() != nspec) {
		//g_log.notice("deadTimes length is not equal to nspec");
		return deadTimeTable;
	}

	for (int spec = 0; spec < deadTimes.size() ; spec++) {
		TableRow newRow = deadTimeTable->appendRow();
		newRow << spec+1;
		newRow << deadTimes[spec];
	}

	return deadTimeTable;

}

} // namespace

class ApplyMuonDetectorsGroupingTest : public CxxTest::TestSuite {
public:
  // WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ApplyMuonDetectorsGroupingTest() {
    Mantid::API::FrameworkManager::Instance();
  };
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

  void test_createCountsWorkspace() {

    MatrixWorkspace_sptr ws = createCountsWorkspace(2, 10, 0.0);

    // Correct number of spectra?
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);

    const std::set<detid_t> detids0 = ws->getSpectrum(0).getDetectorIDs();
    const std::set<detid_t> detids1 = ws->getSpectrum(1).getDetectorIDs();

    // Correct detector numbering?
    TS_ASSERT_EQUALS(*detids0.begin(), 1);
    TS_ASSERT_EQUALS(*detids1.begin(), 2);
    // Correct number of detectors?
    TS_ASSERT_EQUALS(detids0.size(), 1);
    TS_ASSERT_EQUALS(detids1.size(), 1);

    Mantid::MantidVec vecX1 = ws->getSpectrum(0).readX();
    Mantid::MantidVec vecX2 = ws->getSpectrum(1).readX();
    TS_ASSERT_DELTA(vecX1.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX1.at(9), 0.900, 0.001);
    TS_ASSERT_DELTA(vecX2.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX2.at(9), 0.900, 0.001);

    Mantid::MantidVec vecY1 = ws->getSpectrum(0).readY();
    Mantid::MantidVec vecY2 = ws->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY1.at(0), 0, 0.1);
    TS_ASSERT_DELTA(vecY1.at(9), 9, 0.1);
    TS_ASSERT_DELTA(vecY2.at(0), 10, 0.1);
    TS_ASSERT_DELTA(vecY2.at(9), 19, 0.1);

    Mantid::MantidVec vecE1 = ws->getSpectrum(0).readE();
    Mantid::MantidVec vecE2 = ws->getSpectrum(1).readE();
    TS_ASSERT_DELTA(vecE1.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE1.at(9), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(9), 0.005, 0.0001);
  }

  void test_createAsymmetryWorkspace() {

    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(5, 10);
    AnalysisDataService::Instance().addOrReplace("inputData", ws);

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 5);

    const std::set<detid_t> detids0 = ws->getSpectrum(0).getDetectorIDs();
    const std::set<detid_t> detids1 = ws->getSpectrum(1).getDetectorIDs();

    TS_ASSERT_EQUALS(*detids0.begin(), 1);
    TS_ASSERT_EQUALS(*detids1.begin(), 2);

    TS_ASSERT_EQUALS(detids0.size(), 1);
    TS_ASSERT_EQUALS(detids1.size(), 1);

    Mantid::MantidVec vecX1 = ws->getSpectrum(0).readX();
    Mantid::MantidVec vecX2 = ws->getSpectrum(1).readX();
    TS_ASSERT_DELTA(vecX1.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX1.at(9), 0.900, 0.001);
    TS_ASSERT_DELTA(vecX2.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX2.at(9), 0.900, 0.001);

    Mantid::MantidVec vecY1 = ws->getSpectrum(0).readY();
    Mantid::MantidVec vecY2 = ws->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY1.at(0), 220.0, 0.1);
    TS_ASSERT_DELTA(vecY1.at(9), 152.8, 0.1);
    TS_ASSERT_DELTA(vecY2.at(0), 420.0, 0.1);
    TS_ASSERT_DELTA(vecY2.at(9), 285.6, 0.1);

    Mantid::MantidVec vecE1 = ws->getSpectrum(0).readE();
    Mantid::MantidVec vecE2 = ws->getSpectrum(1).readE();
    TS_ASSERT_DELTA(vecE1.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE1.at(9), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(9), 0.005, 0.0001);
  }

  void test_createMultiPeriodWorkspace() {

    auto wsGroup = createMultiPeriodWorkspaceGroup(5, 2, 10, "muonGroup");

    std::vector<std::string> names = wsGroup->getNames();

    TS_ASSERT_EQUALS(names.size(), 5);
    TS_ASSERT_EQUALS(names.at(0), "MuonDataPeriod_1");

    MatrixWorkspace_sptr ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("MuonDataPeriod_1"));
    const std::set<detid_t> detids0 = ws1->getSpectrum(0).getDetectorIDs();
    const std::set<detid_t> detids1 = ws1->getSpectrum(1).getDetectorIDs();

    TS_ASSERT_EQUALS(*detids0.begin(), 1);
    TS_ASSERT_EQUALS(*detids1.begin(), 2);

    TS_ASSERT_EQUALS(detids0.size(), 1);
    TS_ASSERT_EQUALS(detids1.size(), 1);

    Mantid::MantidVec vecX1 = ws1->getSpectrum(0).readX();
    Mantid::MantidVec vecX2 = ws1->getSpectrum(1).readX();
    TS_ASSERT_DELTA(vecX1.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX1.at(9), 0.900, 0.001);
    TS_ASSERT_DELTA(vecX2.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX2.at(9), 0.900, 0.001);

    Mantid::MantidVec vecY1 = ws1->getSpectrum(0).readY();
    Mantid::MantidVec vecY2 = ws1->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY1.at(0), 1, 0.1);
    TS_ASSERT_DELTA(vecY1.at(9), 10, 0.1);
    TS_ASSERT_DELTA(vecY2.at(0), 11, 0.1);
    TS_ASSERT_DELTA(vecY2.at(9), 20, 0.1);

    MatrixWorkspace_sptr ws5 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("MuonDataPeriod_5"));
    Mantid::MantidVec vecY3 = ws5->getSpectrum(0).readY();
    Mantid::MantidVec vecY4 = ws5->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY3.at(0), 5, 0.1);
    TS_ASSERT_DELTA(vecY3.at(9), 14, 0.1);
    TS_ASSERT_DELTA(vecY4.at(0), 15, 0.1);
    TS_ASSERT_DELTA(vecY4.at(9), 24, 0.1);

    Mantid::MantidVec vecE1 = ws1->getSpectrum(0).readE();
    Mantid::MantidVec vecE2 = ws1->getSpectrum(1).readE();
    TS_ASSERT_DELTA(vecE1.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE1.at(9), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(9), 0.005, 0.0001);

    AnalysisDataService::Instance().clear();
  }

  void test_outputProduced() {
    std::string emptyString("");

    auto ws = createCountsWorkspace(5, 10, 0.0);
    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputData", ws);
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroup->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1,2,3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SummedPeriods", std::to_string(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_singleGroupDoesntChangeData() {
    std::string emptyString("");

    auto ws = createCountsWorkspace(1, 10, 0.0);
    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputData", ws);
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroup->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
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
    TS_ASSERT_THROWS_NOTHING(algCompare.setProperty("CheckAllData", true));
    TS_ASSERT_THROWS_NOTHING(algCompare.execute());
    TS_ASSERT(algCompare.isExecuted());

    TS_ASSERT(algCompare.getProperty("Result"));

    AnalysisDataService::Instance().clear();
  }

  void test_groupingWithCounts() {
    std::string emptyString("");

    auto ws = createCountsWorkspace(5, 10, 0.0);
    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputData", ws);
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    std::string wsName = ws->getName();
    std::string wsGroupName = (wsGroup)->getName();

    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 5);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroup->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1,2,3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
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
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 30.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 42.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 57.000, 0.001);
    // Quadrature errors : Sqrt(0.005)
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.00866, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.00866, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.00866, 0.00001);

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_singleDetectorGroupingWithAsymmetry() {

    std::string emptyString("");

    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(5, 10);
    AnalysisDataService::Instance().addOrReplace("inputData", ws);

    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    std::string wsName = ws->getName();
    std::string wsGroupName = (wsGroup)->getName();

    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroupName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Asymmetry"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SummedPeriods", std::to_string(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }

    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Asym; #1"));
    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Asym; #1_Raw"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Asym; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.02195579, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.00420021, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 0.02306048, 0.0001);
    // Errors are simply normalized by a constant.
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0000222282776, 0.0000001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.0000266671712, 0.0000001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.0000334823452, 0.0000001);

    AnalysisDataService::Instance().clear();
  }

  void test_multipleDetectorGroupingWithAsymmetry() {

    std::string emptyString("");

    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(5, 10);
    AnalysisDataService::Instance().addOrReplace("inputData", ws);

    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    std::string wsName = ws->getName();
    std::string wsGroupName = (wsGroup)->getName();

    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroupName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1,2,3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Asymmetry"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SummedPeriods", std::to_string(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }

    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Asym; #1"));
    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Asym; #1_Raw"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test2; Asym; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.01162, 0.00001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.00222, 0.00001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 0.01221, 0.00001);
    // Errors : quadrature addition + normalized by a constant.
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.00000679, 0.00000001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.00000815, 0.00000001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.00001023, 0.00000001);

    AnalysisDataService::Instance().clear();
  }

  void test_groupingCountsMultiplePeriodAdd() {
    std::string emptyString("");

    // Period 1 yvalues : 1,2,3,4,5,6,7,8,9,10
    // Period 2 yvalues : 2,3,4,5,6,7,8,9,10,11
    WorkspaceGroup_sptr ws =
        createMultiPeriodWorkspaceGroup(3, 1, 10, "MuonAnalysis");

    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    std::string wsName = ws->getName();
    std::string wsGroupName = (wsGroup)->getName();

    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroupName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriods", "1,2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }

    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Counts; #1"));
    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Counts; #1_Raw"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test2; Counts; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 3, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 11, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 21, 0.0001);
    // Errors : quadrature addition from periods (1 + 2).
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.00707, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.00707, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.00707, 0.0001);

    AnalysisDataService::Instance().clear();
  }

  void test_groupingCountsMultiplePeriodSubtract() {
    std::string emptyString("");

    // Period 1 y-values : 1,2,3,4,5,6,7,8,9,10
    // Period 2 y-values : 2,3,4,5,6,7,8,9,10,11
    // Period 3 y-values : 3,4,5,6,7,8,9,10,11,12
    WorkspaceGroup_sptr ws =
        createMultiPeriodWorkspaceGroup(3, 1, 10, "MuonAnalysis");

    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    std::string wsName = ws->getName();
    std::string wsGroupName = (wsGroup)->getName();

    TS_ASSERT_EQUALS(ws->getNumberOfEntries(), 3);
    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);

    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspaceGroup", wsGroupName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Counts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriods", "2,3"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(wsGroup);
    if (wsGroup) {
      TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    }

    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Counts; #1"));
    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Counts; #1_Raw"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test2; Counts; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 4, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 8, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 13, 0.0001);
    // Errors : quadrature addition from periods (1 + 2 - 3)
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.00866, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.00866, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.00866, 0.00001);
  }

  void test_deadTimeCorrection() {
	  std::string emptyString("");

	  std::vector<double> deadTimes = {0.0025};
	  ITableWorkspace_sptr deadTimeTable = createDeadTimeTable(1, deadTimes);

	  auto ws = createAsymmetryWorkspace(1, 10);
	  auto wsGroup = boost::make_shared<WorkspaceGroup>();
	  AnalysisDataService::Instance().addOrReplace("inputData", ws);
	  AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

	  TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 0);

	  ApplyMuonDetectorGrouping alg;
	  TS_ASSERT_THROWS_NOTHING(alg.initialize());
	  TS_ASSERT(alg.isInitialized());

	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
	  TS_ASSERT_THROWS_NOTHING(
		  alg.setProperty("InputWorkspaceGroup", wsGroup->getName()));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("groupName", "test2"));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("Grouping", "1"));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("AnalysisType", "Counts"));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMin", 0.0));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeMax", 30.0));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinArgs", emptyString));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeOffset", 0.0));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("SummedPeriods", "1"));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("SubtractedPeriods", emptyString));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("ApplyDeadTimeCorrection", true));
	  TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeadTimeTable", deadTimeTable));
	  TS_ASSERT_THROWS_NOTHING(alg.execute());
	  TS_ASSERT(alg.isExecuted());

	  TS_ASSERT(wsGroup);
	  if (wsGroup) {
		  TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
	  }

	  TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Counts; #1"));
	  TS_ASSERT(wsGroup->getItem("inputGroup; Group; test2; Counts; #1_Raw"));

	  auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
		  wsGroup->getItem("inputGroup; Group; test2; Counts; #1_Raw"));

	  // Check values against calculation by hand.
	  TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
	  TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
	  TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

	  TS_ASSERT_DELTA(wsOut->readY(0)[0], 488.9, 0.1);
	  TS_ASSERT_DELTA(wsOut->readY(0)[4], 350.1, 0.1);
	  TS_ASSERT_DELTA(wsOut->readY(0)[9], 247.2, 0.1);

	  TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0050, 0.0001);
	  TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.0050, 0.0001);
	  TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.0050, 0.0001);
  }

};

#endif /* MANTID_MUON_APPLYMUONDETECTORSGROUPINGTEST_H_ */