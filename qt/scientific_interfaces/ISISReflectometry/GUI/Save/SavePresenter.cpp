// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "SavePresenter.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "ISaveView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "Poco/File.h"
#include "Poco/Path.h"

#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;

/**
 * @param saver :: The model to use to save the files
 * @param view :: The view we are handling
 */
SavePresenter::SavePresenter(ISaveView *view,
                             std::unique_ptr<IAsciiSaver> saver)
    : m_view(view), m_saver(std::move(saver)), m_shouldAutosave(false) {

  m_view->subscribe(this);
}

void SavePresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

void SavePresenter::notifyPopulateWorkspaceList() { populateWorkspaceList(); }

void SavePresenter::notifyFilterWorkspaceList() { filterWorkspaceNames(); }

void SavePresenter::notifyPopulateParametersList() { populateParametersList(); }

void SavePresenter::notifySaveSelectedWorkspaces() { saveSelectedWorkspaces(); }

void SavePresenter::notifySuggestSaveDir() { suggestSaveDir(); }

void SavePresenter::notifyAutosaveDisabled() { disableAutosave(); }

void SavePresenter::notifyAutosaveEnabled() { enableAutosave(); }

void SavePresenter::notifySavePathChanged() { onSavePathChanged(); }

bool SavePresenter::isProcessing() const {
  return m_mainPresenter->isProcessing();
}

bool SavePresenter::isAutoreducing() const {
  return m_mainPresenter->isAutoreducing();
}

/** Tells the view to update the enabled/disabled state of all relevant
 * widgets based on whether processing is in progress or not.
 */
void SavePresenter::updateWidgetEnabledState() const {
  if (isProcessing() || isAutoreducing()) {
    m_view->disableAutosaveControls();
    if (shouldAutosave())
      m_view->disableFileFormatAndLocationControls();
    else
      m_view->enableFileFormatAndLocationControls();
  } else {
    m_view->enableAutosaveControls();
    m_view->enableFileFormatAndLocationControls();
  }
}

void SavePresenter::reductionPaused() {
  populateWorkspaceList();
  updateWidgetEnabledState();
}

void SavePresenter::reductionResumed() { updateWidgetEnabledState(); }

void SavePresenter::autoreductionPaused() { updateWidgetEnabledState(); }

void SavePresenter::autoreductionResumed() { updateWidgetEnabledState(); }

void SavePresenter::enableAutosave() {
  if (isValidSaveDirectory(m_view->getSavePath())) {
    m_shouldAutosave = true;
  } else {
    m_shouldAutosave = false;
    m_view->disallowAutosave();
    errorInvalidSaveDirectory();
  }
}

void SavePresenter::disableAutosave() { m_shouldAutosave = false; }

void SavePresenter::onSavePathChanged() {
  if (shouldAutosave() && !isValidSaveDirectory(m_view->getSavePath()))
    warnInvalidSaveDirectory();
}

bool SavePresenter::shouldAutosave() const { return m_shouldAutosave; }

/** Fills the 'List of Workspaces' widget with the names of all available
 * workspaces
 */
void SavePresenter::populateWorkspaceList() {
  m_view->clearWorkspaceList();
  m_view->setWorkspaceList(getAvailableWorkspaceNames());
}

/** Filters the names in the 'List of Workspaces' widget
 */
void SavePresenter::filterWorkspaceNames() {
  m_view->clearWorkspaceList();

  std::string filter = m_view->getFilter();
  bool regexCheck = m_view->getRegexCheck();
  auto wsNames = getAvailableWorkspaceNames();
  std::vector<std::string> validNames(wsNames.size());
  auto it = validNames.begin();

  if (regexCheck) {
    // Use regex search to find names that contain the filter sequence
    try {
      boost::regex rgx(filter);
      it = std::copy_if(
          wsNames.begin(), wsNames.end(), validNames.begin(),
          [rgx](std::string s) { return boost::regex_search(s, rgx); });
      m_view->showFilterEditValid();
    } catch (boost::regex_error &) {
      m_view->showFilterEditInvalid();
    }
  } else {
    // Otherwise simply add names where the filter string is found in
    it = std::copy_if(wsNames.begin(), wsNames.end(), validNames.begin(),
                      [filter](std::string s) {
                        return s.find(filter) != std::string::npos;
                      });
  }

  validNames.resize(std::distance(validNames.begin(), it));
  m_view->setWorkspaceList(validNames);
}

