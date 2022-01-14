// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DeleteWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm
DECLARE_ALGORITHM(DeleteWorkspace)

/// Initialize the algorithm properties
void DeleteWorkspace::init() {
  declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>("Workspace", "", Kernel::Direction::Input),
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

bool DeleteWorkspace::checkGroups() {
  AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
  const std::string wsName = getProperty("Workspace");
  if (dataStore.doesExist(wsName)) {
    auto wsPtr = dataStore.retrieve(wsName);
    if (wsPtr->isGroup() && !dataStore.retrieveWS<WorkspaceGroup>(wsName)->isEmpty()) {
      return Algorithm::checkGroups();
    }
    return false;
  } else {
    return Algorithm::checkGroups();
  }
}

} // namespace Mantid::Algorithms
