// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ProjectSavePresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <iterator>
#include <vector>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

/**
 * Construct a new presenter with the given view
 * @param view :: a handle to a view for this presenter
 */
ProjectSavePresenter::ProjectSavePresenter(IProjectSaveView *view)
    : m_view(view),
      m_model(m_view->getWindows(), m_view->getAllPythonInterfaces()) {
  auto workspaceNames = m_model.getWorkspaceNames();
  auto info = m_model.getWorkspaceInformation();
  auto winInfo = m_model.getWindowInformation(workspaceNames, true);
  m_view->updateIncludedWindowsList(winInfo);
  m_view->updateWorkspacesList(info);
  m_view->updateInterfacesList(m_model.getAllPythonInterfaces());
}

/**
 * Notify the presenter to perform an action
 * @param notification :: notification to choose the action to perform
 */
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

/**
 * @brief Check if the user needs to be warned about saving a large project.
 *
 * @param wsNames Vector of workspace names to check.
 * @return true If a warning is required.
 * @return false If a warning is not neccessary.
 */
bool ProjectSavePresenter::needsSizeWarning(
    const std::vector<std::string> &wsNames) {
  if (!wsNames.empty()) {
    return m_model.needsSizeWarning(wsNames);
  }
  return false;
}

/**
 * Update the view with a new list of windows that are included in project
 * saving based on the currently checked workspaces
 */
void ProjectSavePresenter::includeWindowsForCheckedWorkspace() {
  auto wsNames = m_view->getCheckedWorkspaceNames();
  auto names = m_model.getWindowNames(wsNames);
  // true flag gets windows unattached to workspaces as well
  auto info = m_model.getWindowInformation(wsNames, true);
  m_view->updateIncludedWindowsList(info);
  m_view->removeFromExcludedWindowsList(names);
}

/**
 * Update the view with a new list of windows that are excluded from project
 * saving based on the currently checked workspaces
 */
void ProjectSavePresenter::excludeWindowsForUncheckedWorkspace() {
  auto wsNames = m_view->getUncheckedWorkspaceNames();
  auto names = m_model.getWindowNames(wsNames);
  auto info = m_model.getWindowInformation(wsNames);
  m_view->updateExcludedWindowsList(info);
  m_view->removeFromIncludedWindowsList(names);
}

/**
 * Prepare a project folder for serialistion
 *
 * This will check the file path and if necessary create a new project folder
 */
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
