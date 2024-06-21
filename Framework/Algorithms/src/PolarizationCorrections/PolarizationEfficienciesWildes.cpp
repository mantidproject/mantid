// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizationEfficienciesWildes.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Unit.h"

namespace {
/// Property Names
namespace PropNames {
auto constexpr INPUT_NON_MAG_WS{"InputNonMagWorkspace"};
auto constexpr INPUT_MAG_WS{"InputMagWorkspace"};
auto constexpr FLIPPERS{"Flippers"};
auto constexpr INPUT_P_EFF_WS{"InputPolarizerEfficiency"};
auto constexpr INPUT_A_EFF_WS{"InputAnalyserEfficiency"};
auto constexpr OUTPUT_P_EFF_WS{"OutputPolarizerEfficiency"};
auto constexpr OUTPUT_F_P_EFF_WS{"OutputFpEfficiency"};
auto constexpr OUTPUT_F_A_EFF_WS{"OutputFaEfficiency"};
auto constexpr OUTPUT_A_EFF_WS{"OutputAnalyserEfficiency"};
auto constexpr OUTPUT_PHI_WS{"OutputPhi"};
auto constexpr OUTPUT_RHO_WS{"OutputRho"};
auto constexpr OUTPUT_ALPHA_WS{"OutputAlpha"};
auto constexpr OUTPUT_TPMO_WS{"OutputTwoPMinusOne"};
auto constexpr OUTPUT_TAMO_WS{"OutputTwoAMinusOne"};
auto constexpr INCLUDE_DIAGNOSTICS{"IncludeDiagnosticOutputs"};

auto constexpr OUTPUT_EFF_GROUP{"Efficiency Outputs"};
auto constexpr OUTPUT_DIAGNOSTIC_GROUP{"Diagnostic Outputs"};
} // namespace PropNames

auto constexpr INPUT_EFF_WS_ERROR{
    "If a magnetic workspace group has been provided then input efficiency workspaces should not be provided."};

auto constexpr INITIAL_CONFIG{"00,01,10,11"};
} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;
using PolarizationCorrectionsHelpers::workspaceForSpinState;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(PolarizationEfficienciesWildes)

std::string const PolarizationEfficienciesWildes::summary() const {
  return "Calculates the efficiencies of the polarizer, flippers and the analyser for a two-flipper instrument setup.";
}

void PolarizationEfficienciesWildes::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropNames::INPUT_NON_MAG_WS, "", Direction::Input),
      "Group workspace containing the transmission measurements for the non-magnetic sample.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropNames::INPUT_MAG_WS, "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "Group workspace containing the transmission measurements for the magnetic sample.");
  const auto spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropNames::FLIPPERS, INITIAL_CONFIG, spinValidator,
                  "Flipper configurations of the input group workspace(s)");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::INPUT_P_EFF_WS, "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Workspace containing the known wavelength-dependent efficiency for the polarizer.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::INPUT_A_EFF_WS, "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Workspace containing the known wavelength-dependent efficiency for the analyser.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_F_P_EFF_WS, "", Direction::Output),
      "Workspace containing the wavelength-dependent efficiency for the polarizing flipper.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_F_A_EFF_WS, "", Direction::Output),
      "Workspace containing the wavelength-dependent efficiency for the analysing flipper.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_P_EFF_WS, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the polarizer.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_A_EFF_WS, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the analyser.");
  declareProperty(PropNames::INCLUDE_DIAGNOSTICS, false, "Whether to include additional diagnostic outputs.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_PHI_WS, "phi",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for the Phi.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_RHO_WS, "rho",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for Rho.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_ALPHA_WS, "alpha",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for Alpha.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_TPMO_WS, "two_p_minus_one",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for the term (2p-1).");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_TAMO_WS, "two_a_minus_one",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for the term (2a-1).");

  auto makeSettingIncludeDiagnosticsIsSelected = [] {
    return std::make_unique<Kernel::EnabledWhenProperty>(PropNames::INCLUDE_DIAGNOSTICS, IS_EQUAL_TO, "1");
  };

  setPropertySettings(PropNames::OUTPUT_PHI_WS, makeSettingIncludeDiagnosticsIsSelected());
  setPropertySettings(PropNames::OUTPUT_RHO_WS, makeSettingIncludeDiagnosticsIsSelected());
  setPropertySettings(PropNames::OUTPUT_ALPHA_WS, makeSettingIncludeDiagnosticsIsSelected());
  setPropertySettings(PropNames::OUTPUT_TPMO_WS, makeSettingIncludeDiagnosticsIsSelected());
  setPropertySettings(PropNames::OUTPUT_TAMO_WS, makeSettingIncludeDiagnosticsIsSelected());

  const auto &effOutputGroup = PropNames::OUTPUT_EFF_GROUP;
  setPropertyGroup(PropNames::OUTPUT_P_EFF_WS, effOutputGroup);
  setPropertyGroup(PropNames::OUTPUT_F_P_EFF_WS, effOutputGroup);
  setPropertyGroup(PropNames::OUTPUT_F_A_EFF_WS, effOutputGroup);
  setPropertyGroup(PropNames::OUTPUT_A_EFF_WS, effOutputGroup);

  const auto &diagnosticOutputGroup = PropNames::OUTPUT_DIAGNOSTIC_GROUP;
  setPropertyGroup(PropNames::OUTPUT_PHI_WS, diagnosticOutputGroup);
  setPropertyGroup(PropNames::OUTPUT_RHO_WS, diagnosticOutputGroup);
  setPropertyGroup(PropNames::OUTPUT_ALPHA_WS, diagnosticOutputGroup);
  setPropertyGroup(PropNames::OUTPUT_TPMO_WS, diagnosticOutputGroup);
  setPropertyGroup(PropNames::OUTPUT_TAMO_WS, diagnosticOutputGroup);
}

