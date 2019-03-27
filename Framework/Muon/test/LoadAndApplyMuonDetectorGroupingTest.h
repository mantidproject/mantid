// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPINGTEST_H_
#define MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidMuon/LoadAndApplyMuonDetectorGrouping.h"

#include "MantidTestHelpers/MuonGroupingXMLHelper.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace {

// Set algorithm properties
Mantid::API::IAlgorithm_sptr
algorithmWithPropertiesSet(const std::string &inputWSName,
                           const std::string &filename) {
  auto alg = boost::make_shared<Muon::LoadAndApplyMuonDetectorGrouping>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("Filename", filename);
  alg->setProperty("ApplyAsymmetryToGroups", true);
  alg->setProperty("AddGroupingTable", false);
  alg->setLogging(false);
  return alg;
}

// Simple class to set up the ADS with the configuration required by the
// algorithm (a MatrixWorkspace and an empty group).
class setUpADSWithWorkspace {
public:
  setUpADSWithWorkspace(Workspace_sptr ws) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, ws);
    wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace(groupWSName, wsGroup);
  };

  ~setUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
  WorkspaceGroup_sptr wsGroup;

  static constexpr const char *inputWSName = "inputData";
  static constexpr const char *groupWSName = "inputGroup";
};

} // namespace

class LoadAndApplyMuonDetectorGroupingTest : public CxxTest::TestSuite {
public:
  // WorkflowAlgorithms do not appear in the FrameworkManager without this line
  LoadAndApplyMuonDetectorGroupingTest() {
    Mantid::API::FrameworkManager::Instance();
  }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadAndApplyMuonDetectorGroupingTest *createSuite() {
    return new LoadAndApplyMuonDetectorGroupingTest();
  };
  static void destroySuite(LoadAndApplyMuonDetectorGroupingTest *suite) {
    delete suite;
  };

  void test_init() {
    Muon::LoadAndApplyMuonDetectorGrouping alg;
    alg.setLogging(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  };

  void test_init_and_exec_with_simple_properties() {

    auto ws = MuonWorkspaceCreationHelper::createCountsWorkspace(5, 10, 0.0);
    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().addOrReplace("inputData", ws);
    AnalysisDataService::Instance().addOrReplace("inputGroup", wsGroup);

    auto file =
        MuonGroupingXMLHelper::createGroupingXMLSingleGroup("test", "1,2,3");
    std::string filename = file.getFileName();

    Muon::LoadAndApplyMuonDetectorGrouping alg;
    alg.setLogging(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("WorkspaceGroup", wsGroup->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ApplyAsymmetryToGroups", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    AnalysisDataService::Instance().clear();
  }

  void test_workspaces_named_and_grouped_correctly() {
    size_t workspacesBeforeExec =
        AnalysisDataService::Instance().getObjectNames().size();

    auto ws = MuonWorkspaceCreationHelper::createCountsWorkspace(10, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 5);
    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("AddGroupingTable", true);
    alg->execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("EMU00012345"));
    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));
    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 14);
    // Group Counts
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group1; Counts; #1"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group1; Counts; #1_Raw"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group2; Counts; #1"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group2; Counts; #1_Raw"));
    // Group Asymmetry
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group1; Asym; #1"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group1; Asym; #1_Raw"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group2; Asym; #1"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group2; Asym; #1_Raw"));
    // Group Asymmetry, unNormalized
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group1; Asym; #1_unNorm"));
    TS_ASSERT(
        wsGroup->contains("EMU00012345; Group; group1; Asym; #1_unNorm_Raw"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Group; group2; Asym; #1_unNorm"));
    TS_ASSERT(
        wsGroup->contains("EMU00012345; Group; group2; Asym; #1_unNorm_Raw"));
    // Pair Asymmetry
    TS_ASSERT(wsGroup->contains("EMU00012345; Pair; pair1; Asym; #1"));
    TS_ASSERT(wsGroup->contains("EMU00012345; Pair; pair1; Asym; #1_Raw"));
    // Analysed workspaces as above : (14)
    //    unBinned (5)
    //	  _Raw (5)
    //	  _unNorm (2)
    //	  _unNorm_Raw (2)
    // "inputData" "inputGroup" from setup (2)
    // "MuonGroupings" from load algorithm (1)
    // "MuonAnalysisTFNormalizations" (1)
    // "tmp_unNorm" (1)
    auto names = AnalysisDataService::Instance().getObjectNames();
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().getObjectNames().size(),
                     workspacesBeforeExec + 19);
  }

