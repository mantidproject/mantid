// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationEfficienciesWildes.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "PolarizationCorrectionsTestUtils.h"

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::Algorithms::PolarizationEfficienciesWildes;
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::DataObjects::TableWorkspace;
using namespace PolCorrTestUtils;

namespace PropErrors {
auto constexpr PREFIX{"Some invalid Properties found: \n "};
auto constexpr WS_GRP_SIZE_ERROR{"The input group must contain a workspace for all four flipper configurations."};
auto constexpr WS_GRP_CHILD_TYPE_ERROR{"All input workspaces must be matrix workspaces."};
auto constexpr WS_UNIT_ERROR{"All input workspaces must be in units of Wavelength."};
auto constexpr WS_SPECTRUM_ERROR{"All input workspaces must contain only a single spectrum."};
auto constexpr WS_BINS_ERROR{"All input workspaces must have the same X values."};
auto constexpr INPUT_EFF_WS_ERROR{
    "If a magnetic workspace group has been provided then input efficiency workspaces should not be provided."};
auto constexpr OUTPUT_P_EFF_ERROR{"If output polarizer efficiency is requested then either the magnetic workspace or "
                                  "the known analyser efficiency should be provided."};
auto constexpr OUTPUT_A_EFF_ERROR{"If output analyser efficiency is requested then either the magnetic workspace or "
                                  "the known polarizer efficiency should be provided."};

std::string createPropertyErrorMessage(const std::string &propertyName, const std::string &errorMsg) {
  return PropErrors::PREFIX + propertyName + ": " + errorMsg;
}
} // namespace PropErrors

namespace InputPropNames {
auto constexpr NON_MAG_WS{"InputNonMagWorkspace"};
auto constexpr MAG_WS{"InputMagWorkspace"};
auto constexpr P_EFF_WS{"InputPolarizerEfficiency"};
auto constexpr A_EFF_WS{"InputAnalyserEfficiency"};
auto constexpr INCLUDE_DIAGNOSTICS{"IncludeDiagnosticOutputs"};
} // namespace InputPropNames

namespace OutputPropNames {
auto constexpr F_P_EFF_WS{"OutputFpEfficiency"};
auto constexpr F_A_EFF_WS{"OutputFaEfficiency"};
auto constexpr P_EFF_WS{"OutputPolarizerEfficiency"};
auto constexpr A_EFF_WS{"OutputAnalyserEfficiency"};
auto constexpr PHI_WS{"OutputPhi"};
auto constexpr RHO_WS{"OutputRho"};
auto constexpr ALPHA_WS{"OutputAlpha"};
auto constexpr TPMO_WS{"OutputTwoPMinusOne"};
auto constexpr TAMO_WS{"OutputTwoAMinusOne"};
} // namespace OutputPropNames

namespace {
static std::string const EFF_FUNC_STR = fillFuncStr({0.9});
// as of C++20 we can have constexpr std::string
std::string constexpr NON_MAG_WS{"nonMagWs"};
std::string constexpr MAG_WS{"magWs"};
std::string constexpr EFF_WS{"polEff"};
std::string constexpr HE_WS{"analyserEff"};
} // unnamed namespace

class PolarizationEfficienciesWildesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationEfficienciesWildesTest *createSuite() { return new PolarizationEfficienciesWildesTest(); }
  static void destroySuite(PolarizationEfficienciesWildesTest *suite) { delete suite; }

  void setUp() override { m_parameters = TestWorkspaceParameters(); }
  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  /// Validation tests - WorkspaceGroup size

  void test_invalid_non_mag_group_size_throws_error() {
    const auto group = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    AnalysisDataService::Instance().remove(NON_MAG_WS + "_11"); // Remove first member of group
    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_GRP_SIZE_ERROR);
  }

  void test_invalid_mag_group_size_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    AnalysisDataService::Instance().remove(MAG_WS + "_11"); // Remove first member of group
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_GRP_SIZE_ERROR);
  }

  /// Validation tests - WorkspaceGroup child workspace types

  void test_non_mag_group_child_ws_not_matrix_ws_throws_error() {
    const auto group = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    AnalysisDataService::Instance().add("table", std::make_shared<TableWorkspace>());
    AnalysisDataService::Instance().remove(NON_MAG_WS + "_11"); // Remove first member of group
    AnalysisDataService::Instance().addToGroup(NON_MAG_WS, "table");

    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_GRP_CHILD_TYPE_ERROR);
  }

  void test_mag_group_child_ws_not_matrix_ws_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    AnalysisDataService::Instance().add("table", std::make_shared<TableWorkspace>());
    AnalysisDataService::Instance().remove(MAG_WS + "_11"); // Remove first member of group
    AnalysisDataService::Instance().addToGroup(MAG_WS, "table");

    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_GRP_CHILD_TYPE_ERROR);
  }

  /// Validation tests - workspace units

  void test_non_mag_group_child_ws_not_wavelength_throws_error() {
    m_parameters.xUnit = "TOF";
    const auto group = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_UNIT_ERROR);
  }

  void test_mag_group_child_ws_not_wavelength_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    m_parameters.xUnit = "TOF";
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);

    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_UNIT_ERROR);
  }

  void test_input_polarizer_efficiency_ws_not_wavelength_throws_error() {
    const auto group = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto polarizerEffWs = generateFunctionDefinedWorkspace(TestWorkspaceParameters(EFF_WS, EFF_FUNC_STR, "TOF"));
    auto alg = createEfficiencyAlg(group);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::WS_UNIT_ERROR);
  }

  void test_input_analyser_efficiency_ws_not_wavelength_throws_error() {
    const auto group = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto analyserEffWs = generateFunctionDefinedWorkspace(TestWorkspaceParameters(HE_WS, EFF_FUNC_STR, "TOF"));
    auto alg = createEfficiencyAlg(group);
    alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    assertValidationError(alg, InputPropNames::A_EFF_WS, PropErrors::WS_UNIT_ERROR);
  }

  /// Validation tests - workspace num spectra

  void test_non_mag_group_child_ws_not_single_spectrum_throws_error() {
    m_parameters.numBanks = 2;
    const auto group = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  void test_mag_group_child_ws_not_single_spectrum_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    m_parameters.numBanks = 2;
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  void test_input_polarizer_efficiency_ws_not_single_spectrum_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto polarizerEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(EFF_WS, EFF_FUNC_STR, "Wavelength", 2));
    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  void test_input_analyser_efficiency_ws_not_single_spectrum_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto analyserEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(HE_WS, EFF_FUNC_STR, "Wavelength", 2));

    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    assertValidationError(alg, InputPropNames::A_EFF_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  /// Validation tests - workspace bin boundaries

  void test_non_mag_group_child_ws_bin_mismatch_throws_error() {
    const auto group = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto mismatchedWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(INPUT_NAME, EFF_FUNC_STR, "Wavelength", 1, 1, 8, 0.1));
    AnalysisDataService::Instance().addOrReplace(NON_MAG_WS + "_11", mismatchedWs);

    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_mag_group_child_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto mismatchedWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(INPUT_NAME, EFF_FUNC_STR, "Wavelength", 1, 1, 8, 0.1));
    AnalysisDataService::Instance().addOrReplace(MAG_WS + "_11", mismatchedWs);

    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_non_mag_and_mag_group_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    m_parameters.binWidth = 0.1;
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);

    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_input_polarizer_efficiency_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto polarizerEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(EFF_WS, EFF_FUNC_STR, "Wavelength", 1, 1, 8, 0.1));

    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_input_analyser_efficiency_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto analyserEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(HE_WS, EFF_FUNC_STR, "Wavelength", 1, 1, 8, 0.1));

    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    assertValidationError(alg, InputPropNames::A_EFF_WS, PropErrors::WS_BINS_ERROR);
  }

  /// Validation tests - input property types

  void test_input_non_mag_not_ws_group_throws_error() {
    const auto invalidWsType = TableWorkspace();
    assertSetPropertyThrowsInvalidArgumentError<TableWorkspace>(InputPropNames::NON_MAG_WS, invalidWsType);
  }

  void test_input_mag_not_ws_group_throws_error() {
    const auto invalidWsType = TableWorkspace();
    assertSetPropertyThrowsInvalidArgumentError<TableWorkspace>(InputPropNames::MAG_WS, invalidWsType);
  }

  void test_input_polarizer_efficiency_not_matrix_ws_throws_error() {
    const auto invalidWsType = TableWorkspace();
    assertSetPropertyThrowsInvalidArgumentError<TableWorkspace>(InputPropNames::P_EFF_WS, invalidWsType);
  }

  void test_input_analyser_efficiency_not_matrix_ws_throws_error() {
    const auto invalidWsType = TableWorkspace();
    assertSetPropertyThrowsInvalidArgumentError<TableWorkspace>(InputPropNames::A_EFF_WS, invalidWsType);
  }

  void test_input_non_mag_not_matrix_ws_group_throws_error() {
    const auto group = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; i++) {
      group->addWorkspace(std::make_shared<TableWorkspace>());
    }
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(group, magGrp);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_GRP_CHILD_TYPE_ERROR);
  }

  void test_input_mag_not_matrix_ws_group_throws_error() {
    const auto magGrp = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; i++) {
      magGrp->addWorkspace(std::make_shared<TableWorkspace>());
    }
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_GRP_CHILD_TYPE_ERROR);
  }

  /// Validation tests - valid property combinations

  void test_providing_both_mag_and_input_polarizer_efficiency_ws_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto polarizerEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(EFF_WS, EFF_FUNC_STR, "Wavelength", 1, 1, 8, 0.1));
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::INPUT_EFF_WS_ERROR);
  }

  void test_providing_both_mag_and_input_analyser_efficiency_ws_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto analyserEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(HE_WS, EFF_FUNC_STR, "Wavelength", 1, 1, 8, 0.1));
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    assertValidationError(alg, InputPropNames::A_EFF_WS, PropErrors::INPUT_EFF_WS_ERROR);
  }

  void test_requesting_p_eff_output_without_relevant_inputs_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setPropertyValue(OutputPropNames::P_EFF_WS, "pEff");
    assertValidationError(alg, OutputPropNames::P_EFF_WS, PropErrors::OUTPUT_P_EFF_ERROR);
  }

  void test_requesting_a_eff_output_without_relevant_inputs_throws_error() {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setPropertyValue(OutputPropNames::A_EFF_WS, "aEff");
    assertValidationError(alg, OutputPropNames::A_EFF_WS, PropErrors::OUTPUT_A_EFF_ERROR);
  }

  ///  Test calculations

  void test_all_calculations_are_correct_using_mag_ws() {
    runCalculationTest(nullptr, nullptr, {1.03556249, 0.1951872164}, {0.93515155, 0.1727967724},
                       {1.07112498, 0.3903744329}, {0.87030310, 0.3455935448});
  }

  void test_all_calculations_are_correct_using_input_P_ws() {
    const std::pair expectedPEfficiency = {0.98, 0.9899494934};
    const std::pair expectedTPMO = {0.96, 1.9798989879};
    const std::pair expectedAEfficiency = {0.9855226, 1.0315912829};
    const std::pair expectedTAMO = {0.9710452, 2.0631825648};

    const auto polarizerEffWs = generateFunctionDefinedWorkspace(TestWorkspaceParameters(
        EFF_WS, "name=UserFunction, Formula=x*0 +" + std::to_string(expectedPEfficiency.first)));

    polarizerEffWs->setDistribution(true);

    runCalculationTest(polarizerEffWs, nullptr, expectedPEfficiency, expectedAEfficiency, expectedTPMO, expectedTAMO);
  }

  void test_all_calculations_are_correct_using_input_A_ws() {
    const std::pair expectedPEfficiency = {0.99, 1.0479338884};
    const std::pair expectedTPMO = {0.98, 2.0958677778};
    const std::pair expectedAEfficiency = {0.975614, 0.9877317447};
    const std::pair expectedTAMO = {0.9512279, 1.9754634895};

    const auto analyserEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(HE_WS, fillFuncStr({expectedAEfficiency.first})));
    analyserEffWs->setDistribution(true);

    runCalculationTest(nullptr, analyserEffWs, expectedPEfficiency, expectedAEfficiency, expectedTPMO, expectedTAMO);
  }

  void test_all_calculations_are_correct_using_input_P_and_input_A_workspaces() {
    const std::pair expectedPEfficiency = {0.98, 0.9899494934};
    const std::pair expectedTPMO = {0.96, 1.9798989879};
    const std::pair expectedAEfficiency = {0.99, 0.9949874379};
    const std::pair expectedTAMO = {0.98, 1.9899748748};

    const auto polarizerEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(EFF_WS, fillFuncStr({expectedPEfficiency.first})));
    const auto analyserEffWs =
        generateFunctionDefinedWorkspace(TestWorkspaceParameters(HE_WS, fillFuncStr({expectedAEfficiency.first})));
    analyserEffWs->setDistribution(true);

    runCalculationTest(polarizerEffWs, analyserEffWs, expectedPEfficiency, expectedAEfficiency, expectedTPMO,
                       expectedTAMO);
  }

  ///  Test setting of outputs when using mag workspace group
  ///   (the case where both the P and A efficiency output workspaces are set with diagnostic outputs are covered by the
  ///   calculation tests)

  void test_correct_outputs_when_P_and_A_requested_with_no_diagnostics() {
    runTestOutputWorkspacesSetCorrectly(true, true, false);
  }

  void test_correct_outputs_when_P_and_A_not_requested_with_no_diagnostics() {
    runTestOutputWorkspacesSetCorrectly(false, false, false);
  }

  void test_correct_outputs_when_only_A_requested_with_no_diagnostics() {
    runTestOutputWorkspacesSetCorrectly(false, true, false);
  }

  void test_correct_outputs_when_only_P_requested_with_no_diagnostics() {
    runTestOutputWorkspacesSetCorrectly(true, false, false);
  }

  void test_correct_outputs_when_P_and_A_not_requested_with_diagnostics() {
    runTestOutputWorkspacesSetCorrectly(false, false, true);
  }

  void test_correct_outputs_when_only_A_requested_with_diagnostics() {
    runTestOutputWorkspacesSetCorrectly(false, true, true);
  }

  void test_correct_outputs_when_only_P_requested_with_diagnostics() {
    runTestOutputWorkspacesSetCorrectly(true, false, true);
  }

  ///  Test setting of outputs when using input efficiency workspaces
  ///   (cases where both the P and A efficiency output workspaces are set are covered by the calculation tests)

  void test_only_P_output_set_when_requested_with_only_input_A_ws() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(false, true, true, false);
  }

  void test_only_P_output_set_when_requested_with_only_input_P_ws() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(true, false, true, false);
  }

  void test_only_P_output_set_when_requested_with_both_input_efficiency_workspaces() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(true, true, true, false);
  }

  void test_only_A_output_set_when_requested_with_only_input_P_ws() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(true, false, false, true);
  }

  void test_only_A_output_set_when_requested_with_only_input_A_ws() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(false, true, false, true);
  }

  void test_only_A_output_set_when_requested_with_both_input_efficiency_workspaces() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(true, true, false, true);
  }

  void test_no_outputs_requested_with_input_P_ws() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(true, false, false, false);
  }

  void test_no_outputs_requested_with_input_A_ws() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(false, true, false, false);
  }

  void test_no_outputs_requested_with_both_input_efficiency_workspaces() {
    runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(true, true, false, false);
  }

  void test_input_P_ws_not_overwritten_when_set_as_an_output() {
    runTestInputEfficiencyWorkspaceNotOverwrittenWhenSetAsOutput(InputPropNames::P_EFF_WS, OutputPropNames::P_EFF_WS);
  }

  void test_input_A_ws_not_overwritten_when_set_as_an_output() {
    runTestInputEfficiencyWorkspaceNotOverwrittenWhenSetAsOutput(InputPropNames::A_EFF_WS, OutputPropNames::A_EFF_WS);
  }

  void test_algorithm_clears_optional_outputs_on_second_run_with_same_include_diagnostics() {
    runTestOutputWorkspacesSetCorrectlyForMultipleRuns(true);
  }

  void test_algorithm_clears_optional_outputs_on_second_run_with_different_include_diagnostics() {
    runTestOutputWorkspacesSetCorrectlyForMultipleRuns(false);
  }

