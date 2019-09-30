// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_MUONGROUPINGASYMMETRYTEST_H_
#define MANTID_MUON_MUONGROUPINGASYMMETRYTEST_H_

#include <cxxtest/TestSuite.h>
#include <string>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMuon/MuonGroupingAsymmetry.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

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

  const std::string inputWSName = "inputData";
};

// Set only mandatory fields; input and output workspace
IAlgorithm_sptr
algorithmWithWorkspacePropertiesSet(const std::string &inputWSName) {

  auto alg = boost::make_shared<MuonGroupingAsymmetry>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);
  return alg;
}

// Set up algorithm without any optional properties
// i.e. just the input workspace and group name.
IAlgorithm_sptr
setUpAlgorithmWithoutOptionalProperties(WorkspaceGroup_sptr ws,
                                        const std::string &name,
                                        const std::vector<int> &grouping) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg = algorithmWithWorkspacePropertiesSet(setup.inputWSName);
  alg->setProperty("GroupName", name);
  alg->setProperty("Grouping", grouping);
  return alg;
}

// Retrieve the output workspace from an executed algorithm
MatrixWorkspace_sptr getOutputWorkspace(IAlgorithm_sptr alg) {
  MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
  return outputWS;
}

} // namespace

class MuonGroupingAsymmetryTest : public CxxTest::TestSuite {
public:
  static MuonGroupingAsymmetryTest *createSuite() {
    return new MuonGroupingAsymmetryTest();
  }
  static void destroySuite(MuonGroupingAsymmetryTest *suite) { delete suite; }

  // --------------------------------------------------------------------------
  // Initialization / Execution
  // --------------------------------------------------------------------------

  void test_algorithm_initializes() {
    MuonGroupingAsymmetry alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_algorithm_executes_with_default_arguments() {
    const std::vector<int> group = {1};
    auto ws = createMultiPeriodWorkspaceGroup(1, 5, 10, "asym");
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "asym", group);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
  }

  // --------------------------------------------------------------------------
  // Validation : InputWorkspace and GroupName
  // --------------------------------------------------------------------------

  void test_that_input_workspace_cannot_be_a_Workspace2D() {

    auto ws = createCountsWorkspace(5, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    auto alg = boost::make_shared<MuonGroupingAsymmetry>();
    alg->initialize();

    TS_ASSERT_THROWS_ANYTHING(
        alg->setProperty("InputWorkspace", setup.inputWSName));
  }

  void test_that_input_workspace_can_be_a_WorkspaceGroup() {

    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "group1");
    setUpADSWithWorkspace setup(ws);
    auto alg = boost::make_shared<MuonGroupingAsymmetry>();
    alg->initialize();

    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", setup.inputWSName))
  }

  void test_that_group_name_must_be_supplied() {

    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "group1");
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithWorkspacePropertiesSet(setup.inputWSName);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_group_names_with_alphanumeric_characters_or_underscores_are_allowed() {
    const std::vector<int> group = {1};
    auto ws = createMultiPeriodWorkspaceGroup(1, 1, 10, "group1");

    const std::vector<std::string> validNames = {"fwd", "fwd2", "bwd_2"};
    for (auto &&validName : validNames) {
      auto alg = setUpAlgorithmWithoutOptionalProperties(ws, validName, group);
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void
  test_that_exec_throws_if_group_name_is_not_alphanumeric_or_underscored() {
    const std::vector<int> group = {1};
    auto ws = createMultiPeriodWorkspaceGroup(1, 1, 10, "group1");

    const std::vector<std::string> invalidNames = {"@", "fwd!", "#1", "fwd @",
                                                   "   "};
    for (auto &&invalidName : invalidNames) {
      auto alg =
          setUpAlgorithmWithoutOptionalProperties(ws, invalidName, group);
      TS_ASSERT_THROWS_ANYTHING(alg->execute());
    }
  }

  // --------------------------------------------------------------------------
  // Validation : Grouping
  // --------------------------------------------------------------------------

  void
  test_that_cannot_add_spectra_to_group_which_exceed_those_in_the_workspace() {
    auto ws = createMultiPeriodWorkspaceGroup(1, 5, 10, "asym");

    const std::vector<int> detectors = {6, 7, 8, 9, 10};
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "asym", detectors);

    alg->execute();
    TS_ASSERT(!alg->isExecuted());
  }

  // --------------------------------------------------------------------------
  // Validation : multi period data
  // --------------------------------------------------------------------------

  void test_that_at_least_one_period_must_be_specified() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    const std::vector<int> detectors = {1, 2};
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "group", detectors);

