// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DeleteWorkspaces.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm
DECLARE_ALGORITHM(DeleteWorkspaces)

/// Initialize the algorithm properties
void DeleteWorkspaces::init() {
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>(
                      "WorkspaceList", boost::make_shared<API::ADSValidator>()),
                  "A list of the workspaces to delete.");
}

/// Execute the algorithm
void DeleteWorkspaces::exec() {
  const std::vector<std::string> wsNames = getProperty("WorkspaceList");

  // Set up progress reporting
  API::Progress prog(this, 0.0, 1.0, wsNames.size());

  for (const auto &wsName : wsNames) {
    // run delete workspace as a child algorithm
    if (Mantid::API::AnalysisDataService::Instance().doesExist(wsName)) {
      // The existence of input workspaces should have been verified when
      // properties were set. If a workspace is missing, it was probably
      // a group workspace whose contents were deleted before the group
      // itself.
      auto deleteAlg = createChildAlgorithm("DeleteWorkspace", -1, -1, false);
      deleteAlg->initialize();
      deleteAlg->setPropertyValue("Workspace", wsName);
      bool success = deleteAlg->execute();
      if (!deleteAlg->isExecuted() || !success) {
        g_log.error() << "Failed to delete " << wsName << ".\n";
      }
    }
    prog.report();
  }
}
} // namespace Algorithms
} // namespace Mantid
