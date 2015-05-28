#include "MantidAlgorithms/ConvertDiffCal.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertDiffCal)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertDiffCal::ConvertDiffCal() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertDiffCal::~ConvertDiffCal() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvertDiffCal::name() const { return "ConvertDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConvertDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertDiffCal::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertDiffCal::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertDiffCal::init() {
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
void ConvertDiffCal::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid