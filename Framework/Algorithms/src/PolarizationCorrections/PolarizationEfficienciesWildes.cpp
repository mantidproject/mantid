// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizationEfficienciesWildes.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/SpinStateValidator.h"
#include "MantidKernel/Unit.h"
#include <cmath>

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
auto constexpr MAG_KEY_PREFIX = "mag_";

constexpr auto fnPhi = [](const auto &x) { return ((x[0] - x[1]) * (x[0] - x[2])) / (x[0] * x[3] - x[1] * x[2]); };
constexpr auto fnFp = [](const auto &x) { return (x[0] - x[1] - x[2] + x[3]) / (2 * (x[0] - x[1])); };
constexpr auto fnFa = [](const auto &x) { return (x[0] - x[1] - x[2] + x[3]) / (2 * (x[0] - x[2])); };
constexpr auto fnNumerator = [](const auto &x, const auto &fa) {
  return (1 - 2 * fa) * x[4] + (2 * fa - 1) * x[6] - x[5] + x[7];
};
constexpr auto fnDenominator = [](const auto &x, const auto &fp) {
  return (1 - 2 * fp) * x[4] + (2 * fp - 1) * x[5] - x[6] + x[7];
};
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
      "Group workspace containing the transmission measurements for the non-magnetic sample");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropNames::INPUT_MAG_WS, "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "Group workspace containing the transmission measurements for the magnetic sample.");
  const auto spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropNames::FLIPPERS, INITIAL_CONFIG, spinValidator,
                  "Flipper configurations of the input group workspace(s).");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::INPUT_P_EFF_WS, "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Workspace containing the known wavelength-dependent efficiency for the polarizer.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::INPUT_A_EFF_WS, "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Workspace containing the known wavelength-dependent efficiency for the analyser.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_F_P_EFF_WS, "", Direction::Output),
      "Output workspace containing the polarizing flipper efficiencies");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_F_A_EFF_WS, "", Direction::Output),
      "Output workspace containing the analysing flipper efficiencies");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_P_EFF_WS, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace containing the polarizer efficiencies.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_A_EFF_WS, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace containing the analyser efficiencies.");
  declareProperty(PropNames::INCLUDE_DIAGNOSTICS, false, "Whether to include additional diagnostic outputs.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_PHI_WS, "phi",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace containing the values for Phi.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_RHO_WS, "rho",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace containing the values for Rho.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_ALPHA_WS, "alpha",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace containing the values for Alpha.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_TPMO_WS, "two_p_minus_one",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace containing the values for the term (2p-1).");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_TAMO_WS, "two_a_minus_one",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace containing the values for the term (2a-1).");

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
bool hasMatchingBins(const Mantid::API::MatrixWorkspace_sptr &workspace, const Mantid::API::MatrixWorkspace_sptr &refWs,
                     const std::string &propertyName, std::map<std::string, std::string> &problems) {
  if (!WorkspaceHelpers::matchingBins(workspace, refWs, true)) {
    problems[propertyName] = "All input workspaces must have the same X values.";
    return false;
  }

  return true;
}

bool isValidInputWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                           const Mantid::API::MatrixWorkspace_sptr &refWs, const std::string &propertyName,
                           std::map<std::string, std::string> &problems) {
  if (workspace == nullptr) {
    problems[propertyName] = "All input workspaces must be matrix workspaces.";
    return false;
  }

  Kernel::Unit_const_sptr unit = workspace->getAxis(0)->unit();
  if (unit->unitID() != "Wavelength") {
    problems[propertyName] = "All input workspaces must be in units of Wavelength.";
    return false;
  }

  if (workspace->getNumberHistograms() != 1) {
    problems[propertyName] = "All input workspaces must contain only a single spectrum.";
    return false;
  }

  return hasMatchingBins(workspace, refWs, propertyName, problems);
}

bool isValidInputWSGroup(const Mantid::API::WorkspaceGroup_sptr &groupWs, const std::string &propertyName,
                         std::map<std::string, std::string> &problems) {
  if (groupWs == nullptr) {
    problems[propertyName] = "The input workspace must be a group workspace.";
    return false;
  }

  if (groupWs->size() != 4) {
    problems[propertyName] = "The input group must contain a workspace for all four flipper configurations.";
    return false;
  }

  const MatrixWorkspace_sptr refWs = std::dynamic_pointer_cast<MatrixWorkspace>(groupWs->getItem(0));
  for (size_t i = 0; i < groupWs->size(); ++i) {
    const MatrixWorkspace_sptr childWs = std::dynamic_pointer_cast<MatrixWorkspace>(groupWs->getItem(i));
    if (!isValidInputWorkspace(childWs, refWs, propertyName, problems)) {
      return false;
    }
  }

  return true;
}
} // namespace

