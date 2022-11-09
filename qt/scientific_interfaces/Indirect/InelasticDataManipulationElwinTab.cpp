// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTab.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

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
    : InelasticDataManipulationTab(parent), m_elwTree(nullptr), m_dataModel(std::make_unique<IndirectFitDataModel>()),
      m_selectedSpectrum(0) {

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);

  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_uiForm.ipoPlotOptions, PlotWidget::Spectra));
  connect(m_uiForm.wkspAdd, SIGNAL(clicked()), this, SLOT(showAddWorkspaceDialog()));
  connect(m_uiForm.wkspRemove, SIGNAL(clicked()), this, SLOT(removeSelectedData()));
  connect(m_uiForm.wkspRemove, SIGNAL(clicked()), this, SIGNAL(dataRemoved()));

  m_parent = dynamic_cast<InelasticDataManipulation *>(parent);
  m_dataTable = getDataTable();

  const QStringList headers = defaultHeaders();
  setHorizontalHeaders(headers);
  m_dataTable->setItemDelegateForColumn(headers.size() - 1, std::make_unique<ExcludeRegionDelegate>().release());
  m_dataTable->verticalHeader()->setVisible(false);
}

InelasticDataManipulationElwinTab::~InelasticDataManipulationElwinTab() {
  m_elwTree->unsetFactoryForManager(m_dblManager);
  m_elwTree->unsetFactoryForManager(m_blnManager);
}

