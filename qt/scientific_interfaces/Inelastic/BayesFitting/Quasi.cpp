// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Quasi.h"
#include "MantidAPI/TextAxis.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

using namespace Mantid::API;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
Mantid::Kernel::Logger g_log("Quasi");
}

namespace MantidQt::CustomInterfaces {

Quasi::Quasi(QWidget *parent) : BayesFittingTab(parent), m_previewSpec(0) {
  m_uiForm.setupUi(parent);

  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("QuasiERange");
  connect(eRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minValueChanged(double)));
  connect(eRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxValueChanged(double)));

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
  connect(m_uiForm.chkFixWidth, SIGNAL(toggled(bool)), m_uiForm.mwFixWidthDat, SLOT(setEnabled(bool)));
  connect(m_uiForm.chkUseResNorm, SIGNAL(toggled(bool)), m_uiForm.dsResNorm, SLOT(setEnabled(bool)));

  // Connect the data selector for the sample to the mini plot
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this, SLOT(handleSampleInputReady(const QString &)));

  connect(m_uiForm.dsResolution, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleResolutionInputReady(const QString &)));

  // Connect the program selector to its handler
  connect(m_uiForm.cbProgram, SIGNAL(currentIndexChanged(int)), this, SLOT(handleProgramChange(int)));

  // Connect preview spectrum spinner to handler
  connect(m_uiForm.spPreviewSpectrum, SIGNAL(valueChanged(int)), this, SLOT(previewSpecChanged(int)));

  // Plot current preview
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this, SLOT(plotCurrentPreview()));

  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));

  // Allows empty workspace selector when initially selected
  m_uiForm.dsSample->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
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

void Quasi::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Quasi");
  m_uiForm.dsSample->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsSample->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? getResolutionFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsResolution->setWSSuffixes(filter ? getResolutionWSSuffixes(tabName) : noSuffixes);
}

void Quasi::handleValidation(IUserInputValidator *validator) const {
  validator->checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  validator->checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  // check that the ResNorm file is valid if we are using it
  if (m_uiForm.chkUseResNorm->isChecked()) {
    validator->checkDataSelectorIsValid("ResNorm", m_uiForm.dsResNorm);
  }

  // check fixed width file exists
  if (m_uiForm.chkFixWidth->isChecked() && !m_uiForm.mwFixWidthDat->isValid()) {
    validator->checkFileFinderWidgetIsValid("Width", m_uiForm.mwFixWidthDat);
  }

  // check eMin and eMax values
  const auto eMin = m_dblManager->value(m_properties["EMin"]);
  const auto eMax = m_dblManager->value(m_properties["EMax"]);
  if (eMin >= eMax)
    validator->addErrorMessage("EMin must be strictly less than EMax.\n");

  // Validate program
  QString program = m_uiForm.cbProgram->currentText();
  if (program == "Stretched Exponential") {
    QString resName = m_uiForm.dsResolution->getCurrentDataName();
    if (!resName.endsWith("_res")) {
      validator->addErrorMessage("Stretched Exponential program can only be used with "
                                 "a resolution file.");
    }
  }

  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");
  if (useQuickBayes && m_uiForm.cbBackground->currentText() == "Sloping") {
    validator->addErrorMessage("The 'quickBayes' package does not support a 'Sloping' background type.");
  }
}

