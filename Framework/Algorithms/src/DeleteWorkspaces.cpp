#include "MantidAlgorithms/DeleteWorkspaces.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm
DECLARE_ALGORITHM(DeleteWorkspaces)

/// Initialize the algorithm properties
void DeleteWorkspaces::init() {
  declareProperty(Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
                      "WorkspaceList", boost::make_shared<API::ADSValidator>()),
                  "A list of the workspaces to delete.");
}

/// Execute the algorithm
void DeleteWorkspaces::exec() {
  const std::vector<std::string> wsNames = getProperty("WorkspaceList");

  // Set up progress reporting
  API::Progress prog(this, 0.0, 1.0, wsNames.size());

  for (auto wsName : wsNames) {
    // run delete workspace as a child algorithm
    auto deleteAlg = createChildAlgorithm("DeleteWorkspace", -1, -1, false);
    deleteAlg->initialize();

    deleteAlg->setPropertyValue("Workspace", wsName);
    bool success = deleteAlg->execute();
    if (!deleteAlg->isExecuted() || !success) {
      g_log.error() << "Failed to delete wsName. \n";
    }
    prog.report();
  }
}
}
}
