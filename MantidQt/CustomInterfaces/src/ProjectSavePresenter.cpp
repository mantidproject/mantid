#include "MantidQtCustomInterfaces/ProjectSavePresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

#include <algorithm>
#include <iterator>
#include <vector>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

ProjectSavePresenter::ProjectSavePresenter(IProjectSaveView *view)
  : m_view(view), m_model(m_view->getWindows())
{
  auto workspaceNames = m_model.getWorkspaceNames();
  auto names = m_model.getWindowNames(workspaceNames);
  m_view->updateIncludedWindowsList(names);
  m_view->updateWorkspacesList(workspaceNames);
}

void ProjectSavePresenter::notify(Notification notification)
{
  switch(notification) {
  case Notification::CheckWorkspace:
    includeWindowsForCheckedWorkspace();
    break;
  case Notification::UncheckWorkspace:
    excludeWindowsForUncheckedWorkspace();
    break;
  }
}

void ProjectSavePresenter::includeWindowsForCheckedWorkspace()
{
  auto wsNames = m_view->getCheckedWorkspaceNames();
  auto names = m_model.getWindowNames(wsNames);
  m_view->updateIncludedWindowsList(names);
}

void ProjectSavePresenter::excludeWindowsForUncheckedWorkspace()
{
  auto wsNames = m_view->getUncheckedWorkspaceNames();
  auto names = m_model.getWindowNames(wsNames);
  m_view->updateExcludedWindowsList(names);
}