void Quasi::handleRun() {
  auto const saveDirectory = Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory");
  if (saveDirectory.empty()) {
    int const result = displaySaveDirectoryMessage();
    if (result == QMessageBox::No) {
      m_runPresenter->setRunEnabled(true);
      return;
    }
  }

  m_uiForm.ppPlot->watchADS(false);

  bool elasticPeak = false;
  bool sequence = false;

  bool fixedWidth = false;
  std::string fixedWidthFile("");

  bool useResNorm = false;
  std::string resNormFile("");

  std::string const sampleName = m_uiForm.dsSample->getCurrentDataName().toStdString();
  std::string const resName = m_uiForm.dsResolution->getCurrentDataName().toStdString();

  std::string program = m_uiForm.cbProgram->currentText().toStdString();

  if (program == "Lorentzians") {
    program = "QL";
  } else {
    program = "QSe";
  }

  // Collect input from fit options section
  std::string const background = m_uiForm.cbBackground->currentText().toStdString();

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

  auto const sampleBins = m_properties["SampleBinning"]->valueText().toInt();
  auto const resBins = m_properties["ResBinning"]->valueText().toInt();

  // Construct an output base name for the output workspaces
  auto const resType = resName.substr(resName.length() - 3);
  auto const programName = program == "QL" ? resType == "res" ? "QLr" : "QLd" : program;
  m_outputBaseName = sampleName.substr(0, sampleName.length() - 3) + programName;

  // Temporary developer flag to allow the testing of quickBayes in the Bayes fitting interface
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  std::string const algorithmName = useQuickBayes ? "BayesQuasi2" : "BayesQuasi";
  IAlgorithm_sptr runAlg = AlgorithmManager::Instance().create(algorithmName);
  runAlg->initialize();
  runAlg->setProperty("Program", program);
  runAlg->setProperty("SampleWorkspace", sampleName);
  runAlg->setProperty("ResolutionWorkspace", resName);
  runAlg->setProperty("OutputWorkspaceFit", m_outputBaseName + "_Fit");
  runAlg->setProperty("OutputWorkspaceProb", m_outputBaseName + "_Prob");
  runAlg->setProperty("OutputWorkspaceResult", m_outputBaseName + "_Result");
  runAlg->setProperty("Elastic", elasticPeak);
  if (useQuickBayes) {
    // Use quickBayes package in BayesQuasi2 algorithm
    runAlg->setProperty("Background", background == "Flat" ? background : "None");
    runAlg->setProperty("EMin", eMin);
    runAlg->setProperty("EMax", eMax);
  } else {
    // Use quasielasticbayes package in BayesQuasi algorithm
    runAlg->setProperty("ResNormWorkspace", resNormFile);
    runAlg->setProperty("Background", background);
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
  }

  m_QuasiAlg = runAlg;
  m_batchAlgoRunner->addAlgorithm(runAlg);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Enable plotting and saving and fit curves on the mini plot.
 */
void Quasi::algorithmComplete(bool error) {
  m_runPresenter->setRunEnabled(true);
  setPlotResultEnabled(!error);
  setSaveResultEnabled(!error);
  if (!error) {
    updateMiniPlot();
    m_uiForm.ppPlot->watchADS(true);
  }
}

void Quasi::updateMiniPlot() {
  // Update sample plot
  if (!m_uiForm.dsSample->isValid())
    return;

  m_uiForm.ppPlot->clear();

  QString sampleName = m_uiForm.dsSample->getCurrentDataName();
  try {
    m_uiForm.ppPlot->addSpectrum("Sample", sampleName, m_previewSpec);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }

  auto const fitGroupName = m_outputBaseName + "_Fit";
  if (!AnalysisDataService::Instance().doesExist(fitGroupName))
    return;

  auto const fitGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(fitGroupName);
  if (!fitGroup || fitGroup->getNumberOfEntries() <= m_previewSpec) {
    return;
  }

  auto const outputWorkspace =
      std::dynamic_pointer_cast<MatrixWorkspace>(fitGroup->getItem(static_cast<std::size_t>(m_previewSpec)));

  TextAxis *axis = dynamic_cast<TextAxis *>(outputWorkspace->getAxis(1));

  for (std::size_t histIndex = 0; histIndex < outputWorkspace->getNumberHistograms(); histIndex++) {
    QString specName = QString::fromStdString(axis->label(histIndex));
    QColor curveColour;

    if (specName.contains("fit 1"))
      curveColour = Qt::red;
    else if (specName.contains("fit 2"))
      curveColour = Qt::magenta;

    else if (specName.contains("diff 1"))
      curveColour = Qt::blue;
    else if (specName.contains("diff 2"))
      curveColour = Qt::cyan;

    else
      continue;

    try {
      m_uiForm.ppPlot->addSpectrum(specName, outputWorkspace, histIndex, curveColour);
    } catch (std::exception const &ex) {
      g_log.warning(ex.what());
    }
  }
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void Quasi::handleSampleInputReady(const QString &filename) {
  MatrixWorkspace_sptr inWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(filename.toStdString());
  int numHist = static_cast<int>(inWs->getNumberHistograms()) - 1;
  m_uiForm.spPreviewSpectrum->setMaximum(numHist);
  updateMiniPlot();

  auto const range = getXRangeFromWorkspace(filename.toStdString());

  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("QuasiERange");

  setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  setPlotPropertyRange(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);
}

/**
 * Plots the current preview on the miniplot
 */
void Quasi::plotCurrentPreview() {
  auto const errorBars = SettingsHelper::externalPlotErrorBars();

  if (m_uiForm.ppPlot->hasCurve("fit 1")) {
    QString program = m_uiForm.cbProgram->currentText();
    auto fitName = m_QuasiAlg->getPropertyValue("OutputWorkspaceFit");
    if (checkADSForPlotSaveWorkspace(fitName, false)) {
      auto wsFit = Mantid::API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(fitName);
      std::vector<std::string> names = (wsFit)->getNames();
      if (program == "Lorentzians")
        m_plotter->plotSpectra(names.at(m_previewSpec), "0-4", errorBars);
      else
        m_plotter->plotSpectra(names.at(m_previewSpec), "0-2", errorBars);
    }
  } else if (m_uiForm.ppPlot->hasCurve("Sample")) {
    m_plotter->plotSpectra(m_uiForm.dsSample->getCurrentDataName().toStdString(), std::to_string(m_previewSpec),
                           errorBars);
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
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));
  m_dblManager->setValue(m_properties["EMin"], min);
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void Quasi::maxValueChanged(double max) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));
  m_dblManager->setValue(m_properties["EMax"], max);
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void Quasi::updateProperties(QtProperty *prop, double val) {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("QuasiERange");

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateProperties(QtProperty *, double)));

  if (prop == m_properties["EMin"]) {
    setRangeSelectorMin(m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  } else if (prop == m_properties["EMax"]) {
    setRangeSelectorMax(m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateProperties(QtProperty *, double)));
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
  QString saveDirectory =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));
  auto const fitWS = m_QuasiAlg->getPropertyValue("OutputWorkspaceFit");
  InelasticTab::checkADSForPlotSaveWorkspace(fitWS, false);
  QString const QfitWS = QString::fromStdString(fitWS);
  auto const fitPath = saveDirectory + QfitWS + ".nxs";
  addSaveWorkspaceToQueue(QfitWS, fitPath);

  auto const resultWS = m_QuasiAlg->getPropertyValue("OutputWorkspaceResult");
  InelasticTab::checkADSForPlotSaveWorkspace(resultWS, false);
  QString const QresultWS = QString::fromStdString(resultWS);
  auto const resultPath = saveDirectory + QresultWS + ".nxs";
  addSaveWorkspaceToQueue(QresultWS, resultPath);
  m_batchAlgoRunner->executeBatchAsync();
}

