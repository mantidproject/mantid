// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarizationCorrections/FlipperEfficiency.h"

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
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::INPUT_WS, "", Direction::Input),
                  "Group workspace containing the 4 polarisation periods.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropNames::OUTPUT_WS, "", Direction::Output),
                  "Workspace containing the wavelength-dependent efficiency for the flipper.");
  declareProperty(PropNames::OUTPUT_FILE, "", "File name or path for the output to be saved to.");
}

void FlipperEfficiency::exec() {}

} // namespace Mantid::Algorithms
