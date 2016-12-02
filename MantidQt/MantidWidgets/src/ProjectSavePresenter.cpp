#include "MantidQtMantidWidgets/ProjectSavePresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

#include <QFileInfo>
#include <QDir>

#include <algorithm>
#include <iterator>
#include <vector>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

ProjectSavePresenter::ProjectSavePresenter(IProjectSaveView *view)
    : m_view(view), m_model(m_view->getWindows()) {
  auto workspaceNames = m_model.getWorkspaceNames();
  auto names = m_model.getWindowNames(workspaceNames);
  m_view->updateIncludedWindowsList(names);
  m_view->updateWorkspacesList(workspaceNames);
}

void ProjectSavePresenter::notify(Notification notification) {
  switch (notification) {
  case Notification::CheckWorkspace:
    includeWindowsForCheckedWorkspace();
    break;
  case Notification::UncheckWorkspace:
    excludeWindowsForUncheckedWorkspace();
    break;
  case Notification::PrepareProjectFolder:
    prepareProjectFolder();
  }
}

void ProjectSavePresenter::includeWindowsForCheckedWorkspace() {
  auto wsNames = m_view->getCheckedWorkspaceNames();
  auto names = m_model.getWindowNames(wsNames);
  m_view->updateIncludedWindowsList(names);
  m_view->removeFromExcludedWindowsList(names);
}

void ProjectSavePresenter::excludeWindowsForUncheckedWorkspace() {
  auto wsNames = m_view->getUncheckedWorkspaceNames();
  auto names = m_model.getWindowNames(wsNames);
  m_view->updateExcludedWindowsList(names);
  m_view->removeFromIncludedWindowsList(names);
}

void ProjectSavePresenter::prepareProjectFolder() {
  auto path = m_view->getProjectPath();

  QFileInfo fileInfo(path);
  bool isFile = fileInfo.filePath().endsWith(".mantid") ||
                fileInfo.filePath().endsWith(".mantid.gz");

  if (!isFile) {
    QDir directory(path);
    if (!directory.exists()) {
      // Make the directory
      directory.mkdir(path);
    }

    auto projectFileName = directory.dirName();
    projectFileName.append(".mantid");
    path = directory.absoluteFilePath(projectFileName);
  } else {
    path = fileInfo.absoluteFilePath();
  }

  m_view->setProjectPath(path);
}
