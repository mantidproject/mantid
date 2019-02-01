// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_APPLYMUONDETECTORGROUPINGTEST_H_
#define MANTID_MUON_APPLYMUONDETECTORGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidMuon/ApplyMuonDetectorGrouping.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Muon;
using namespace MuonWorkspaceCreationHelper;

namespace {

// Set sensible default algorithm properties
IAlgorithm_sptr algorithmWithPropertiesSet(const std::string &inputWSName,
                                           const std::string &inputGroupName) {
  auto alg = boost::make_shared<ApplyMuonDetectorGrouping>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("InputWorkspaceGroup", inputGroupName);
  alg->setProperty("groupName", "test");
  alg->setProperty("Grouping", "1,2,3");
  alg->setProperty("AnalysisType", "Counts");
  alg->setProperty("TimeMin", 0.0);
  alg->setProperty("TimeMax", 30.0);
  alg->setProperty("RebinArgs", "");
  alg->setProperty("TimeOffset", 0.0);
  alg->setProperty("SummedPeriods", std::to_string(1));
  alg->setProperty("SubtractedPeriods", "");
  alg->setLogging(false);
  return alg;
}

// Simple class to set up the ADS with the configuration required by the
// algorithm (a MatrixWorkspace and an empty GroupWorkspace).
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

class ApplyMuonDetectorGroupingTest : public CxxTest::TestSuite {
public:
  // WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ApplyMuonDetectorGroupingTest() { Mantid::API::FrameworkManager::Instance(); }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyMuonDetectorGroupingTest *createSuite() {
    return new ApplyMuonDetectorGroupingTest();
  }
  static void destroySuite(ApplyMuonDetectorGroupingTest *suite) {
    delete suite;
  }

  void test_algorithm_initializes() {
    ApplyMuonDetectorGrouping alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_algorithm_executes_with_default_arguments() {

    auto ws = createCountsWorkspace(5, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_output_produced_in_ADS_for_counts_analysis() {

    auto ws = createCountsWorkspace(5, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("inputGroup"))
    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));
    // Raw + Rebinned
    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
  }

  void test_output_produced_in_ADS_for_asymmetry_analysis() {

    auto ws = createAsymmetryWorkspace(3, 10);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("AnalysisType", "Asymmetry");
    alg->execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("inputGroup"))
    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));
    // Raw + Rebinned + unNorm + unNorm_Raw
    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 4);
  }

  void test_workspaces_named_correctly() {

    auto ws = createCountsWorkspace(3, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->execute();
    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Counts; #1"));
    TS_ASSERT(wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));
  }

  void test_grouping_a_single_detector_does_not_change_the_data() {

    auto ws = createCountsWorkspace(1, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("Grouping", "1");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    auto wsOut = wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw");

    CompareWorkspaces algCompare;
    algCompare.initialize();
    algCompare.setProperty("Workspace1", ws->getName());
    algCompare.setProperty("Workspace2", wsOut->getName());
    algCompare.setProperty("Tolerance", 0.001);
    algCompare.setProperty("CheckAllData", true);
    algCompare.execute();

    TS_ASSERT(algCompare.getProperty("Result"));
  }

  void test_grouping_with_counts_analysis_gives_correct_values() {

    auto ws = createCountsWorkspace(5, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));
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
  }

  void
  test_grouping_with_single_detector_and_asymmetry_analysis_gives_correct_values() {

    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(1, 10);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("AnalysisType", "Asymmetry");
    alg->setProperty("Grouping", "1");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Asym; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 1.350, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.771, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.2158, 0.001);
    // Errors are simply normalized by a constant.
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.00094, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.00113, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.00142, 0.00001);
  }

  void
  test_grouping_with_multiple_detectors_and_asymmetry_analysis_gives_correct_values() {

    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(3, 10);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("AnalysisType", "Asymmetry");
    alg->setProperty("Grouping", "1,2,3");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Asym; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 1.410, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], -0.876, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.053, 0.001);
    // Errors : quadrature addition + normalized by a constant.
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.000282, 0.000001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.000338, 0.000001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.000424, 0.000001);
  }

  void
  test_grouping_with_summed_multiple_periods_and_counts_analysis_gives_correct_values() {

    // Period 1 yvalues : 1,2,3,4,5,6,7,8,9,10
    // Period 2 yvalues : 2,3,4,5,6,7,8,9,10,11
    WorkspaceGroup_sptr ws =
        createMultiPeriodWorkspaceGroup(3, 1, 10, "MuonAnalysis");
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("Grouping", "1");
    alg->setProperty("SummedPeriods", "1,2");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));

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
  }

  void
  test_grouping_with_subtracted_multiple_periods_and_counts_analysis_gives_correct_values() {

    // Period 1 y-values : 1,2,3,4,5,6,7,8,9,10
    // Period 2 y-values : 2,3,4,5,6,7,8,9,10,11
    // Period 3 y-values : 3,4,5,6,7,8,9,10,11,12
    WorkspaceGroup_sptr ws =
        createMultiPeriodWorkspaceGroup(3, 1, 10, "MuonAnalysis");
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("SummedPeriods", "2,3");
    alg->setProperty("SubtractedPeriods", "1");
    alg->setProperty("Grouping", "1");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));

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

  void test_dead_time_correction_is_applied_correctly() {

    std::vector<double> deadTimes = {0.0025};
    ITableWorkspace_sptr deadTimeTable = createDeadTimeTable(1, deadTimes);

    auto ws = createAsymmetryWorkspace(1, 10);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("Grouping", "1");
    alg->setProperty("ApplyDeadTimeCorrection", true);
    alg->setProperty("DeadTimeTable", deadTimeTable);
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 12.86, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 1.01, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 2.78, 0.1);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0050, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.0050, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.0050, 0.0001);
  }

  void test_rebinning_is_applied_correctly() {

    auto ws = createCountsWorkspace(3, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("RebinArgs", "0.2");
    alg->execute();

    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("inputGroup"));

    auto wsOutNoRebin = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Counts; #1_Raw"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOutNoRebin->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOutNoRebin->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOutNoRebin->readX(0)[9], 0.900, 0.001);

    auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("inputGroup; Group; test; Counts; #1"));

    // Check values against calculation by hand.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.200, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.800, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 63, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 75, 0.1);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 111, 0.1);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0122, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[1], 0.0122, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.0122, 0.0001);
  }

  /// 26/06/18 : unNorm workspaces required in ADS
  void test_unNorm_workspaces_named_correctly() {
    auto ws = createCountsWorkspace(3, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    Mantid::API::IAlgorithm_sptr alg =
        algorithmWithPropertiesSet("inputData", "inputGroup");
    alg->setProperty("AnalysisType", "Asymmetry");
    alg->execute();

    auto name = AnalysisDataService::Instance().getObjectNames();
    TS_ASSERT(AnalysisDataService::Instance().doesExist(
        "inputGroup; Group; test; Asym; #1_unNorm"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist(
        "inputGroup; Group; test; Asym; #1_unNorm_Raw"));
  }
};

#endif /* MANTID_MUON_APPLYMUONDETECTORGROUPINGTEST_H_ */