std::map<std::string, std::string> PolarizationEfficienciesWildes::validateInputs() {
  std::map<std::string, std::string> problems;

  const bool hasMagWsGrp = !isDefault(PropNames::INPUT_MAG_WS);
  const bool hasInputPWs = !isDefault(PropNames::INPUT_P_EFF_WS);
  const bool hasInputAWs = !isDefault(PropNames::INPUT_A_EFF_WS);

  if (!isDefault(PropNames::OUTPUT_P_EFF_WS) && !hasMagWsGrp && !hasInputPWs && !hasInputAWs) {
    problems[PropNames::OUTPUT_P_EFF_WS] = "If output polarizer efficiency is requested then either the magnetic "
                                           "workspace or the known analyser efficiency should be provided.";
  }

  if (!isDefault(PropNames::OUTPUT_A_EFF_WS) && !hasMagWsGrp && !hasInputPWs && !hasInputAWs) {
    problems[PropNames::OUTPUT_A_EFF_WS] = "If output analyser efficiency is requested then either the magnetic "
                                           "workspace or the known polarizer efficiency should be provided.";
  }

  const WorkspaceGroup_sptr nonMagWsGrp = getProperty(PropNames::INPUT_NON_MAG_WS);
  if (!isValidInputWSGroup(nonMagWsGrp, PropNames::INPUT_NON_MAG_WS, problems)) {
    // We need to use a child workspace from the nonMag group as a reference for later checks, so stop here if there are
    // any issues with this input
    return problems;
  }

  const MatrixWorkspace_sptr nonMagRefWs = std::dynamic_pointer_cast<MatrixWorkspace>(nonMagWsGrp->getItem(0));

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
    if (isValidInputWSGroup(magWsGrp, PropNames::INPUT_MAG_WS, problems)) {
      // Check that bins match between the input mag and nonMag workspace groups
      const MatrixWorkspace_sptr magWs = std::dynamic_pointer_cast<MatrixWorkspace>(magWsGrp->getItem(0));
      hasMatchingBins(magWs, nonMagRefWs, PropNames::INPUT_MAG_WS, problems);
    }
  } else {
    if (hasInputPWs) {
      const MatrixWorkspace_sptr inputPolEffWs = getProperty(PropNames::INPUT_P_EFF_WS);
      isValidInputWorkspace(inputPolEffWs, nonMagRefWs, PropNames::INPUT_P_EFF_WS, problems);
    }

    if (hasInputAWs) {
      const MatrixWorkspace_sptr inputAnaEffWs = getProperty(PropNames::INPUT_A_EFF_WS);
      isValidInputWorkspace(inputAnaEffWs, nonMagRefWs, PropNames::INPUT_A_EFF_WS, problems);
    }
  }

  return problems;
}

void PolarizationEfficienciesWildes::exec() {
  Progress progress(this, 0.0, 1.0, 10);

  progress.report(0, "Extracting spin state workspaces");
  mapSpinStateWorkspaces();

  progress.report(1, "Calculating flipper efficiencies");
  calculateFlipperEfficienciesAndPhi();

  const bool solveForP = !isDefault(PropNames::OUTPUT_P_EFF_WS);
  const bool solveForA = !isDefault(PropNames::OUTPUT_A_EFF_WS);
  if (solveForP || solveForA) {
    progress.report(4, "Finding polarizer and analyser efficiencies");
    calculatePolarizerAndAnalyserEfficiencies(solveForP, solveForA);
  }

  progress.report(8, "Setting algorithm outputs");
  setOutputs();

  // Ensure that we don't carry over values from a previous run if an instance of this algorithm is run twice
  resetMemberVariables();
}

namespace {
void setUnitAndDistributionToMatch(const MatrixWorkspace_sptr &wsToUpdate, const MatrixWorkspace_sptr &matchWs) {
  wsToUpdate->setYUnit(matchWs->YUnit());
  wsToUpdate->setDistribution(matchWs->isDistribution());
}
} // unnamed namespace

void PolarizationEfficienciesWildes::calculateFlipperEfficienciesAndPhi() {
  // Calculate the polarizing and analysing flipper efficiencies
  const auto &[ws00, ws01, ws10, ws11] = getFlipperWorkspaces();

  constexpr int var_num = 4;
  // Calculate fp
  const auto errorPropFp = Arithmetic::make_error_propagation<var_num>([](const auto &x) { return fnFp(x); });
  m_wsFp = errorPropFp.evaluateWorkspaces(ws00, ws01, ws10, ws11);

  // Calculate fa
  const auto errorPropFa = Arithmetic::make_error_propagation<var_num>([](const auto &x) { return fnFa(x); });
  m_wsFa = errorPropFa.evaluateWorkspaces(ws00, ws01, ws10, ws11);

  // Calculate phi
  const auto errorPropPhi = Arithmetic::make_error_propagation<var_num>([](const auto &x) { return fnPhi(x); });
  m_wsPhi = errorPropPhi.evaluateWorkspaces(ws00, ws01, ws10, ws11);
}

