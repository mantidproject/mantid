#include "MantidDataHandling/LoadDiffCal.h"

namespace Mantid {
namespace DataHandling {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDiffCal)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadDiffCal::LoadDiffCal() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadDiffCal::~LoadDiffCal() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadDiffCal::name() const { return "LoadDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadDiffCal::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadDiffCal::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadDiffCal::init() {
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
void LoadDiffCal::exec() {
  // TODO Auto-generated execute stub
}

} // namespace DataHandling
} // namespace Mantid