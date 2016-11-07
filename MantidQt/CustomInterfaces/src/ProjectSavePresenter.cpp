#include "MantidQtCustomInterfaces/ProjectSavePresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

#include <algorithm>
#include <iterator>
#include <vector>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

ProjectSavePresenter::ProjectSavePresenter(IProjectSaveView *view)
  : m_view(view)
{
  auto workspaces = getWorkspaces();
  auto workspaceNames = getWorkspaceNames(workspaces);
  auto windows = m_view->getWindows();
  m_view->updateWorkspacesList(workspaceNames);
}

std::vector<Workspace_sptr> ProjectSavePresenter::getWorkspaces() const {
  auto &ads = AnalysisDataService::Instance();
  return ads.getObjects();
}

std::set<std::string> ProjectSavePresenter::getWorkspaceNames(const std::vector<Workspace_sptr> &workspaces) const
{
  std::set<std::string> workspaceNames;
  std::transform(workspaces.begin(), workspaces.end(),
                 std::inserter(workspaceNames, workspaceNames.end()),
                 [](const Workspace_sptr ws) { return ws->name(); });
  return workspaceNames;
}
