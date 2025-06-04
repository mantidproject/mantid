// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/DetermineSpinStateOrder.h"

namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DetermineSpinStateOrder)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string DetermineSpinStateOrder::name() const { return "DetermineSpinStateOrder"; }

/// Algorithm's version for identification. @see Algorithm::version
int DetermineSpinStateOrder::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DetermineSpinStateOrder::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string DetermineSpinStateOrder::summary() const { return "TODO: FILL IN A SUMMARY"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DetermineSpinStateOrder::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DetermineSpinStateOrder::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
