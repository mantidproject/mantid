#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/ProjectSaveModel.h"

#include <unordered_set>

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::CustomInterfaces;

ProjectSaveModel::ProjectSaveModel(std::vector<IProjectSerialisable*> windows)
{
  auto workspaces = getWorkspaces();
  for (auto & ws : workspaces) {
    std::pair<std::string, std::vector<IProjectSerialisable*>> item(ws->name(), std::vector<IProjectSerialisable*>());
    m_workspaceWindows.insert(item);
  }

  for (auto window : windows) {
    auto wsNames = window->getWorkspaceNames();
    for (auto & name : wsNames) {
      m_workspaceWindows[name].push_back(window);
    }
  }
}

std::vector<IProjectSerialisable *> ProjectSaveModel::getWindows(const std::string &wsName) const
{
  if(hasWindows(wsName)) {
    return m_workspaceWindows.at(wsName);
  }

  return std::vector<IProjectSerialisable*>();
}

std::vector<std::string> ProjectSaveModel::getWindowNames(const std::vector<std::string> &wsNames) const
{
  std::unordered_set<std::string> windowNames;

  for(auto &name : wsNames) {
    auto windows = getWindows(name);
    for (auto window : windows) {
      windowNames.insert(window->getWindowName());
    }
  }

  std::vector<std::string> names(windowNames.cbegin(), windowNames.cend());
  std::sort(names.begin(), names.end());
  return names;
}

std::vector<std::string> ProjectSaveModel::getWorkspaceNames() const
{
  std::vector<std::string> names;
  for(auto &item : m_workspaceWindows) {
    names.push_back(item.first);
  }

  std::sort(names.begin(), names.end());
  return names;
}

std::vector<Workspace_sptr> ProjectSaveModel::getWorkspaces() const {
  auto &ads = AnalysisDataService::Instance();
  return ads.getObjects();
}

bool ProjectSaveModel::hasWindows(const std::string& wsName) const
{
  auto item = m_workspaceWindows.find(wsName);
  if (item != m_workspaceWindows.end()) {
    return item->second.size() > 0;
  }

  return false;
}
