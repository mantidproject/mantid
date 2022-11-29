// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RemoveInstrumentGeometry.h"

namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemoveInstrumentGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RemoveInstrumentGeometry::name() const { return "RemoveInstrumentGeometry"; }

/// Algorithm's version for identification. @see Algorithm::version
int RemoveInstrumentGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RemoveInstrumentGeometry::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RemoveInstrumentGeometry::summary() const { return "TODO: FILL IN A SUMMARY"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RemoveInstrumentGeometry::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RemoveInstrumentGeometry::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
