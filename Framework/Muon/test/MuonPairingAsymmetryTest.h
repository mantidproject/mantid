// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_MUONPAIRINGASYMMETRYTEST_H_
#define MANTID_MUON_MUONPAIRINGASYMMETRYTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMuon/MuonGroupingCounts.h"
#include "MantidMuon/MuonPairingAsymmetry.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;
using namespace MuonWorkspaceCreationHelper;

namespace {

// Simple class to set up the ADS with the configuration required by the
// algorithm (a MatrixWorkspace).
class setUpADSWithWorkspace {
public:
  setUpADSWithWorkspace(Workspace_sptr ws) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, ws);
  };
  ~setUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };

  std::string const inputWSName = "inputData";
};

// Set only mandatory fields
IAlgorithm_sptr algorithmWithoutOptionalPropertiesSet(
    const std::string &inputWSName, const std::string &pairName,
    const std::vector<int> &group1, const std::vector<int> &group2) {

  auto alg = boost::make_shared<MuonPairingAsymmetry>();
  alg->initialize();
  alg->setProperty("SpecifyGroupsManually", true);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("PairName", pairName);
  alg->setProperty("Group1", group1);
  alg->setProperty("Group2", group2);
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);
  return alg;
}

// Set up algorithm without any optional properties.
IAlgorithm_sptr
setUpAlgorithmWithoutOptionalProperties(WorkspaceGroup_sptr ws,
                                        const std::string &name) {
  const std::vector<int> group1 = {1, 2};
  const std::vector<int> group2 = {3, 4};

  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg = algorithmWithoutOptionalPropertiesSet(
      setup.inputWSName, name, group1, group2);
  return alg;
}

// Set up algorithm with groupings
IAlgorithm_sptr setUpAlgorithmWithGroups(WorkspaceGroup_sptr ws,
                                         const std::vector<int> &group1,
                                         const std::vector<int> &group2) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg = algorithmWithoutOptionalPropertiesSet(
      setup.inputWSName, "pair1", group1, group2);
  return alg;
}

// Set up algorithm to accept two group workspaces
IAlgorithm_sptr
setUpAlgorithmWithGroupWorkspaces(MatrixWorkspace_sptr groupedWS1,
                                  MatrixWorkspace_sptr groupedWS2) {
  auto alg = boost::make_shared<MuonPairingAsymmetry>();
  alg->initialize();
  alg->setProperty("SpecifyGroupsManually", false);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setProperty("InputWorkspace1", groupedWS1);
  alg->setProperty("InputWorkspace2", groupedWS2);
  alg->setProperty("PairName", "pair1");
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);
  return alg;
}

// Set up MuonPairingAsymmetry algorithm to accept two WorkspaceGroups
IAlgorithm_sptr
setUpAlgorithmWithGroupWorkspaceGroups(WorkspaceGroup_sptr groupedWS1,
                                       WorkspaceGroup_sptr groupedWS2) {
  auto alg = boost::make_shared<MuonPairingAsymmetry>();
  alg->setRethrows(true);
  alg->initialize();
  alg->setProperty("SpecifyGroupsManually", false);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setProperty("InputWorkspace1", groupedWS1);
  alg->setProperty("InputWorkspace2", groupedWS2);
  alg->setProperty("PairName", "pair1");
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);
  return alg;
}

// Retrieve the output workspace from an executed algorithm
MatrixWorkspace_sptr getOutputWorkspace(IAlgorithm_sptr alg) {
  MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
  return outputWS;
}

MatrixWorkspace_sptr createGroupWorkspace(const std::string &groupName,
                                          const std::vector<int> &grouping,
                                          const int &nPeriods) {
  auto ws = createMultiPeriodAsymmetryData(nPeriods, 4, 10, "group");
  setUpADSWithWorkspace setup(ws);

  auto alg = boost::make_shared<MuonGroupingCounts>();
  alg->initialize();
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setProperty("InputWorkspace", setup.inputWSName);
  alg->setProperty("GroupName", groupName);
  alg->setProperty("Grouping", grouping);
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);

  alg->execute();

  Workspace_sptr outputWS = alg->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
}

WorkspaceGroup_sptr
createMultiPeriodGroupedWorkspace(const std::string &groupName,
                                  const std::vector<int> &grouping,
                                  const int &nPeriods) {
  auto ws = createMultiPeriodAsymmetryData(nPeriods, 4, 10, "group");

  auto wsGroup = boost::make_shared<WorkspaceGroup>();

  for (int i = 1; i < nPeriods + 1; i++) {
    std::vector<int> periods = {i};
    auto alg = boost::make_shared<MuonGroupingCounts>();
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__notUsed");
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("GroupName", groupName);
    alg->setProperty("Grouping", grouping);
    alg->setProperty("SummedPeriods", periods);
    alg->setAlwaysStoreInADS(false);
    alg->setLogging(false);

    alg->execute();

    Workspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    wsGroup->addWorkspace(outputWS);
  }

  return wsGroup;
}

} // namespace

class MuonPairingAsymmetryTest : public CxxTest::TestSuite {
public:
  static MuonPairingAsymmetryTest *createSuite() {
    return new MuonPairingAsymmetryTest();
  }
  static void destroySuite(MuonPairingAsymmetryTest *suite) { delete suite; }

  // --------------------------------------------------------------------------
  // Initialization / Execution
  // --------------------------------------------------------------------------

  void test_that_algorithm_initializes() {
    MuonPairingAsymmetry alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_that_algorithm_executes_with_no_optional_properties_set() {

    auto ws = createMultiPeriodWorkspaceGroup(1, 6, 10, "pair1");
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "pair1");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
  }

  // --------------------------------------------------------------------------
  // Validation : Pair Name
  // --------------------------------------------------------------------------

  void test_that_input_workspace_cannot_be_a_Workspace2D() {

    auto ws = createCountsWorkspace(5, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    auto alg = boost::make_shared<MuonPairingAsymmetry>();
    alg->initialize();

    TS_ASSERT_THROWS_ANYTHING(
        alg->setProperty("InputWorkspace", setup.inputWSName));
  }

  void test_that_input_workspace_can_be_a_WorkspaceGroup() {

    auto ws = createMultiPeriodWorkspaceGroup(1, 6, 10, "group1");
    setUpADSWithWorkspace setup(ws);
    auto alg = boost::make_shared<MuonPairingAsymmetry>();
    alg->initialize();

    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", setup.inputWSName))
  }

  void test_that_non_empty_pair_name_must_be_supplied() {
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};

    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "pair1");
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg = algorithmWithoutOptionalPropertiesSet(
        setup.inputWSName, "", group1, group2);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_pair_names_with_alphanumeric_characters_or_underscores_are_allowed() {
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "pairWS");

    std::vector<std::string> validNames = {"fwd", "fwd2", "bwd_2"};
    for (auto &&validName : validNames) {
      auto alg = algorithmWithoutOptionalPropertiesSet("pairWS", validName,
                                                       group1, group2);
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void test_that_exec_throws_if_pair_name_is_not_alphanumeric_or_underscored() {
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "pairWS");

    std::vector<std::string> invalidNames = {"@", "fwd!", "#1", "fwd @", "   "};
    for (auto &&invalidName : invalidNames) {
      auto alg = algorithmWithoutOptionalPropertiesSet("pairWS", invalidName,
                                                       group1, group2);
      TS_ASSERT_THROWS_ANYTHING(alg->execute());
    }
  }

  // --------------------------------------------------------------------------
  // Validation : Alpha
  // --------------------------------------------------------------------------

  void test_that_exec_throws_if_alpha_is_negative() {
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "pairWS");
    auto alg =
        algorithmWithoutOptionalPropertiesSet("pairWS", "pair", group1, group2);

    alg->setProperty("Alpha", -0.1);

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  // --------------------------------------------------------------------------
  // Validation : Grouping
  // --------------------------------------------------------------------------

  void test_that_two_groupings_must_be_supplied() {

    auto ws = createMultiPeriodWorkspaceGroup(1, 5, 10, "pair1");
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "pair1");

    std::vector<int> group1 = {};
    std::vector<int> group2 = {};
    alg->setProperty("Group1", group1);
    alg->setProperty("Group2", group2);

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_that_two_different_groupings_must_be_supplied() {

    auto ws = createMultiPeriodWorkspaceGroup(1, 5, 10, "pair1");
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "pair1");

    std::vector<int> group1 = {1, 2, 3};
    std::vector<int> group2 = {1, 2, 3};
    alg->setProperty("Group1", group1);
    alg->setProperty("Group2", group2);

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  // --------------------------------------------------------------------------
  // Validation : Multi period data
  // --------------------------------------------------------------------------

  void test_that_at_least_one_period_must_be_specified() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    const std::vector<int> detectors = {1, 2};
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "pair1");

    std::vector<int> summedPeriods = {};
    std::vector<int> subtractedPeriods = {};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_supplying_too_many_periods_to_SummedPeriods_throws_on_execute() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "pair1");

    std::vector<int> summedPeriods = {3};
    alg->setProperty("SummedPeriods", summedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_supplying_too_many_periods_to_SubtractedPeriods_throws_on_execute() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "pair1");

    std::vector<int> subtractedPeriods = {3};
    alg->setProperty("SubtractedPeriods", subtractedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  // --------------------------------------------------------------------------
  // Correct Output : Single Period
  // --------------------------------------------------------------------------

  void
  test_that_single_period_data_combines_detectors_correctly_for_manually_specified_detectors() {
    // 4 spectra per period, 10 bins
    auto ws = createMultiPeriodAsymmetryData(1, 4, 10, "pairWS");
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};

    auto alg = setUpAlgorithmWithGroups(ws, group1, group2);
    alg->execute();
    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.3889, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.8211, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.04641, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 1.00000, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.19818, 0.0001);
  }

  void
  test_that_single_period_data_combines_detectors_correctly_for_two_group_workspaces() {

    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto ws1 = createGroupWorkspace("fwd", group1, 1);
    auto ws2 = createGroupWorkspace("bwd", group2, 1);

    auto alg = setUpAlgorithmWithGroupWorkspaces(ws1, ws2);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.05, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.45, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.95, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.3889, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.8211, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.04641, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 1.00000, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.19818, 0.0001);
  }

  // //
  // --------------------------------------------------------------------------
  // // Correct Output : Multi Period
  // //
  // --------------------------------------------------------------------------

  void
  test_that_multi_period_data_combines_detectors_correctly_for_manually_specified_detectors_and_summed_periods() {

    auto ws = createMultiPeriodAsymmetryData(2, 4, 10, "pairWS");
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto alg = setUpAlgorithmWithGroups(ws, group1, group2);
    alg->setProperty("SummedPeriods", group1);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.38484955, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.74269249, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.02743, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 1.0000, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.098512, 0.0001);
  }

  void
  test_that_multi_period_data_combines_detectors_correctly_for_manually_specified_detectors_and_subtracted_periods() {

    auto ws = createMultiPeriodAsymmetryData(2, 4, 10, "pairWS");
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto alg = setUpAlgorithmWithGroups(ws, group1, group2);
    const std::vector<int> summedPeriods = {1};
    const std::vector<int> subtractedPeriods = {2};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.00630986, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.10690094, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.05754263, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.2283730, 0.0001);
  }

  void
  test_that_multi_period_data_combines_detectors_correctly_for_manually_specified_detectors_summed_and_subtracted_periods() {

    auto ws = createMultiPeriodAsymmetryData(3, 4, 10, "pairWS");
    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto alg = setUpAlgorithmWithGroups(ws, group1, group2);
    const std::vector<int> summedPeriods = {1, 2};
    const std::vector<int> subtractedPeriods = {3};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.00879057, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.0, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.130944, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0395697, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.122684, 0.0001);
  }

  void
  test_that_multi_period_data_combines_detectors_correctly_for_group_workspaces_summed_and_subtracted_periods() {

    const std::vector<int> group1 = {1, 2};
    const std::vector<int> group2 = {3, 4};
    auto ws1 = createMultiPeriodGroupedWorkspace("fwd", group1, 3);
    auto ws2 = createMultiPeriodGroupedWorkspace("bwd", group2, 3);

    auto alg = setUpAlgorithmWithGroupWorkspaceGroups(ws1, ws2);
    const std::vector<int> summedPeriods = {1, 2};
    const std::vector<int> subtractedPeriods = {3};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.050, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.450, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.950, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.00879057, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 0.0, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], -0.130944, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0395697, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.122684, 0.0001);
  }
};

#endif /* MANTID_MUON_MUONPAIRINGASYMMETRYTEST_H_ */