void InelasticDataManipulationElwinTab::setup() {
  // Create QtTreePropertyBrowser object
  m_elwTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_elwTree);

  // Editor Factories
  m_elwTree->setFactoryForManager(m_dblManager, m_dblEdFac);
  m_elwTree->setFactoryForManager(m_blnManager, m_blnEdFac);

  // Number of decimal places in property browsers.
  static const unsigned int NUM_DECIMALS = 6;
  // Create Properties
  m_properties["IntegrationStart"] = m_dblManager->addProperty("Start");
  m_dblManager->setDecimals(m_properties["IntegrationStart"], NUM_DECIMALS);
  m_properties["IntegrationEnd"] = m_dblManager->addProperty("End");
  m_dblManager->setDecimals(m_properties["IntegrationEnd"], NUM_DECIMALS);
  m_properties["BackgroundStart"] = m_dblManager->addProperty("Start");
  m_dblManager->setDecimals(m_properties["BackgroundStart"], NUM_DECIMALS);
  m_properties["BackgroundEnd"] = m_dblManager->addProperty("End");
  m_dblManager->setDecimals(m_properties["BackgroundEnd"], NUM_DECIMALS);

  m_properties["BackgroundSubtraction"] = m_blnManager->addProperty("Background Subtraction");
  m_properties["Normalise"] = m_blnManager->addProperty("Normalise to Lowest Temp");

  m_properties["IntegrationRange"] = m_grpManager->addProperty("Integration Range");
  m_properties["IntegrationRange"]->addSubProperty(m_properties["IntegrationStart"]);
  m_properties["IntegrationRange"]->addSubProperty(m_properties["IntegrationEnd"]);
  m_properties["BackgroundRange"] = m_grpManager->addProperty("Background Range");
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundStart"]);
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundEnd"]);

  m_elwTree->addProperty(m_properties["IntegrationRange"]);
  m_elwTree->addProperty(m_properties["BackgroundSubtraction"]);
  m_elwTree->addProperty(m_properties["BackgroundRange"]);
  m_elwTree->addProperty(m_properties["Normalise"]);

  // We always want one range selector... the second one can be controlled from
  // within the elwinTwoRanges(bool state) function
  auto integrationRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinIntegrationRange");
  integrationRangeSelector->setBounds(-DBL_MAX, DBL_MAX);
  connect(integrationRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
  connect(integrationRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
  // create the second range
  auto backgroundRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinBackgroundRange");
  backgroundRangeSelector->setColour(Qt::darkGreen); // dark green for background
  backgroundRangeSelector->setBounds(-DBL_MAX, DBL_MAX);
  connect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
          SLOT(setRange(double, double)));
  connect(backgroundRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
  connect(backgroundRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
  connect(m_blnManager, SIGNAL(valueChanged(QtProperty *, bool)), this, SLOT(twoRanges(QtProperty *, bool)));
  twoRanges(m_properties["BackgroundSubtraction"], false);

  connect(m_uiForm.dsInputFiles, SIGNAL(filesFound()), this, SLOT(checkLoadedFiles()));
  connect(m_uiForm.cbPreviewFile, SIGNAL(currentIndexChanged(int)), this, SLOT(checkNewPreviewSelected(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(handlePreviewSpectrumChanged()));
  connect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(handlePreviewSpectrumChanged()));

  // Handle plot and save
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this, SLOT(plotCurrentPreview()));

  // Set any default values
  m_dblManager->setValue(m_properties["IntegrationStart"], -0.02);
  m_dblManager->setValue(m_properties["IntegrationEnd"], 0.02);

  m_dblManager->setValue(m_properties["BackgroundStart"], -0.24);
  m_dblManager->setValue(m_properties["BackgroundEnd"], -0.22);

  updateAvailableSpectra();
}

void InelasticDataManipulationElwinTab::run() {
  if (m_uiForm.inputChoice->currentIndex() == 0) {
    runFileInput();
  } else {
    runWorkspaceInput();
  }
}

void InelasticDataManipulationElwinTab::runFileInput() {
  setRunIsRunning(true);

  QStringList inputFilenames = m_uiForm.dsInputFiles->getFilenames();
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

  elwinMultAlg->setProperty("SampleEnvironmentLogName", m_uiForm.leLogName->text().toStdString());
  elwinMultAlg->setProperty("SampleEnvironmentLogValue", m_uiForm.leLogValue->currentText().toStdString());

  elwinMultAlg->setProperty("IntegrationRangeStart", m_dblManager->value(m_properties["IntegrationStart"]));
  elwinMultAlg->setProperty("IntegrationRangeEnd", m_dblManager->value(m_properties["IntegrationEnd"]));

  if (m_blnManager->value(m_properties["BackgroundSubtraction"])) {
    elwinMultAlg->setProperty("BackgroundRangeStart", m_dblManager->value(m_properties["BackgroundStart"]));
    elwinMultAlg->setProperty("BackgroundRangeEnd", m_dblManager->value(m_properties["BackgroundEnd"]));
  }

  if (m_blnManager->value(m_properties["Normalise"])) {
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
  setRunIsRunning(true);

  // Get workspace names
  std::string inputGroupWsName = "IDA_Elwin_Input";

  auto workspaceBaseName = m_uiForm.cbPreviewFile->currentText();

  workspaceBaseName += "_elwin_";

  const auto qWorkspace = (workspaceBaseName + "eq").toStdString();
  const auto qSquaredWorkspace = (workspaceBaseName + "eq2").toStdString();
  const auto elfWorkspace = (workspaceBaseName + "elf").toStdString();
  const auto eltWorkspace = (workspaceBaseName + "elt").toStdString();

  // Load input files
  std::string inputWorkspacesString = m_uiForm.cbPreviewFile->currentText().toStdString();

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

  elwinMultAlg->setProperty("SampleEnvironmentLogName", m_uiForm.leLogName->text().toStdString());
  elwinMultAlg->setProperty("SampleEnvironmentLogValue", m_uiForm.leLogValue->currentText().toStdString());

  elwinMultAlg->setProperty("IntegrationRangeStart", m_dblManager->value(m_properties["IntegrationStart"]));
  elwinMultAlg->setProperty("IntegrationRangeEnd", m_dblManager->value(m_properties["IntegrationEnd"]));

  if (m_blnManager->value(m_properties["BackgroundSubtraction"])) {
    elwinMultAlg->setProperty("BackgroundRangeStart", m_dblManager->value(m_properties["BackgroundStart"]));
    elwinMultAlg->setProperty("BackgroundRangeEnd", m_dblManager->value(m_properties["BackgroundEnd"]));
  }

  if (m_blnManager->value(m_properties["Normalise"])) {
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
  setRunIsRunning(false);

  if (!error) {
    if (!m_uiForm.ckGroupInput->isChecked()) {
      auto ungroupAlg = AlgorithmManager::Instance().create("UnGroupWorkspace");
      ungroupAlg->initialize();
      ungroupAlg->setProperty("InputWorkspace", "IDA_Elwin_Input");
      ungroupAlg->execute();
    }

    setOutputPlotOptionsWorkspaces(getOutputWorkspaceNames());

    if (m_blnManager->value(m_properties["Normalise"]))
      checkForELTWorkspace();

  } else {
    setSaveResultEnabled(false);
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

  if (m_uiForm.inputChoice->currentIndex() == 0) {
    uiv.checkFileFinderWidgetIsValid("Input", m_uiForm.dsInputFiles);
    auto const suffixes = getFilteredSuffixes(m_uiForm.dsInputFiles->getFilenames());
    if (std::adjacent_find(suffixes.begin(), suffixes.end(), std::not_equal_to<>()) != suffixes.end())
      uiv.addErrorMessage("The input files must be all _red or all _sqw.");
  }

  auto rangeOne = std::make_pair(m_dblManager->value(m_properties["IntegrationStart"]),
                                 m_dblManager->value(m_properties["IntegrationEnd"]));
  uiv.checkValidRange("Range One", rangeOne);

  bool useTwoRanges = m_blnManager->value(m_properties["BackgroundSubtraction"]);
  if (useTwoRanges) {
    auto rangeTwo = std::make_pair(m_dblManager->value(m_properties["BackgroundStart"]),
                                   m_dblManager->value(m_properties["BackgroundEnd"]));
    uiv.checkValidRange("Range Two", rangeTwo);
    uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
  }

  QString error = uiv.generateErrorMessage();
  showMessageBox(error);

  return error.isEmpty();
}

void InelasticDataManipulationElwinTab::loadTabSettings(const QSettings &settings) {
  m_uiForm.dsInputFiles->readSettings(settings.group());
}

void InelasticDataManipulationElwinTab::setFileExtensionsByName(bool filter) {
  auto const tabName("Elwin");
  m_uiForm.dsInputFiles->setFileExtensions(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
}

void InelasticDataManipulationElwinTab::setDefaultResolution(const Mantid::API::MatrixWorkspace_const_sptr &ws,
                                                             const QPair<double, double> &range) {
  auto inst = ws->getInstrument();
  auto analyser = inst->getStringParameter("analyser");

  if (analyser.size() > 0) {
    auto comp = inst->getComponentByName(analyser[0]);

    if (comp) {
      auto params = comp->getNumberParameter("resolution", true);

      // set the default instrument resolution
      if (!params.empty()) {
        double res = params[0];
        m_dblManager->setValue(m_properties["IntegrationStart"], -res);
        m_dblManager->setValue(m_properties["IntegrationEnd"], res);

        m_dblManager->setValue(m_properties["BackgroundStart"], -10 * res);
        m_dblManager->setValue(m_properties["BackgroundEnd"], -9 * res);
      } else {
        m_dblManager->setValue(m_properties["IntegrationStart"], range.first);
        m_dblManager->setValue(m_properties["IntegrationEnd"], range.second);
      }
    } else {
      showMessageBox("Warning: The instrument definition file for the input "
                     "workspace contains an invalid value.");
    }
  }
}

void InelasticDataManipulationElwinTab::setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws) {
  auto inst = ws->getInstrument();
  // Set sample environment log name
  auto log = inst->getStringParameter("Workflow.SE-log");
  QString logName("sample");
  if (log.size() > 0) {
    logName = QString::fromStdString(log[0]);
  }
  m_uiForm.leLogName->setText(logName);
  // Set sample environment log value
  auto logval = inst->getStringParameter("Workflow.SE-log-value");
  if (logval.size() > 0) {
    auto logValue = QString::fromStdString(logval[0]);
    int index = m_uiForm.leLogValue->findText(logValue);
    if (index >= 0) {
      m_uiForm.leLogValue->setCurrentIndex(index);
    }
  }
}

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTab::newInputFiles() {
  // Clear the existing list of files
  m_uiForm.cbPreviewFile->clear();

  // Populate the combo box with the filenames
  QStringList filenames = m_uiForm.dsInputFiles->getFilenames();
  for (auto rawFilename : filenames) {
    QFileInfo inputFileInfo(rawFilename);
    QString sampleName = inputFileInfo.baseName();

    // Add the item using the base filename as the display string and the raw
    // filename as the data value
    m_uiForm.cbPreviewFile->addItem(sampleName, rawFilename);
  }

  // Default to the first file
  m_uiForm.cbPreviewFile->setCurrentIndex(0);
  QString const wsname = m_uiForm.cbPreviewFile->currentText();
  auto const inputWs = getADSMatrixWorkspace(wsname.toStdString());
  setInputWorkspace(inputWs);

  const auto range = getXRangeFromWorkspace(inputWs);

  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), m_properties["IntegrationStart"],
                   m_properties["IntegrationEnd"], range);
  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), m_properties["BackgroundStart"],
                   m_properties["BackgroundEnd"], range);
}

/**
 * Handles a new input file being selected for preview.
 *
 * Loads the file and resets the spectra selection spinner.
 *
 * @param index Index of the new selected file
 */

void InelasticDataManipulationElwinTab::checkNewPreviewSelected(int index) {
  auto const workspaceName = m_uiForm.cbPreviewFile->itemText(index);
  auto const filename = m_uiForm.cbPreviewFile->itemData(index).toString();

  if (!workspaceName.isEmpty()) {
    if (!filename.isEmpty())
      newPreviewFileSelected(workspaceName, filename);
    else
      newPreviewWorkspaceSelected(workspaceName);
  }
}

void InelasticDataManipulationElwinTab::newPreviewFileSelected(const QString &workspaceName, const QString &filename) {
  auto const loadHistory = m_uiForm.ckLoadHistory->isChecked();
  if (loadFile(filename, workspaceName, -1, -1, loadHistory)) {
    auto const workspace = getADSMatrixWorkspace(workspaceName.toStdString());

    setInputWorkspace(workspace);

    if (m_uiForm.inputChoice->currentIndex() == 0) {
      int const numHist = static_cast<int>(workspace->getNumberHistograms()) - 1;
      m_uiForm.spPlotSpectrum->setMaximum(numHist);
      m_uiForm.spPlotSpectrum->setValue(0);
    } else
      updateAvailableSpectra();

    plotInput();
  }
}

void InelasticDataManipulationElwinTab::newPreviewWorkspaceSelected(const QString &workspaceName) {
  if (m_uiForm.inputChoice->currentIndex() == 1) {
    auto const workspace = getADSMatrixWorkspace(workspaceName.toStdString());
    setInputWorkspace(workspace);
    updateAvailableSpectra();
    plotInput();
  }
}

/**
 * Replots the preview plot.
 */
void InelasticDataManipulationElwinTab::plotInput() {
  plotInput(m_uiForm.ppPlot);
  setDefaultSampleLog(getInputWorkspace());
}

/**
 * Plots the selected spectrum of the input workspace.
 *
 * @param previewPlot The preview plot widget in which to plot the input
 *                    input workspace.
 */
void InelasticDataManipulationElwinTab::plotInput(MantidQt::MantidWidgets::PreviewPlot *previewPlot) {
  previewPlot->clear();
  auto inputWS = getInputWorkspace();
  auto spectrum = getSelectedSpectrum();

  if (inputWS && inputWS->x(spectrum).size() > 1)
    previewPlot->addSpectrum("Sample", getInputWorkspace(), spectrum);
}

void InelasticDataManipulationElwinTab::handlePreviewSpectrumChanged() {
  if (m_uiForm.elwinPreviewSpec->currentIndex() == 1)
    setSelectedSpectrum(m_uiForm.cbPlotSpectrum->currentText().toInt());
  plotInput(m_uiForm.ppPlot);
}

void InelasticDataManipulationElwinTab::updateIntegrationRange() {
  setDefaultResolution(getInputWorkspace(), getXRangeFromWorkspace(getInputWorkspace()));
}

void InelasticDataManipulationElwinTab::twoRanges(QtProperty *prop, bool enabled) {
  if (prop == m_properties["BackgroundSubtraction"]) {
    auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
    auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");
    backgroundRangeSelector->setVisible(enabled);
    m_properties["BackgroundStart"]->setEnabled(enabled);
    m_properties["BackgroundEnd"]->setEnabled(enabled);

    disconnect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
               SLOT(setRange(double, double)));
    if (!enabled) {
      backgroundRangeSelector->setRange(integrationRangeSelector->getRange());
      connect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
              SLOT(setRange(double, double)));
    }
  }
}

void InelasticDataManipulationElwinTab::minChanged(double val) {
  auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
  if (from == integrationRangeSelector) {
    m_dblManager->setValue(m_properties["IntegrationStart"], val);
  } else if (from == backgroundRangeSelector) {
    m_dblManager->setValue(m_properties["BackgroundStart"], val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
}

void InelasticDataManipulationElwinTab::maxChanged(double val) {
  auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));

  if (from == integrationRangeSelector) {
    m_dblManager->setValue(m_properties["IntegrationEnd"], val);
  } else if (from == backgroundRangeSelector) {
    m_dblManager->setValue(m_properties["BackgroundEnd"], val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
}

void InelasticDataManipulationElwinTab::updateRS(QtProperty *prop, double val) {
  auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));

  if (prop == m_properties["IntegrationStart"])
    setRangeSelectorMin(m_properties["IntegrationStart"], m_properties["IntegrationEnd"], integrationRangeSelector,
                        val);
  else if (prop == m_properties["IntegrationEnd"])
    setRangeSelectorMax(m_properties["IntegrationStart"], m_properties["IntegrationEnd"], integrationRangeSelector,
                        val);
  else if (prop == m_properties["BackgroundStart"])
    setRangeSelectorMin(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);
  else if (prop == m_properties["BackgroundEnd"])
    setRangeSelectorMax(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
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

void InelasticDataManipulationElwinTab::setRunIsRunning(const bool &running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
  m_uiForm.ppPlot->watchADS(!running);
}

void InelasticDataManipulationElwinTab::setButtonsEnabled(const bool &enabled) {
  setRunEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void InelasticDataManipulationElwinTab::setRunEnabled(const bool &enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void InelasticDataManipulationElwinTab::setSaveResultEnabled(const bool &enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
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
    plotInput();
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
      m_uiForm.dsInputFiles->clear();
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

QTableWidget *InelasticDataManipulationElwinTab::getDataTable() const { return m_uiForm.tbElwinData; }

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

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTab::newInputFilesFromDialog(IAddWorkspaceDialog const *dialog) {
  // Clear the existing list of files
  if (m_dataModel->getNumberOfWorkspaces().value < 2)
    m_uiForm.cbPreviewFile->clear();

  // Populate the combo box with the filenames
  QString workspaceNames;
  QString filename;
  if (const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog)) {
    workspaceNames = QString::fromStdString(indirectDialog->workspaceName());
    filename = QString::fromStdString(indirectDialog->getFileName());
  }

  m_uiForm.cbPreviewFile->addItem(workspaceNames, filename);

  // Default to the first file
  m_uiForm.cbPreviewFile->setCurrentIndex(0);
  QString const wsname = m_uiForm.cbPreviewFile->currentText();
  auto const inputWs = getADSMatrixWorkspace(wsname.toStdString());
  setInputWorkspace(inputWs);

  const auto range = getXRangeFromWorkspace(inputWs);

  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), m_properties["IntegrationStart"],
                   m_properties["IntegrationEnd"], range);
  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), m_properties["BackgroundStart"],
                   m_properties["BackgroundEnd"], range);
}