namespace {
void validateInputWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &propertyName,
                            std::map<std::string, std::string> &problems) {
  if (workspace == nullptr) {
    problems[propertyName] = "All input workspaces must be matrix workspaces.";
    return;
  }

  Kernel::Unit_const_sptr unit = workspace->getAxis(0)->unit();
  if (unit->unitID() != "Wavelength") {
    problems[propertyName] = "All input workspaces must be in units of Wavelength.";
    return;
  }

  if (workspace->getNumberHistograms() != 1) {
    problems[propertyName] = "All input workspaces must contain only a single spectrum.";
    return;
  }
}

void validateInputWSGroup(const Mantid::API::WorkspaceGroup_sptr &groupWs, const std::string &propertyName,
                          std::map<std::string, std::string> &problems) {
  if (groupWs == nullptr) {
    problems[propertyName] = "The input workspace must be a group workspace.";
    return;
  }

  if (groupWs->size() != 4) {
    problems[propertyName] = "The input group must contain a workspace for all four flipper configurations.";
    return;
  }

  for (size_t i = 0; i < groupWs->size(); ++i) {
    const MatrixWorkspace_sptr childWs = std::dynamic_pointer_cast<MatrixWorkspace>(groupWs->getItem(i));
    validateInputWorkspace(childWs, propertyName, problems);
  }
}
} // namespace

std::map<std::string, std::string> PolarizationEfficienciesWildes::validateInputs() {
  std::map<std::string, std::string> problems;

  const WorkspaceGroup_sptr nonMagWsGrp = getProperty(PropNames::INPUT_NON_MAG_WS);
  validateInputWSGroup(nonMagWsGrp, PropNames::INPUT_NON_MAG_WS, problems);

  const bool hasMagWsGrp = !isDefault(PropNames::INPUT_MAG_WS);
  const bool hasInputPWs = !isDefault(PropNames::INPUT_P_EFF_WS);
  const bool hasInputAWs = !isDefault(PropNames::INPUT_A_EFF_WS);

  // If a magnetic workspace has been provided then we will use that to calculate the polarizer and analyser
  // efficiencies, so individual efficiency workspaces should not be provided as well
  if (hasMagWsGrp) {
    if (hasInputPWs) {
      problems[PropNames::INPUT_P_EFF_WS] = INPUT_EFF_WS_ERROR;
    }

    if (hasInputAWs) {
      problems[PropNames::INPUT_A_EFF_WS] = INPUT_EFF_WS_ERROR;
    }

    const WorkspaceGroup_sptr magWsGrp = getProperty(PropNames::INPUT_MAG_WS);
    validateInputWSGroup(magWsGrp, PropNames::INPUT_MAG_WS, problems);
  } else {
    if (hasInputPWs) {
      const MatrixWorkspace_sptr inputPolEffWs = getProperty(PropNames::INPUT_P_EFF_WS);
      validateInputWorkspace(inputPolEffWs, PropNames::INPUT_P_EFF_WS, problems);
    }

    if (hasInputAWs) {
      const MatrixWorkspace_sptr inputAnaEffWs = getProperty(PropNames::INPUT_A_EFF_WS);
      validateInputWorkspace(inputAnaEffWs, PropNames::INPUT_A_EFF_WS, problems);
    }
  }

  if (!isDefault(PropNames::OUTPUT_P_EFF_WS) && !hasMagWsGrp && !hasInputPWs && !hasInputAWs) {
    problems[PropNames::OUTPUT_P_EFF_WS] = "If output polarizer efficiency is requested then either the magnetic "
                                           "workspace or the known analyser efficiency should be provided.";
  }

  if (!isDefault(PropNames::OUTPUT_A_EFF_WS) && !hasMagWsGrp && !hasInputPWs && !hasInputAWs) {
    problems[PropNames::OUTPUT_A_EFF_WS] = "If output analyser efficiency is requested then either the magnetic "
                                           "workspace or the known polarizer efficiency should be provided.";
  }

  return problems;
}

