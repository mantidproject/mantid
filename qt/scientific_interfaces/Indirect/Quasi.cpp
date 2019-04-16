// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Quasi.h"
#include "../General/UserInputValidator.h"
#include "MantidAPI/TextAxis.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
Quasi::Quasi(QWidget *parent) : IndirectBayesTab(parent), m_previewSpec(0) {
  m_uiForm.setupUi(parent);

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("QuasiERange");
  connect(eRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minValueChanged(double)));
  connect(eRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxValueChanged(double)));

  // Add the properties browser to the UI form
  m_uiForm.treeSpace->addWidget(m_propTree);

  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
  m_properties["ResBinning"] = m_dblManager->addProperty("Resolution Binning");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
  m_dblManager->setDecimals(m_properties["ResBinning"], INT_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);
  m_propTree->addProperty(m_properties["SampleBinning"]);
  m_propTree->addProperty(m_properties["ResBinning"]);

  formatTreeWidget(m_propTree, m_properties);

  // Set default values
  m_dblManager->setValue(m_properties["SampleBinning"], 1);
  m_dblManager->setMinimum(m_properties["SampleBinning"], 1);
  m_dblManager->setValue(m_properties["ResBinning"], 1);
  m_dblManager->setMinimum(m_properties["ResBinning"], 1);

  // Connect optional form elements with enabling checkboxes
  connect(m_uiForm.chkFixWidth, SIGNAL(toggled(bool)), m_uiForm.mwFixWidthDat,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.chkUseResNorm, SIGNAL(toggled(bool)), m_uiForm.dsResNorm,
          SLOT(setEnabled(bool)));

  // Connect the data selector for the sample to the mini plot
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleSampleInputReady(const QString &)));

  connect(m_uiForm.dsResolution, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleResolutionInputReady(const QString &)));

  // Connect the program selector to its handler
  connect(m_uiForm.cbProgram, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleProgramChange(int)));

  // Connect preview spectrum spinner to handler
  connect(m_uiForm.spPreviewSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(previewSpecChanged(int)));

  // Plot current preview
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void Quasi::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
  m_uiForm.dsResNorm->readSettings(settings.group());
  m_uiForm.mwFixWidthDat->readSettings(settings.group());
}

void Quasi::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool Quasi::validate() {
  UserInputValidator uiv;
  QString errors("");
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  // check that the ResNorm file is valid if we are using it
  if (m_uiForm.chkUseResNorm->isChecked()) {
    uiv.checkDataSelectorIsValid("ResNorm", m_uiForm.dsResNorm);
  }

  // check fixed width file exists
  if (m_uiForm.chkFixWidth->isChecked() && !m_uiForm.mwFixWidthDat->isValid()) {
    uiv.checkMWRunFilesIsValid("Width", m_uiForm.mwFixWidthDat);
  }

  // check eMin and eMax values
  const auto eMin = m_dblManager->value(m_properties["EMin"]);
  const auto eMax = m_dblManager->value(m_properties["EMax"]);
  if (eMin >= eMax)
    errors.append("EMin must be strictly less than EMax.\n");

  // Create and show error messages
  errors.append(uiv.generateErrorMessage());
  auto test = errors.toStdString();
  if (!errors.isEmpty()) {
    emit showMessageBox(errors);
    return false;
  }

  // Validate program
  QString program = m_uiForm.cbProgram->currentText();
  if (program == "Stretched Exponential") {
    QString resName = m_uiForm.dsResolution->getCurrentDataName();
    if (!resName.endsWith("_res")) {
      emit showMessageBox("Stretched Exponential program can only be used with "
                          "a resolution file.");
      return false;
    }
  }

  return true;
}

/**
 * Run the BayesQuasi algorithm
 */
