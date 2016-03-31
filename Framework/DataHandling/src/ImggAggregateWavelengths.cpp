#include "MantidDataHandling/ImggAggregateWavelengths.h"

namespace Mantid {
namespace DataHandling {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImggAggregateWavelengths)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ImggAggregateWavelengths::name() const { return "ImggAggregateWavelengths"; }

/// Algorithm's version for identification. @see Algorithm::version
int ImggAggregateWavelengths::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ImggAggregateWavelengths::category() const {
  return "DataHandling\\Imaging";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ImggAggregateWavelengths::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ImggAggregateWavelengths::init() {
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
void ImggAggregateWavelengths::exec() {
  // TODO Auto-generated execute stub
}

} // namespace DataHandling
} // namespace Mantid
