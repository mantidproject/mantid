#include "MantidAlgorithms/DeleteWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm
DECLARE_ALGORITHM(DeleteWorkspace)

/// Initialize the algorithm properties
void DeleteWorkspace::init() {
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
                      "Workspace", "", Kernel::Direction::Input),
                  "Name of the workspace to delete.");
}

/// Execute the algorithm
void DeleteWorkspace::exec() {
  using API::AnalysisDataService;
  using API::AnalysisDataServiceImpl;
  AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
  const std::string wsName = getProperty("Workspace");
  dataStore.remove(wsName); // Logs if it doesn't exist
}
}
}
