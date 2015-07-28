#include "MantidAlgorithms/PDDetermineCharacterizations2.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDDetermineCharacterizations2)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDDetermineCharacterizations2::PDDetermineCharacterizations2() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDDetermineCharacterizations2::~PDDetermineCharacterizations2() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PDDetermineCharacterizations2::name() const { return "PDDetermineCharacterizations2"; }

/// Algorithm's version for identification. @see Algorithm::version
int PDDetermineCharacterizations2::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PDDetermineCharacterizations2::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PDDetermineCharacterizations2::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PDDetermineCharacterizations2::init() {
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
void PDDetermineCharacterizations2::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid