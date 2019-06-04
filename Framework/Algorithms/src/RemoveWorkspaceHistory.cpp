// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemoveWorkspaceHistory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemoveWorkspaceHistory)

/// Algorithm's name for identification. @see Algorithm::name
const std::string RemoveWorkspaceHistory::name() const {
  return "RemoveWorkspaceHistory";
}

/// Algorithm's version for identification. @see Algorithm::version
int RemoveWorkspaceHistory::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RemoveWorkspaceHistory::category() const {
  return "Utility\\Workspaces";
}

/// Algorithm's summary for identification. @see Algorithm::summary
const std::string RemoveWorkspaceHistory::summary() const {
  return "Removes all algorithm history records from a given workspace.";
}

/** Initialize the algorithm's properties.
 */
void RemoveWorkspaceHistory::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "",
                                                            Direction::InOut),
                  "Workspace to remove the algorithm history from.");
}

/** Execute the algorithm.
 */
void RemoveWorkspaceHistory::exec() {
  Workspace_sptr ws = getProperty("Workspace");
  ws->history().clearHistory();
}

} // namespace Algorithms
} // namespace Mantid
