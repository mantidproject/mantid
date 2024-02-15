// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarisedSANS/SANSCalcDepolarisedAnalyserTransmission.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace {
/// Property Names
namespace Prop {
std::string_view constexpr DEP_WORKSPACE{"DepolarisedWorkspace"};
std::string_view constexpr MT_WORKSPACE{"EmptyCellWorkspace"};
std::string_view constexpr T_E_START{"T_EStartingValue"};
std::string_view constexpr PXD_START{"PxDStartingValue"};
} // namespace Prop

/// Initial fitting function values.
namespace FitStartValues {
double constexpr T_E = 0.9;
double constexpr PXD = 12.6;
} // namespace FitStartValues

double constexpr LAMBDA_CONVERSION_FACTOR = 0.0733;
} // namespace

namespace Mantid::Algorithms {

using namespace API;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(SANSCalcDepolarisedAnalyserTransmission)

std::string const SANSCalcDepolarisedAnalyserTransmission::summary() const {
  return "Calculate the transmission rate through a depolarised He3 cell.";
}

void SANSCalcDepolarisedAnalyserTransmission::init() {
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(std::string(Prop::DEP_WORKSPACE), "",
                                                                      Kernel::Direction::Input),
                  "The group of fully depolarised workspaces.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(std::string(Prop::MT_WORKSPACE), "",
                                                                      Kernel::Direction::Input),
                  "The group of empty cell workspaces.");
}

void SANSCalcDepolarisedAnalyserTransmission::exec() {}

} // namespace Mantid::Algorithms
