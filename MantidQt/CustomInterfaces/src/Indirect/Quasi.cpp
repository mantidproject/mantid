#include "MantidAPI/TextAxis.h"
#include "MantidQtCustomInterfaces/Indirect/Quasi.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

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

  // Connect the progrm selector to its handler
  connect(m_uiForm.cbProgram, SIGNAL(currentIndexChanged(int)), this,
          SLOT(handleProgramChange(int)));

  // Connect preview spectrum spinner to handler
  connect(m_uiForm.spPreviewSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(previewSpecChanged(int)));
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

  //Validate program
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

  auto saveDirectory = Mantid::Kernel::ConfigService::Instance().getString(
      "defaultsave.directory");
  if (saveDirectory.compare("") == 0) {
    const char *textMessage =
        "BayesQuasi requires a default save directory and "
        "one is not currently set."
        " If run, the algorithm will default to saving files "
        "to the current working directory."
        " Would you still like to run the algorithm?";
    int result = QMessageBox::question(NULL, tr("Save Directory"),
                                       tr(textMessage), QMessageBox::Yes,
                                       QMessageBox::No, QMessageBox::NoButton);
    if (result == QMessageBox::No) {
      return;
    }
  }

  bool save = false;
  bool elasticPeak = false;
  bool sequence = false;

  bool fixedWidth = false;
  std::string fixedWidthFile("");

  bool useResNorm = false;
  std::string resNormFile("");

  std::string sampleName =
      m_uiForm.dsSample->getCurrentDataName().toStdString();
  std::string resName =
      m_uiForm.dsResolution->getCurrentDataName().toStdString();

  std::string program = m_uiForm.cbProgram->currentText().toStdString();

  if (program == "Lorentzians") {
    program = "QL";
  } else {
    program = "QSe";
  }

  // Collect input from fit options section
  std::string background = m_uiForm.cbBackground->currentText().toStdString();

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
  double eMin = m_properties["EMin"]->valueText().toDouble();
  double eMax = m_properties["EMax"]->valueText().toDouble();

  long sampleBins = m_properties["SampleBinning"]->valueText().toLong();
  long resBins = m_properties["ResBinning"]->valueText().toLong();

  // Output options
  if (m_uiForm.chkSave->isChecked()) {
    save = true;
  }
  std::string plot = m_uiForm.cbPlot->currentText().toStdString();

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
  runAlg->setProperty("Save", save);
  runAlg->setProperty("Plot", plot);

  m_QuasiAlg = runAlg;
  m_batchAlgoRunner->addAlgorithm(runAlg);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Updates the data and fit curves on the mini plot.
 */
void Quasi::algorithmComplete(bool error) {
  if (error)
    return;
  else
    updateMiniPlot();
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
      curveColour = Qt::green;
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
}

/**
 * Toggles the use ResNorm option depending on if the resolution file is a
 * resolution or vanadoum reduction.
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
 * Handles when the slected item in the program combobox
 * is changed
 *
 * @param index :: The current index of the combobox
 */
void Quasi::handleProgramChange(int index) {
  int numberOptions = m_uiForm.cbPlot->count();
  switch (index) {
  case 0:
    m_uiForm.cbPlot->setItemText(numberOptions - 1, "Prob");
    break;
  case 1:
    m_uiForm.cbPlot->setItemText(numberOptions - 1, "Beta");
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

} // namespace CustomInterfaces
} // namespace MantidQt