void InelasticDataManipulationElwinTab::setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  m_uiForm.elwinPreviewSpec->setCurrentIndex(0);
  m_uiForm.spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum.value));
  m_uiForm.spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum.value));
}

void InelasticDataManipulationElwinTab::setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                                            const std::vector<WorkspaceIndex>::const_iterator &to) {
  m_uiForm.elwinPreviewSpec->setCurrentIndex(1);
  m_uiForm.cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_uiForm.cbPlotSpectrum->addItem(QString::number(spectrum->value));
}

void InelasticDataManipulationElwinTab::updateAvailableSpectra() {
  auto spectra = m_dataModel->getSpectra(findWorkspaceID());
  if (m_uiForm.inputChoice->currentIndex() == 1) {
    if (spectra.isContinuous()) {
      auto const minmax = spectra.getMinMax();
      setAvailableSpectra(minmax.first, minmax.second);
    } else {
      setAvailableSpectra(spectra.begin(), spectra.end());
    }
  }
}

size_t InelasticDataManipulationElwinTab::findWorkspaceID() {
  auto currentWorkspace = m_uiForm.cbPreviewFile->currentText().toStdString();
  auto allWorkspaces = m_dataModel->getWorkspaceNames();
  auto findWorkspace = find(allWorkspaces.begin(), allWorkspaces.end(), currentWorkspace);
  size_t workspaceID = findWorkspace - allWorkspaces.begin();
  return workspaceID;
}

void InelasticDataManipulationElwinTab::checkLoadedFiles() {
  UserInputValidator uiv;
  size_t noOfFiles = m_uiForm.dsInputFiles->getFilenames().size();
  auto const suffixes = getFilteredSuffixes(m_uiForm.dsInputFiles->getFilenames());
  if (suffixes.size() != noOfFiles) {
    uiv.addErrorMessage("The input files must be all _red or all _sqw.");
    m_uiForm.dsInputFiles->clear();
  }
  QString error = uiv.generateErrorMessage();
  showMessageBox(error);

  if (error.isEmpty()) {
    newInputFiles();
    plotInput();
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

} // namespace MantidQt::CustomInterfaces