int Quasi::displaySaveDirectoryMessage() {
  char const *textMessage = "BayesQuasi requires a default save directory and "
                            "one is not currently set."
                            " If run, the algorithm will default to saving files "
                            "to the current working directory."
                            " Would you still like to run the algorithm?";
  return QMessageBox::question(nullptr, tr("Save Directory"), tr(textMessage), QMessageBox::Yes, QMessageBox::No,
                               QMessageBox::NoButton);
}

/**
 * Handles plotting the selected plot when plot is clicked
 */
void Quasi::plotClicked() {
  setPlotResultIsPlotting(true);
  auto const errorBars = SettingsHelper::externalPlotErrorBars();

  // Output options
  std::string const plot = m_uiForm.cbPlot->currentText().toLower().toStdString();
  QString const program = m_uiForm.cbProgram->currentText();
  auto const resultName = m_QuasiAlg->getPropertyValue("OutputWorkspaceResult");
  if ((plot == "prob" || plot == "all") && (program == "Lorentzians")) {
    auto const probWS = m_QuasiAlg->getPropertyValue("OutputWorkspaceProb");
    // Check workspace exists
    InelasticTab::checkADSForPlotSaveWorkspace(probWS, true);
    m_plotter->plotSpectra(probWS, "1-2", errorBars);
  }

  auto const resultWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(resultName);
  int const numSpectra = (int)resultWS->getNumberHistograms();
  InelasticTab::checkADSForPlotSaveWorkspace(resultName, true);
  auto const paramNames = {"amplitude", "fwhm", "beta"};
  for (std::string const &paramName : paramNames) {

    if (plot == paramName || plot == "all") {
      std::vector<std::size_t> spectraIndices = {};
      for (auto i = 0u; i < static_cast<std::size_t>(numSpectra); i++) {
        auto axisLabel = resultWS->getAxis(1)->label(i);
        // Convert to lower case
        std::transform(axisLabel.begin(), axisLabel.end(), axisLabel.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        auto const found = axisLabel.find(paramName);
        if (found != std::string::npos) {
          spectraIndices.emplace_back(i);

          if (program == "Lorentzians") {
            if (spectraIndices.size() == 3) {
              auto const workspaceIndices = std::to_string(spectraIndices[0]) + "," +
                                            std::to_string(spectraIndices[1]) + "," + std::to_string(spectraIndices[2]);
              m_plotter->plotSpectra(resultName, workspaceIndices, errorBars);
            }
          } else
            m_plotter->plotSpectra(resultName, std::to_string(spectraIndices[0]), errorBars);
        }
      }
    }
  }
  setPlotResultIsPlotting(false);
}

void Quasi::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void Quasi::setSaveResultEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void Quasi::setButtonsEnabled(bool enabled) {
  m_runPresenter->setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void Quasi::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

} // namespace MantidQt::CustomInterfaces
