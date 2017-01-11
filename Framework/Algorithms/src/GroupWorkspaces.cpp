#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(GroupWorkspaces)

using namespace API;
using namespace Kernel;

/// Default constructor
GroupWorkspaces::GroupWorkspaces() : API::Algorithm(), m_group() {}

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

  m_group = boost::make_shared<WorkspaceGroup>();
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
    m_group->addWorkspace(workspace);
  }
}
}
}