void Quasi::run() {
  bool elasticPeak = false;
  bool sequence = false;

  bool fixedWidth = false;
  std::string fixedWidthFile("");

  bool useResNorm = false;
  std::string resNormFile("");

  std::string const sampleName =
      m_uiForm.dsSample->getCurrentDataName().toStdString();
  std::string const resName =
      m_uiForm.dsResolution->getCurrentDataName().toStdString();

  std::string program = m_uiForm.cbProgram->currentText().toStdString();

  if (program == "Lorentzians") {
    program = "QL";
  } else {
    program = "QSe";
  }

  // Collect input from fit options section
  std::string const background =
      m_uiForm.cbBackground->currentText().toStdString();

  if (m_uiForm.chkElasticPeak->isChecked()) {
    elasticPeak = true;
  }
  if (m_uiForm.chkSequentialFit->isChecked()) {
    sequence = true;
  }

  if (m_uiForm.chkFixWidth->isChecked()) {
    fixedWidth = true;
    fixedWidthFile = m_uiForm.mwFixWidthDat->getFirstFilename().toStdString();
  }

  if (m_uiForm.chkUseResNorm->isChecked()) {
    useResNorm = true;
    resNormFile = m_uiForm.dsResNorm->getCurrentDataName().toStdString();
  }

  // Collect input from the properties browser
  double const eMin = m_properties["EMin"]->valueText().toDouble();
  double const eMax = m_properties["EMax"]->valueText().toDouble();

  long const sampleBins = m_properties["SampleBinning"]->valueText().toLong();
  long const resBins = m_properties["ResBinning"]->valueText().toLong();

  IAlgorithm_sptr runAlg = AlgorithmManager::Instance().create("BayesQuasi");
  runAlg->initialize();
  runAlg->setProperty("Program", program);
  runAlg->setProperty("SampleWorkspace", sampleName);
  runAlg->setProperty("ResolutionWorkspace", resName);
  runAlg->setProperty("ResNormWorkspace", resNormFile);
  runAlg->setProperty("OutputWorkspaceFit", "fit");
  runAlg->setProperty("OutputWorkspaceProb", "prob");
  runAlg->setProperty("OutputWorkspaceResult", "result");
  runAlg->setProperty("MinRange", eMin);
  runAlg->setProperty("MaxRange", eMax);
  runAlg->setProperty("SampleBins", sampleBins);
  runAlg->setProperty("ResolutionBins", resBins);
  runAlg->setProperty("Elastic", elasticPeak);
  runAlg->setProperty("Background", background);
  runAlg->setProperty("FixedWidth", fixedWidth);
  runAlg->setProperty("UseResNorm", useResNorm);
  runAlg->setProperty("WidthFile", fixedWidthFile);
  runAlg->setProperty("Loop", sequence);

  m_QuasiAlg = runAlg;
  m_batchAlgoRunner->addAlgorithm(runAlg);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));

  m_batchAlgoRunner->executeBatchAsync();
}
/**
 * Enable plotting and saving and fit curves on the mini plot.
 */
void Quasi::algorithmComplete(bool error) {
  setRunIsRunning(false);
  if (!error)
    updateMiniPlot();
  else {
    setPlotResultEnabled(false);
    setSaveResultEnabled(false);
  }
}

