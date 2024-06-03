// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

#include <boost/regex.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using namespace Mantid::API;

/**
 * @param saver :: The model to use to save the files
 * @param view :: The view we are handling
 */
SavePresenter::SavePresenter(ISaveView *view, std::unique_ptr<IFileSaver> saver)
    : m_mainPresenter(nullptr), m_view(view), m_saver(std::move(saver)), m_shouldAutosave(false),
      m_shouldSaveIndividualRows(false) {

  m_view->subscribe(this);
  populateWorkspaceList();
  suggestSaveDir();
  // this call needs to come last in order to avoid notifySettingsChanged being
  // called with a nullptr, i.e. before the main presenter is accepted
  m_view->connectSaveSettingsWidgets();
}

void SavePresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

void SavePresenter::notifySettingsChanged() {
  m_mainPresenter->setBatchUnsaved();
  updateWidgetEnabledState();
}

void SavePresenter::notifyPopulateWorkspaceList() { populateWorkspaceList(); }

void SavePresenter::notifyFilterWorkspaceList() { filterWorkspaceNames(); }

void SavePresenter::notifyPopulateParametersList() { populateParametersList(); }

void SavePresenter::notifySaveSelectedWorkspaces() { saveSelectedWorkspaces(); }

void SavePresenter::notifyAutosaveDisabled() { disableAutosave(); }

void SavePresenter::notifyAutosaveEnabled() { enableAutosave(); }

void SavePresenter::notifySavePathChanged() { onSavePathChanged(); }

bool SavePresenter::isProcessing() const { return m_mainPresenter->isProcessing(); }

bool SavePresenter::isAutoreducing() const { return m_mainPresenter->isAutoreducing(); }

namespace {
bool isORSOFormat(const NamedFormat &fileFormat) {
  return fileFormat == NamedFormat::ORSOAscii || fileFormat == NamedFormat::ORSONexus;
}
} // unnamed namespace

bool SavePresenter::hasSelectedORSOFormat() const {
  const auto selectedFormat = SavePresenter::formatFromIndex(SavePresenter::m_view->getFileFormatIndex());
  return isORSOFormat(selectedFormat);
}

/** Tells the view to enable/disable certain widgets based on the
 * selected file format
 */
void SavePresenter::updateWidgetStateBasedOnFileFormat() const {
  auto const fileFormat = formatFromIndex(m_view->getFileFormatIndex());
  // Enable/disable the log list for formats that include the header from the SaveReflectometryAscii algorithm.
  // Note that at the moment the log list is used in SaveReflectometryAscii for
  // ILLCosmos (MFT) but I'm not sure if it should be.
  if ((fileFormat == NamedFormat::Custom && m_view->getHeaderCheck()) || fileFormat == NamedFormat::ILLCosmos)
    m_view->enableLogList();
  else
    m_view->disableLogList();

  // Enable/disable the Q resolution checkbox for formats that can optionally include resolution
  if (fileFormat == NamedFormat::Custom || isORSOFormat(fileFormat))
    m_view->enableQResolutionCheckBox();
  else
    m_view->disableQResolutionCheckBox();

  // Enable/disable the additional columns checkbox for formats that can optionally include these
  if (isORSOFormat(fileFormat))
    m_view->enableAdditionalColumnsCheckBox();
  else
    m_view->disableAdditionalColumnsCheckBox();

  // Enable/disable the save to single file checkbox for formats that support this
  if (shouldAutosave() && isORSOFormat(fileFormat))
    m_view->enableSaveToSingleFileCheckBox();
  else
    m_view->disableSaveToSingleFileCheckBox();

  // Everything else is enabled for Custom and disabled otherwise
  if (fileFormat == NamedFormat::Custom) {
    m_view->enableHeaderCheckBox();
    m_view->enableSeparatorButtonGroup();
  } else {
    m_view->disableHeaderCheckBox();
    m_view->disableSeparatorButtonGroup();
  }
}

/** Tells the view to update the enabled/disabled state of all relevant
 * widgets based on whether processing is in progress or not.
 */
void SavePresenter::updateWidgetEnabledState() const {
  if (isProcessing() || isAutoreducing()) {
    m_view->disableAutosaveControls();
    if (shouldAutosave()) {
      m_view->disableFileFormatControls();
      m_view->disableLocationControls();
      updateWidgetStateBasedOnFileFormat();
    } else {
      m_view->enableFileFormatControls();
      m_view->enableLocationControls();
      updateWidgetStateBasedOnFileFormat();
    }
  } else {
    m_view->enableAutosaveControls();
    m_view->enableFileFormatControls();
    m_view->enableLocationControls();
    updateWidgetStateBasedOnFileFormat();
  }
}

void SavePresenter::notifyReductionPaused() {
  populateWorkspaceList();
  updateWidgetEnabledState();
}

void SavePresenter::notifyReductionResumed() { updateWidgetEnabledState(); }

void SavePresenter::notifyAutoreductionPaused() { updateWidgetEnabledState(); }

void SavePresenter::notifyAutoreductionResumed() { updateWidgetEnabledState(); }