    const std::vector<int> summedPeriods = {};
    const std::vector<int> subtractedPeriods = {};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_supplying_too_many_periods_to_SummedPeriods_throws_on_execute() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    const std::vector<int> detectors = {1, 2, 3};
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "group", detectors);

    const std::vector<int> summedPeriods = {3};
    alg->setProperty("SummedPeriods", summedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_supplying_too_many_periods_to_SubtractedPeriods_throws_on_execute() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    const std::vector<int> detectors = {1, 2, 3};
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "group", detectors);

    const std::vector<int> subtractedPeriods = {3};
    alg->setProperty("SubtractedPeriods", subtractedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_algorithm_fails_if_summed_periods_has_negative_entry() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    const std::vector<int> detectors = {1, 2, 3};
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "group", detectors);

    const std::vector<int> summedPeriods = {-1};
    alg->setProperty("SummedPeriods", summedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_algorithm_fails_if_subtracted_periods_has_negative_entry() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    const std::vector<int> detectors = {1, 2, 3};
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "group", detectors);

    const std::vector<int> subtractedPeriods = {-1};
    alg->setProperty("SubtractedPeriods", subtractedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  // --------------------------------------------------------------------------
  // Correct output
  // --------------------------------------------------------------------------

  void
  test_grouping_with_single_detector_and_asymmetry_analysis_gives_correct_values() {

    auto ws = createMultiPeriodAsymmetryData(1, 3, 10, "group_asym");
    const std::vector<int> detectors = {1};
    auto alg =
        setUpAlgorithmWithoutOptionalProperties(ws, "group_asym", detectors);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 2.18243, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 1.68932, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0002906, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[1], 0.0003041, 0.00001);
  }

  void
  test_grouping_with_multiple_detectors_and_asymmetry_analysis_gives_correct_values() {

    auto ws = createMultiPeriodAsymmetryData(1, 2, 10, "group_asym");
    const std::vector<int> detectors = {1, 2};
    auto alg =
        setUpAlgorithmWithoutOptionalProperties(ws, "group_asym", detectors);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 2.2751, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 1.7005, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0001418, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[1], 0.0001418, 0.00001);
  }

  void
  test_grouping_asymmetry_with_subtracted_multiple_periods_gives_correct_values() {

    auto ws = createMultiPeriodAsymmetryData(3, 2, 10, "group_asym");
    std::vector<int> detectors = {1, 2};
    auto alg =
        setUpAlgorithmWithoutOptionalProperties(ws, "group_asym", detectors);

    const std::vector<int> summedPeriods = {1};
    const std::vector<int> subtractedPeriods = {2, 3};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], -0.29901, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 0.06680, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0001497, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[1], 0.0001567, 0.00001);
  }

  void
  test_grouping_asymmetry_with_summed_multiple_periods_gives_correct_values() {
    auto ws = createMultiPeriodAsymmetryData(3, 2, 10, "group_asym");
    std::vector<int> detectors = {1, 2};
    auto alg =
        setUpAlgorithmWithoutOptionalProperties(ws, "group_asym", detectors);

    const std::vector<int> summedPeriods = {3, 2};
    const std::vector<int> subtractedPeriods = {1};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 0.29901, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], -0.06680, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0001497, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[1], 0.0001567, 0.00001);
  }

  void
  test_grouping_asymmetry_with_specified_normalization_gives_correct_values() {
    auto ws = createMultiPeriodAsymmetryData(3, 2, 10, "group_asym");
    std::vector<int> detectors = {1, 2};
    auto alg =
        setUpAlgorithmWithoutOptionalProperties(ws, "group_asym", detectors);

    const std::vector<int> summedPeriods = {3, 2};
    const std::vector<int> subtractedPeriods = {1};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);
    alg->setProperty("NormalizationIn", 15.0);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 1.39055, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 0.92922, 0.001);

    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0000577, 0.00001);
    TS_ASSERT_DELTA(wsOut->readE(0)[1], 0.0000604, 0.00001);

    std::string norm = wsOut->getLog("analysis_asymmetry_norm")->value();

    TS_ASSERT_EQUALS(std::stod(norm), 15.0);
  }
};

#endif /* MANTID_MUON_MUONGROUPINGASYMMETRYTEST_H_ */
