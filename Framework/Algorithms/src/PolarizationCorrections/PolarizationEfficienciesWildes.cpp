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

auto constexpr OUTPUT_EFF_GROUP{"Efficiency Outputs"};
auto constexpr OUTPUT_DIAGNOSTIC_GROUP{"Diagnostic Outputs"};
} // namespace PropNames

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
                  "Workspace containing the wavelength-dependent efficiency for the polarizer.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::INPUT_A_EFF_WS, "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the analyser.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_P_EFF_WS, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the polarizer.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_F_P_EFF_WS, "", Direction::Output),
      "Workspace containing the wavelength-dependent efficiency for the polarizing flipper.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_F_A_EFF_WS, "", Direction::Output),
      "Workspace containing the wavelength-dependent efficiency for the analysing flipper.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_A_EFF_WS, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the analyser.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_PHI_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for the Phi.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_RHO_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for Rho.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_ALPHA_WS, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for Alpha.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_TPMO_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for the term (2p-1).");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_TAMO_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent value for the term (2a-1).");

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

  const WorkspaceGroup_sptr groupNonMagWs = getProperty(PropNames::INPUT_NON_MAG_WS);
  validateInputWSGroup(groupNonMagWs, PropNames::INPUT_NON_MAG_WS, problems);

  if (!isDefault(PropNames::INPUT_MAG_WS)) {
    const WorkspaceGroup_sptr groupMagWs = getProperty(PropNames::INPUT_MAG_WS);
    validateInputWSGroup(groupMagWs, PropNames::INPUT_MAG_WS, problems);
  }

  if (!isDefault(PropNames::INPUT_P_EFF_WS)) {
    const MatrixWorkspace_sptr inputPolEffWs = getProperty(PropNames::INPUT_P_EFF_WS);
    validateInputWorkspace(inputPolEffWs, PropNames::INPUT_P_EFF_WS, problems);
  }

  if (!isDefault(PropNames::INPUT_A_EFF_WS)) {
    const MatrixWorkspace_sptr inputAnaEffWs = getProperty(PropNames::INPUT_A_EFF_WS);
    validateInputWorkspace(inputAnaEffWs, PropNames::INPUT_A_EFF_WS, problems);
  }

  return problems;
}

void PolarizationEfficienciesWildes::exec() {}
} // namespace Mantid::Algorithms
