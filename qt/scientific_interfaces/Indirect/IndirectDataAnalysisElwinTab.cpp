// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisElwinTab.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QFileInfo>

#include <algorithm>

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

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
IndirectDataAnalysisElwinTab::IndirectDataAnalysisElwinTab(QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_elwTree(nullptr) {
  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_uiForm.ipoPlotOptions, this, PlotWidget::Spectra));
}

IndirectDataAnalysisElwinTab::~IndirectDataAnalysisElwinTab() {
  m_elwTree->unsetFactoryForManager(m_dblManager);
  m_elwTree->unsetFactoryForManager(m_blnManager);
}

void IndirectDataAnalysisElwinTab::setup() {
  // Create QtTreePropertyBrowser object
  m_elwTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_elwTree);

  // Editor Factories
  m_elwTree->setFactoryForManager(m_dblManager, m_dblEdFac);
  m_elwTree->setFactoryForManager(m_blnManager, m_blnEdFac);

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
  connect(integrationRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
  connect(integrationRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
  // create the second range
  auto backgroundRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinBackgroundRange");
  backgroundRangeSelector->setColour(Qt::darkGreen); // dark green for background
  connect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
          SLOT(setRange(double, double)));
  connect(backgroundRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
  connect(backgroundRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
  connect(m_blnManager, SIGNAL(valueChanged(QtProperty *, bool)), this, SLOT(twoRanges(QtProperty *, bool)));
  twoRanges(m_properties["BackgroundSubtraction"], false);

  connect(m_uiForm.dsInputFiles, SIGNAL(filesFound()), this, SLOT(newInputFiles()));
  connect(m_uiForm.dsInputFiles, SIGNAL(filesFound()), this, SLOT(plotInput()));
  connect(m_uiForm.dsInputFiles, SIGNAL(filesFound()), this, SLOT(updateIntegrationRange()));
  connect(m_uiForm.cbPreviewFile, SIGNAL(currentIndexChanged(int)), this, SLOT(newPreviewFileSelected(int)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this, SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this, SLOT(handlePreviewSpectrumChanged()));

  // Handle plot and save
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this, SLOT(plotCurrentPreview()));

  // Set any default values
  m_dblManager->setValue(m_properties["IntegrationStart"], -0.02);
  m_dblManager->setValue(m_properties["IntegrationEnd"], 0.02);

  m_dblManager->setValue(m_properties["BackgroundStart"], -0.24);
  m_dblManager->setValue(m_properties["BackgroundEnd"], -0.22);
}

void IndirectDataAnalysisElwinTab::run() {
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
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps runTimeProps;
  runTimeProps["InputWorkspaces"] = inputWorkspacesString;
  groupWsAlg->setProperty("OutputWorkspace", inputGroupWsName);

  m_batchAlgoRunner->addAlgorithm(groupWsAlg, runTimeProps);

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

  BatchAlgorithmRunner::AlgorithmRuntimeProps elwinInputProps;
  elwinInputProps["InputWorkspaces"] = inputGroupWsName;

  m_batchAlgoRunner->addAlgorithm(elwinMultAlg, elwinInputProps);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(unGroupInput(bool)));
  m_batchAlgoRunner->executeBatchAsync();

  // Set the result workspace for Python script export
  m_pythonExportWsName = qSquaredWorkspace;
}

/**
 * Ungroups the output after the execution of the algorithm
 */
void IndirectDataAnalysisElwinTab::unGroupInput(bool error) {
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

void IndirectDataAnalysisElwinTab::checkForELTWorkspace() {
  auto const workspaceName = getOutputBasename().toStdString() + "_elt";
  if (!doesExistInADS(workspaceName))
    showMessageBox("ElasticWindowMultiple successful. \nThe _elt workspace "
                   "was not produced - temperatures were not found.");
}

bool IndirectDataAnalysisElwinTab::validate() {
  UserInputValidator uiv;

  uiv.checkFileFinderWidgetIsValid("Input", m_uiForm.dsInputFiles);

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

  auto const suffixes = getFilteredSuffixes(m_uiForm.dsInputFiles->getFilenames());
  if (std::adjacent_find(suffixes.begin(), suffixes.end(), std::not_equal_to<>()) != suffixes.end())
    uiv.addErrorMessage("The input files must be all _red or all _sqw.");

  QString error = uiv.generateErrorMessage();
  showMessageBox(error);

  return error.isEmpty();
}

void IndirectDataAnalysisElwinTab::loadSettings(const QSettings &settings) {
  m_uiForm.dsInputFiles->readSettings(settings.group());
}

void IndirectDataAnalysisElwinTab::setFileExtensionsByName(bool filter) {
  auto const tabName("Elwin");
  m_uiForm.dsInputFiles->setFileExtensions(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
}

void IndirectDataAnalysisElwinTab::setDefaultResolution(const Mantid::API::MatrixWorkspace_const_sptr &ws,
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

void IndirectDataAnalysisElwinTab::setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws) {
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
void IndirectDataAnalysisElwinTab::newInputFiles() {
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
void IndirectDataAnalysisElwinTab::newPreviewFileSelected(int index) {
  auto const workspaceName = m_uiForm.cbPreviewFile->itemText(index);
  auto const filename = m_uiForm.cbPreviewFile->itemData(index).toString();

  if (!filename.isEmpty()) {
    auto const loadHistory = m_uiForm.ckLoadHistory->isChecked();

    if (loadFile(filename, workspaceName, -1, -1, loadHistory)) {
      auto const workspace = getADSMatrixWorkspace(workspaceName.toStdString());
      int const numHist = static_cast<int>(workspace->getNumberHistograms()) - 1;

      setInputWorkspace(workspace);
      m_uiForm.spPreviewSpec->setMaximum(numHist);
      m_uiForm.spPreviewSpec->setValue(0);
      plotInput();
    } else
      g_log.error("Failed to load input workspace.");
  }
}

/**
 * Replots the preview plot.
 */
void IndirectDataAnalysisElwinTab::plotInput() {
  IndirectDataAnalysisTab::plotInput(m_uiForm.ppPlot);
  setDefaultSampleLog(getInputWorkspace());
}

void IndirectDataAnalysisElwinTab::handlePreviewSpectrumChanged() {
  IndirectDataAnalysisTab::plotInput(m_uiForm.ppPlot);
}

void IndirectDataAnalysisElwinTab::updateIntegrationRange() {
  setDefaultResolution(getInputWorkspace(), getXRangeFromWorkspace(getInputWorkspace()));
}

void IndirectDataAnalysisElwinTab::twoRanges(QtProperty *prop, bool enabled) {
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

void IndirectDataAnalysisElwinTab::minChanged(double val) {
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

void IndirectDataAnalysisElwinTab::maxChanged(double val) {
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

void IndirectDataAnalysisElwinTab::updateRS(QtProperty *prop, double val) {
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

void IndirectDataAnalysisElwinTab::runClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}

/**
 * Handles saving of workspaces
 */
void IndirectDataAnalysisElwinTab::saveClicked() {
  for (auto const &name : getOutputWorkspaceNames())
    addSaveWorkspaceToQueue(name);
  m_batchAlgoRunner->executeBatchAsync();
}

std::vector<std::string> IndirectDataAnalysisElwinTab::getOutputWorkspaceNames() {
  auto outputNames = attachPrefix(getOutputWorkspaceSuffices(), getOutputBasename().toStdString());
  removeElementsIf(outputNames, [](std::string const &workspaceName) { return !doesExistInADS(workspaceName); });
  return outputNames;
}

QString IndirectDataAnalysisElwinTab::getOutputBasename() {
  return getWorkspaceBasename(QString::fromStdString(m_pythonExportWsName));
}

void IndirectDataAnalysisElwinTab::setRunIsRunning(const bool &running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
  m_uiForm.ppPlot->watchADS(!running);
}

void IndirectDataAnalysisElwinTab::setButtonsEnabled(const bool &enabled) {
  setRunEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void IndirectDataAnalysisElwinTab::setRunEnabled(const bool &enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void IndirectDataAnalysisElwinTab::setSaveResultEnabled(const bool &enabled) { m_uiForm.pbSave->setEnabled(enabled); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