MatrixWorkspace_sptr PolarizationEfficienciesWildes::calculateTPMO() {
  const auto &[ws00, ws01, ws10, ws11] = getFlipperWorkspaces();
  const auto &[ws00Mag, ws01Mag, ws10Mag, ws11Mag] = getFlipperWorkspaces(true);

  constexpr int var_num = 8;
  const auto errorProp = Arithmetic::make_error_propagation<var_num>([](const auto &x) {
    const auto fp = fnFp(x);
    const auto fa = fnFa(x);
    const auto numerator = fnNumerator(x, fa);
    const auto denominator = fnDenominator(x, fp);
    return sqrt(fnPhi(x) * (numerator / denominator));
  });
  const auto outWs = errorProp.evaluateWorkspaces(ws00, ws01, ws10, ws11, ws00Mag, ws01Mag, ws10Mag, ws11Mag);
  return outWs;
}

void PolarizationEfficienciesWildes::calculatePolarizerAndAnalyserEfficiencies(const bool solveForP,
                                                                               const bool solveForA) {
  const auto &[ws00, ws01, ws10, ws11] = getFlipperWorkspaces();
  if (m_magWsProvided) {
    const auto &[ws00Mag, ws01Mag, ws10Mag, ws11Mag] = getFlipperWorkspaces(true);
    constexpr int var_num = 8;
    const MatrixWorkspace_sptr wsTPMO = calculateTPMO();

    if (solveForP) {
      m_wsP = (wsTPMO + 1) / 2;
    }

    if (solveForA) {
      const auto errorProp = Arithmetic::make_error_propagation<var_num>([](const auto &x) {
        const auto phi = fnPhi(x);
        const auto fp = fnFp(x);
        const auto fa = fnFa(x);
        const auto numerator = fnNumerator(x, fa);
        const auto denominator = fnDenominator(x, fp);
        const auto TPMO = sqrt(phi * (numerator / denominator));
        return (phi / (2 * TPMO)) + 0.5;
      });
      m_wsA = errorProp.evaluateWorkspaces(ws00, ws01, ws10, ws11, ws00Mag, ws01Mag, ws10Mag, ws11Mag);
    }

    return;
  }

  if (solveForP) {
    if (const MatrixWorkspace_sptr inWsP = getProperty(PropNames::INPUT_P_EFF_WS)) {
      m_wsP = inWsP->clone();
    } else {
      const MatrixWorkspace_sptr inWsA = getProperty(PropNames::INPUT_A_EFF_WS);
      constexpr int var_num = 5;
      const auto errorProp = Arithmetic::make_error_propagation<var_num>([](const auto &x) {
        const auto TXMO = (2 * x[4]) - 1;
        return (fnPhi(x) / (2 * TXMO)) + 0.5;
      });
      m_wsP = errorProp.evaluateWorkspaces(ws00, ws01, ws10, ws11, inWsA);
    }
  }

  if (solveForA) {
    if (const MatrixWorkspace_sptr inWsA = getProperty(PropNames::INPUT_A_EFF_WS)) {
      m_wsA = inWsA->clone();
    } else {
      const MatrixWorkspace_sptr inWsP = getProperty(PropNames::INPUT_P_EFF_WS);
      constexpr int var_num = 5;
      const auto errorProp = Arithmetic::make_error_propagation<var_num>([](const auto &x) {
        const auto TXMO = (2 * x[4]) - 1;
        return (fnPhi(x) / (2 * TXMO)) + 0.5;
      });
      m_wsA = errorProp.evaluateWorkspaces(ws00, ws01, ws10, ws11, inWsP);
    }
  }
}

