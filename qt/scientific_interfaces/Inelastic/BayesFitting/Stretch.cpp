// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Stretch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

using namespace Mantid::API;

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
Mantid::Kernel::Logger g_log("Stretch");
struct PlotType {
  inline static const std::string ALL = "All";
  inline static const std::string SIGMA = "Sigma";
  inline static const std::string BETA = "Beta";
  inline static const std::string FWHM = "FWHM";
};
} // namespace

namespace MantidQt::CustomInterfaces {
Stretch::Stretch(QWidget *parent, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner)
    : BayesFittingTab(parent, std::move(algorithmRunner)), m_previewSpec(0), m_save(false) {
  m_uiForm.setupUi(parent);

  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("StretchERange");
  connect(eRangeSelector, &MantidWidgets::RangeSelector::minValueChanged, this, &Stretch::minValueChanged);
  connect(eRangeSelector, &MantidWidgets::RangeSelector::maxValueChanged, this, &Stretch::maxValueChanged);
  setupFitOptions();
  setupPropertyBrowser();
  setupPlotOptions();

  // Connect the data selector for the sample to the mini plot
  connect(m_uiForm.dsSample, &DataSelector::dataReady, this, &Stretch::handleSampleInputReady);
  connect(m_uiForm.chkSequentialFit, &QCheckBox::toggled, m_uiForm.cbPlot, &QComboBox::setEnabled);
  // Connect preview spectrum spinner to handler
  connect(m_uiForm.spPreviewSpectrum, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &Stretch::previewSpecChanged);
  m_uiForm.spPreviewSpectrum->setMaximum(0);

  // Connect the plot and save push buttons
  connect(m_uiForm.pbPlot, &QPushButton::clicked, this, &Stretch::plotWorkspaces);
  connect(m_uiForm.pbPlotContour, &QPushButton::clicked, this, &Stretch::plotContourClicked);
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &Stretch::saveWorkspaces);
  connect(m_uiForm.pbPlotPreview, &QPushButton::clicked, this, &Stretch::plotCurrentPreview);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsSample->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
}

void Stretch::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Stretch");
  m_uiForm.dsSample->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsSample->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? getResolutionFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsResolution->setWSSuffixes(filter ? getResolutionWSSuffixes(tabName) : noSuffixes);
}

void Stretch::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsSample->setLoadProperty("LoadHistory", doLoadHistory);
  m_uiForm.dsResolution->setLoadProperty("LoadHistory", doLoadHistory);
}

void Stretch::handleValidation(IUserInputValidator *validator) const {
  validator->checkDataSelectorIsValid("Sample", m_uiForm.dsSample);
  validator->checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);
}

void Stretch::handleRun() {
  auto const saveDirectory = Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory");
  if (saveDirectory.empty()) {
    int const result = displaySaveDirectoryMessage();
    if (result == QMessageBox::No) {
      m_runPresenter->setRunEnabled(true);
      return;
    }
  }

  m_uiForm.ppPlot->watchADS(false);

  // Workspace input
  auto const sampleName = m_uiForm.dsSample->getCurrentDataName().toStdString();
  auto const resName = m_uiForm.dsResolution->getCurrentDataName().toStdString();

  // Collect input from options section
  auto const background = m_uiForm.cbBackground->currentText().toStdString();

  // Collect input from the properties browser
  auto const eMin = m_properties["EMin"]->valueText().toDouble();
  auto const eMax = m_properties["EMax"]->valueText().toDouble();
  auto const beta = m_properties["Beta"]->valueText().toInt();

  // Bool options
  auto const elasticPeak = m_uiForm.chkElasticPeak->isChecked();

  // Construct OutputNames
  auto const cutIndex = sampleName.find_last_of("_");
  auto const baseName = sampleName.substr(0, cutIndex);
  m_fitWorkspaceName = baseName + "_Stretch_Fit";
  m_contourWorkspaceName = baseName + "_Stretch_Contour";

  // Temporary developer flag to allow the testing of quickBayes in the Bayes fitting interface
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  std::string const algorithmName = useQuickBayes ? "BayesStretch2" : "BayesStretch";
  auto stretch = AlgorithmManager::Instance().create(algorithmName);
  stretch->initialize();
  stretch->setProperty("SampleWorkspace", sampleName);
  stretch->setProperty("ResolutionWorkspace", resName);
  stretch->setProperty("EMin", eMin);
  stretch->setProperty("EMax", eMax);
  stretch->setProperty("NumberBeta", beta);
  stretch->setProperty("Elastic", elasticPeak);
  stretch->setProperty("OutputWorkspaceFit", m_fitWorkspaceName);
  stretch->setProperty("OutputWorkspaceContour", m_contourWorkspaceName);
  stretch->setProperty("Background", background);
  if (!useQuickBayes) {
    auto const sigma = m_properties["Sigma"]->valueText().toInt();
    auto const nBins = m_properties["SampleBinning"]->valueText().toInt();
    auto const sequence = m_uiForm.chkSequentialFit->isChecked();

    stretch->setProperty("SampleBins", nBins);
    stretch->setProperty("NumberSigma", sigma);
    stretch->setProperty("Loop", sequence);
  }

  m_batchAlgoRunner->addAlgorithm(stretch);
  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &Stretch::algorithmComplete);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles the saving and plotting of workspaces after execution
 */
