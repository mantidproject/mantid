// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidParallel/Communicator.h"

#include "Poco/Glob.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(GroupWorkspaces)

using namespace API;
using namespace Kernel;

/// Initialisation method
void GroupWorkspaces::init() {

  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          "InputWorkspaces", boost::make_shared<ADSValidator>(true, true)),
      "Names of the Input Workspaces to Group");
  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("GlobExpression", ""),
      "Add all Workspaces that match Glob expression to Group");
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "",
                                                          Direction::Output),
      "Name of the workspace to be created as the output of grouping ");
}

/** Executes the algorithm
 *  @throw std::runtime_error If the selected workspaces are not of same types
 */
void GroupWorkspaces::exec() {
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");

  const std::string globExpression = getProperty("GlobExpression");

  // Clear WorkspaceGroup in case algorithm instance is reused.
  m_group = nullptr;

  if (!inputWorkspaces.empty())
    addToGroup(inputWorkspaces);
  if (!globExpression.empty())
    addToGroup(globExpression);
  if ((m_group == nullptr) || m_group->isEmpty())
    throw std::invalid_argument(
        "Glob pattern " + globExpression +
        " does not match any workspace names in the ADS.");
  setProperty("OutputWorkspace", m_group);
  auto &notifier = API::AnalysisDataService::Instance().notificationCenter;
  notifier.postNotification(new WorkspacesGroupedNotification(inputWorkspaces));
}

std::map<std::string, std::string> GroupWorkspaces::validateInputs() {
  std::map<std::string, std::string> results;
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");
  std::string globExpression = getProperty("GlobExpression");

  for (auto it = globExpression.begin(); it < globExpression.end(); ++it) {
    if (*it == '\\') {
      it = globExpression.erase(it, it + 2);
    }
  }

  if (inputWorkspaces.empty() && globExpression.empty()) {
    results["InputWorkspaces"] =
        "No InputWorkspace names specified. Rerun with a list of workspaces "
        "names or a glob expression";
    return results;
  }

  // ADSValidator already checks names in inputWorkspaces

  if (!globExpression.empty()) {
    // This is only a sanity check. If may fail to detect subtle errors in
    // complex expressions.
    if (globExpression.find_first_of("*?[]") == std::string::npos) {
      results["GlobExpression"] = "Expression is expected to contain one or "
                                  "more of the following characters: *?[";
      return results;
    }
    if (std::count(globExpression.cbegin(), globExpression.cend(), '[') !=
        std::count(globExpression.cbegin(), globExpression.cend(), ']')) {
      results["GlobExpression"] = "Expression has a mismatched number of []";
      return results;
    }
  }
  return results;
}

/**
 * Add a list of names to the new group
 * @param globExpression glob pattern for selecting names from the ADS
 */
void GroupWorkspaces::addToGroup(const std::string &globExpression) {

  Poco::Glob glob(globExpression);

  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  const auto names = ads.topLevelItems();
  for (const auto &name : names) {
    if (glob.match(name.first)) {
      addToGroup(name.second);
    }
  }
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
             m_group->storageMode() != workspace->storageMode()) {
      throw std::runtime_error(
          "WorkspaceGroup with mixed Parallel::Storage mode is not supported.");
    }
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
} // namespace Algorithms
} // namespace Mantid