void Quasi::updateMiniPlot() {
  // Update sample plot
  if (!m_uiForm.dsSample->isValid())
    return;

  m_uiForm.ppPlot->clear();

  QString sampleName = m_uiForm.dsSample->getCurrentDataName();
  m_uiForm.ppPlot->addSpectrum("Sample", sampleName, m_previewSpec);

  // Update fit plot
  QString program = m_uiForm.cbProgram->currentText();
  if (program == "Lorentzians")
    program = "QL";
  else
    program = "QSe";

  QString resName = m_uiForm.dsResolution->getCurrentDataName();

  // Should be either "red", "sqw" or "res"
  QString resType = resName.right(3);

  // Get the correct workspace name based on the type of resolution file
  if (program == "QL") {
    if (resType == "res")
      program += "r";
    else
      program += "d";
  }

  QString outWsName = sampleName.left(sampleName.size() - 3) + program +
                      "_Workspace_" + QString::number(m_previewSpec);
  if (!AnalysisDataService::Instance().doesExist(outWsName.toStdString()))
    return;

  MatrixWorkspace_sptr outputWorkspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          outWsName.toStdString());

  TextAxis *axis = dynamic_cast<TextAxis *>(outputWorkspace->getAxis(1));

  for (size_t histIndex = 0; histIndex < outputWorkspace->getNumberHistograms();
       histIndex++) {
    QString specName = QString::fromStdString(axis->label(histIndex));
    QColor curveColour;

    if (specName.contains("fit.1"))
      curveColour = Qt::red;
    else if (specName.contains("fit.2"))
      curveColour = Qt::magenta;

    else if (specName.contains("diff.1"))
      curveColour = Qt::blue;
    else if (specName.contains("diff.2"))
      curveColour = Qt::cyan;

    else
      continue;

    m_uiForm.ppPlot->addSpectrum(specName, outputWorkspace, histIndex,
                                 curveColour);
  }
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void Quasi::handleSampleInputReady(const QString &filename) {
  MatrixWorkspace_sptr inWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          filename.toStdString());
  int numHist = static_cast<int>(inWs->getNumberHistograms()) - 1;
  m_uiForm.spPreviewSpectrum->setMaximum(numHist);
  updateMiniPlot();

  QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("QuasiERange");

  setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                   range);
  setPlotPropertyRange(eRangeSelector, m_properties["EMin"],
                       m_properties["EMax"], range);
  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);
}

/**
 * Plots the current preview on the miniplot
 */
void Quasi::plotCurrentPreview() {

  if (m_uiForm.ppPlot->hasCurve("fit.1")) {
    QString program = m_uiForm.cbProgram->currentText();
    auto fitName = m_QuasiAlg->getPropertyValue("OutputWorkspaceFit");
    checkADSForPlotSaveWorkspace(fitName, false);
    fitName.pop_back();
    QString QfitWS = QString::fromStdString(fitName + "_");
    QfitWS += QString::number(m_previewSpec);
    if (program == "Lorentzians")
      plotSpectra(QfitWS, {0, 1, 2, 3, 4});
    else
      plotSpectra(QfitWS, {0, 1, 2});
  } else if (m_uiForm.ppPlot->hasCurve("Sample")) {
    plotSpectrum(m_uiForm.dsSample->getCurrentDataName(), m_previewSpec);
  }
}

/**
 * Toggles the use ResNorm option depending on if the resolution file is a
 * resolution or vanadium reduction.
 * @param wsName The name of the workspace loaded
 */
void Quasi::handleResolutionInputReady(const QString &wsName) {
  bool isResolution(wsName.endsWith("_res"));

  m_uiForm.chkUseResNorm->setEnabled(isResolution);
  if (!isResolution)
    m_uiForm.chkUseResNorm->setChecked(false);
}

/**
 * Updates the property manager when the lower guide is moved on the mini plot
 *
 * @param min :: The new value of the lower guide
 */
void Quasi::minValueChanged(double min) {
  m_dblManager->setValue(m_properties["EMin"], min);
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void Quasi::maxValueChanged(double max) {
  m_dblManager->setValue(m_properties["EMax"], max);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void Quasi::updateProperties(QtProperty *prop, double val) {
  UNUSED_ARG(val);

  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("QuasiERange");

  if (prop == m_properties["EMin"] || prop == m_properties["EMax"]) {
    auto bounds = qMakePair(m_dblManager->value(m_properties["EMin"]),
                            m_dblManager->value(m_properties["EMax"]));
    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                     bounds);
  }
}

/**
 * Handles when the selected item in the program combobox
 * is changed
 *
 * @param index :: The current index of the combobox
 */
void Quasi::handleProgramChange(int index) {
  int numberOptions = m_uiForm.cbPlot->count();
  switch (index) {
  case 0:
    m_uiForm.cbPlot->setItemText(numberOptions - 2, "Prob");
    break;
  case 1:
    m_uiForm.cbPlot->setItemText(numberOptions - 2, "Beta");
    break;
  }
}

/**
 * Handles setting a new preview spectrum on the preview plot.
 *
 * @param value workspace index
 */
void Quasi::previewSpecChanged(int value) {
  m_previewSpec = value;
  updateMiniPlot();
}

/**
 * Handles saving the workspace when save is clicked
 */
void Quasi::saveClicked() {
  QString saveDirectory = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "defaultsave.directory"));
  auto const fitWS = m_QuasiAlg->getPropertyValue("OutputWorkspaceFit");
  IndirectTab::checkADSForPlotSaveWorkspace(fitWS, false);
  QString const QfitWS = QString::fromStdString(fitWS);
  auto const fitPath = saveDirectory + QfitWS + ".nxs";
  addSaveWorkspaceToQueue(QfitWS, fitPath);

  auto const resultWS = m_QuasiAlg->getPropertyValue("OutputWorkspaceResult");
  IndirectTab::checkADSForPlotSaveWorkspace(resultWS, false);
  QString const QresultWS = QString::fromStdString(resultWS);
  auto const resultPath = saveDirectory + QresultWS + ".nxs";
  addSaveWorkspaceToQueue(QresultWS, resultPath);
  m_batchAlgoRunner->executeBatchAsync();
}

