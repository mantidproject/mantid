// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <filesystem>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/PolSANSWorkspaceValidator.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/SpinStateValidator.h"
#include "MantidKernel/Unit.h"

namespace {
/// Property Names
namespace PropNames {
auto constexpr INPUT_WS{"InputWorkspace"};
auto constexpr OUTPUT_WS{"OutputWorkspace"};
auto constexpr OUTPUT_FILE{"OutputFilePath"};
auto constexpr SPIN_STATES{"SpinStates"};
auto constexpr FLIPPER_LOC{"Flipper"};
auto constexpr POS_OPTIONS = {"Polarizer", "Analyzer"};
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

std::string const FlipperEfficiency::summary() const {
  return "Calculate the efficiency of the polarization or analyzer flipper.";
}

void FlipperEfficiency::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropNames::INPUT_WS, "", Direction::Input,
                                                          std::make_shared<Mantid::API::PolSANSWorkspaceValidator>()),
      "Group workspace containing flipper transmissions for all 4 polarization states.");
  auto const spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropNames::FLIPPER_LOC, *PropNames::POS_OPTIONS.begin(),
                  std::make_shared<StringListValidator>(PropNames::POS_OPTIONS)),
      declareProperty(PropNames::SPIN_STATES, INITIAL_SPIN, spinValidator,
                      "Order of individual flipper configurations in the input group workspace, e.g. \"01,11,00,10\"");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the flipper.");
  declareProperty(std::make_unique<FileProperty>(PropNames::OUTPUT_FILE, "", FileProperty::OptionalSave),
                  "File name or path for the output to be saved to.");
}

namespace {
double calculateErrorValue(const std::vector<double> &TijY, const std::vector<double> &TijE) {
  double const denom_1 = std::pow(TijY[0] + TijY[1], 2) * (TijY[3] - TijY[2]);
  double const denom_0 = (TijY[0] + TijY[1]) * std::pow(TijY[3] - TijY[2], 2);

  double const deff_dt11 = (TijY[1] * (TijY[3] + TijY[2])) / denom_1;
  double const deff_dt10 = (-TijY[0] * (TijY[3] + TijY[2])) / denom_1;
  double const deff_dt00 = (TijY[2] * (TijY[1] - TijY[0])) / denom_0;
  double const deff_dt01 = (TijY[3] * (TijY[0] - TijY[1])) / denom_0;

  double const sigma_squared = std::pow(deff_dt11 * TijE[0], 2) + std::pow(deff_dt00 * TijE[3], 2) +
                               std::pow(deff_dt10 * TijE[1], 2) + std::pow(deff_dt01 * TijE[2], 2);

  return std::sqrt(sigma_squared);
}
} // namespace

std::map<std::string, std::string> FlipperEfficiency::validateInputs() {
  std::map<std::string, std::string> problems;

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
  auto const &efficiency = calculateEfficiency(groupWs, !isDefault(PropNames::FLIPPER_LOC));

  auto const &filename = getPropertyValue(PropNames::OUTPUT_FILE);
  if (!filename.empty()) {
    saveToFile(efficiency, filename);
  }

  auto const &outputWsName = getPropertyValue(PropNames::OUTPUT_WS);
  if (!outputWsName.empty()) {
    setProperty(PropNames::OUTPUT_WS, efficiency);
  }
}

MatrixWorkspace_sptr FlipperEfficiency::calculateEfficiency(WorkspaceGroup_sptr const &groupWs,
                                                            bool const isFlipperAnalyser) {
  using namespace FlipperConfigurations;
  auto const &spinConfig = getPropertyValue(PropNames::SPIN_STATES);
  std::vector<MatrixWorkspace_sptr> Tij;
  for (auto const &flipperConf : {ON_ON, ON_OFF, OFF_ON, OFF_OFF}) {
    Tij.push_back(workspaceForSpinState(groupWs, spinConfig, flipperConf));
  }

  auto const &numerator = (Tij[0] * Tij[3]) - (Tij[2] * Tij[1]);
  auto const &denominator =
      isFlipperAnalyser ? (Tij[0] + Tij[2]) * (Tij[3] - Tij[1]) : (Tij[0] + Tij[1]) * (Tij[3] - Tij[2]);
  auto const &efficiency = numerator / denominator;

  // Calculate the errors
  auto &efficiencyE = efficiency->mutableE(0);
  auto const numBins = efficiencyE.size();
  for (size_t i = 0; i < numBins; ++i) {
    std::vector<double> tVec, tVecE;
    for (auto const &wk : Tij) {
      tVec.push_back(wk->y(0)[i]);
      tVecE.push_back(wk->e(0)[i]);
    }
    if (isFlipperAnalyser) {
      std::swap(tVec[1], tVec[2]);
      std::swap(tVecE[1], tVecE[2]);
    }
    efficiencyE[i] = calculateErrorValue(tVec, tVecE);
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
