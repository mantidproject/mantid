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
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationEfficienciesWildes.h"
#include "MantidDataObjects/TableWorkspace.h"

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::Algorithms::PolarizationEfficienciesWildes;
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::DataObjects::TableWorkspace;

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
// The default bin width used by the CreateSampleWorkspace algorithm
auto constexpr DEFAULT_BIN_WIDTH = 200;
} // unnamed namespace

class PolarizationEfficienciesWildesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationEfficienciesWildesTest *createSuite() { return new PolarizationEfficienciesWildesTest(); }
  static void destroySuite(PolarizationEfficienciesWildesTest *suite) { delete suite; }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  /// Validation tests - WorkspaceGroup size

  void test_invalid_non_mag_group_size_throws_error() {
    const auto group = createNonMagWSGroup("nonMagWs");
    group->removeItem(0);
    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_GRP_SIZE_ERROR);
  }

  void test_invalid_mag_group_size_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs");
    magGrp->removeItem(0);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_GRP_SIZE_ERROR);
  }

  /// Validation tests - WorkspaceGroup child workspace types

  void test_non_mag_group_child_ws_not_matrix_ws_throws_error() {
    const auto group = createNonMagWSGroup("nonMagWs");
    const auto tableWs = std::make_shared<TableWorkspace>();

    group->removeItem(0);
    group->addWorkspace(tableWs);

    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_GRP_CHILD_TYPE_ERROR);
  }

  void test_mag_group_child_ws_not_matrix_ws_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs");
    const auto tableWs = std::make_shared<TableWorkspace>();

    magGrp->removeItem(0);
    magGrp->addWorkspace(tableWs);

    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_GRP_CHILD_TYPE_ERROR);
  }

  /// Validation tests - workspace units

  void test_non_mag_group_child_ws_not_wavelength_throws_error() {
    const auto group = createNonMagWSGroup("nonMagWs", false);
    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_UNIT_ERROR);
  }

  void test_mag_group_child_ws_not_wavelength_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs", false);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_UNIT_ERROR);
  }

  void test_input_polarizer_efficiency_ws_not_wavelength_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto polarizerEffWs = createWS("polEff", 0.9, false);
    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::WS_UNIT_ERROR);
  }

  void test_input_analyser_efficiency_ws_not_wavelength_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto analyserEffWs = createWS("analyserEff", 0.9, false);
    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    assertValidationError(alg, InputPropNames::A_EFF_WS, PropErrors::WS_UNIT_ERROR);
  }

  /// Validation tests - workspace num spectra

  void test_non_mag_group_child_ws_not_single_spectrum_throws_error() {
    const auto group = createNonMagWSGroup("nonMagWs", true, false);
    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  void test_mag_group_child_ws_not_single_spectrum_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs", true, false);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  void test_input_polarizer_efficiency_ws_not_single_spectrum_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto polarizerEffWs = createWS("polEff", 0.9, true, false);
    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  void test_input_analyser_efficiency_ws_not_single_spectrum_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto analyserEffWs = createWS("analyserEff", 0.9, true, false);
    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    assertValidationError(alg, InputPropNames::A_EFF_WS, PropErrors::WS_SPECTRUM_ERROR);
  }

  /// Validation tests - workspace bin boundaries

  void test_non_mag_group_child_ws_bin_mismatch_throws_error() {
    const auto group = createNonMagWSGroup("nonMagWs", true, true, true);
    const auto alg = createEfficiencyAlg(group);
    assertValidationError(alg, InputPropNames::NON_MAG_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_mag_group_child_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs", true, true, true);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_non_mag_and_mag_group_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs", true, true, false, DEFAULT_BIN_WIDTH + 100);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    assertValidationError(alg, InputPropNames::MAG_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_input_polarizer_efficiency_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto polarizerEffWs = createWS("polEff", 0.9, true, true, 300);
    auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::WS_BINS_ERROR);
  }

  void test_input_analyser_efficiency_ws_bin_mismatch_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto analyserEffWs = createWS("analyserEff", 0.9, true, true, 300);
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

  /// Validation tests - valid property combinations

  void test_providing_both_mag_and_input_polarizer_efficiency_ws_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs");
    const auto polarizerEffWs = createWS("polEff", 0.9);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    assertValidationError(alg, InputPropNames::P_EFF_WS, PropErrors::INPUT_EFF_WS_ERROR);
  }

  void test_providing_both_mag_and_input_analyser_efficiency_ws_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs");
    const auto analyserEffWs = createWS("analyserEff", 0.9);
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    assertValidationError(alg, InputPropNames::A_EFF_WS, PropErrors::INPUT_EFF_WS_ERROR);
  }

  void test_requesting_p_eff_output_without_relevant_inputs_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setPropertyValue(OutputPropNames::P_EFF_WS, "pEff");
    assertValidationError(alg, OutputPropNames::P_EFF_WS, PropErrors::OUTPUT_P_EFF_ERROR);
  }

  void test_requesting_a_eff_output_without_relevant_inputs_throws_error() {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto alg = createEfficiencyAlg(nonMagGrp);
    alg->setPropertyValue(OutputPropNames::A_EFF_WS, "aEff");
    assertValidationError(alg, OutputPropNames::A_EFF_WS, PropErrors::OUTPUT_A_EFF_ERROR);
  }

  ///  Test calculations

  void test_all_calculations_are_correct_using_mag_ws() {
    runCalculationTest(nullptr, nullptr, 1.03556249, 0.93515155, 1.07112498, 0.87030310);
  }

  void test_all_calculations_are_correct_using_input_P_ws() {
    const double expectedPEfficiency = 0.98;
    const double expectedTPMO = (2 * expectedPEfficiency) - 1;
    const double expectedAEfficiency = (EXPECTED_PHI / (2 * expectedTPMO)) + 0.5;
    const double expectedTAMO = (2 * expectedAEfficiency) - 1;

    const auto polarizerEffWs = createWS("polEff", expectedPEfficiency);

    runCalculationTest(polarizerEffWs, nullptr, expectedPEfficiency, expectedAEfficiency, expectedTPMO, expectedTAMO);
  }

  void test_all_calculations_are_correct_using_input_A_ws() {
    const double expectedAEfficiency = 0.99;
    const double expectedTAMO = (2 * expectedAEfficiency) - 1;
    const double expectedPEfficiency = (EXPECTED_PHI / (2 * expectedTAMO)) + 0.5;
    const double expectedTPMO = (2 * expectedPEfficiency) - 1;

    const auto analyserEffWs = createWS("analyserEff", expectedAEfficiency);

    runCalculationTest(nullptr, analyserEffWs, expectedPEfficiency, expectedAEfficiency, expectedTPMO, expectedTAMO);
  }

  void test_all_calculations_are_correct_using_input_P_and_input_A_workspaces() {
    const double expectedPEfficiency = 0.98;
    const double expectedTPMO = (2 * expectedPEfficiency) - 1;
    const double expectedAEfficiency = 0.99;
    const double expectedTAMO = (2 * expectedAEfficiency) - 1;

    const auto polarizerEffWs = createWS("polEff", expectedPEfficiency);
    const auto analyserEffWs = createWS("analyserEff", expectedAEfficiency);

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

private:
  const std::vector<double> NON_MAG_Y_VALS = {12.0, 1.0, 2.0, 10.0};
  const std::vector<double> MAG_Y_VALS = {6.0, 0.2, 0.3, 1.0};
  const double EXPECTED_F_P = 0.86363636;
  const double EXPECTED_F_A = 0.95;
  const double EXPECTED_PHI = 0.93220339;
  const double EXPECTED_ALPHA = 0.9;
  const double EXPECTED_RHO = 0.72727273;

  WorkspaceGroup_sptr createNonMagWSGroup(const std::string &outName, const bool isWavelength = true,
                                          const bool isSingleSpectrum = true, const bool includeBinMismatch = false,
                                          const double binWidth = DEFAULT_BIN_WIDTH) {
    return createWSGroup(outName, NON_MAG_Y_VALS, isWavelength, isSingleSpectrum, includeBinMismatch, binWidth);
  }

  WorkspaceGroup_sptr createMagWSGroup(const std::string &outName, const bool isWavelength = true,
                                       const bool isSingleSpectrum = true, const bool includeBinMismatch = false,
                                       const double binWidth = DEFAULT_BIN_WIDTH) {
    return createWSGroup(outName, MAG_Y_VALS, isWavelength, isSingleSpectrum, includeBinMismatch, binWidth);
  }

  WorkspaceGroup_sptr createWSGroup(const std::string &outName, const std::vector<double> &yValues,
                                    const bool isWavelength = true, const bool isSingleSpectrum = true,
                                    const bool includeBinMismatch = false, const double binWidth = DEFAULT_BIN_WIDTH) {

    const std::vector<std::string> &wsNames{outName + "_00", outName + "_01", outName + "_10", outName + "_11"};
    const size_t lastWsIdx = wsNames.size() - 1;

    for (size_t i = 0; i < lastWsIdx; ++i) {
      const auto ws = createWS(wsNames[i], yValues[i], isWavelength, isSingleSpectrum, binWidth);
      AnalysisDataService::Instance().addOrReplace(wsNames[i], ws);
    }

    const auto finalWs = createWS(wsNames[lastWsIdx], yValues[lastWsIdx], isWavelength, isSingleSpectrum,
                                  includeBinMismatch ? binWidth + 100 : binWidth);
    AnalysisDataService::Instance().addOrReplace(wsNames[lastWsIdx], finalWs);

    GroupWorkspaces groupAlg;
    groupAlg.initialize();
    groupAlg.setChild(true);
    groupAlg.setProperty("InputWorkspaces", wsNames);
    groupAlg.setPropertyValue("OutputWorkspace", outName);
    groupAlg.execute();

    return groupAlg.getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr createWS(const std::string &outName, const double yValue = 1, const bool isWavelength = true,
                                const bool isSingleSpectrum = true, const double binWidth = DEFAULT_BIN_WIDTH) {
    CreateSampleWorkspace alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("XUnit", isWavelength ? "wavelength" : "TOF");
    alg.setProperty("NumBanks", isSingleSpectrum ? 1 : 2);
    alg.setProperty("BankPixelWidth", 1);
    alg.setProperty("BinWidth", binWidth);
    alg.setPropertyValue("Function", "User Defined");
    alg.setPropertyValue("UserDefinedFunction", "name=UserFunction, Formula=x*0+" + std::to_string(yValue));
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.execute();

    return alg.getProperty("OutputWorkspace");
  }

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
                            const double expectedYValue) {
    const MatrixWorkspace_sptr outWs = alg->getProperty(outputPropertyName);
    TS_ASSERT(outWs != nullptr);
    TS_ASSERT_EQUALS(expectedNumHistograms, outWs->getNumberHistograms())
    for (const double yVal : outWs->dataY(0)) {
      TS_ASSERT_DELTA(expectedYValue, yVal, 1e-8);
    }
  }

  void checkOutputWorkspaceIsSet(const std::unique_ptr<PolarizationEfficienciesWildes> &alg,
                                 const std::string &outputPropertyName, const bool isSet) {
    const MatrixWorkspace_sptr outWs = alg->getProperty(outputPropertyName);
    TS_ASSERT_EQUALS(isSet, outWs != nullptr);
  }

  void runTestOutputWorkspacesSetCorrectly(const bool includeP, const bool includeA, const bool includeDiagnostics) {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = createMagWSGroup("magWs");
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);
    alg->setProperty(InputPropNames::INCLUDE_DIAGNOSTICS, includeDiagnostics);
    if (includeP) {
      alg->setPropertyValue(OutputPropNames::P_EFF_WS, "pEff");
    }

    if (includeA) {
      alg->setPropertyValue(OutputPropNames::A_EFF_WS, "aEff");
    }
    alg->execute();

    checkOutputWorkspaceIsSet(alg, OutputPropNames::F_P_EFF_WS, true);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::F_A_EFF_WS, true);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::P_EFF_WS, includeP);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::A_EFF_WS, includeA);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::PHI_WS, includeDiagnostics);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::ALPHA_WS, includeDiagnostics);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::RHO_WS, includeDiagnostics);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::TPMO_WS, includeDiagnostics && includeP);
    checkOutputWorkspaceIsSet(alg, OutputPropNames::TAMO_WS, includeDiagnostics && includeA);
  }

  void runTestOutputWorkspacesSetCorrectlyWithInputEfficiencies(const bool includeInputP, const bool includeInputA,
                                                                const bool includeOutputP, const bool includeOutputA) {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto alg = createEfficiencyAlg(nonMagGrp);

    if (includeInputP) {
      const auto polEffWs = createWS("pEff");
      alg->setProperty(InputPropNames::P_EFF_WS, polEffWs);
    }

    if (includeInputA) {
      const auto analyserEffWs = createWS("aEff");
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
                          const double expectedP, const double expectedA, const double expectedTPMO,
                          const double expectedTAMO) {
    const bool hasPEffWs = polarizerEffWs != nullptr;
    const bool hasanalyserEffWs = analyserEffWs != nullptr;

    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto magGrp = (hasPEffWs || hasanalyserEffWs) ? nullptr : createMagWSGroup("magWs");
    const auto alg = createEfficiencyAlg(nonMagGrp, magGrp);

    if (hasPEffWs) {
      alg->setProperty(InputPropNames::P_EFF_WS, polarizerEffWs);
    }
    if (hasanalyserEffWs) {
      alg->setProperty(InputPropNames::A_EFF_WS, analyserEffWs);
    }

    alg->setProperty(InputPropNames::INCLUDE_DIAGNOSTICS, true);
    alg->setPropertyValue(OutputPropNames::P_EFF_WS, "pEff");
    alg->setPropertyValue(OutputPropNames::A_EFF_WS, "aEff");
    alg->execute();

    const size_t expectedNumHistograms =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(nonMagGrp->getItem(0))->getNumberHistograms();

    checkOutputWorkspace(alg, OutputPropNames::F_P_EFF_WS, expectedNumHistograms, EXPECTED_F_P);
    checkOutputWorkspace(alg, OutputPropNames::F_A_EFF_WS, expectedNumHistograms, EXPECTED_F_A);
    checkOutputWorkspace(alg, OutputPropNames::P_EFF_WS, expectedNumHistograms, expectedP);
    checkOutputWorkspace(alg, OutputPropNames::A_EFF_WS, expectedNumHistograms, expectedA);
    checkOutputWorkspace(alg, OutputPropNames::PHI_WS, expectedNumHistograms, EXPECTED_PHI);
    checkOutputWorkspace(alg, OutputPropNames::ALPHA_WS, expectedNumHistograms, EXPECTED_ALPHA);
    checkOutputWorkspace(alg, OutputPropNames::RHO_WS, expectedNumHistograms, EXPECTED_RHO);
    checkOutputWorkspace(alg, OutputPropNames::TPMO_WS, expectedNumHistograms, expectedTPMO);
    checkOutputWorkspace(alg, OutputPropNames::TAMO_WS, expectedNumHistograms, expectedTAMO);
  }

  void runTestInputEfficiencyWorkspaceNotOverwrittenWhenSetAsOutput(const std::string &inputPropName,
                                                                    const std::string &outputPropName) {
    const auto nonMagGrp = createNonMagWSGroup("nonMagWs");
    const auto alg = createEfficiencyAlg(nonMagGrp);
    const auto inEffWs = createWS("inEff");
    alg->setProperty(inputPropName, inEffWs);
    alg->setPropertyValue(outputPropName, "outEff");
    alg->execute();

    const MatrixWorkspace_sptr outEffWs = alg->getProperty(outputPropName);
    TS_ASSERT(outEffWs != inEffWs);
  }
};
