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
constexpr char const *INPUT_WS{"InputWorkspace"};
constexpr char const *OUTPUT_WS{"OutputWorkspace"};
constexpr char const *OUTPUT_FILE{"OutputFilePath"};
constexpr char const *SPIN_STATES{"SpinStates"};
} // namespace PropNames

constexpr char const *FILE_EXTENSION{".nxs"};
constexpr char const *INITIAL_SPIN{"11,10,01,00"};
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
                  "Group workspace containing flipper transmissions for all 4 polarisation states.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the flipper.");
  declareProperty(std::make_unique<FileProperty>(PropNames::OUTPUT_FILE, "", FileProperty::OptionalSave),
                  "File name or path for the output to be saved to.");
  auto const spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropNames::SPIN_STATES, INITIAL_SPIN, spinValidator,
                  "Order of individual spin states in the input group workspace, e.g. \"01,11,00,10\"");
}

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
  }
  for (size_t i = 0; i < groupWs->size(); ++i) {
    MatrixWorkspace_sptr const stateWs = std::dynamic_pointer_cast<MatrixWorkspace>(groupWs->getItem(i));
    Unit_const_sptr unit = stateWs->getAxis(0)->unit();
    if (unit->unitID() != "Wavelength") {
      problems[PropNames::INPUT_WS] = "All input workspaces must be in units of Wavelength.";
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

  auto const &numerator = t00Ws - t01Ws + t11Ws - t10Ws;
  auto const &denominator = 2 * (t00Ws - t01Ws);
  auto const &efficiency = numerator / denominator;
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
