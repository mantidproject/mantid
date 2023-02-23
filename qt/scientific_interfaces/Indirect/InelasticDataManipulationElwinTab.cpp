// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTab.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "IndirectSettingsHelper.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"

#include <QFileInfo>

#include <algorithm>

#include "IndirectAddWorkspaceDialog.h"

using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("Elwin");

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
}

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

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

QStringList getSampleWSSuffices() {
  QStringList const wsSampleSuffixes{"red", "sqw"};
  return wsSampleSuffixes;
}

QStringList getSampleFBSuffices() {
  QStringList const fbSampleSuffixes{"red.*", "sqw.*"};
  return fbSampleSuffixes;
}

} // namespace

namespace MantidQt::CustomInterfaces {
using namespace IDA;
InelasticDataManipulationElwinTab::InelasticDataManipulationElwinTab(QWidget *parent)
    : InelasticDataManipulationTab(parent), m_view(std::make_unique<InelasticDataManipulationElwinTabView>(parent)),
      m_model(std::make_unique<InelasticDataManipulationElwinTabModel>()),
      m_dataModel(std::make_unique<IndirectFitDataModel>()), m_selectedSpectrum(0) {

  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra));
  connect(m_view.get(), SIGNAL(showMessageBox(const QString &)), this, SIGNAL(showMessageBox(const QString &)));
  connect(m_view.get(), SIGNAL(addDataClicked()), this, SLOT(showAddWorkspaceDialog()));
  connect(m_view.get(), SIGNAL(removeDataClicked()), this, SLOT(removeSelectedData()));

  m_parent = dynamic_cast<InelasticDataManipulation *>(parent);
}

InelasticDataManipulationElwinTab::~InelasticDataManipulationElwinTab() {}

void InelasticDataManipulationElwinTab::setup() {
  connect(m_view.get(), SIGNAL(filesFound()), this, SLOT(checkLoadedFiles()));
  connect(m_view.get(), SIGNAL(previewIndexChanged(int)), this, SLOT(checkNewPreviewSelected(int)));
  connect(m_view.get(), SIGNAL(selectedSpectrumChanged(int)), this, SLOT(handlePreviewSpectrumChanged(int)));
  connect(m_view.get(), SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(handleValueChanged(QtProperty *, double)));
  connect(m_view.get(), SIGNAL(valueChanged(QtProperty *, bool)), this, SLOT(handleValueChanged(QtProperty *, bool)));

  // Handle plot and save
  connect(m_view.get(), SIGNAL(runClicked()), this, SLOT(runClicked()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveClicked()));
  connect(m_view.get(), SIGNAL(plotPreviewClicked()), this, SLOT(plotCurrentPreview()));

  updateAvailableSpectra();
}

void InelasticDataManipulationElwinTab::run() {
  if (m_view->getCurrentInputIndex() == 0) {
    runFileInput();
  } else {
    runWorkspaceInput();
  }
}