void PolarizationEfficienciesWildes::exec() {
  // Calculate the polarizing and analysing flipper efficiencies
  const WorkspaceGroup_sptr &nonMagWsGrp = getProperty(PropNames::INPUT_NON_MAG_WS);
  const auto &flipperConfig = getPropertyValue(PropNames::FLIPPERS);
  const auto &ws00 = workspaceForSpinState(nonMagWsGrp, flipperConfig, SpinStateValidator::ZERO_ZERO);
  const auto &ws01 = workspaceForSpinState(nonMagWsGrp, flipperConfig, SpinStateValidator::ZERO_ONE);
  const auto &ws10 = workspaceForSpinState(nonMagWsGrp, flipperConfig, SpinStateValidator::ONE_ZERO);
  const auto &ws11 = workspaceForSpinState(nonMagWsGrp, flipperConfig, SpinStateValidator::ONE_ONE);

  const auto numerator = ws00 - ws01 - ws10 + ws11;

  const auto wsFp = numerator / (2 * (ws00 - ws01));
  const auto wsFa = numerator / (2 * (ws00 - ws10));

  const auto wsPhi = calculatePhi(ws00, ws01, ws10, ws11);

  MatrixWorkspace_sptr wsP = nullptr;
  MatrixWorkspace_sptr wsA = nullptr;
  const bool solveForP = !isDefault(PropNames::OUTPUT_P_EFF_WS);
  const bool solveForA = !isDefault(PropNames::OUTPUT_A_EFF_WS);
  if (solveForP || solveForA) {
    calculatePolarizerAndAnalyserEfficiencies(wsFp, wsFa, wsPhi, solveForP, wsP, solveForA, wsA);
  }
  setOutputs(wsPhi, wsFp, wsFa, wsP, wsA);
}

namespace {
/// Solve for the unknown efficiency from either (2p-1) or (2a-1)
MatrixWorkspace_sptr solveUnknownEfficiencyFromTXMO(const MatrixWorkspace_sptr &wsPhi,
                                                    const MatrixWorkspace_sptr &wsTXMO) {
  return (wsPhi / (2 * wsTXMO)) + 0.5;
}

void setUnitAndDistributionToMatch(const MatrixWorkspace_sptr &wsToUpdate, const MatrixWorkspace_sptr &matchWs) {
  wsToUpdate->setYUnit(matchWs->YUnit());
  wsToUpdate->setDistribution(matchWs->isDistribution());
}
} // unnamed namespace

MatrixWorkspace_sptr PolarizationEfficienciesWildes::calculatePhi(const MatrixWorkspace_sptr &ws00,
                                                                  const MatrixWorkspace_sptr &ws01,
                                                                  const MatrixWorkspace_sptr &ws10,
                                                                  const MatrixWorkspace_sptr &ws11) {
  return ((ws00 - ws01) * (ws00 - ws10)) / ((ws00 * ws11) - (ws01 * ws10));
}

MatrixWorkspace_sptr PolarizationEfficienciesWildes::calculateRho(const MatrixWorkspace_sptr &wsFp) {
  return (2 * wsFp) - 1;
}

MatrixWorkspace_sptr PolarizationEfficienciesWildes::calculateAlpha(const MatrixWorkspace_sptr &wsFa) {
  return (2 * wsFa) - 1;
}

MatrixWorkspace_sptr PolarizationEfficienciesWildes::calculateTPMOFromPhi(const WorkspaceGroup_sptr &magWsGrp,
                                                                          const MatrixWorkspace_sptr &wsFp,
                                                                          const MatrixWorkspace_sptr &wsFa,
                                                                          const MatrixWorkspace_sptr &wsPhi) {
  const auto &flipperConfig = getPropertyValue(PropNames::FLIPPERS);
  const auto &ws00 = workspaceForSpinState(magWsGrp, flipperConfig, SpinStateValidator::ZERO_ZERO);
  const auto &ws01 = workspaceForSpinState(magWsGrp, flipperConfig, SpinStateValidator::ZERO_ONE);
  const auto &ws10 = workspaceForSpinState(magWsGrp, flipperConfig, SpinStateValidator::ONE_ZERO);
  const auto &ws11 = workspaceForSpinState(magWsGrp, flipperConfig, SpinStateValidator::ONE_ONE);

  // We use the flipper efficiency to multiply the mag ws counts, but the resulting workspace will have lost the Y unit
  // and distribution information. We need to put these back otherwise the rest of the calculation fails when it tries
  // to add and subtract workspaces with different Y units.
  const auto twoFp = 2 * wsFp;
  const auto twoFa = 2 * wsFa;

  const auto twoFa00 = (1 - twoFa) * ws00;
  setUnitAndDistributionToMatch(twoFa00, ws00);

  const auto twoFa10 = (twoFa - 1) * ws10;
  setUnitAndDistributionToMatch(twoFa10, ws10);

  const auto twoFp00 = (1 - twoFp) * ws00;
  setUnitAndDistributionToMatch(twoFp00, ws00);

  const auto twoFp01 = (twoFp - 1) * ws01;
  setUnitAndDistributionToMatch(twoFp01, ws01);

  const auto numerator = twoFa00 + twoFa10 - ws01 + ws11;
  const auto denominator = twoFp00 + twoFp01 - ws10 + ws11;
  const auto tpmoSquared = wsPhi * (numerator / denominator);

  auto alg = createChildAlgorithm("Power");
  alg->initialize();
  alg->setProperty("InputWorkspace", tpmoSquared);
  alg->setProperty("Exponent", 0.5);
  alg->execute();

  return alg->getProperty("OutputWorkspace");
}

