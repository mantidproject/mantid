// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace {
/// Property Names
namespace PropNames {
std::string const &INPUT_WS{"InputWorkspace"};
std::string const &OUTPUT_WS{"OutputWorkspace"};
std::string const &OUTPUT_FILE{"OutputFilePath"};
} // namespace PropNames
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
  declareProperty(std::make_unique<FileProperty>(PropNames::OUTPUT_FILE, "", FileProperty::OptionalSave, ".nxs"),
                  "File name or path for the output to be saved to.");
}

void FlipperEfficiency::exec() {
  WorkspaceGroup_sptr const groupWs = getProperty(PropNames::INPUT_WS);
  MatrixWorkspace_sptr const firstWs = std::dynamic_pointer_cast<MatrixWorkspace>(groupWs->getItem(0));
  auto const filePath = getPropertyValue(PropNames::OUTPUT_FILE);
  saveToFile(firstWs, filePath);
}

void FlipperEfficiency::saveToFile(MatrixWorkspace_sptr const &workspace, std::string const &filePath) {
  auto saveAlg = createChildAlgorithm("SaveNexus");
  saveAlg->initialize();
  saveAlg->setProperty("Filename", filePath);
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->execute();
}

} // namespace Mantid::Algorithms