void InelasticDataManipulationElwinTab::runFileInput() {
  m_view->setRunIsRunning(true);

  QStringList inputFilenames = m_view->getInputFilenames();
  inputFilenames.sort();

  // Get workspace names
  std::string inputGroupWsName = "IDA_Elwin_Input";

  QFileInfo firstFileInfo(inputFilenames[0]);
  const auto filename = firstFileInfo.baseName();

  auto workspaceBaseName = filename.left(filename.lastIndexOf("_"));

  if (inputFilenames.size() > 1) {
    QFileInfo fileInfo(inputFilenames[inputFilenames.length() - 1]);
    auto runNumber = fileInfo.baseName().toStdString();
    runNumber = runNumber.substr(0, runNumber.find_first_of("_"));
    size_t runNumberStart = 0;
    const auto strLength = runNumber.length();
    for (size_t i = 0; i < strLength; i++) {
      if (std::isdigit(runNumber[i])) {
        runNumberStart = i;
        break;
      }
    }
    // reassemble workspace base name with additional run number
    runNumber = runNumber.substr(runNumberStart, strLength);
    auto baseName = firstFileInfo.baseName();
    const auto prefix = baseName.left(baseName.indexOf("_"));
    const auto suffix = baseName.right(baseName.length() - baseName.indexOf("_"));
    workspaceBaseName = prefix + QString::fromStdString("-" + runNumber) + suffix;
  }

  // Load input files
  std::string inputWorkspacesString;

  for (auto &inputFilename : inputFilenames) {
    QFileInfo inputFileInfo(inputFilename);
    auto const workspaceName = inputFileInfo.baseName().toStdString();
    m_model->setupLoadAlgorithm(m_batchAlgoRunner, inputFilename.toStdString(), workspaceName);
    inputWorkspacesString += workspaceName + ",";
  }

  m_model->setupGroupAlgorithm(m_batchAlgoRunner, inputWorkspacesString, inputGroupWsName);
  m_model->setupElasticWindowMultiple(m_batchAlgoRunner, workspaceBaseName, inputGroupWsName, m_view->getLogName(),
                                      m_view->getLogValue());

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(unGroupInput(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = (workspaceBaseName + "_elwin_eq2").toStdString();
}

void InelasticDataManipulationElwinTab::runWorkspaceInput() {
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

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(unGroupInput(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = "ELWIN_workspace_output_elwin_eq2";
}

/**
 * Ungroups the output after the execution of the algorithm
 */
void InelasticDataManipulationElwinTab::unGroupInput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(unGroupInput(bool)));
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
  auto const workspaceName = getOutputBasename().toStdString() + "_elt";
  if (!doesExistInADS(workspaceName))
    showMessageBox("ElasticWindowMultiple successful. \nThe _elt workspace "
                   "was not produced - temperatures were not found.");
}

bool InelasticDataManipulationElwinTab::validate() {
  UserInputValidator uiv;
  if (m_view->getCurrentInputIndex() == 0) {
    auto const inputFileWidget = m_view->getFileFinderWidget();
    uiv.checkFileFinderWidgetIsValid("Input", inputFileWidget);
    auto const suffixes = getFilteredSuffixes(inputFileWidget->getFilenames());
    if (std::adjacent_find(suffixes.begin(), suffixes.end(), std::not_equal_to<>()) != suffixes.end())
      uiv.addErrorMessage("The input files must be all _red or all _sqw.");
  }

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
    showMessageBox(errorMessage);
  return errorMessage.isEmpty();
}

void InelasticDataManipulationElwinTab::setFileExtensionsByName(bool filter) {
  auto const tabName("Elwin");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
}

void InelasticDataManipulationElwinTab::handleValueChanged(QtProperty *prop, double value) {
  if (prop->propertyName() == "IntegrationStart") {
    m_model->setIntegrationStart(value);
  } else if (prop->propertyName() == "IntegrationEnd") {
    m_model->setIntegrationEnd(value);
  } else if (prop->propertyName() == "BackgroundStart") {
    m_model->setBackgroundStart(value);
  } else if (prop->propertyName() == "BackgroundEnd") {
    m_model->setBackgroundEnd(value);
  }
}

void InelasticDataManipulationElwinTab::handleValueChanged(QtProperty *prop, bool value) {
  if (prop->propertyName() == "BackgroundSubtraction") {
    m_model->setBackgroundSubtraction(value);
  } else if (prop->propertyName() == "Normalise") {
    m_model->setNormalise(value);
  }
}

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTab::newInputFiles() {
  m_view->clearPreviewFile();
  m_view->newInputFiles();

  QString const wsname = m_view->getPreviewWorkspaceName(0);
  auto const inputWs = getADSMatrixWorkspace(wsname.toStdString());
  setInputWorkspace(inputWs);
}

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTab::newInputFilesFromDialog(IAddWorkspaceDialog const *dialog) {
  // Clear the existing list of files
  if (m_dataModel->getNumberOfWorkspaces().value < 2)
    m_view->clearPreviewFile();

  m_view->newInputFilesFromDialog(dialog);

  QString const wsname = m_view->getPreviewWorkspaceName(0);
  auto const inputWs = getADSMatrixWorkspace(wsname.toStdString());
  setInputWorkspace(inputWs);
}

/**
 * Handles a new input file being selected for preview.
 *
 * Loads the file and resets the spectra selection spinner.
 *
 * @param index Index of the new selected file
 */

void InelasticDataManipulationElwinTab::checkNewPreviewSelected(int index) {
  auto const workspaceName = m_view->getPreviewWorkspaceName(index);
  auto const filename = m_view->getPreviewFilename(index);

  if (!workspaceName.isEmpty()) {
    if (!filename.isEmpty())
      newPreviewFileSelected(workspaceName, filename);
    else
      newPreviewWorkspaceSelected(workspaceName);
  }
}

void InelasticDataManipulationElwinTab::newPreviewFileSelected(const QString &workspaceName, const QString &filename) {
  auto loadHistory = m_view->isLoadHistory();
  if (loadFile(filename, workspaceName, -1, -1, loadHistory)) {
    auto const workspace = getADSMatrixWorkspace(workspaceName.toStdString());

    setInputWorkspace(workspace);

    m_view->newPreviewFileSelected(workspace);
    updateAvailableSpectra();
    m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  }
}

void InelasticDataManipulationElwinTab::newPreviewWorkspaceSelected(const QString &workspaceName) {
  if (m_view->getCurrentInputIndex() == 1) {
    auto const workspace = getADSMatrixWorkspace(workspaceName.toStdString());
    setInputWorkspace(workspace);
    updateAvailableSpectra();
    m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  }
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
        auto range = getXRangeFromWorkspace(getInputWorkspace());
        m_view->setIntegrationStart(range.first);
        m_view->setIntegrationEnd(range.second);
      }
    } else {
      showMessageBox("Warning: The instrument definition file for the input "
                     "workspace contains an invalid value.");
    }
  }
}