private:
  TestWorkspaceParameters m_parameters;
  const std::vector<double> NON_MAG_Y_VALS = {12.0, 1.0, 2.0, 10.0};
  const std::vector<double> MAG_Y_VALS = {6.0, 0.2, 0.3, 1.0};
  const std::pair<double, double> EXPECTED_F_P{0.86363636, 0.19748435};
  const std::pair<double, double> EXPECTED_F_A = {0.95, 0.2363260459};
  const std::pair<double, double> EXPECTED_PHI = {0.93220339, 0.4761454221};
  const std::pair<double, double> EXPECTED_ALPHA = {0.9, 0.4726520913};
  const std::pair<double, double> EXPECTED_RHO = {0.72727273, 0.3949686990};

  std::unique_ptr<PolarizationEfficienciesWildes> createEfficiencyAlg(const WorkspaceGroup_sptr &nonMagWSGroup,
                                                                      const WorkspaceGroup_sptr &magWsGroup = nullptr) {
    auto alg = std::make_unique<PolarizationEfficienciesWildes>();
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->setProperty(InputPropNames::NON_MAG_WS, nonMagWSGroup);
    if (magWsGroup != nullptr) {
      alg->setProperty(InputPropNames::MAG_WS, magWsGroup);
    }
    alg->setProperty("Flippers", "00,01,10,11");
    alg->setPropertyValue(OutputPropNames::F_P_EFF_WS, "outFp");
    alg->setPropertyValue(OutputPropNames::F_A_EFF_WS, "outFa");
    return alg;
  }

  void assertValidationError(const std::unique_ptr<PolarizationEfficienciesWildes> &alg,
                             const std::string &propertyName, const std::string &errorMsg) {
    const auto expectedError = PropErrors::createPropertyErrorMessage(propertyName, errorMsg);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e, std::string(e.what()), expectedError);
  }

  template <typename T>
  void assertSetPropertyThrowsInvalidArgumentError(const std::string &propertyName, const T &propertyValue) {
    auto alg = std::make_unique<PolarizationEfficienciesWildes>();
    alg->initialize();
    TS_ASSERT_THROWS(alg->setProperty(propertyName, propertyValue), const std::invalid_argument &);
  }

  void checkOutputWorkspace(const std::unique_ptr<PolarizationEfficienciesWildes> &alg,
                            const std::string &outputPropertyName, const size_t expectedNumHistograms,
                            const std::pair<double, double> &expectedValue) {
    const MatrixWorkspace_sptr outWs = alg->getProperty(outputPropertyName);

    TS_ASSERT_EQUALS(true, outWs->isDistribution());
    TS_ASSERT_EQUALS("Counts", outWs->YUnit());

    TS_ASSERT(outWs != nullptr);
    TS_ASSERT_EQUALS(expectedNumHistograms, outWs->getNumberHistograms())
    for (size_t i = 0; i < outWs->blocksize(); i++) {
      const double yVal = outWs->dataY(0)[i];
      const double eVal = outWs->dataE(0)[i];
      TS_ASSERT_DELTA(expectedValue.first, yVal, 1e-6);
      TS_ASSERT_DELTA(expectedValue.second, eVal, 1e-6);
    }
  }

  void checkOutputWorkspaceIsSet(const std::unique_ptr<PolarizationEfficienciesWildes> &alg,
                                 const std::string &outputPropertyName, const bool isSet) {
    const MatrixWorkspace_sptr outWs = alg->getProperty(outputPropertyName);
    TS_ASSERT_EQUALS(isSet, outWs != nullptr);
  }

  void runTestOutputWorkspacesSetCorrectly(const bool includeP, const bool includeA, const bool includeDiagnostics) {
    using namespace OutputPropNames;
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    alg->setProperty(InputPropNames::INCLUDE_DIAGNOSTICS, includeDiagnostics);
    if (includeP) {
      alg->setPropertyValue(OutputPropNames::P_EFF_WS, "pEff");
    }

    if (includeA) {
      alg->setPropertyValue(OutputPropNames::A_EFF_WS, "aEff");
    }
    alg->execute();
    const std::vector<std::string> outputProps = {F_P_EFF_WS, F_A_EFF_WS, P_EFF_WS, A_EFF_WS, PHI_WS,
                                                  ALPHA_WS,   RHO_WS,     TPMO_WS,  TAMO_WS};
    const std::vector isSetValues = {true,
                                     true,
                                     includeP,
                                     includeA,
                                     includeDiagnostics,
                                     includeDiagnostics,
                                     includeDiagnostics,
                                     includeDiagnostics && includeP,
                                     includeDiagnostics && includeA};
    for (size_t index = 0; index < outputProps.size(); ++index) {
      checkOutputWorkspaceIsSet(alg, outputProps.at(index), isSetValues.at(index));
    }
  }

  void runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(const bool includeInputP, const bool includeInputA,
                                                                const bool includeOutputP, const bool includeOutputA) {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp);

    if (includeInputP) {
      const auto polEffWs =
          generateFunctionDefinedWorkspace(TestWorkspaceParameters(EFF_WS, "name=UserFunction, Formula=x*0 + 1"));
      alg->setProperty(InputPropNames::P_EFF_WS, polEffWs);
    }

    if (includeInputA) {
      const auto analyserEffWs =
          generateFunctionDefinedWorkspace(TestWorkspaceParameters(HE_WS, "name=UserFunction, Formula=x*0 + 1"));
      alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    }

    if (includeOutputP) {
      alg->setPropertyValue(OutputPropNames::P_EFF_WS, "pEff");
    }

    if (includeOutputA) {
      alg->setPropertyValue(OutputPropNames::A_EFF_WS, "aEff");
    }
    alg->execute();

    checkOutputWorkspaceIsSet(alg, OutputPropNames::P_EFF_WS, includeOutputP);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::A_EFF_WS, includeOutputA);
  }

  void runCalculationTest(const MatrixWorkspace_sptr &polarizerEffWs, const MatrixWorkspace_sptr &analyserEffWs,
                          const std::pair<double, double> &expectedP, const std::pair<double, double> &expectedA,
                          const std::pair<double, double> &expectedTPMO,
                          const std::pair<double, double> &expectedTAMO) {
    using namespace OutputPropNames;
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp =
        (polarizerEffWs || analyserEffWs) ? nullptr : createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);

    if (polarizerEffWs) {
      alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    }
    if (analyserEffWs) {
      alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    }

    alg->setProperty(InputPropNames::INCLUDE_DIAGNOSTICS, true);
    alg->setPropertyValue(P_EFF_WS, "pEff");
    alg->setPropertyValue(A_EFF_WS, "aEff");
    alg->execute();

    const size_t expectedNumHistograms =
        std::dynamic_pointer_cast<MatrixWorkspace>(nonMagGrp->getItem(0))->getNumberHistograms();

    const std::vector<std::string> outputProps = {F_P_EFF_WS, F_A_EFF_WS, P_EFF_WS, A_EFF_WS, PHI_WS,
                                                  ALPHA_WS,   RHO_WS,     TPMO_WS,  TAMO_WS};
    const std::vector expectedValues = {EXPECTED_F_P,   EXPECTED_F_A, expectedP,    expectedA,   EXPECTED_PHI,
                                        EXPECTED_ALPHA, EXPECTED_RHO, expectedTPMO, expectedTAMO};
    for (size_t index = 0; index < outputProps.size(); ++index) {
      checkOutputWorkspace(alg, outputProps.at(index), expectedNumHistograms, expectedValues.at(index));
    }
  }

  void runTestInputEfficiencyWorkspaceNotOverwrittenWhenSetAsOutput(const std::string &inputPropName,
                                                                    const std::string &outputPropName) {
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp);
    const auto inEffWs = generateFunctionDefinedWorkspace(TestWorkspaceParameters("inEff", fillFuncStr({1.0})));
    alg->setProperty(inputPropName, inEffWs);
    alg->setPropertyValue(outputPropName, "outEff");
    alg->execute();

    const MatrixWorkspace_sptr outEffWs = alg->getProperty(outputPropName);
    TS_ASSERT(outEffWs != inEffWs);
  }

  void runTestOutputWorkspacesSetCorrectlyForMultipleRuns(const bool secondRunIncludeDiagnostics) {
    // We need to make sure we don't get outputs from previous runs if the same instance of the algorithm is run twice,
    // or is being run as a child algorithm.
    using namespace OutputPropNames;
    const auto nonMagGrp = createPolarizedTestGroup(NON_MAG_WS, m_parameters, NON_MAG_Y_VALS);
    const auto magGrp = createPolarizedTestGroup(MAG_WS, m_parameters, MAG_Y_VALS);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);

    alg->setPropertyValue(P_EFF_WS, "pEff");
    alg->setPropertyValue(A_EFF_WS, "aEff");
    alg->setProperty(InputPropNames::INCLUDE_DIAGNOSTICS, true);
    alg->execute();
    const std::vector<std::string> outputProps = {P_EFF_WS, A_EFF_WS, PHI_WS, ALPHA_WS, RHO_WS, TPMO_WS, TAMO_WS};
    auto isSetValues = std::vector(outputProps.size(), true);
    for (size_t index = 0; index < outputProps.size(); ++index) {
      checkOutputWorkspaceIsSet(alg, outputProps.at(index), isSetValues.at(index));
    }

    alg->setPropertyValue(P_EFF_WS, "");
    alg->setPropertyValue(A_EFF_WS, "");
    alg->setProperty(InputPropNames::INCLUDE_DIAGNOSTICS, secondRunIncludeDiagnostics);
    alg->execute();
    isSetValues.assign({false, false, secondRunIncludeDiagnostics, secondRunIncludeDiagnostics,
                        secondRunIncludeDiagnostics, false, false});
    for (size_t index = 0; index < outputProps.size(); ++index) {
      checkOutputWorkspaceIsSet(alg, outputProps.at(index), isSetValues.at(index));
    }
  }
};
