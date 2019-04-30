// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflSaveTabPresenter.h"
#include "IReflMainWindowPresenter.h"
#include "IReflSaveTabView.h"
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
ReflSaveTabPresenter::ReflSaveTabPresenter(
    std::unique_ptr<IReflAsciiSaver> saver,
    std::unique_ptr<IReflSaveTabView> view)
    : m_view(std::move(view)), m_saver(std::move(saver)), m_mainPresenter(),
      m_shouldAutosave(false) {

  m_view->subscribe(this);
}

/** Destructor
 */
ReflSaveTabPresenter::~ReflSaveTabPresenter() {}

/** Accept a main presenter
 * @param mainPresenter :: [input] The main presenter
 */
void ReflSaveTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

void ReflSaveTabPresenter::onAnyReductionPaused() {
  populateWorkspaceList();
  m_view->enableAutosaveControls();
  m_view->enableFileFormatAndLocationControls();
}

void ReflSaveTabPresenter::onAnyReductionResumed() {
  m_view->disableAutosaveControls();
  if (shouldAutosave())
    m_view->disableFileFormatAndLocationControls();
}

void ReflSaveTabPresenter::notify(IReflSaveTabPresenter::Flag flag) {
  switch (flag) {
  case populateWorkspaceListFlag:
    populateWorkspaceList();
    break;
  case filterWorkspaceListFlag:
    filterWorkspaceNames();
    break;
  case workspaceParamsFlag:
    populateParametersList();
    break;
  case saveWorkspacesFlag:
    saveSelectedWorkspaces();
    break;
  case suggestSaveDirFlag:
    suggestSaveDir();
    break;
  case autosaveDisabled:
    disableAutosave();
    break;
  case autosaveEnabled:
    enableAutosave();
    break;
  case savePathChanged:
    onSavePathChanged();
  }
}

void ReflSaveTabPresenter::enableAutosave() {
  if (isValidSaveDirectory(m_view->getSavePath())) {
    m_shouldAutosave = true;
  } else {
    m_shouldAutosave = false;
    m_view->disallowAutosave();
    errorInvalidSaveDirectory();
  }
}

void ReflSaveTabPresenter::disableAutosave() { m_shouldAutosave = false; }

void ReflSaveTabPresenter::onSavePathChanged() {
  if (shouldAutosave() && !isValidSaveDirectory(m_view->getSavePath()))
    warnInvalidSaveDirectory();
}

void ReflSaveTabPresenter::completedGroupReductionSuccessfully(
    MantidWidgets::DataProcessor::GroupData const &group,
    std::string const &workspaceName) {
  UNUSED_ARG(group);
  if (shouldAutosave()) {
    try {
      saveWorkspaces(std::vector<std::string>({workspaceName}));
    } catch (InvalidWorkspaceName &) {
      // ignore workspaces that don't exist
    }
  }
}

bool ReflSaveTabPresenter::shouldAutosave() const { return m_shouldAutosave; }

void ReflSaveTabPresenter::completedRowReductionSuccessfully(
    MantidWidgets::DataProcessor::GroupData const &group,
    std::string const &workspaceName) {
  if (!MantidWidgets::DataProcessor::canPostprocess(group) &&
      shouldAutosave()) {
    try {
      saveWorkspaces(std::vector<std::string>({workspaceName}));
    } catch (InvalidWorkspaceName &) {
      // ignore workspaces that don't exist
    }
  }
}

/** Fills the 'List of Workspaces' widget with the names of all available
 * workspaces
 */
void ReflSaveTabPresenter::populateWorkspaceList() {
  m_view->clearWorkspaceList();
  m_view->setWorkspaceList(getAvailableWorkspaceNames());
}

/** Filters the names in the 'List of Workspaces' widget
 */
void ReflSaveTabPresenter::filterWorkspaceNames() {
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
    } catch (boost::regex_error &) {
      m_mainPresenter->giveUserCritical("Error, invalid regular expression\n",
                                        "Invalid regex");
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
void ReflSaveTabPresenter::populateParametersList() {
  m_view->clearParametersList();

  std::string wsName = m_view->getCurrentWorkspaceName();
  std::vector<std::string> logs;
  const auto &properties = AnalysisDataService::Instance()
                               .retrieveWS<MatrixWorkspace>(wsName)
                               ->run()
                               .getProperties();
  for (auto property : properties) {
    logs.push_back(property->name());
  }
  m_view->setParametersList(logs);
}

bool ReflSaveTabPresenter::isValidSaveDirectory(std::string const &directory) {
  return m_saver->isValidSaveDirectory(directory);
}

void ReflSaveTabPresenter::error(std::string const &message,
                                 std::string const &title) {
  m_view->giveUserCritical(message, title);
}

void ReflSaveTabPresenter::warn(std::string const &message,
                                std::string const &title) {
  m_view->giveUserInfo(message, title);
}

void ReflSaveTabPresenter::warnInvalidSaveDirectory() {
  warn("You just changed the save path to a directory which "
       "doesn't exist or is not writable.",
       "Invalid directory");
}

void ReflSaveTabPresenter::errorInvalidSaveDirectory() {
  error("The save path specified doesn't exist or is "
        "not writable.",
        "Invalid directory");
}

NamedFormat ReflSaveTabPresenter::formatFromIndex(int formatIndex) const {
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

FileFormatOptions ReflSaveTabPresenter::getSaveParametersFromView() const {
  return FileFormatOptions(
      /*format=*/formatFromIndex(m_view->getFileFormatIndex()),
      /*prefix=*/m_view->getPrefix(),
      /*includeTitle=*/m_view->getTitleCheck(),
      /*separator=*/m_view->getSeparator(),
      /*includeQResolution=*/m_view->getQResolutionCheck());
}

void ReflSaveTabPresenter::saveWorkspaces(
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
void ReflSaveTabPresenter::saveWorkspaces(
    std::vector<std::string> const &workspaceNames) {
  auto selectedLogParameters = m_view->getSelectedParameters();
  saveWorkspaces(workspaceNames, selectedLogParameters);
}

/** Saves selected workspaces */
void ReflSaveTabPresenter::saveSelectedWorkspaces() {
  // Check that at least one workspace has been selected for saving
  auto workspaceNames = m_view->getSelectedWorkspaces();
  if (workspaceNames.empty()) {
    error("No workspaces selected", "No workspaces selected. "
                                    "You must select the workspaces to save.");
  } else {
    try {
      saveWorkspaces(workspaceNames);
    } catch (std::exception &e) {
      error(e.what(), "Error");
    } catch (...) {
      error("Unknown error while saving workspaces", "Error");
    }
  }
}

/** Suggests a save directory and sets it in the 'Save path' text field
 */
void ReflSaveTabPresenter::suggestSaveDir() {
  std::string path = Mantid::Kernel::ConfigService::Instance().getString(
      "defaultsave.directory");
  m_view->setSavePath(path);
}

/** Obtains all available workspace names to save
 * @return :: list of workspace names
 */
std::vector<std::string> ReflSaveTabPresenter::getAvailableWorkspaceNames() {
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