void Stretch::algorithmComplete(const bool &error) {
  disconnect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &Stretch::algorithmComplete);

  m_runPresenter->setRunEnabled(true);
  setPlotResultEnabled(!error);
  setPlotContourEnabled(!error);
  setSaveResultEnabled(!error);
  if (!error) {
    if (doesExistInADS(m_contourWorkspaceName))
      populateContourWorkspaceComboBox();
    else
      setPlotContourEnabled(false);

    m_uiForm.ppPlot->watchADS(true);
  }
}

void Stretch::populateContourWorkspaceComboBox() {
  m_uiForm.cbPlotContour->clear();
  auto const contourGroup = getADSWorkspace<WorkspaceGroup>(m_contourWorkspaceName);
  auto const contourNames = contourGroup->getNames();
  for (auto const &name : contourNames)
    m_uiForm.cbPlotContour->addItem(QString::fromStdString(name));
}

/**
 * Handles the saving of workspaces post algorithm completion
 * when save button is clicked
 */
void Stretch::saveWorkspaces() {

  auto fitWorkspace = QString::fromStdString(m_fitWorkspaceName);
  auto contourWorkspace = QString::fromStdString(m_contourWorkspaceName);
  // Check workspaces exist
  InelasticTab::checkADSForPlotSaveWorkspace(m_fitWorkspaceName, false);
  InelasticTab::checkADSForPlotSaveWorkspace(m_contourWorkspaceName, false);

  const auto saveDir =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));
  // Check validity of save path
  const auto fitFullPath = saveDir + fitWorkspace + QString::fromStdString(".nxs");
  const auto contourFullPath = saveDir + contourWorkspace + QString::fromStdString(".nxs");
  addSaveWorkspaceToQueue(fitWorkspace, fitFullPath);
  addSaveWorkspaceToQueue(contourWorkspace, contourFullPath);
  m_batchAlgoRunner->executeBatchAsync();
}

int Stretch::displaySaveDirectoryMessage() {
  char const *textMessage = "BayesStretch requires a default save directory and "
                            "one is not currently set."
                            " If run, the algorithm will default to saving files "
                            "to the current working directory."
                            " Would you still like to run the algorithm?";
  return QMessageBox::question(nullptr, tr("Save Directory"), tr(textMessage), QMessageBox::Yes, QMessageBox::No,
                               QMessageBox::NoButton);
}

/**
 * Handles the plotting of workspace post algorithm completion
 */
void Stretch::plotWorkspaces() {
  setPlotResultIsPlotting(true);

  std::string const plotType = m_uiForm.cbPlot->currentText().toStdString();
  auto const plotErrors = SettingsHelper::externalPlotErrorBars();
  auto const plotSigma = (plotType == "All" || plotType == "Sigma");
  auto const plotBeta = (plotType == "All" || plotType == "Beta");

  auto const fitWorkspace = getADSWorkspace<WorkspaceGroup>(m_fitWorkspaceName);
  for (auto it = fitWorkspace->begin(); it < fitWorkspace->end(); ++it) {
    auto const name = (*it)->getName();
    if (plotSigma && name.substr(name.length() - 5) == "Sigma") {
      m_plotter->plotSpectra(name, "0", plotErrors);
    } else if (plotBeta && name.substr(name.length() - 4) == "Beta") {
      m_plotter->plotSpectra(name, "0", plotErrors);
    }
  }
  setPlotResultIsPlotting(false);
}

void Stretch::plotContourClicked() {
  setPlotContourIsPlotting(true);

  auto const workspaceName = m_uiForm.cbPlotContour->currentText().toStdString();
  if (checkADSForPlotSaveWorkspace(workspaceName, true))
    m_plotter->plotContour(workspaceName);

  setPlotContourIsPlotting(false);
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void Stretch::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

/**
 * Get called whenever the settings are updated
 *
 * @param settings :: The current settings
 */
void Stretch::applySettings(std::map<std::string, QVariant> const &settings) {
  setupFitOptions();
  setupPropertyBrowser();
  setupPlotOptions();
  setFileExtensionsByName(settings.at("RestrictInput").toBool());
  setLoadHistory(settings.at("LoadHistory").toBool());
}

/**
 * Setup the fit options based on the algorithm used
 *
 */
void Stretch::setupFitOptions() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");
  m_uiForm.cbBackground->clear();
  if (useQuickBayes) {
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::LINEAR));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::FLAT));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::ZERO));
    m_uiForm.chkSequentialFit->hide();
  } else {
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::SLOPING));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::FLAT));
    m_uiForm.cbBackground->addItem(QString::fromStdString(BackgroundType::ZERO));
    m_uiForm.chkSequentialFit->show();
  }
}

/**
 * Setup the property browser based on the algorithm used
 *
 */
void Stretch::setupPropertyBrowser() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  m_properties.clear();
  m_dblManager->clear();
  m_propTree->clear();

  m_uiForm.treeSpace->addWidget(m_propTree);

  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");
  m_properties["Beta"] = m_dblManager->addProperty("Beta");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["Beta"], INT_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);
  m_propTree->addProperty(m_properties["Beta"]);

  m_dblManager->setValue(m_properties["Beta"], 50);
  m_dblManager->setMinimum(m_properties["Beta"], 1);
  m_dblManager->setMaximum(m_properties["Beta"], 200);

  if (!useQuickBayes) {
    m_properties["SampleBinning"] = m_dblManager->addProperty("Sample Binning");
    m_properties["Sigma"] = m_dblManager->addProperty("Sigma");

    m_dblManager->setDecimals(m_properties["SampleBinning"], INT_DECIMALS);
    m_dblManager->setDecimals(m_properties["Sigma"], INT_DECIMALS);

    m_propTree->addProperty(m_properties["SampleBinning"]);
    m_propTree->addProperty(m_properties["Sigma"]);

    m_dblManager->setValue(m_properties["Sigma"], 50);
    m_dblManager->setMinimum(m_properties["Sigma"], 1);
    m_dblManager->setMaximum(m_properties["Sigma"], 200);
    m_dblManager->setValue(m_properties["SampleBinning"], 1);
    m_dblManager->setMinimum(m_properties["SampleBinning"], 1);
  }

  formatTreeWidget(m_propTree, m_properties);
}

/**
 * Setup the plot options based on the algorithm used
 *
 */
void Stretch::setupPlotOptions() {
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");
  m_uiForm.cbPlot->clear();
  if (useQuickBayes) {
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::ALL));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::FWHM));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::BETA));
  } else {
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::ALL));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::SIGMA));
    m_uiForm.cbPlot->addItem(QString::fromStdString(PlotType::BETA));
  }
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void Stretch::handleSampleInputReady(const QString &filename) {
  try {
    m_uiForm.ppPlot->clear();
    m_uiForm.ppPlot->addSpectrum("Sample", filename, 0);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
    return;
  }

  // update the maximum and minimum range bar positions
  auto const range = getXRangeFromWorkspace(filename.toStdString());
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");
  setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  setPlotPropertyRange(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  // update the current positions of the range bars
  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);

  // set the max spectrum
  MatrixWorkspace_const_sptr sampleWs = getADSWorkspace(filename.toStdString());
  const int spectra = static_cast<int>(sampleWs->getNumberHistograms());
  m_uiForm.spPreviewSpectrum->setMaximum(spectra - 1);
}

/**
 * Sets a new preview spectrum for the mini plot.
 *
 * @param value workspace index
 */
void Stretch::previewSpecChanged(int value) {
  m_previewSpec = value;

  if (!m_uiForm.dsSample->isValid())
    return;

  m_uiForm.ppPlot->clear();

  auto const sampleName = m_uiForm.dsSample->getCurrentDataName();
  try {
    m_uiForm.ppPlot->addSpectrum("Sample", sampleName, m_previewSpec);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }
}

/**
 * plots the current miniplot preview
 */
void Stretch::plotCurrentPreview() {
  if (m_uiForm.ppPlot->hasCurve("Sample")) {
    m_plotter->plotSpectra(m_uiForm.dsSample->getCurrentDataName().toStdString(), std::to_string(m_previewSpec),
                           SettingsHelper::externalPlotErrorBars());
  }
}

/**
 * Updates the property manager when the lower guide is moved on the mini plot
 *
 * @param min :: The new value of the lower guide
 */
void Stretch::minValueChanged(double min) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &Stretch::updateProperties);
  m_dblManager->setValue(m_properties["EMin"], min);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &Stretch::updateProperties);
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void Stretch::maxValueChanged(double max) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &Stretch::updateProperties);
  m_dblManager->setValue(m_properties["EMax"], max);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &Stretch::updateProperties);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void Stretch::updateProperties(QtProperty *prop, double val) {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("StretchERange");

  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &Stretch::updateProperties);

  if (prop == m_properties["EMin"]) {
    setRangeSelectorMin(m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  } else if (prop == m_properties["EMax"]) {
    setRangeSelectorMax(m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  }

  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &Stretch::updateProperties);
}

void Stretch::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void Stretch::setPlotContourEnabled(bool enabled) {
  m_uiForm.pbPlotContour->setEnabled(enabled);
  m_uiForm.cbPlotContour->setEnabled(enabled);
}

void Stretch::setSaveResultEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void Stretch::setButtonsEnabled(bool enabled) {
  m_runPresenter->setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setPlotContourEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void Stretch::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

void Stretch::setPlotContourIsPlotting(bool plotting) {
  m_uiForm.pbPlotContour->setText(plotting ? "Plotting..." : "Plot Contour");
  setButtonsEnabled(!plotting);
}

} // namespace MantidQt::CustomInterfaces
