#include "MantidAlgorithms/MaxEnt.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaxEnt)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MaxEnt::MaxEnt() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MaxEnt::~MaxEnt() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MaxEnt::name() const { return "MaxEnt"; }

/// Algorithm's version for identification. @see Algorithm::version
int MaxEnt::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MaxEnt::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MaxEnt::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MaxEnt::init() {
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
void MaxEnt::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
