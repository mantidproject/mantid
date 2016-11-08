#include "MantidQtCustomInterfaces/ProjectSavePresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <vector>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

ProjectSavePresenter::ProjectSavePresenter(IProjectSaveView *view)
  : m_view(view), m_model(m_view->getWindows())
{
  auto workspaceNames = m_model.getWorkspaceNames();
  std::unordered_set<std::string> windowNames;

  for(auto &name : workspaceNames) {
    auto windows = m_model.getWindows(name);
    for (auto window : windows) {
      windowNames.insert(window->getWindowName());
    }
  }

  std::vector<std::string> names(windowNames.cbegin(), windowNames.cend());
  std::sort(names.begin(), names.end());

  m_view->updateIncludedWindowsList(names);
  m_view->updateWorkspacesList(workspaceNames);
}
