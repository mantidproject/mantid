// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTab.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "IndirectSettingsHelper.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

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

std::string extractLastOf(const std::string &str, const std::string &delimiter) {
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

IAlgorithm_sptr loadAlgorithm(std::string const &filepath, std::string const &outputName) {
  auto loadAlg = AlgorithmManager::Instance().create("LoadNexus");
  loadAlg->initialize();
  loadAlg->setProperty("Filename", filepath);
  loadAlg->setProperty("OutputWorkspace", outputName);
  return loadAlg;
}

namespace Regexes {
const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString NATURAL_NUMBER = "(0|[1-9][0-9]*)";
const QString REAL_NUMBER = "(-?" + NATURAL_NUMBER + "(\\.[0-9]*)?)";
const QString REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const QString MASK_LIST = "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes

class ExcludeRegionDelegate : public QItemDelegate {
public:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                        const QModelIndex & /*index*/) const override {
    auto lineEdit = std::make_unique<QLineEdit>(parent);
    auto validator = std::make_unique<QRegExpValidator>(QRegExp(Regexes::MASK_LIST), parent);
    lineEdit->setValidator(validator.release());
    return lineEdit.release();
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
    const auto value = index.model()->data(index, Qt::EditRole).toString();
    static_cast<QLineEdit *>(editor)->setText(value);
  }

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
    auto *lineEdit = static_cast<QLineEdit *>(editor);
    model->setData(index, lineEdit->text(), Qt::EditRole);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex & /*index*/) const override {
    editor->setGeometry(option.rect);
  }
};

QStringList defaultHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "WS Index";
  return headers;
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
    : InelasticDataManipulationTab(parent), m_elwTree(nullptr),
      m_view(std::make_unique<InelasticDataManipulationElwinTabView>(parent)),
      m_dataModel(std::make_unique<IndirectFitDataModel>()), m_selectedSpectrum(0) {

  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra));
  connect(m_view.get(), SIGNAL(addDataClicked()), this, SLOT(showAddWorkspaceDialog()));
  connect(m_view.get(), SIGNAL(removeDataClicked()), this, SLOT(removeSelectedData()));

  m_parent = dynamic_cast<InelasticDataManipulation *>(parent);
  m_dataTable = m_view->getDataTable();

  const QStringList headers = defaultHeaders();
  setHorizontalHeaders(headers);
  m_dataTable->setItemDelegateForColumn(headers.size() - 1, std::make_unique<ExcludeRegionDelegate>().release());
  m_dataTable->verticalHeader()->setVisible(false);
}

InelasticDataManipulationElwinTab::~InelasticDataManipulationElwinTab() {}

void InelasticDataManipulationElwinTab::setup() {

  // Number of decimal places in property browsers.
  static const unsigned int NUM_DECIMALS = 6;

  connect(m_view.get(), SIGNAL(filesFound()), this, SLOT(checkLoadedFiles()));
  connect(m_view.get(), SIGNAL(previewIndexChanged(int)), this, SLOT(checkNewPreviewSelected(int)));
  connect(m_view.get(), SIGNAL(selectedSpectrumChanged(int)), this, SLOT(handlePreviewSpectrumChanged(int)));

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

  workspaceBaseName += "_elwin_";

  const auto qWorkspace = (workspaceBaseName + "eq").toStdString();
  const auto qSquaredWorkspace = (workspaceBaseName + "eq2").toStdString();
  const auto elfWorkspace = (workspaceBaseName + "elf").toStdString();
  const auto eltWorkspace = (workspaceBaseName + "elt").toStdString();

  // Load input files
  std::string inputWorkspacesString;

  for (auto &inputFilename : inputFilenames) {
    QFileInfo inputFileInfo(inputFilename);
    auto const workspaceName = inputFileInfo.baseName().toStdString();
    m_batchAlgoRunner->addAlgorithm(loadAlgorithm(inputFilename.toStdString(), workspaceName));
    inputWorkspacesString += workspaceName + ",";
  }

  // Group input workspaces
  auto groupWsAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupWsAlg->initialize();
  auto runTimeProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  runTimeProps->setPropertyValue("InputWorkspaces", inputWorkspacesString);
  groupWsAlg->setProperty("OutputWorkspace", inputGroupWsName);

  m_batchAlgoRunner->addAlgorithm(groupWsAlg, std::move(runTimeProps));

  // Configure ElasticWindowMultiple algorithm
  auto elwinMultAlg = AlgorithmManager::Instance().create("ElasticWindowMultiple");
  elwinMultAlg->initialize();

  elwinMultAlg->setProperty("OutputInQ", qWorkspace);
  elwinMultAlg->setProperty("OutputInQSquared", qSquaredWorkspace);
  elwinMultAlg->setProperty("OutputELF", elfWorkspace);

  elwinMultAlg->setProperty("SampleEnvironmentLogName", m_view->getLogName());
  elwinMultAlg->setProperty("SampleEnvironmentLogValue", m_view->getLogValue());

  elwinMultAlg->setProperty("IntegrationRangeStart", m_view->getIntegrationStart());
  elwinMultAlg->setProperty("IntegrationRangeEnd", m_view->getIntegrationEnd());

  if (m_view->getBackgroundSubtraction()) {
    elwinMultAlg->setProperty("BackgroundRangeStart", m_view->getBackgroundStart());
    elwinMultAlg->setProperty("BackgroundRangeEnd", m_view->getBackgroundEnd());
  }

  if (m_view->getNormalise()) {
    elwinMultAlg->setProperty("OutputELT", eltWorkspace);
  }

  auto elwinInputProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  elwinInputProps->setPropertyValue("InputWorkspaces", inputGroupWsName);

  m_batchAlgoRunner->addAlgorithm(elwinMultAlg, std::move(elwinInputProps));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(unGroupInput(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = qSquaredWorkspace;
}

void InelasticDataManipulationElwinTab::runWorkspaceInput() {
  m_view->setRunIsRunning(true);

  // Get workspace names
  std::string inputGroupWsName = "IDA_Elwin_Input";

  auto workspaceBaseName = m_view->getCurrentPreview();

  workspaceBaseName += "_elwin_";

  const auto qWorkspace = (workspaceBaseName + "eq").toStdString();
  const auto qSquaredWorkspace = (workspaceBaseName + "eq2").toStdString();
  const auto elfWorkspace = (workspaceBaseName + "elf").toStdString();
  const auto eltWorkspace = (workspaceBaseName + "elt").toStdString();

  // Load input files
  std::string inputWorkspacesString = m_view->getCurrentPreview().toStdString();

  // Group input workspaces
  auto groupWsAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupWsAlg->initialize();
  auto runTimeProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  runTimeProps->setPropertyValue("InputWorkspaces", inputWorkspacesString);
  groupWsAlg->setProperty("OutputWorkspace", inputGroupWsName);

  m_batchAlgoRunner->addAlgorithm(groupWsAlg, std::move(runTimeProps));

  // Configure ElasticWindowMultiple algorithm
  auto elwinMultAlg = AlgorithmManager::Instance().create("ElasticWindowMultiple");
  elwinMultAlg->initialize();

  elwinMultAlg->setProperty("OutputInQ", qWorkspace);
  elwinMultAlg->setProperty("OutputInQSquared", qSquaredWorkspace);
  elwinMultAlg->setProperty("OutputELF", elfWorkspace);

  elwinMultAlg->setProperty("SampleEnvironmentLogName", m_view->getLogName());
  elwinMultAlg->setProperty("SampleEnvironmentLogValue", m_view->getLogValue());

  elwinMultAlg->setProperty("IntegrationRangeStart", m_view->getIntegrationStart());
  elwinMultAlg->setProperty("IntegrationRangeEnd", m_view->getIntegrationEnd());

  if (m_view->getBackgroundSubtraction()) {
    elwinMultAlg->setProperty("BackgroundRangeStart", m_view->getBackgroundStart());
    elwinMultAlg->setProperty("BackgroundRangeEnd", m_view->getBackgroundEnd());
  }

  if (m_view->getNormalise()) {
    elwinMultAlg->setProperty("OutputELT", eltWorkspace);
  }

  auto elwinInputProps = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();
  elwinInputProps->setPropertyValue("InputWorkspaces", inputGroupWsName);

  m_batchAlgoRunner->addAlgorithm(elwinMultAlg, std::move(elwinInputProps));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(unGroupInput(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = qSquaredWorkspace;
}

/**
 * Ungroups the output after the execution of the algorithm
 */
void InelasticDataManipulationElwinTab::unGroupInput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(unGroupInput(bool)));
  m_view->setRunIsRunning(false);

  if (!error) {
    if (!m_view->isGroupInput()) {
      auto ungroupAlg = AlgorithmManager::Instance().create("UnGroupWorkspace");
      ungroupAlg->initialize();
      ungroupAlg->setProperty("InputWorkspace", "IDA_Elwin_Input");
      ungroupAlg->execute();
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

bool InelasticDataManipulationElwinTab::validate() { return m_view->validate(); }

void InelasticDataManipulationElwinTab::loadTabSettings(const QSettings &settings) {
  m_uiForm.dsInputFiles->readSettings(settings.group());
}

void InelasticDataManipulationElwinTab::setFileExtensionsByName(bool filter) {
  auto const tabName("Elwin");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
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
  m_view->setDefaultResolution(getInputWorkspace(), getXRangeFromWorkspace(getInputWorkspace()));
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
  ScopedFalse _signalBlock(m_emitCellChanged);
  m_dataTable->setRowCount(0);
  for (auto domainIndex = FitDomainIndex{0}; domainIndex < m_dataModel->getNumberOfDomains(); domainIndex++) {
    addTableEntry(domainIndex);
  }
}

void InelasticDataManipulationElwinTab::addTableEntry(FitDomainIndex row) {
  m_dataTable->insertRow(static_cast<int>(row.value));
  const auto &name = m_dataModel->getWorkspace(row)->getName();
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row.value, 0);

  cell = std::make_unique<QTableWidgetItem>(QString::number(m_dataModel->getSpectrum(row)));
  cell->setFlags(flags);
  setCell(std::move(cell), row.value, workspaceIndexColumn());
}

void InelasticDataManipulationElwinTab::setCell(std::unique_ptr<QTableWidgetItem> cell, FitDomainIndex row,
                                                int column) {
  m_dataTable->setItem(static_cast<int>(row.value), column, cell.release());
}

void InelasticDataManipulationElwinTab::setCellText(const QString &text, FitDomainIndex row, int column) {
  m_dataTable->item(static_cast<int>(row.value), column)->setText(text);
}

int InelasticDataManipulationElwinTab::workspaceIndexColumn() const { return 1; }

void InelasticDataManipulationElwinTab::setHorizontalHeaders(const QStringList &headers) {
  m_dataTable->setColumnCount(headers.size());
  m_dataTable->setHorizontalHeaderLabels(headers);

  auto header = m_dataTable->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
}

void InelasticDataManipulationElwinTab::removeSelectedData() {
  auto selectedIndices = m_dataTable->selectionModel()->selectedIndexes();
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
  if (m_view->validateFileSuffix()) {
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
  auto previewWs = getPreviewPlotWorkspace();
  auto inputWs = getInputWorkspace();
  auto index = boost::numeric_cast<size_t>(m_selectedSpectrum);
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