// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <filesystem>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Unit.h"

namespace {
/// Property Names
namespace PropNames {
auto constexpr INPUT_WS{"InputWorkspace"};
auto constexpr OUTPUT_WS{"OutputWorkspace"};
auto constexpr OUTPUT_FILE{"OutputFilePath"};
auto constexpr SPIN_STATES{"SpinStates"};
} // namespace PropNames

auto constexpr FILE_EXTENSION{".nxs"};
auto constexpr INITIAL_SPIN{"11,10,01,00"};
} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;
using PolarizationCorrectionsHelpers::workspaceForSpinState;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(FlipperEfficiency)

std::string const FlipperEfficiency::summary() const { return "Calculate the efficiency of the polarization flipper."; }

void FlipperEfficiency::init() {
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropNames::INPUT_WS, "", Direction::Input),
                  "Group workspace containing flipper transmissions for all 4 polarization states.");
  auto const spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropNames::SPIN_STATES, INITIAL_SPIN, spinValidator,
                  "Order of individual spin states in the input group workspace, e.g. \"01,11,00,10\"");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the flipper.");
  declareProperty(std::make_unique<FileProperty>(PropNames::OUTPUT_FILE, "", FileProperty::OptionalSave),
                  "File name or path for the output to be saved to.");
}

namespace {
void validateInputWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                            std::map<std::string, std::string> &problems) {
  Kernel::Unit_const_sptr unit = workspace->getAxis(0)->unit();
  if (unit->unitID() != "Wavelength") {
    problems[PropNames::INPUT_WS] = "All input workspaces must be in units of Wavelength.";
    return;
  }

  if (workspace->getNumberHistograms() != 1) {
    problems[PropNames::INPUT_WS] = "All input workspaces must contain only a single spectrum.";
    return;
  }
}

double calculateErrorValue(double const t11Y, double const t11E, double const t10Y, double const t10E,
                           double const t01Y, double const t01E, double const t00Y, double const t00E) {
  double const denom_1 = std::pow((t11Y + t10Y), 2) * (t00Y - t01Y);
  double const denom_0 = (t11Y + t10Y) * std::pow((t00Y - t01Y), 2);

  double const deff_dt11 = (t10Y * (t00Y + t01Y)) / denom_1;
  double const deff_dt10 = (-t11Y * (t00Y + t01Y)) / denom_1;
  double const deff_dt00 = (t01Y * (t10Y - t11Y)) / denom_0;
  double const deff_dt01 = (t00Y * (t11Y - t10Y)) / denom_0;

  double const sigma_squared = std::pow(deff_dt11, 2) * std::pow(t11E, 2) + std::pow(deff_dt00, 2) * std::pow(t00E, 2) +
                               std::pow(deff_dt10, 2) * std::pow(t10E, 2) + std::pow(deff_dt01, 2) * std::pow(t01E, 2);

  return std::sqrt(sigma_squared);
}
} // namespace

std::map<std::string, std::string> FlipperEfficiency::validateInputs() {
  std::map<std::string, std::string> problems;
  // Check input.
  WorkspaceGroup_sptr const groupWs = getProperty(PropNames::INPUT_WS);
  if (groupWs == nullptr) {
    // Return early so the following checks don't freak out.
    problems[PropNames::INPUT_WS] = "The input workspace must be a group workspace";
    return problems;
  }
  if (groupWs->size() != 4) {
    problems[PropNames::INPUT_WS] = "The input group must contain a workspace for all four spin states.";
  } else {
    for (size_t i = 0; i < groupWs->size(); ++i) {
      MatrixWorkspace_sptr const stateWs = std::dynamic_pointer_cast<MatrixWorkspace>(groupWs->getItem(i));
      validateInputWorkspace(stateWs, problems);
    }
  }

  // Check outputs.
  auto const &outputWs = getPropertyValue(PropNames::OUTPUT_WS);
  auto const &outputFile = getPropertyValue(PropNames::OUTPUT_FILE);
  if (outputWs.empty() && outputFile.empty()) {
    problems[PropNames::OUTPUT_FILE] = "Either an output workspace or output file must be provided.";
    problems[PropNames::OUTPUT_WS] = "Either an output workspace or output file must be provided.";
  }
  return problems;
}

void FlipperEfficiency::exec() {
  WorkspaceGroup_sptr const &groupWs = getProperty(PropNames::INPUT_WS);
  auto const &efficiency = calculateEfficiency(groupWs);

  auto const &filename = getPropertyValue(PropNames::OUTPUT_FILE);
  if (!filename.empty()) {
    saveToFile(efficiency, filename);
  }

  auto const &outputWsName = getPropertyValue(PropNames::OUTPUT_WS);
  if (!outputWsName.empty()) {
    setProperty(PropNames::OUTPUT_WS, efficiency);
  }
}

MatrixWorkspace_sptr FlipperEfficiency::calculateEfficiency(WorkspaceGroup_sptr const &groupWs) {
  auto const &spinConfig = getPropertyValue(PropNames::SPIN_STATES);
  auto const &t11Ws = workspaceForSpinState(groupWs, spinConfig, SpinStateValidator::ONE_ONE);
  auto const &t10Ws = workspaceForSpinState(groupWs, spinConfig, SpinStateValidator::ONE_ZERO);
  auto const &t01Ws = workspaceForSpinState(groupWs, spinConfig, SpinStateValidator::ZERO_ONE);
  auto const &t00Ws = workspaceForSpinState(groupWs, spinConfig, SpinStateValidator::ZERO_ZERO);

  auto const &numerator = (t11Ws * t00Ws) - (t10Ws * t01Ws);
  auto const &denominator = (t11Ws + t10Ws) * (t00Ws - t01Ws);
  auto const &efficiency = numerator / denominator;

  // Calculate the errors
  auto const &t11Y = t11Ws->y(0);
  auto const &t11E = t11Ws->e(0);
  auto const &t10Y = t10Ws->y(0);
  auto const &t10E = t10Ws->e(0);
  auto const &t01Y = t01Ws->y(0);
  auto const &t01E = t01Ws->e(0);
  auto const &t00Y = t00Ws->y(0);
  auto const &t00E = t00Ws->e(0);
  auto &efficiencyE = efficiency->mutableE(0);
  auto const numBins = efficiencyE.size();
  for (size_t i = 0; i < numBins; ++i) {
    efficiencyE[i] = calculateErrorValue(t11Y[i], t11E[i], t10Y[i], t10E[i], t01Y[i], t01E[i], t00Y[i], t00E[i]);
  }
  return efficiency;
}

void FlipperEfficiency::saveToFile(MatrixWorkspace_sptr const &workspace, std::string const &filePathStr) {
  std::filesystem::path filePath = filePathStr;
  // Add the nexus extension if it's not been applied already.
  if (filePath.extension() != FILE_EXTENSION) {
    filePath.replace_extension(FILE_EXTENSION);
  }
  auto saveAlg = createChildAlgorithm("SaveNexus");
  saveAlg->initialize();
  saveAlg->setProperty("Filename", filePath.string());
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->execute();
}

} // namespace Mantid::Algorithms