void SavePresenter::enableAutosave() {
  if (isValidSaveDirectory(m_view->getSavePath())) {
    m_shouldAutosave = true;
    m_view->enableSaveIndividualRowsCheckbox();
    if (hasSelectedORSOFormat()) {
      m_view->enableSaveToSingleFileCheckBox();
    }
  } else {
    m_shouldAutosave = false;
    m_view->disallowAutosave();
    errorInvalidSaveDirectory();
  }
}

void SavePresenter::disableAutosave() {
  m_shouldAutosave = false;
  m_view->disableSaveIndividualRowsCheckbox();
  m_view->disableSaveToSingleFileCheckBox();
}

void SavePresenter::notifySaveIndividualRowsEnabled() { m_shouldSaveIndividualRows = true; }

void SavePresenter::notifySaveIndividualRowsDisabled() { m_shouldSaveIndividualRows = false; }

void SavePresenter::onSavePathChanged() {
  if (shouldAutosave() && !isValidSaveDirectory(m_view->getSavePath()))
    warnInvalidSaveDirectory();
}

bool SavePresenter::shouldAutosave() const { return m_shouldAutosave; }

bool SavePresenter::shouldAutosaveGroupRows() const { return m_shouldSaveIndividualRows; }

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
      it = std::copy_if(wsNames.begin(), wsNames.end(), validNames.begin(),
                        [rgx](const std::string &s) { return boost::regex_search(s, rgx); });
      m_view->showFilterEditValid();
    } catch (boost::regex_error &) {
      m_view->showFilterEditInvalid();
    }
  } else {
    // Otherwise simply add names where the filter string is found in
    it = std::copy_if(wsNames.begin(), wsNames.end(), validNames.begin(),
                      [filter](const std::string &s) { return s.find(filter) != std::string::npos; });
  }

  validNames.resize(std::distance(validNames.begin(), it));
  m_view->setWorkspaceList(validNames);
}

/** Fills the 'List of Logged Parameters' widget with the parameters of the
 * currently selected workspace
 */
void SavePresenter::populateParametersList() {
  m_view->clearParametersList();

  const std::string wsName = m_view->getCurrentWorkspaceName();
  if (!AnalysisDataService::Instance().doesExist(wsName)) {
    return;
  }

  if (const auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName)) {
    std::vector<std::string> logs;
    const auto &properties = ws->run().getProperties();
    for (auto it = properties.begin(); it != properties.end(); it++) {
      logs.emplace_back((*it)->name());
    }
    m_view->setParametersList(logs);
  }
}

bool SavePresenter::isValidSaveDirectory(std::string const &directory) {
  return m_saver->isValidSaveDirectory(directory);
}

void SavePresenter::warnInvalidSaveDirectory() { m_view->warnInvalidSaveDirectory(); }

void SavePresenter::errorInvalidSaveDirectory() { m_view->errorInvalidSaveDirectory(); }

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
  case 4:
    return NamedFormat::ORSOAscii;
  case 5:
    return NamedFormat::ORSONexus;
  default:
    throw std::runtime_error("Unknown save format.");
  }
}

FileFormatOptions SavePresenter::getSaveParametersFromView(bool const isAutoSave) const {
  return FileFormatOptions(
      /*format=*/formatFromIndex(m_view->getFileFormatIndex()),
      /*prefix=*/m_view->getPrefix(),
      /*includeHeader=*/m_view->getHeaderCheck(),
      /*separator=*/m_view->getSeparator(),
      /*includeQResolution=*/m_view->getQResolutionCheck(),
      /*includeAdditionalColumns=*/m_view->getAdditionalColumnsCheck(),
      /*shouldSaveToSingleFile=*/isAutoSave && m_view->getSaveToSingleFileCheck());
}

void SavePresenter::saveWorkspaces(std::vector<std::string> const &workspaceNames,
                                   std::vector<std::string> const &logParameters, bool const isAutoSave) {
  auto savePath = m_view->getSavePath();
  if (m_saver->isValidSaveDirectory(savePath))
    m_saver->save(savePath, workspaceNames, logParameters, getSaveParametersFromView(isAutoSave));
  else
    errorInvalidSaveDirectory();
}

/** Saves workspaces with the names specified. */
void SavePresenter::saveWorkspaces(std::vector<std::string> const &workspaceNames, bool const isAutoSave) {
  auto selectedLogParameters = m_view->getSelectedParameters();
  saveWorkspaces(workspaceNames, selectedLogParameters, isAutoSave);
}

/** Saves selected workspaces */
void SavePresenter::saveSelectedWorkspaces() {
  // Check that at least one workspace has been selected for saving
  auto wkspNames = m_view->getSelectedWorkspaces();
  if (wkspNames.empty()) {
    m_view->noWorkspacesSelected();
  } else {
    try {
      saveWorkspaces(wkspNames);
    } catch (std::runtime_error const &e) {
      m_view->cannotSaveWorkspaces(e.what());
    } catch (std::exception const &e) {
      m_view->cannotSaveWorkspaces(e.what());
    } catch (...) {
      m_view->cannotSaveWorkspaces();
    }
  }
}

/** Suggests a save directory and sets it in the 'Save path' text field
 */
void SavePresenter::suggestSaveDir() {
  std::string path = Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory");
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
  std::remove_copy_if(allNames.begin(), allNames.end(), std::back_inserter(validNames), [](const std::string &wsName) {
    return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName) ||
           AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(wsName);
  });

  return validNames;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
