#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidQtAPI/WindowIcons.h"
#include "MantidQtAPI/WorkspaceIcons.h"
#include "MantidQtMantidWidgets/ProjectSaveModel.h"

#include <unordered_set>

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

/**
 * Construct a new model with a list of window handles
 * @param windows :: vector of handles to windows open in Mantid
 */
ProjectSaveModel::ProjectSaveModel(
    std::vector<IProjectSerialisable *> windows) {
  auto workspaces = getWorkspaces();
  for (auto &ws : workspaces) {
    // Currently we don't support saving workspace groups.
    // so, here just skip any and just include the other workspaces
    if(ws->id() == "WorkspaceGroup")
      continue;

    std::pair<std::string, std::vector<IProjectSerialisable *>> item(
        ws->name(), std::vector<IProjectSerialisable *>());
    m_workspaceWindows.insert(item);
  }

  for (auto window : windows) {
    auto wsNames = window->getWorkspaceNames();
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
 * @param wsNames :: vector of workspace names to get associated window names for
 * @return an ordered vector of unique window names sorted alphabetically
 */
std::vector<std::string> ProjectSaveModel::getWindowNames(
    const std::vector<std::string> &wsNames) const {
  std::unordered_set<std::string> windowNames;

  for (auto &name : wsNames) {
    auto windows = getWindows(name);
    for (auto window : windows) {
      windowNames.insert(window->getWindowName());
    }
  }

  std::vector<std::string> names(windowNames.cbegin(), windowNames.cend());
  std::sort(names.begin(), names.end());
  return names;
}

/**
 * Get all workspace names in the model
 * @return vector of all workspace names in the model
 */
std::vector<std::string> ProjectSaveModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  for (auto &item : m_workspaceWindows) {
    names.push_back(item.first);
  }

  std::sort(names.begin(), names.end());
  return names;
}

/**
 * Get window information for a selection of workspaces
 * @param wsNames :: vector of workspace names to find associated windows for
 * @return vector of window info objects associated with the workpaces
 */
std::vector<WindowInfo> ProjectSaveModel::getWindowInformation(
    const std::vector<std::string> &wsNames) const {
  std::vector<WindowInfo> winInfo;
  WindowIcons icons;

  for(auto window : getUniqueWindows(wsNames)) {
    WindowInfo info;
    info.name = window->getWindowName();
    info.type = window->getWindowType();
    info.icon_id = icons.getIconID(window->getWindowType());
    winInfo.push_back(info);
  }

  return winInfo;
}

/**
 * Get workspace information for all workspaces
 * @return vector of workspace info objects for all workspaces
 */
std::vector<WorkspaceInfo> ProjectSaveModel::getWorkspaceInformation() const {
  WorkspaceIcons icons;
  std::vector<WorkspaceInfo> wsInfo;

  for (auto item : m_workspaceWindows) {
    auto ws = AnalysisDataService::Instance().retrieve(item.first);
    WorkspaceInfo info;
    auto id = ws->id();

    info.name = ws->name();
    info.numWindows = getWindows(ws->name()).size();
    info.size = ws->getMemorySizeAsStr();
    info.icon_id = icons.getIconID(id);
    info.type = id;

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