void PolarizationEfficienciesWildes::setOutputs() {
  setProperty(PropNames::OUTPUT_F_P_EFF_WS, m_wsFp);
  setProperty(PropNames::OUTPUT_F_A_EFF_WS, m_wsFa);

  if (m_wsP != nullptr) {
    setProperty(PropNames::OUTPUT_P_EFF_WS, m_wsP);
  }

  if (m_wsA != nullptr) {
    setProperty(PropNames::OUTPUT_A_EFF_WS, m_wsA);
  }

  if (getProperty(PropNames::INCLUDE_DIAGNOSTICS)) {
    setProperty(PropNames::OUTPUT_PHI_WS, m_wsPhi);

    const auto wsRho = (2 * m_wsFp) - 1;
    setProperty(PropNames::OUTPUT_RHO_WS, wsRho);

    const auto wsAlpha = (2 * m_wsFa) - 1;
    setProperty(PropNames::OUTPUT_ALPHA_WS, wsAlpha);

    if (m_wsP != nullptr) {
      const auto wsTPMO = (2 * m_wsP) - 1;
      setProperty(PropNames::OUTPUT_TPMO_WS, wsTPMO);
    } else if (isChild()) {
      // Clear diagnostic output property that may have been populated in a previous run as a child algorithm
      resetPropertyValue(PropNames::OUTPUT_TPMO_WS);
    }

    if (m_wsA != nullptr) {
      const auto wsTAMO = (2 * m_wsA) - 1;
      setProperty(PropNames::OUTPUT_TAMO_WS, wsTAMO);
    } else if (isChild()) {
      // Clear diagnostic output property that may have been populated in a previous run as a child algorithm
      resetPropertyValue(PropNames::OUTPUT_TAMO_WS);
    }
  } else if (isChild()) {
    // Clear diagnostic output properties that may have been populated in a previous run as a child algorithm
    resetPropertyValue(PropNames::OUTPUT_PHI_WS);
    resetPropertyValue(PropNames::OUTPUT_RHO_WS);
    resetPropertyValue(PropNames::OUTPUT_ALPHA_WS);
    resetPropertyValue(PropNames::OUTPUT_TPMO_WS);
    resetPropertyValue(PropNames::OUTPUT_TAMO_WS);
  }
}

void PolarizationEfficienciesWildes::resetMemberVariables() {
  m_wsFp = nullptr;
  m_wsFa = nullptr;
  m_wsPhi = nullptr;
  m_wsP = nullptr;
  m_wsA = nullptr;
  m_magWsProvided = false;
  m_spinStateWorkspaces.clear();
}

void PolarizationEfficienciesWildes::resetPropertyValue(const std::string &propertyName) {
  setPropertyValue(propertyName, getPropertyValue(propertyName));
}

void PolarizationEfficienciesWildes::populateSpinStateWorkspaces(const WorkspaceGroup_sptr &wsGrp,
                                                                 const std::string &keyPrefix) {
  const auto &flipperConfig = getPropertyValue(PropNames::FLIPPERS);
  m_spinStateWorkspaces.emplace(keyPrefix + FlipperConfigurations::OFF_OFF,
                                workspaceForSpinState(wsGrp, flipperConfig, FlipperConfigurations::OFF_OFF));
  m_spinStateWorkspaces.emplace(keyPrefix + FlipperConfigurations::OFF_ON,
                                workspaceForSpinState(wsGrp, flipperConfig, FlipperConfigurations::OFF_ON));
  m_spinStateWorkspaces.emplace(keyPrefix + FlipperConfigurations::ON_OFF,
                                workspaceForSpinState(wsGrp, flipperConfig, FlipperConfigurations::ON_OFF));
  m_spinStateWorkspaces.emplace(keyPrefix + FlipperConfigurations::ON_ON,
                                workspaceForSpinState(wsGrp, flipperConfig, FlipperConfigurations::ON_ON));
}

void PolarizationEfficienciesWildes::mapSpinStateWorkspaces() {
  const WorkspaceGroup_sptr magWsGrp = getProperty(PropNames::INPUT_MAG_WS);
  const WorkspaceGroup_sptr nonMagWsGrp = getProperty(PropNames::INPUT_NON_MAG_WS);
  if (magWsGrp != nullptr) {
    m_magWsProvided = true;
    populateSpinStateWorkspaces(magWsGrp, MAG_KEY_PREFIX);
  }
  populateSpinStateWorkspaces(nonMagWsGrp);
}

PolarizationEfficienciesWildes::FlipperWorkspaces PolarizationEfficienciesWildes::getFlipperWorkspaces(const bool mag) {
  if (mag) {
    return {m_spinStateWorkspaces[MAG_KEY_PREFIX + FlipperConfigurations::OFF_OFF],
            m_spinStateWorkspaces[MAG_KEY_PREFIX + FlipperConfigurations::OFF_ON],
            m_spinStateWorkspaces[MAG_KEY_PREFIX + FlipperConfigurations::ON_OFF],
            m_spinStateWorkspaces[MAG_KEY_PREFIX + FlipperConfigurations::ON_ON]};
  }
  return {m_spinStateWorkspaces[FlipperConfigurations::OFF_OFF], m_spinStateWorkspaces[FlipperConfigurations::OFF_ON],
          m_spinStateWorkspaces[FlipperConfigurations::ON_OFF], m_spinStateWorkspaces[FlipperConfigurations::ON_ON]};
}

} // namespace Mantid::Algorithms
