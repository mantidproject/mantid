// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ElwinPresenter.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include "MantidQtWidgets/Common/AddWorkspaceMultiDialog.h"
#include <algorithm>

using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("Elwin");

std::vector<std::string> getOutputWorkspaceSuffices() { return {"_eq", "_eq2", "_elf", "_elt"}; }

class ScopedFalse {
  bool &m_ref;
  bool m_oldValue;

public:
  // this sets the input bool to false whilst this object is in scope and then
  // resets it to its old value when this object drops out of scope.
  explicit ScopedFalse(bool &variable) : m_ref(variable), m_oldValue(variable) { m_ref = false; }
  ~ScopedFalse() { m_ref = m_oldValue; }
};
} // namespace

namespace MantidQt::CustomInterfaces {
using namespace Inelastic;
ElwinPresenter::ElwinPresenter(QWidget *parent, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner,
                               IElwinView *view, std::unique_ptr<IElwinModel> model)
    : DataProcessor(parent, std::move(algorithmRunner)), m_view(view), m_model(std::move(model)),
      m_dataModel(std::make_unique<DataModel>()), m_selectedSpectrum(0) {
  m_view->subscribePresenter(this);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
  setOutputPlotOptionsPresenter(m_view->getPlotOptions(), PlotWidget::SpectraSliceSurface);
  m_view->setup();
  observeRename(true);
  observeDelete(true);
  observeClear(true);
  updateAvailableSpectra();
}

ElwinPresenter::ElwinPresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner,
                               IElwinView *view, std::unique_ptr<IElwinModel> model,
                               std::unique_ptr<IDataModel> dataModel)
    : DataProcessor(parent, std::move(algorithmRunner)), m_view(view), m_model(std::move(model)),
      m_dataModel(std::move(dataModel)), m_selectedSpectrum(0) {
  m_view->subscribePresenter(this);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
  setOutputPlotOptionsPresenter(m_view->getPlotOptions(), PlotWidget::SpectraSliceSurface);
  m_view->setup();
  updateAvailableSpectra();
}

/**
 * Ungroups the output after the execution of the algorithm
 */
void ElwinPresenter::runComplete(Mantid::API::IAlgorithm_sptr const algorithm, bool const error) {
  (void)algorithm;
  m_view->setRunIsRunning(false);

  if (!error) {
    if (!m_view->isGroupInput()) {
      m_model->ungroupAlgorithm("Elwin_Input");
    } else {
      std::string outputNames =
          checkForELTWorkspace()
              ? m_model->getOutputWorkspaceNames()
              : m_model->getOutputWorkspaceNames().substr(0, m_model->getOutputWorkspaceNames().find_last_of(','));
      m_model->groupAlgorithm(outputNames, "Elwin_Output");
    }

    setOutputPlotOptionsWorkspaces(getOutputWorkspaceNames());

    if (m_view->getNormalise() && !checkForELTWorkspace())
      m_view->showMessageBox("ElasticWindowMultiple successful. \nThe _elt workspace "
                             "was not produced - temperatures were not found.");
  } else {
    m_view->setSaveResultEnabled(false);
  }
}

bool ElwinPresenter::checkForELTWorkspace() {
  auto const workspaceName = getOutputBasename() + "_elt";
  return WorkspaceUtils::doesExistInADS(workspaceName);
}

void ElwinPresenter::handleValidation(IUserInputValidator *validator) const {
  if (m_view->isTableEmpty())
    validator->addErrorMessage("Data Table is empty");
  auto rangeOne = std::make_pair(m_view->getIntegrationStart(), m_view->getIntegrationEnd());
  validator->checkValidRange("Range One", rangeOne);
  bool useTwoRanges = m_view->getBackgroundSubtraction();
  if (useTwoRanges) {
    auto rangeTwo = std::make_pair(m_view->getBackgroundStart(), m_view->getBackgroundEnd());
    validator->checkValidRange("Range Two", rangeTwo);
    validator->checkRangesDontOverlap(rangeOne, rangeTwo);
  }
}

void ElwinPresenter::handleValueChanged(std::string const &propName, double value) {
  if (propName == "IntegrationStart") {
    m_model->setIntegrationStart(value);
  } else if (propName == "IntegrationEnd") {
    m_model->setIntegrationEnd(value);
  } else if (propName == "BackgroundStart") {
    m_model->setBackgroundStart(value);
  } else if (propName == "BackgroundEnd") {
    m_model->setBackgroundEnd(value);
  }
}

void ElwinPresenter::handleValueChanged(std::string const &propName, bool value) {
  if (propName == "Background Subtraction") {
    m_model->setBackgroundSubtraction(value);
  } else if (propName == "Normalise to Lowest Temp") {
    m_model->setNormalise(value);
  }
}

/**
 * Handles a new input file being selected for preview.
 *
 * Loads the file and resets the spectra selection spinner.
 *
 * @param index Index of the new selected file
 */

void ElwinPresenter::handlePreviewIndexChanged(int index) {
  auto const workspaceName = m_view->getPreviewWorkspaceName(index);
  if (!workspaceName.empty()) {
    newPreviewWorkspaceSelected(index);
  }
}

void ElwinPresenter::newPreviewWorkspaceSelected(int index) {
  auto const workspace = m_dataModel->getWorkspace(WorkspaceID(index));
  setInputWorkspace(workspace);
  updateAvailableSpectra();
  setSelectedSpectrum(m_view->getPreviewSpec());
  m_view->updateSelectorRange(workspace);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
}

void ElwinPresenter::handlePreviewSpectrumChanged(int spectrum) {
  if (m_view->getPreviewSpec() >= 0)
    setSelectedSpectrum(spectrum);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
}

void ElwinPresenter::updateIntegrationRange() {
  auto inst = getInputWorkspace()->getInstrument();
  auto analyser = inst->getStringParameter("analyser");
  if (analyser.size() > 0) {
    auto comp = inst->getComponentByName(analyser[0]);
    if (comp) {
      auto params = comp->getNumberParameter("resolution", true);

      // set the default instrument resolution
      if (!params.empty()) {
        double res = params[0];

        m_view->setIntegrationStart(-res);
        m_view->setIntegrationEnd(res);

        m_view->setBackgroundStart(-10 * res);
        m_view->setBackgroundEnd(-9 * res);
      } else {
        auto range = WorkspaceUtils::getXRangeFromWorkspace(getInputWorkspace());
        m_view->setIntegrationStart(range.first);
        m_view->setIntegrationEnd(range.second);
      }
    } else {
      showMessageBox("Warning: The instrument definition file for the input "
                     "workspace contains an invalid value.");
    }
  }
}

void ElwinPresenter::handleRun() {
  clearOutputPlotOptionsWorkspaces();
  m_view->setRunIsRunning(true);

  // Get workspace names
  std::string const inputGroupWsName = "Elwin_Input";
  std::string outputWsBasename = WorkspaceUtils::parseRunNumbers(m_dataModel->getWorkspaceNames());
  // Load input files
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algQueue = {};
  std::string inputWorkspacesString;
  for (WorkspaceID i = 0; i < m_dataModel->getNumberOfWorkspaces(); ++i) {
    auto workspace = m_dataModel->getWorkspace(i);
    auto spectra = m_dataModel->getSpectra(i);
    auto spectraWS = workspace->getName() + "_extracted_spectra";
    algQueue.emplace_back(m_model->setupExtractSpectra(workspace, spectra, spectraWS));
    inputWorkspacesString += spectraWS + ",";
  }

  // Group input workspaces
  algQueue.emplace_back(m_model->setupGroupAlgorithm(inputWorkspacesString, inputGroupWsName));
  algQueue.emplace_back(m_model->setupElasticWindowMultiple(outputWsBasename, inputGroupWsName, m_view->getLogName(),
                                                            m_view->getLogValue()));
  m_algorithmRunner->execute(std::move(algQueue));
  // Set the result workspace for Python script export
  m_pythonExportWsName = outputWsBasename + "_elwin_eq2";
}

/**
 * Handles saving of workspaces
 */
void ElwinPresenter::handleSaveClicked() {
  std::deque<IConfiguredAlgorithm_sptr> saveQueue = {};
  for (auto const &name : getOutputWorkspaceNames())
    saveQueue.emplace_back(setupSaveAlgorithm(name));
  m_algorithmRunner->execute(std::move(saveQueue));
}

std::vector<std::string> ElwinPresenter::getOutputWorkspaceNames() {
  auto outputNames = WorkspaceUtils::attachPrefix(getOutputWorkspaceSuffices(), getOutputBasename());
  WorkspaceUtils::removeElementsIf(
      outputNames, [](std::string const &workspaceName) { return !WorkspaceUtils::doesExistInADS(workspaceName); });
  return outputNames;
}

std::string ElwinPresenter::getOutputBasename() { return WorkspaceUtils::getWorkspaceBasename(m_pythonExportWsName); }

void ElwinPresenter::handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  try {
    addDataToModel(dialog);
    updateTableFromModel();
    m_view->updatePreviewWorkspaceNames(m_dataModel->getWorkspaceNames());
    m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void ElwinPresenter::addDataToModel(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog = dynamic_cast<MantidWidgets::AddWorkspaceMultiDialog const *>(dialog)) {
    auto const nameIndexPairs = indirectDialog->selectedNameIndexPairs();
    for (auto const &pair : nameIndexPairs) {
      m_dataModel->addWorkspace(pair.first, FunctionModelSpectra(pair.second));
    }
  }
}

void ElwinPresenter::updateTableFromModel() {
  m_view->clearDataTable();
  if (m_view->isRowCollapsed())
    for (auto workspaceIndex = WorkspaceID{0}; workspaceIndex < m_dataModel->getNumberOfWorkspaces(); workspaceIndex++)
      m_view->addTableEntry(static_cast<int>(workspaceIndex.value),
                            m_dataModel->getWorkspace(workspaceIndex)->getName(),
                            m_dataModel->getSpectra(workspaceIndex).getString());
  else
    for (auto domainIndex = FitDomainIndex{0}; domainIndex < m_dataModel->getNumberOfDomains(); domainIndex++)
      m_view->addTableEntry(static_cast<int>(domainIndex.value), m_dataModel->getWorkspace(domainIndex)->getName(),
                            std::to_string(m_dataModel->getSpectrum(domainIndex)));
}

void ElwinPresenter::handleRemoveSelectedData() {
  auto selectedIndices = m_view->getSelectedData();
  std::sort(selectedIndices.begin(), selectedIndices.end());
  for (auto item = selectedIndices.end(); item != selectedIndices.begin();) {
    --item;
    m_view->isRowCollapsed() ? m_dataModel->removeWorkspace(WorkspaceID(item->row()))
                             : m_dataModel->removeDataByIndex(FitDomainIndex(item->row()));
  }
  updateInterface();
}

void ElwinPresenter::handleRowModeChanged() { updateTableFromModel(); }

void ElwinPresenter::updateAvailableSpectra() {
  if (auto const wsID = findWorkspaceID(m_view->getCurrentPreview()); wsID) {
    auto const spectra = m_dataModel->getSpectra(*wsID);
    m_view->setAvailableSpectra(spectra.begin(), spectra.end());
  }
}

std::optional<WorkspaceID> ElwinPresenter::findWorkspaceID(std::string const &name) const {
  auto allWorkspaces = m_dataModel->getWorkspaceNames();
  auto const findWorkspace = find(allWorkspaces.begin(), allWorkspaces.end(), name);
  size_t const workspaceID = findWorkspace - allWorkspaces.begin();
  if (allWorkspaces.empty() || (allWorkspaces.size() == workspaceID))
    return std::nullopt;
  return WorkspaceID{workspaceID};
}

/**
 * Retrieves the selected spectrum.
 *
 * @return  The selected spectrum.
 */
int ElwinPresenter::getSelectedSpectrum() const { return m_selectedSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void ElwinPresenter::setSelectedSpectrum(int spectrum) { m_selectedSpectrum = spectrum; }

/**
 * Retrieves the input workspace to be used in data analysis.
 *
 * @return  The input workspace to be used in data analysis.
 */
MatrixWorkspace_sptr ElwinPresenter::getInputWorkspace() const { return m_inputWorkspace; }

/**
 * Sets the input workspace to be used in data analysis.
 *
 * @param inputWorkspace  The workspace to set.
 */
void ElwinPresenter::setInputWorkspace(const MatrixWorkspace_sptr &inputWorkspace) {
  m_inputWorkspace = inputWorkspace;
  updateIntegrationRange();
}

/**
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void ElwinPresenter::handlePlotPreviewClicked() {
  auto const previewWs = getPreviewPlotWorkspace();
  auto const inputWs = getInputWorkspace();
  auto const index = boost::numeric_cast<size_t>(m_selectedSpectrum);
  auto const errorBars = SettingsHelper::externalPlotErrorBars();

  // Check a workspace has been selected
  if (previewWs) {

    if (inputWs && previewWs->getName() == inputWs->getName()) {
      m_plotter->plotSpectra(previewWs->getName(), std::to_string(index), errorBars);
    } else {
      m_plotter->plotSpectra(previewWs->getName(), "0-2", errorBars);
    }
  } else if (inputWs && index < inputWs->getNumberHistograms()) {
    m_plotter->plotSpectra(inputWs->getName(), std::to_string(index), errorBars);
  } else
    m_view->showMessageBox("Workspace not found - data may not be loaded.");
}

void ElwinPresenter::updateInterface() {
  updateTableFromModel();
  m_view->updatePreviewWorkspaceNames(m_dataModel->getWorkspaceNames());
}

/**
 * Retrieves the workspace containing the data to be displayed in
 * the preview plot.
 *
 * @return  The workspace containing the data to be displayed in
 *          the preview plot.
 */
MatrixWorkspace_sptr ElwinPresenter::getPreviewPlotWorkspace() { return m_previewPlotWorkspace.lock(); }

/**
 * Sets the workspace containing the data to be displayed in the
 * preview plot.
 *
 * @param previewPlotWorkspace The workspace to set.
 */
void ElwinPresenter::setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace) {
  m_previewPlotWorkspace = previewPlotWorkspace;
}

/**
 * Remove a workspace from the data model.
 *
 * @param workspaceName The name of the workspace to remove from the data model.
 */

void ElwinPresenter::removeWorkspace(std::string const &workspaceName) const {
  if (auto const wsID = findWorkspaceID(workspaceName); wsID) {
    m_dataModel->removeWorkspace(*wsID);
  }
}

void ElwinPresenter::deleteHandle(std::string const &wsName, Workspace_sptr const &ws) {
  (void)ws;
  removeWorkspace(wsName);
  updateInterface();
}
void ElwinPresenter::clearHandle() {
  m_dataModel->clear();
  updateInterface();
}
void ElwinPresenter::renameHandle(std::string const &wsName, std::string const &newName) {
  // Remove renamed workspace if it is on the data model
  removeWorkspace(wsName);
  // Remove renamed workspace if new name replaces a workspace in data model
  removeWorkspace(newName);
  updateInterface();
}

} // namespace MantidQt::CustomInterfaces
