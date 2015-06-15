#include "MantidDataHandling/SaveDiffCal.h"

namespace Mantid {
namespace DataHandling {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDiffCal)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveDiffCal::SaveDiffCal() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveDiffCal::~SaveDiffCal() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SaveDiffCal::name() const { return "SaveDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveDiffCal::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveDiffCal::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveDiffCal::init() {
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
void SaveDiffCal::exec() {
  // TODO Auto-generated execute stub
}

} // namespace DataHandling
} // namespace Mantid