void InelasticDataManipulationElwinTab::runClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}

/**
 * Handles saving of workspaces
 */
void InelasticDataManipulationElwinTab::saveClicked() {
  for (auto const &name : getOutputWorkspaceNames())
    addSaveWorkspaceToQueue(name);
  m_batchAlgoRunner->executeBatchAsync();
}

std::vector<std::string> InelasticDataManipulationElwinTab::getOutputWorkspaceNames() {
  auto outputNames = attachPrefix(getOutputWorkspaceSuffices(), getOutputBasename().toStdString());
  removeElementsIf(outputNames, [](std::string const &workspaceName) { return !doesExistInADS(workspaceName); });
  return outputNames;
}

QString InelasticDataManipulationElwinTab::getOutputBasename() {
  return getWorkspaceBasename(QString::fromStdString(m_pythonExportWsName));
}

std::unique_ptr<IAddWorkspaceDialog> InelasticDataManipulationElwinTab::getAddWorkspaceDialog(QWidget *parent) const {
  return std::make_unique<IndirectAddWorkspaceDialog>(parent);
}

void InelasticDataManipulationElwinTab::showAddWorkspaceDialog() {
  if (!m_addWorkspaceDialog)
    m_addWorkspaceDialog = getAddWorkspaceDialog(m_parent);
  m_addWorkspaceDialog->setWSSuffices(getSampleWSSuffices());
  m_addWorkspaceDialog->setFBSuffices(getSampleFBSuffices());
  m_addWorkspaceDialog->updateSelectedSpectra();
  m_addWorkspaceDialog->show();
  connect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  connect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeDialog()));
}

void InelasticDataManipulationElwinTab::closeDialog() {
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeDialog()));
  m_addWorkspaceDialog->close();
  m_addWorkspaceDialog = nullptr;
}

void InelasticDataManipulationElwinTab::addData() { checkData(m_addWorkspaceDialog.get()); }

/** This method checks whether a Workspace or a File is being uploaded through the AddWorkspaceDialog
 * A File requiresd additional checks to ensure a file of the correct type is being loaded. The Workspace list is
 * already filtered.
 */
void InelasticDataManipulationElwinTab::checkData(IAddWorkspaceDialog const *dialog) {
  try {
    const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog);
    if (indirectDialog) {
      // getFileName will be empty if the addWorkspaceDialog is set to Workspace instead of File.
      if (indirectDialog->getFileName().empty()) {
        addData(dialog);
      } else
        addDataFromFile(dialog);
    } else
      (throw std::invalid_argument("Unable to access IndirectAddWorkspaceDialog"));

  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void InelasticDataManipulationElwinTab::addData(IAddWorkspaceDialog const *dialog) {
  try {
    addDataToModel(dialog);
    updateTableFromModel();
    emit dataAdded();
    emit dataChanged();
    newInputFilesFromDialog(dialog);
    m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void InelasticDataManipulationElwinTab::addDataFromFile(IAddWorkspaceDialog const *dialog) {
  try {
    UserInputValidator uiv;
    const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog);
    QList<QString> allFiles;
    allFiles.append(QString::fromStdString(indirectDialog->getFileName()));
    auto const suffixes = getFilteredSuffixes(allFiles);
    if (suffixes.size() < 1) {
      uiv.addErrorMessage("The input files must be all _red or all _sqw.");
      m_view->clearInputFiles();
      closeDialog();
    }
    QString error = uiv.generateErrorMessage();
    showMessageBox(error);

    if (error.isEmpty()) {
      addData(dialog);
    }
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void InelasticDataManipulationElwinTab::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog))
    m_dataModel->addWorkspace(indirectDialog->workspaceName(), indirectDialog->workspaceIndices());
}

void InelasticDataManipulationElwinTab::updateTableFromModel() {
  m_view->clearDataTable();
  for (auto domainIndex = FitDomainIndex{0}; domainIndex < m_dataModel->getNumberOfDomains(); domainIndex++) {
    m_view->addTableEntry(static_cast<int>(domainIndex.value), m_dataModel->getWorkspace(domainIndex)->getName(),
                          static_cast<int>(m_dataModel->getSpectrum(domainIndex)));
  }
}

void InelasticDataManipulationElwinTab::removeSelectedData() {
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
  if (m_view->getCurrentInputIndex() == 1) {
    if (spectra.isContinuous()) {
      auto const minmax = spectra.getMinMax();
      m_view->setAvailableSpectra(minmax.first, minmax.second);
    } else {
      m_view->setAvailableSpectra(spectra.begin(), spectra.end());
    }
  }
}

size_t InelasticDataManipulationElwinTab::findWorkspaceID() {
  auto currentWorkspace = m_view->getCurrentPreview().toStdString();
  auto allWorkspaces = m_dataModel->getWorkspaceNames();
  auto findWorkspace = find(allWorkspaces.begin(), allWorkspaces.end(), currentWorkspace);
  size_t workspaceID = findWorkspace - allWorkspaces.begin();
  return workspaceID;
}

void InelasticDataManipulationElwinTab::checkLoadedFiles() {
  if (validate()) {
    newInputFiles();
    m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
    updateIntegrationRange();
  }
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
}

/**
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void InelasticDataManipulationElwinTab::plotCurrentPreview() {
  auto const previewWs = getPreviewPlotWorkspace();
  auto const inputWs = getInputWorkspace();
  auto const index = boost::numeric_cast<size_t>(m_selectedSpectrum);
  auto const errorBars = IndirectSettingsHelper::externalPlotErrorBars();

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
    showMessageBox("Workspace not found - data may not be loaded.");
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