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
  for(auto name : workspaceNames) {
    if(m_model.hasWindows(name)) {
      auto windows = m_model.getWindows(name);
    }
  }
  m_view->updateWorkspacesList(workspaceNames);
}
