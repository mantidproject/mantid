// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <filesystem>

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"

namespace {
/// Property Names
namespace PropNames {
std::string const INPUT_WS{"InputWorkspace"};
std::string const OUTPUT_WS{"OutputWorkspace"};
std::string const OUTPUT_FILE{"OutputFilePath"};
} // namespace PropNames

std::string const FILE_EXTENSION{".nxs"};
} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(FlipperEfficiency)

std::string const FlipperEfficiency::summary() const { return "Calculate the efficiency of the polarization flipper."; }

void FlipperEfficiency::init() {
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropNames::INPUT_WS, "", Direction::Input),
                  "Group workspace containing the 4 polarisation periods.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_WS, "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Workspace containing the wavelength-dependent efficiency for the flipper.");
  declareProperty(std::make_unique<FileProperty>(PropNames::OUTPUT_FILE, "", FileProperty::OptionalSave),
                  "File name or path for the output to be saved to.");
}

std::map<std::string, std::string> FlipperEfficiency::validateInputs() {
  std::map<std::string, std::string> problems;
  auto const &outputWs = getPropertyValue(PropNames::OUTPUT_WS);
  auto const &outputFile = getPropertyValue(PropNames::OUTPUT_FILE);
  if (outputWs.empty() && outputFile.empty()) {
    problems[PropNames::OUTPUT_FILE] = "Either an output workspace or output file must be provided.";
    problems[PropNames::OUTPUT_WS] = "Either an output workspace or output file must be provided.";
  }
  return problems;
}

void FlipperEfficiency::exec() {
  WorkspaceGroup_sptr const groupWs = getProperty(PropNames::INPUT_WS);
  MatrixWorkspace_sptr const firstWs = std::dynamic_pointer_cast<MatrixWorkspace>(groupWs->getItem(0));
  saveToFile(firstWs);
}

void FlipperEfficiency::saveToFile(MatrixWorkspace_sptr const &workspace) {
  std::filesystem::path filePath = getPropertyValue(PropNames::OUTPUT_FILE);
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
