// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ProjectSaveModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/WindowIcons.h"
#include "MantidQtWidgets/Common/WorkspaceIcons.h"

#include <unordered_set>

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

/**
 * Construct a new model with a list of window handles
 * @param windows :: vector of handles to windows open in Mantid
 * @param activePythonInterfaces The list of active Python interfaces
 */
ProjectSaveModel::ProjectSaveModel(
    std::vector<IProjectSerialisable *> windows,
    std::vector<std::string> activePythonInterfaces)
    : m_activePythonInterfaces(std::move(activePythonInterfaces)) {
  auto workspaces = getWorkspaces();
  for (auto &ws : workspaces) {
    std::pair<std::string, std::vector<IProjectSerialisable *>> item(
        ws->getName(), std::vector<IProjectSerialisable *>());
    m_workspaceWindows.insert(item);
  }

  for (auto window : windows) {
    auto wsNames = window->getWorkspaceNames();
    // if the window is not associated with any workspaces
    // then track it so we can always add it to the included
    // window list
    if (wsNames.size() == 0) {
      m_unattachedWindows.push_back(window);
      continue;
    }

    // otherwise add a reference mapping the window to the
    // it's various connected workspaces
    for (auto &name : wsNames) {
      m_workspaceWindows[name].push_back(window);
    }
  }
}

/**
 * Get windows which are associated with a given workspace name
 * @param wsName :: the name of the workspace to get window for
 * @return vector of window handles for the workspace
 */
std::vector<IProjectSerialisable *>
ProjectSaveModel::getWindows(const std::string &wsName) const {
  if (hasWindows(wsName)) {
    return m_workspaceWindows.at(wsName);
  }

  return std::vector<IProjectSerialisable *>();
}

/**
 * Get unique windows for a list of workspace names
 * @param wsNames :: vector of workspace names to get associated windows for
 * @return an ordered vector of unique window handles sorted by window name
 */
std::vector<IProjectSerialisable *> ProjectSaveModel::getUniqueWindows(
    const std::vector<std::string> &wsNames) const {
  std::unordered_set<IProjectSerialisable *> uniqueWindows;

  for (auto &name : wsNames) {
    for (auto window : getWindows(name)) {
      uniqueWindows.insert(window);
    }
  }

  std::vector<IProjectSerialisable *> windows(uniqueWindows.cbegin(),
                                              uniqueWindows.cend());
  std::sort(windows.begin(), windows.end(),
            [](IProjectSerialisable *lhs, IProjectSerialisable *rhs) {
              return lhs->getWindowName() < rhs->getWindowName();
            });

  return windows;
}

/**
 * Get all unique window names for a list of workspaces
 *
 * @param wsNames :: vector of workspace names to get associated window names
 * for
 * @return an ordered vector of unique window names sorted alphabetically
 */
std::vector<std::string> ProjectSaveModel::getWindowNames(
    const std::vector<std::string> &wsNames) const {
  std::vector<std::string> names;
  auto windows = getUniqueWindows(wsNames);
  names.reserve(windows.size());
  for (auto window : windows) {
    names.emplace_back(window->getWindowName());
  }
  return names;
}

/**
 * Get all workspace names in the model
 * @return vector of all workspace names in the model
 */
std::vector<std::string> ProjectSaveModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_workspaceWindows.size());
  for (auto &item : m_workspaceWindows) {
    names.emplace_back(item.first);
  }

  std::sort(names.begin(), names.end());
  return names;
}

/**
 * Get all workspace names in the model
 * @return vector of all python interfaces names in the model
 */
std::vector<std::string> ProjectSaveModel::getAllPythonInterfaces() const {
  return m_activePythonInterfaces;
}

/**
 * Get window information for a selection of workspaces
 * @param wsNames :: vector of workspace names to find associated windows for
 * @param includeUnattached :: whether to append windows not attached to any
 * workspace
 * @return vector of window info objects associated with the workpaces
 */
std::vector<WindowInfo>
ProjectSaveModel::getWindowInformation(const std::vector<std::string> &wsNames,
                                       bool includeUnattached) const {
  std::vector<WindowInfo> winInfo;

  for (auto window : getUniqueWindows(wsNames)) {
    auto info = makeWindowInfoObject(window);
    winInfo.push_back(info);
  }

  if (includeUnattached) {
    for (const auto window : m_unattachedWindows) {
      auto info = makeWindowInfoObject(window);
      winInfo.push_back(info);
    }
  }

  return winInfo;
}

WindowInfo
ProjectSaveModel::makeWindowInfoObject(IProjectSerialisable *window) const {
  WindowIcons icons;
  WindowInfo info;
  info.name = window->getWindowName();
  info.type = window->getWindowType();
  info.icon_id = icons.getIconID(window->getWindowType());
  return info;
}

/**
 * Get workspace information for all workspaces
 * @return vector of workspace info objects for all workspaces
 */
std::vector<WorkspaceInfo> ProjectSaveModel::getWorkspaceInformation() const {
  std::vector<WorkspaceInfo> wsInfo;

  auto items = AnalysisDataService::Instance().topLevelItems();
  for (auto item : items) {
    auto ws = item.second;
    auto info = makeWorkspaceInfoObject(ws);

    if (ws->id() == "WorkspaceGroup") {
      auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
      for (int i = 0; i < group->getNumberOfEntries(); ++i) {
        auto subInfo = makeWorkspaceInfoObject(group->getItem(i));
        info.subWorkspaces.push_back(subInfo);
      }
    }

    wsInfo.push_back(info);
  }

  return wsInfo;
}

/**
 * Get all workspaces from the ADS
 * @return vector of workspace handles from the ADS
 */
std::vector<Workspace_sptr> ProjectSaveModel::getWorkspaces() const {
  auto &ads = AnalysisDataService::Instance();
  return ads.getObjects();
}

WorkspaceInfo
ProjectSaveModel::makeWorkspaceInfoObject(Workspace_const_sptr ws) const {
  WorkspaceIcons icons;
  WorkspaceInfo info;
  info.name = ws->getName();
  info.numWindows = getWindows(ws->getName()).size();
  info.size = ws->getMemorySizeAsStr();
  info.icon_id = icons.getIconID(ws->id());
  info.type = ws->id();
  return info;
}

/**
 * Check is a workspace has any windows associated with it
 * @param wsName :: the name of workspace
 * @return whether the workspace has > 0 windows associated with it
 */
bool ProjectSaveModel::hasWindows(const std::string &wsName) const {
  auto item = m_workspaceWindows.find(wsName);
  if (item != m_workspaceWindows.end()) {
    return item->second.size() > 0;
  }

  return false;
}
