#include "MantidAlgorithms/OverlayEventWorkspace.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(OverlayEventWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
OverlayEventWorkspace::OverlayEventWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
OverlayEventWorkspace::~OverlayEventWorkspace() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string OverlayEventWorkspace::name() const { return "OverlayEventWorkspace"; }

/// Algorithm's version for identification. @see Algorithm::version
int OverlayEventWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string OverlayEventWorkspace::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string OverlayEventWorkspace::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void OverlayEventWorkspace::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void OverlayEventWorkspace::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid