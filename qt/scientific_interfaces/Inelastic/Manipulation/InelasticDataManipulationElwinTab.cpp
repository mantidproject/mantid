// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTab.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "Common/InterfaceUtils.h"
#include "Common/SettingsHelper.h"
#include "Common/WorkspaceUtils.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"

#include <QFileInfo>

#include <algorithm>

#include "MantidQtWidgets/Common/AddWorkspaceMultiDialog.h"

using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("Elwin");

std::vector<std::string> getOutputWorkspaceSuffices() { return {"_eq", "_eq2", "_elf", "_elt"}; }

std::string extractLastOf(std::string const &str, std::string const &delimiter) {
  auto const cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(cutIndex + 1, str.size() - cutIndex);
  return str;
}

template <typename Iterator, typename Functor>
std::vector<std::string> transformElements(Iterator const fromIter, Iterator const toIter, Functor const &functor) {
  std::vector<std::string> newVector;
  newVector.reserve(toIter - fromIter);
  std::transform(fromIter, toIter, std::back_inserter(newVector), functor);
  return newVector;
}

template <typename T, typename Predicate> void removeElementsIf(std::vector<T> &vector, Predicate const &filter) {
  auto const iter = std::remove_if(vector.begin(), vector.end(), filter);
  if (iter != vector.end())
    vector.erase(iter, vector.end());
}

std::vector<std::string> extractSuffixes(QStringList const &files, std::string const &delimiter) {
  return transformElements(files.begin(), files.end(), [&](QString const &file) {
    QFileInfo const fileInfo(file);
    return extractLastOf(fileInfo.baseName().toStdString(), delimiter);
  });
}

std::vector<std::string> attachPrefix(std::vector<std::string> const &strings, std::string const &prefix) {
  return transformElements(strings.begin(), strings.end(), [&prefix](std::string const &str) { return prefix + str; });
}

std::vector<std::string> getFilteredSuffixes(QStringList const &files) {
  auto suffixes = extractSuffixes(files, "_");

  removeElementsIf(suffixes, [&](std::string const &suffix) { return suffix != "red" && suffix != "sqw"; });
  return suffixes;
}

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
using namespace IDA;
InelasticDataManipulationElwinTab::InelasticDataManipulationElwinTab(QWidget *parent, IElwinView *view)
    : InelasticDataManipulationTab(parent), m_view(view),
      m_model(std::make_unique<InelasticDataManipulationElwinTabModel>()),
      m_dataModel(std::make_unique<FitDataModel>()), m_selectedSpectrum(0) {
  m_view->subscribePresenter(this);
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra));
}

InelasticDataManipulationElwinTab::~InelasticDataManipulationElwinTab() {}

void InelasticDataManipulationElwinTab::setup() {
  m_view->setup();
  updateAvailableSpectra();
}

void InelasticDataManipulationElwinTab::run() {
  m_view->setRunIsRunning(true);

  // Get workspace names
  std::string inputGroupWsName = "IDA_Elwin_Input";

  // Load input files
  std::string inputWorkspacesString;
  for (WorkspaceID i = 0; i < m_dataModel->getNumberOfWorkspaces(); ++i) {

    auto workspace = m_dataModel->getWorkspace(i);
    auto spectra = m_dataModel->getSpectra(i);
    auto spectraWS = m_model->createGroupedWorkspaces(workspace, spectra);
    inputWorkspacesString += spectraWS + ",";
  }
  // Group input workspaces
  m_model->setupGroupAlgorithm(m_batchAlgoRunner, inputWorkspacesString, inputGroupWsName);

  m_model->setupElasticWindowMultiple(m_batchAlgoRunner, "ELWIN_workspace_output", inputGroupWsName,
                                      m_view->getLogName(), m_view->getLogValue());

  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = "ELWIN_workspace_output_elwin_eq2";
}

/**
 * Ungroups the output after the execution of the algorithm
 */
void InelasticDataManipulationElwinTab::runComplete(bool error) {
  m_view->setRunIsRunning(false);

  if (!error) {
    if (!m_view->isGroupInput()) {
      m_model->ungroupAlgorithm("IDA_Elwin_Input");
    }

    setOutputPlotOptionsWorkspaces(getOutputWorkspaceNames());

    if (m_view->getNormalise())
      checkForELTWorkspace();

  } else {
    m_view->setSaveResultEnabled(false);
  }
}