void Quasi::runClicked() {
  if (validateTab()) {
    auto const saveDirectory =
        Mantid::Kernel::ConfigService::Instance().getString(
            "defaultsave.directory");
    displayMessageAndRun(saveDirectory);
  }
}

void Quasi::displayMessageAndRun(std::string const &saveDirectory) {
  if (saveDirectory.empty()) {
    int const result = displaySaveDirectoryMessage();
    if (result != QMessageBox::No) {
      setRunIsRunning(true);
      runTab();
    }
  } else {
    setRunIsRunning(true);
    runTab();
  }
}

int Quasi::displaySaveDirectoryMessage() {
  char const *textMessage =
      "BayesQuasi requires a default save directory and "
      "one is not currently set."
      " If run, the algorithm will default to saving files "
      "to the current working directory."
      " Would you still like to run the algorithm?";
  return QMessageBox::question(nullptr, tr("Save Directory"), tr(textMessage),
                               QMessageBox::Yes, QMessageBox::No,
                               QMessageBox::NoButton);
}

/**
 * Handles plotting the selected plot when plot is clicked
 */
void Quasi::plotClicked() {
  setPlotResultIsPlotting(true);

  // Output options
  std::string const plot = m_uiForm.cbPlot->currentText().toStdString();
  QString const program = m_uiForm.cbProgram->currentText();
  auto const resultName = m_QuasiAlg->getPropertyValue("OutputWorkspaceResult");
  if ((plot == "Prob" || plot == "All") && (program == "Lorentzians")) {
    auto const probWS = m_QuasiAlg->getPropertyValue("OutputWorkspaceProb");
    // Check workspace exists
    IndirectTab::checkADSForPlotSaveWorkspace(probWS, true);
    QString const QprobWS = QString::fromStdString(probWS);
    IndirectTab::plotSpectrum(QprobWS, 1, 2);
  }

  auto const resultWS =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(resultName);
  int const numSpectra = (int)resultWS->getNumberHistograms();
  IndirectTab::checkADSForPlotSaveWorkspace(resultName, true);
  QString const QresultWS = QString::fromStdString(resultName);
  auto const paramNames = {"Amplitude", "FWHM", "Beta"};
  for (std::string const &paramName : paramNames) {

    if (plot == paramName || plot == "All") {
      std::vector<int> spectraIndices = {};
      for (int i = 0; i < numSpectra; i++) {
        auto axisLabel = resultWS->getAxis(1)->label(i);

        auto const found = axisLabel.find(paramName);
        if (found != std::string::npos) {
          spectraIndices.push_back(i);

          if (program == "Lorentzians") {
            if (spectraIndices.size() == 3) {
              IndirectTab::plotSpectra(QresultWS, spectraIndices);
            }
          } else
            IndirectTab::plotSpectrum(QresultWS, spectraIndices[0]);
        }
      }
    }
  }
  setPlotResultIsPlotting(false);
}

void Quasi::setRunEnabled(bool enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void Quasi::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void Quasi::setSaveResultEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void Quasi::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void Quasi::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void Quasi::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
