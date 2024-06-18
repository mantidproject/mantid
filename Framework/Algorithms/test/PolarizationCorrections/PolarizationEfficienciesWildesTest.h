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

std::string createPropertyErrorMessage(const std::string &propertyName, const std::string &errorMsg) {
  return PropErrors::PREFIX + propertyName + ": " + errorMsg;
}
} // namespace PropErrors

namespace InputPropNames {
auto constexpr NON_MAG_WS{"InputNonMagWorkspace"};
auto constexpr MAG_WS{"InputMagWorkspace"};
auto constexpr P_EFF_WS{"InputPolarizerEfficiency"};
auto constexpr A_EFF_WS{"InputAnalyserEfficiency"};
} // namespace InputPropNames

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

private:
  const std::vector<double> NON_MAG_Y_VALS = {12.0, 1.0, 1.0, 12.0};
  const std::vector<double> MAG_Y_VALS = {6.0, 0.2, 0.2, 1.0};

  WorkspaceGroup_sptr createNonMagWSGroup(const std::string &outName, const bool isWavelength = true,
                                          const bool isSingleSpectrum = true) {
    return createWSGroup(outName, NON_MAG_Y_VALS, isWavelength, isSingleSpectrum);
  }

  WorkspaceGroup_sptr createMagWSGroup(const std::string &outName, const bool isWavelength = true,
                                       const bool isSingleSpectrum = true) {
    return createWSGroup(outName, MAG_Y_VALS, isWavelength, isSingleSpectrum);
  }

  WorkspaceGroup_sptr createWSGroup(const std::string &outName, const std::vector<double> &yValues,
                                    const bool isWavelength = true, const bool isSingleSpectrum = true) {

    const std::vector<std::string> &wsNames{outName + "_00", outName + "_01", outName + "_10", outName + "_11"};

    for (size_t i = 0; i < 4; ++i) {
      const auto ws = createWS(wsNames[i], yValues[i], isWavelength, isSingleSpectrum);
      AnalysisDataService::Instance().addOrReplace(wsNames[i], ws);
    }

    GroupWorkspaces groupAlg;
    groupAlg.initialize();
    groupAlg.setChild(true);
    groupAlg.setProperty("InputWorkspaces", wsNames);
    groupAlg.setPropertyValue("OutputWorkspace", outName);
    groupAlg.execute();

    return groupAlg.getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr createWS(const std::string &outName, const double yValue, const bool isWavelength = true,
                                const bool isSingleSpectrum = true) {
    CreateSampleWorkspace alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("XUnit", isWavelength ? "wavelength" : "TOF");
    alg.setProperty("NumBanks", isSingleSpectrum ? 1 : 2);
    alg.setProperty("BankPixelWidth", 1);
    alg.setPropertyValue("Function", "User Defined");
    alg.setPropertyValue("UserDefinedFunction", "name=UserFunction, Formula=x*0+" + std::to_string(yValue));
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.execute();

    return alg.getProperty("OutputWorkspace");
  }

  std::unique_ptr<PolarizationEfficienciesWildes>
  createEfficiencyAlg(const Mantid::API::WorkspaceGroup_sptr &nonMagWSGroup,
                      const Mantid::API::WorkspaceGroup_sptr &magWsGroup = nullptr) {
    auto alg = std::make_unique<PolarizationEfficienciesWildes>();
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->setProperty(InputPropNames::NON_MAG_WS, nonMagWSGroup);
    if (magWsGroup != nullptr) {
      alg->setProperty(InputPropNames::MAG_WS, magWsGroup);
    }
    alg->setProperty("Flippers", "00,01,10,11");
    alg->setPropertyValue("OutputFpEfficiency", "outFp");
    alg->setPropertyValue("OutputFaEfficiency", "outFa");
    return alg;
  }

  void assertRuntimeError(const std::unique_ptr<PolarizationEfficienciesWildes> &alg,
                          const std::string &expectedError) {
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e, std::string(e.what()), expectedError);
  }

  void assertValidationError(const std::unique_ptr<PolarizationEfficienciesWildes> &alg,
                             const std::string &propertyName, const std::string &errorMsg) {
    const auto expectedError = PropErrors::createPropertyErrorMessage(propertyName, errorMsg);
    assertRuntimeError(alg, expectedError);
  }

  template <typename T>
  void assertSetPropertyThrowsInvalidArgumentError(const std::string &propertyName, const T &propertyValue) {
    auto alg = std::make_unique<PolarizationEfficienciesWildes>();
    alg->initialize();
    TS_ASSERT_THROWS(alg->setProperty(propertyName, propertyValue), const std::invalid_argument &);
  }
};