  void test_produces_workspaces_with_correct_entries() {

    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        4, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group2; Counts; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 85.43223343, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.87394, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 25.57248768, 0.0001);
    // Sqrt(2) * 0.005.
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.007071, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.007071, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.007071, 0.00001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 37.2468, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 2.2974, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 8.9759, 0.0001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1_Raw"));

    // Asymmetry converts bin width to point data
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.3928, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.44885, 0.0001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.4804, 0.0001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.09699944, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.6155, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.18874449, 0.00001);
  }

  void test_workspace_overwritten_if_name_is_duplicated() {

    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        1, 10, MuonWorkspaceCreationHelper::yDataAsymmetry());
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(1, 1);

    // Add workspaces which should be overwritten
    auto &ads = AnalysisDataService::Instance();
    auto ws1 = MuonWorkspaceCreationHelper::createCountsWorkspace(1, 20, 5.0);
    auto ws2 = MuonWorkspaceCreationHelper::createCountsWorkspace(1, 20, 10.0);
    ads.addOrReplace("EMU00012345; Group; group1; Counts; #1", ws1);
    ads.addOrReplace("EMU00012345; Group; group1; Counts; #1_Raw", ws2);
    setup.wsGroup->add("EMU00012345; Group; group1; Counts; #1");
    setup.wsGroup->add("EMU00012345; Group; group1; Counts; #1_Raw");

    int numEntriesBefore = setup.wsGroup->getNumberOfEntries();

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->execute();

    TS_ASSERT_EQUALS(setup.wsGroup->getNumberOfEntries(), numEntriesBefore);
    TS_ASSERT(setup.wsGroup->getItem("EMU00012345; Group; group1; Counts; #1"));
    TS_ASSERT(
        setup.wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        setup.wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_EQUALS(wsOut->readX(0).size(), 10 + 1);

    ads.clear();
  }

  void test_default_workspace_name_correct_for_unrecongnized_instrument() {

    auto ws = MuonWorkspaceCreationHelper::createCountsWorkspace(4, 2, 0.0);
    boost::shared_ptr<Geometry::Instrument> inst1 =
        boost::make_shared<Geometry::Instrument>();
    inst1->setName("LHC");
    ws->setInstrument(inst1);

    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    setUpADSWithWorkspace setup(ws);
    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->execute();

    auto names = AnalysisDataService::Instance().getObjectNames();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("LHC12345"));
  }

  void test_grouping_table_produced_with_correct_groups() {

    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        4, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("AddGroupingTable", true);
    alg->execute();

    ITableWorkspace_sptr wsGroupings =
        boost::dynamic_pointer_cast<ITableWorkspace>(
            AnalysisDataService::Instance().retrieve("MuonGroupings"));

    // Col1 : Names, Col2 : Detector IDs
    std::string name1 = wsGroupings->String(0, 0);
    std::string name2 = wsGroupings->String(1, 0);
    TS_ASSERT_EQUALS(name1, "group1");
    TS_ASSERT_EQUALS(name2, "group2");

    std::vector<int> group1 = wsGroupings->cell<std::vector<int>>(0, 1);
    std::stringstream group1String;
    std::copy(group1.begin(), group1.end(),
              std::ostream_iterator<int>(group1String, ","));
    TS_ASSERT_EQUALS(group1String.str(), "1,2,");

    std::vector<int> group2 = wsGroupings->cell<std::vector<int>>(1, 1);
    std::stringstream group2String;
    std::copy(group2.begin(), group2.end(),
              std::ostream_iterator<int>(group2String, ","));
    TS_ASSERT_EQUALS(group2String.str(), "3,4,");
  }

  void test_throws_if_group_name_not_valid() {

    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        4, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file =
        MuonGroupingXMLHelper::createGroupingXMLSingleGroup("group_", "1-2");

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_throws_if_pair_contains_non_existant_group() {

    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        2, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createGroupingXMLSinglePair(
        "pair1", "nonExistantGroup");

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    TS_ASSERT_THROWS(alg->execute(), Mantid::Kernel::Exception::FileError);
  }

  void test_throws_when_file_has_detectors_which_are_not_in_workspace() {

    auto ws = MuonWorkspaceCreationHelper::createCountsWorkspace(5, 3, 0.0);
    setUpADSWithWorkspace setup(ws);
    std::string group = "1-10";
    auto file =
        MuonGroupingXMLHelper::createGroupingXMLSingleGroup("test", group);
    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());

    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_rebinning_applied_correctly() {
    // Bin widths of 0.1
    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        4, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("RebinArgs", "0.2");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.100, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.200, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.800, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1_Raw"));
    // Asymmetry converted to point data
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.150, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.0001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1"));
    // Rebinning happens before conversion to point data
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.100, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.300, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.900, 0.0001);
  }

  void test_TimeOffset_applied_correctly() {
    // Time starts at zero
    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        4, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("TimeOffset", "0.5");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.600, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.900, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.600, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.900, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.550, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.650, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.950, 0.0001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.550, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.650, 0.0001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.950, 0.0001);
  }

  void test_multiple_period_data_summing_periods_gives_correct_result() {

    auto ws = MuonWorkspaceCreationHelper::createMultiPeriodWorkspaceGroup(
        2, 4, 10, "MuonAnalysis");
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("SummedPeriods", "1,2");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 26, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 30, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 42, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group2; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 106, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 110, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 122, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1_Raw"));
    // Asymmetry on group 1 and 2
    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.6061, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], -0.5714, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.4878, 0.001);
  }

  void test_multiple_period_data_subtracting_periods_gives_correct_result() {

    auto ws = MuonWorkspaceCreationHelper::createMultiPeriodWorkspaceGroup(
        2, 4, 10, "MuonAnalysis");
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("SummedPeriods", "1");
    alg->setProperty("SubtractedPeriods", "2");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -2, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], -2, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -2, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group2; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -2, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], -2, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -2, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.03676, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], -0.03268, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.02382, 0.001);
  }

  void test_dead_time_correction_is_applied_correctly() {

    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        4, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    // Apply same dead time to every spectra
    std::vector<double> deadTimes(4, 0.0025);
    ITableWorkspace_sptr deadTimeTable =
        MuonWorkspaceCreationHelper::createDeadTimeTable(4, deadTimes);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("DeadTimeTable", deadTimeTable);
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 39.2846, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 32.9165, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 2.30412, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group2; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 95.8873, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 75.7566, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.87585, 0.001);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.41874, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], -0.39421, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.4491, 0.001);
  }

  void test_workspace_cropped_only_on_lower_limit() {

    auto ws = MuonWorkspaceCreationHelper::createAsymmetryWorkspace(
        4, 10, MuonWorkspaceCreationHelper::yDataAsymmetry(1.5, 0.1));
    setUpADSWithWorkspace setup(ws);
    auto file = MuonGroupingXMLHelper::createXMLwithPairsAndGroups(2, 2);

    auto alg = algorithmWithPropertiesSet(ws->getName(), file.getFileName());
    alg->setProperty("CropWorkspaces", true);
    alg->setProperty("TimeMin", 0.2);
    alg->setProperty("TimeMax", 0.8);
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("EMU00012345"));

    auto delta = 0.001;
    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Group; group1; Counts; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.2, delta);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.3, delta);
    TS_ASSERT_DELTA(wsOut->readX(0)[8], 1.0, delta);

    wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("EMU00012345; Pair; pair1; Asym; #1_Raw"));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.25, delta);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.35, delta);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.65, delta);
  }

  void test_group_asymmetry_range_applied_correctly() {}
};

#endif /* MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPING_H_ */