void InelasticDataManipulationElwinTab::checkForELTWorkspace() {
  auto const workspaceName = getOutputBasename() + "_elt";
  if (!WorkspaceUtils::doesExistInADS(workspaceName))
    m_view->showMessageBox("ElasticWindowMultiple successful. \nThe _elt workspace "
                           "was not produced - temperatures were not found.");
}

bool InelasticDataManipulationElwinTab::validate() {
  UserInputValidator uiv;
  auto rangeOne = std::make_pair(m_view->getIntegrationStart(), m_view->getIntegrationEnd());
  uiv.checkValidRange("Range One", rangeOne);
  bool useTwoRanges = m_view->getBackgroundSubtraction();
  if (useTwoRanges) {
    auto rangeTwo = std::make_pair(m_view->getBackgroundStart(), m_view->getBackgroundEnd());
    uiv.checkValidRange("Range Two", rangeTwo);
    uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
  }

  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    m_view->showMessageBox(errorMessage.toStdString());
  return errorMessage.isEmpty();
}

void InelasticDataManipulationElwinTab::handleValueChanged(std::string const &propName, double value) {
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

void InelasticDataManipulationElwinTab::handleValueChanged(std::string const &propName, bool value) {
  if (propName == "BackgroundSubtraction") {
    m_model->setBackgroundSubtraction(value);
  } else if (propName == "Normalise") {
    m_model->setNormalise(value);
  }
}

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTab::newInputDataFromDialog() {
  // Clear the existing list of files
  m_view->clearPreviewFile();
  m_view->newInputDataFromDialog(m_dataModel->getWorkspaceNames());

  std::string const wsname = m_view->getPreviewWorkspaceName(0);
  auto const inputWs = WorkspaceUtils::getADSWorkspace(wsname);
  setInputWorkspace(inputWs);
}

/**
 * Handles a new input file being selected for preview.
 *
 * Loads the file and resets the spectra selection spinner.
 *
 * @param index Index of the new selected file
 */

void InelasticDataManipulationElwinTab::handlePreviewIndexChanged(int index) {
  auto const workspaceName = m_view->getPreviewWorkspaceName(index);
  auto const filename = m_view->getPreviewFilename(index);

  if (!workspaceName.empty()) {
    if (!filename.empty())
      newPreviewFileSelected(workspaceName, filename);
    else
      newPreviewWorkspaceSelected(workspaceName);
  }
}

void InelasticDataManipulationElwinTab::newPreviewFileSelected(const std::string &workspaceName,
                                                               const std::string &filename) {
  auto loadHistory = true;
  if (loadFile(filename, workspaceName, -1, -1, loadHistory)) {
    auto const workspace = WorkspaceUtils::getADSWorkspace(workspaceName);

    setInputWorkspace(workspace);

    updateAvailableSpectra();
    m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  }
}

void InelasticDataManipulationElwinTab::newPreviewWorkspaceSelected(const std::string &workspaceName) {
  auto const workspace = WorkspaceUtils::getADSWorkspace(workspaceName);
  setInputWorkspace(workspace);
  updateAvailableSpectra();
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
}

void InelasticDataManipulationElwinTab::handlePreviewSpectrumChanged(int spectrum) {
  if (m_view->getPreviewSpec())
    setSelectedSpectrum(spectrum);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
}

void InelasticDataManipulationElwinTab::updateIntegrationRange() {
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

void InelasticDataManipulationElwinTab::handleRunClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}

/**
 * Handles saving of workspaces
 */
void InelasticDataManipulationElwinTab::handleSaveClicked() {
  for (auto const &name : getOutputWorkspaceNames())
    addSaveWorkspaceToQueue(name);
  m_batchAlgoRunner->executeBatchAsync();
}

std::vector<std::string> InelasticDataManipulationElwinTab::getOutputWorkspaceNames() {
  auto outputNames = attachPrefix(getOutputWorkspaceSuffices(), getOutputBasename());
  removeElementsIf(outputNames,
                   [](std::string const &workspaceName) { return !WorkspaceUtils::doesExistInADS(workspaceName); });
  return outputNames;
}

std::string InelasticDataManipulationElwinTab::getOutputBasename() {
  return WorkspaceUtils::getWorkspaceBasename(m_pythonExportWsName);
}

void InelasticDataManipulationElwinTab::handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  try {
    addDataToModel(dialog);
    updateTableFromModel();
    newInputDataFromDialog();
    m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void InelasticDataManipulationElwinTab::addDataToModel(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog = dynamic_cast<MantidWidgets::AddWorkspaceMultiDialog const *>(dialog)) {
    auto const nameIndexPairs = indirectDialog->selectedNameIndexPairs();
    for (auto const &pair : nameIndexPairs) {
      m_dataModel->addWorkspace(pair.first, FunctionModelSpectra(pair.second));
    }
  }
}

void InelasticDataManipulationElwinTab::updateTableFromModel() {
  m_view->clearDataTable();
  for (auto domainIndex = FitDomainIndex{0}; domainIndex < m_dataModel->getNumberOfDomains(); domainIndex++) {
    m_view->addTableEntry(static_cast<int>(domainIndex.value), m_dataModel->getWorkspace(domainIndex)->getName(),
                          static_cast<int>(m_dataModel->getSpectrum(domainIndex)));
  }
}

void InelasticDataManipulationElwinTab::handleRemoveSelectedData() {
  auto selectedIndices = m_view->getSelectedData();
  std::sort(selectedIndices.begin(), selectedIndices.end());
  for (auto item = selectedIndices.end(); item != selectedIndices.begin();) {
    --item;
    m_dataModel->removeDataByIndex(FitDomainIndex(item->row()));
  }
  updateTableFromModel();
  updateAvailableSpectra();
}

void InelasticDataManipulationElwinTab::updateAvailableSpectra() {
  auto spectra = m_dataModel->getSpectra(findWorkspaceID());
  if (spectra.isContinuous()) {
    auto const minmax = spectra.getMinMax();
    m_view->setAvailableSpectra(minmax.first, minmax.second);
  } else
    m_view->setAvailableSpectra(spectra.begin(), spectra.end());
}

size_t InelasticDataManipulationElwinTab::findWorkspaceID() {
  auto currentWorkspace = m_view->getCurrentPreview();
  auto allWorkspaces = m_dataModel->getWorkspaceNames();
  auto findWorkspace = find(allWorkspaces.begin(), allWorkspaces.end(), currentWorkspace);
  size_t workspaceID = findWorkspace - allWorkspaces.begin();
  return workspaceID;
}

/**
 * Retrieves the selected spectrum.
 *
 * @return  The selected spectrum.
 */
int InelasticDataManipulationElwinTab::getSelectedSpectrum() const { return m_selectedSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void InelasticDataManipulationElwinTab::setSelectedSpectrum(int spectrum) { m_selectedSpectrum = spectrum; }

/**
 * Retrieves the input workspace to be used in data analysis.
 *
 * @return  The input workspace to be used in data analysis.
 */
MatrixWorkspace_sptr InelasticDataManipulationElwinTab::getInputWorkspace() const { return m_inputWorkspace; }

/**
 * Sets the input workspace to be used in data analysis.
 *
 * @param inputWorkspace  The workspace to set.
 */
void InelasticDataManipulationElwinTab::setInputWorkspace(MatrixWorkspace_sptr inputWorkspace) {
  m_inputWorkspace = std::move(inputWorkspace);
  updateIntegrationRange();
}

/**
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void InelasticDataManipulationElwinTab::handlePlotPreviewClicked() {
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

/**
 * Retrieves the workspace containing the data to be displayed in
 * the preview plot.
 *
 * @return  The workspace containing the data to be displayed in
 *          the preview plot.
 */
MatrixWorkspace_sptr InelasticDataManipulationElwinTab::getPreviewPlotWorkspace() {
  return m_previewPlotWorkspace.lock();
}

/**
 * Sets the workspace containing the data to be displayed in the
 * preview plot.
 *
 * @param previewPlotWorkspace The workspace to set.
 */
void InelasticDataManipulationElwinTab::setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace) {
  m_previewPlotWorkspace = previewPlotWorkspace;
}

} // namespace MantidQt::CustomInterfaces