/** Fills the 'List of Logged Parameters' widget with the parameters of the
 * currently selected workspace
 */
void SavePresenter::populateParametersList() {
  m_view->clearParametersList();

  std::string wsName = m_view->getCurrentWorkspaceName();
  std::vector<std::string> logs;
  const auto &properties = AnalysisDataService::Instance()
                               .retrieveWS<MatrixWorkspace>(wsName)
                               ->run()
                               .getProperties();
  for (auto it = properties.begin(); it != properties.end(); it++) {
    logs.push_back((*it)->name());
  }
  m_view->setParametersList(logs);
}

bool SavePresenter::isValidSaveDirectory(std::string const &directory) {
  return m_saver->isValidSaveDirectory(directory);
}

void SavePresenter::warnInvalidSaveDirectory() {
  m_view->warnInvalidSaveDirectory();
}

void SavePresenter::errorInvalidSaveDirectory() {
  m_view->errorInvalidSaveDirectory();
}

NamedFormat SavePresenter::formatFromIndex(int formatIndex) const {
  switch (formatIndex) {
  case 0:
    return NamedFormat::Custom;
  case 1:
    return NamedFormat::ThreeColumn;
  case 2:
    return NamedFormat::ANSTO;
  case 3:
    return NamedFormat::ILLCosmos;
  default:
    throw std::runtime_error("Unknown save format.");
  }
}

FileFormatOptions SavePresenter::getSaveParametersFromView() const {
  return FileFormatOptions(
      /*format=*/formatFromIndex(m_view->getFileFormatIndex()),
      /*prefix=*/m_view->getPrefix(),
      /*includeTitle=*/m_view->getTitleCheck(),
      /*separator=*/m_view->getSeparator(),
      /*includeQResolution=*/m_view->getQResolutionCheck());
}

void SavePresenter::saveWorkspaces(
    std::vector<std::string> const &workspaceNames,
    std::vector<std::string> const &logParameters) {
  auto savePath = m_view->getSavePath();
  if (m_saver->isValidSaveDirectory(savePath))
    m_saver->save(savePath, workspaceNames, logParameters,
                  getSaveParametersFromView());
  else
    errorInvalidSaveDirectory();
}

/** Saves workspaces with the names specified. */
void SavePresenter::saveWorkspaces(
    std::vector<std::string> const &workspaceNames) {
  auto selectedLogParameters = m_view->getSelectedParameters();
  saveWorkspaces(workspaceNames, selectedLogParameters);
}

/** Saves selected workspaces */
void SavePresenter::saveSelectedWorkspaces() {
  // Check that at least one workspace has been selected for saving
  auto workspaceNames = m_view->getSelectedWorkspaces();
  if (workspaceNames.empty()) {
    m_view->noWorkspacesSelected();
  } else {
    try {
      saveWorkspaces(workspaceNames);
    } catch (std::exception &e) {
      m_view->cannotSaveWorkspaces(e.what());
    } catch (...) {
      m_view->cannotSaveWorkspaces();
    }
  }
}

/** Suggests a save directory and sets it in the 'Save path' text field
 */
void SavePresenter::suggestSaveDir() {
  std::string path = Mantid::Kernel::ConfigService::Instance().getString(
      "defaultsave.directory");
  m_view->setSavePath(path);
}

/** Obtains all available workspace names to save
 * @return :: list of workspace names
 */
std::vector<std::string> SavePresenter::getAvailableWorkspaceNames() {
  auto allNames = AnalysisDataService::Instance().getObjectNames();
  // Exclude workspace groups and table workspaces as they cannot be saved to
  // ascii
  std::vector<std::string> validNames;
  std::remove_copy_if(
      allNames.begin(), allNames.end(), std::back_inserter(validNames),
      [](const std::string &wsName) {
        return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
                   wsName) ||
               AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
                   wsName);
      });

  return validNames;
}
} // namespace CustomInterfaces
} // namespace MantidQt