void PolarizationEfficienciesWildes::calculatePolarizerAndAnalyserEfficiencies(
    const MatrixWorkspace_sptr &wsFp, const MatrixWorkspace_sptr &wsFa, const MatrixWorkspace_sptr &wsPhi,
    const bool solveForP, MatrixWorkspace_sptr &wsP, const bool solveForA, MatrixWorkspace_sptr &wsA) {
  const WorkspaceGroup_sptr &magWsGrp = getProperty(PropNames::INPUT_MAG_WS);

  if (magWsGrp != nullptr) {
    const MatrixWorkspace_sptr wsTPMO = calculateTPMOFromPhi(magWsGrp, wsFp, wsFa, wsPhi);

    if (solveForP) {
      wsP = (wsTPMO + 1) / 2;
    }

    if (solveForA) {
      wsA = solveUnknownEfficiencyFromTXMO(wsPhi, wsTPMO);
    }

    return;
  }

  if (solveForP) {
    if (const MatrixWorkspace_sptr &inWsP = getProperty(PropNames::INPUT_P_EFF_WS)) {
      wsP = inWsP->clone();
    } else {
      const MatrixWorkspace_sptr &inWsA = getProperty(PropNames::INPUT_A_EFF_WS);
      wsP = solveForUnknownEfficiency(wsPhi, inWsA);
    }
  }

  if (solveForA) {
    if (const MatrixWorkspace_sptr &inWsA = getProperty(PropNames::INPUT_A_EFF_WS)) {
      wsA = inWsA->clone();
    } else {
      const MatrixWorkspace_sptr &inWsP = getProperty(PropNames::INPUT_P_EFF_WS);
      wsA = solveForUnknownEfficiency(wsPhi, inWsP);
    }
  }
}

MatrixWorkspace_sptr
PolarizationEfficienciesWildes::solveForUnknownEfficiency(const MatrixWorkspace_sptr &wsPhi,
                                                          const MatrixWorkspace_sptr &knownEfficiency) {
  const auto wsTXMO = (2 * knownEfficiency) - 1;
  return solveUnknownEfficiencyFromTXMO(wsPhi, wsTXMO);
}

void PolarizationEfficienciesWildes::setOutputs(const MatrixWorkspace_sptr &wsPhi, const MatrixWorkspace_sptr &wsFp,
                                                const MatrixWorkspace_sptr &wsFa, const MatrixWorkspace_sptr &wsP,
                                                const MatrixWorkspace_sptr &wsA) {
  setProperty(PropNames::OUTPUT_F_P_EFF_WS, wsFp);
  setProperty(PropNames::OUTPUT_F_A_EFF_WS, wsFa);

  if (wsP != nullptr) {
    setProperty(PropNames::OUTPUT_P_EFF_WS, wsP);
  }

  if (wsA != nullptr) {
    setProperty(PropNames::OUTPUT_A_EFF_WS, wsA);
  }

  if (getProperty(PropNames::INCLUDE_DIAGNOSTICS)) {
    setProperty(PropNames::OUTPUT_PHI_WS, wsPhi);

    const auto wsRho = calculateRho(wsFp);
    setProperty(PropNames::OUTPUT_RHO_WS, wsRho);

    const auto wsAlpha = calculateAlpha(wsFa);
    setProperty(PropNames::OUTPUT_ALPHA_WS, wsAlpha);

    if (wsP != nullptr) {
      const auto wsTPMO = (2 * wsP) - 1;
      setProperty(PropNames::OUTPUT_TPMO_WS, wsTPMO);
    }

    if (wsA != nullptr) {
      const auto wsTAMO = (2 * wsA) - 1;
      setProperty(PropNames::OUTPUT_TAMO_WS, wsTAMO);
    }
  }
}
} // namespace Mantid::Algorithms
