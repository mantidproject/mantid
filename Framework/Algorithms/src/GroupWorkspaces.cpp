#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidParallel/Communicator.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(GroupWorkspaces)

using namespace API;
using namespace Kernel;

/// Initialisation method
void GroupWorkspaces::init() {

  declareProperty(Kernel::make_unique<ArrayProperty<std::string>>(
                      "InputWorkspaces", boost::make_shared<ADSValidator>()),
                  "Names of the Input Workspaces to Group");
  declareProperty(
      make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "",
                                                     Direction::Output),
      "Name of the workspace to be created as the output of grouping ");
}

/** Executes the algorithm
 *  @throw std::runtime_error If the selected workspaces are not of same types
 */
void GroupWorkspaces::exec() {
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");

  // Clear WorkspaceGroup in case algorithm instance is reused.
  m_group = nullptr;
  addToGroup(inputWorkspaces);

  setProperty("OutputWorkspace", m_group);
  auto &notifier = API::AnalysisDataService::Instance().notificationCenter;
  notifier.postNotification(new WorkspacesGroupedNotification(inputWorkspaces));
}

/**
 * Add a list of names to the new group
 * @param names The list of names to add from the ADS
 */
void GroupWorkspaces::addToGroup(const std::vector<std::string> &names) {

  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  for (const auto &name : names) {
    auto workspace = ads.retrieve(name);
    addToGroup(workspace);
  }
}

/**
 * If it is a group it is unrolled and each member added
 * @param workspace A pointer to the workspace to add
 */
void GroupWorkspaces::addToGroup(const API::Workspace_sptr &workspace) {
  auto localGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace);
  if (localGroup) {
    addToGroup(localGroup->getNames());
    // Remove the group from the ADS
    AnalysisDataService::Instance().remove(workspace->getName());
  } else {
    if (!m_group)
      m_group = boost::make_shared<WorkspaceGroup>(workspace->storageMode());
    else if (communicator().size() != 1 &&
             m_group->storageMode() != workspace->storageMode())
      throw std::runtime_error(
          "WorkspaceGroup with mixed Parallel::Storage mode is not supported.");
    m_group->addWorkspace(workspace);
  }
}

Parallel::ExecutionMode GroupWorkspaces::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  static_cast<void>(storageModes);
  const std::vector<std::string> names = getProperty("InputWorkspaces");
  const auto ws = AnalysisDataService::Instance().retrieve(names.front());
  return Parallel::getCorrespondingExecutionMode(ws->storageMode());
}
}
}
