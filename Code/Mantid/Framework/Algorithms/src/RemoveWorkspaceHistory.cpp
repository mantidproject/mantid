#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/RemoveWorkspaceHistory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemoveWorkspaceHistory)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
RemoveWorkspaceHistory::RemoveWorkspaceHistory() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
RemoveWorkspaceHistory::~RemoveWorkspaceHistory() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string RemoveWorkspaceHistory::name() const {
  return "RemoveWorkspaceHistory";
}

/// Algorithm's version for identification. @see Algorithm::version
int RemoveWorkspaceHistory::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RemoveWorkspaceHistory::category() const { return "Utility"; }

/// Algorithm's summary for identification. @see Algorithm::summary
const std::string RemoveWorkspaceHistory::summary() const {
  return "Removes all algorithm history records from a given workspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RemoveWorkspaceHistory::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("Workspace", "", Direction::InOut),
      "Workspace to remove the algorithm history from.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RemoveWorkspaceHistory::exec() {
  Workspace_sptr ws = getProperty("Workspace");
  ws->history().clearHistory();
}

} // namespace Algorithms
} // namespace Mantid