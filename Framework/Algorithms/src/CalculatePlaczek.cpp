// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CalculatePlaczek.h"

namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculatePlaczek)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculatePlaczek::name() const { return "CalculatePlaczek"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculatePlaczek::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculatePlaczek::category() const { return "CorrectionFunctions"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculatePlaczek::summary() const {
  return "Perform 1st or 2nd order Placzek correction for given spectrum.";
}

/// Algorithm's see also for use in the GUI and help. @see Algorithm::seeAlso
const std::vector<std::string> CalculatePlaczek::seeAlso() const { return {"CalculatePlaczekSelfScattering"}; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculatePlaczek::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> validateInputs() {
  std::map<std::string, std::string> issues;
  // TODO: find edge cases that needs checking
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczek::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
