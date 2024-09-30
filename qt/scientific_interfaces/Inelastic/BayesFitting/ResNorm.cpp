// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ResNorm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <map>
#include <string>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
Mantid::Kernel::Logger g_log("ResNorm");
} // namespace

namespace MantidQt::CustomInterfaces {
ResNorm::ResNorm(QWidget *parent) : BayesFittingTab(parent), m_previewSpec(0) {
  m_uiForm.setupUi(parent);

  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_uiForm.runWidget));

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("ResNormERange");
  connect(eRangeSelector, &MantidWidgets::RangeSelector::minValueChanged, this, &ResNorm::minValueChanged);
  connect(eRangeSelector, &MantidWidgets::RangeSelector::maxValueChanged, this, &ResNorm::maxValueChanged);
  // Add the properties browser to the ui form
  m_uiForm.treeSpace->addWidget(m_propTree);

  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);

  formatTreeWidget(m_propTree, m_properties);

  // Connect data selector to handler method
  connect(m_uiForm.dsVanadium, &DataSelector::dataReady, this, &ResNorm::handleVanadiumInputReady);
  connect(m_uiForm.dsResolution, &DataSelector::dataReady, this, &ResNorm::handleResolutionInputReady);

  // Connect the preview spectrum selector
  connect(m_uiForm.spPreviewSpectrum, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &ResNorm::previewSpecChanged);
  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &ResNorm::handleAlgorithmComplete);
  // Post Plot and Save
  connect(m_uiForm.pbSave, &QPushButton::clicked, this, &ResNorm::saveClicked);
  connect(m_uiForm.pbPlot, &QPushButton::clicked, this, &ResNorm::plotClicked);
  connect(m_uiForm.pbPlotCurrent, &QPushButton::clicked, this, &ResNorm::plotCurrentPreview);

  // Allows empty workspace selector when initially selected
  m_uiForm.dsVanadium->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
}

void ResNorm::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("ResNorm");
  m_uiForm.dsVanadium->setFBSuffixes(filter ? getVanadiumFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsVanadium->setWSSuffixes(filter ? getVanadiumWSSuffixes(tabName) : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? getResolutionFBSuffixes(tabName) : getExtensions(tabName));
  m_uiForm.dsResolution->setWSSuffixes(filter ? getResolutionWSSuffixes(tabName) : noSuffixes);
}

void ResNorm::setLoadHistory(bool doLoadHistory) {
  m_uiForm.dsVanadium->setLoadProperty("LoadHistory", doLoadHistory);
  m_uiForm.dsResolution->setLoadProperty("LoadHistory", doLoadHistory);
}

void ResNorm::handleValidation(IUserInputValidator *validator) const {
  bool const vanValid = validator->checkDataSelectorIsValid("Vanadium", m_uiForm.dsVanadium);
  bool const resValid = validator->checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  if (vanValid) {
    // Check vanadium input is _red or _sqw workspace
    QString const vanName = m_uiForm.dsVanadium->getCurrentDataName();
    int const cutIndex = vanName.lastIndexOf("_");
    QString const vanSuffix = vanName.right(vanName.size() - (cutIndex + 1));
    if (vanSuffix.compare("red") != 0 && vanSuffix.compare("sqw") != 0)
      validator->addErrorMessage("The Vanadium run is not _red or _sqw workspace");

    // Check Res and Vanadium are the same Run
    if (resValid) {
      // Check that Res file is still in ADS if not, load it
      auto const resolutionWs = getADSWorkspace(m_uiForm.dsResolution->getCurrentDataName().toStdString());
      auto const vanadiumWs = getADSWorkspace(vanName.toStdString());

      int const resRun = resolutionWs->getRunNumber();
      int const vanRun = vanadiumWs->getRunNumber();

      if (resRun != vanRun)
        validator->addErrorMessage("The provided Vanadium and Resolution do not have "
                                   "matching run numbers");
    }
  }

  // check eMin and eMax values
  auto const eMin = getDoubleManagerProperty("EMin");
  auto const eMax = getDoubleManagerProperty("EMax");
  if (eMin >= eMax)
    validator->addErrorMessage("EMin must be strictly less than EMax.\n");
}

void ResNorm::handleRun() {
  m_uiForm.ppPlot->watchADS(false);

  auto const vanWsName(m_uiForm.dsVanadium->getCurrentDataName());
  auto const resWsName(m_uiForm.dsResolution->getCurrentDataName());

  auto const eMin(getDoubleManagerProperty("EMin"));
  auto const eMax(getDoubleManagerProperty("EMax"));

  auto const outputWsName = getWorkspaceBasename(resWsName.toStdString()) + "_ResNorm";

  auto resNorm = AlgorithmManager::Instance().create("ResNorm", 2);
  resNorm->initialize();
  resNorm->setProperty("VanadiumWorkspace", vanWsName.toStdString());
  resNorm->setProperty("ResolutionWorkspace", resWsName.toStdString());
  resNorm->setProperty("EnergyMin", eMin);
  resNorm->setProperty("EnergyMax", eMax);
  resNorm->setProperty("CreateOutput", true);
  resNorm->setProperty("OutputWorkspace", outputWsName);
  resNorm->setProperty("OutputWorkspaceTable", (outputWsName + "_Fit"));
  m_batchAlgoRunner->addAlgorithm(resNorm);
  m_pythonExportWsName = outputWsName;
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle completion of the algorithm.
 *
 * @param error If the algorithm failed
 */
void ResNorm::handleAlgorithmComplete(bool error) {
  m_runPresenter->setRunEnabled(true);
  setPlotResultEnabled(!error);
  setSaveResultEnabled(!error);
  if (!error) {
    // Update preview plot
    previewSpecChanged(m_previewSpec);
    // Copy and add sample logs to result workspaces
    processLogs();

    m_uiForm.ppPlot->watchADS(true);
  }
}

void ResNorm::processLogs() {
  auto const resWsName(m_uiForm.dsResolution->getCurrentDataName());
  auto const outputWsName = getWorkspaceBasename(resWsName.toStdString()) + "_ResNorm";
  auto const resolutionWorkspace = getADSWorkspace(resWsName.toStdString());
  auto const resultWorkspace = getADSWorkspace<WorkspaceGroup>(outputWsName);

  copyLogs(resolutionWorkspace, resultWorkspace);
  addAdditionalLogs(resultWorkspace);
}

void ResNorm::addAdditionalLogs(const WorkspaceGroup_sptr &resultGroup) const {
  for (auto const &workspace : *resultGroup)
    addAdditionalLogs(workspace);
}

void ResNorm::addAdditionalLogs(const Workspace_sptr &resultWorkspace) const {
  auto logAdder = AlgorithmManager::Instance().create("AddSampleLog");
  auto const name = resultWorkspace->getName();

  for (auto const &log : getAdditionalLogStrings()) {
    logAdder->setProperty("Workspace", name);
    logAdder->setProperty("LogType", "String");
    logAdder->setProperty("LogName", log.first);
    logAdder->setProperty("LogText", log.second);
    logAdder->execute();
  }

  for (auto const &log : getAdditionalLogNumbers()) {
    logAdder->setProperty("Workspace", name);
    logAdder->setProperty("LogType", "Number");
    logAdder->setProperty("LogName", log.first);
    logAdder->setProperty("LogText", log.second);
    logAdder->execute();
  }
}

std::map<std::string, std::string> ResNorm::getAdditionalLogStrings() const {
  auto logs = std::map<std::string, std::string>();
  logs["sample_filename"] = m_uiForm.dsVanadium->getCurrentDataName().toStdString();
  logs["resolution_filename"] = m_uiForm.dsResolution->getCurrentDataName().toStdString();
  logs["fit_program"] = "ResNorm";
  logs["create_output"] = "true";
  return logs;
}

std::map<std::string, std::string> ResNorm::getAdditionalLogNumbers() const {
  auto logs = std::map<std::string, std::string>();
  logs["e_min"] = boost::lexical_cast<std::string>(getDoubleManagerProperty("EMin"));
  logs["e_max"] = boost::lexical_cast<std::string>(getDoubleManagerProperty("EMax"));
  return logs;
}

double ResNorm::getDoubleManagerProperty(QString const &propName) const {
  return m_dblManager->value(m_properties[propName]);
}

void ResNorm::copyLogs(const MatrixWorkspace_sptr &resultWorkspace, const WorkspaceGroup_sptr &resultGroup) const {
  for (auto const &workspace : *resultGroup)
    copyLogs(resultWorkspace, workspace);
}

void ResNorm::copyLogs(const MatrixWorkspace_sptr &resultWorkspace, const Workspace_sptr &workspace) const {
  auto logCopier = AlgorithmManager::Instance().create("CopyLogs");
  logCopier->setProperty("InputWorkspace", resultWorkspace->getName());
  logCopier->setProperty("OutputWorkspace", workspace->getName());
  logCopier->execute();
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void ResNorm::loadSettings(const QSettings &settings) {
  m_uiForm.dsVanadium->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void ResNorm::handleVanadiumInputReady(const QString &filename) {
  // Plot the vanadium
  try {
    if (!m_uiForm.ppPlot->hasCurve("Resolution"))
      m_uiForm.ppPlot->clear();

    m_uiForm.ppPlot->addSpectrum("Vanadium", filename, m_previewSpec);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
    return;
  }

  QPair<double, double> res;
  auto const range = getXRangeFromWorkspace(filename.toStdString());

  auto const vanWs = getADSWorkspace(filename.toStdString());
  if (vanWs)
    m_uiForm.spPreviewSpectrum->setMaximum(static_cast<int>(vanWs->getNumberHistograms()) - 1);

  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormERange");

  // Use the values from the instrument parameter file if we can
  // The maximum and minimum value of the plot
  if (getResolutionRangeFromWs(filename.toStdString(), res)) {
    // ResNorm resolution should be +/- 10 * the IPF resolution
    res.first = res.first * 10;
    res.second = res.second * 10;

    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"], res);
  } else {
    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);
  }

  setPlotPropertyRange(eRangeSelector, m_properties["EMin"], m_properties["EMax"], range);

  // Set the current values of the range bars
  eRangeSelector->setMinimum(range.first);
  eRangeSelector->setMaximum(range.second);
}

/**
 * Plots the loaded resolution file on the mini plot.
 *
 * @param filename Name of the workspace to plot
 */
void ResNorm::handleResolutionInputReady(const QString &filename) {
  try {
    if (!m_uiForm.ppPlot->hasCurve("Vanadium"))
      m_uiForm.ppPlot->clear();

    m_uiForm.ppPlot->addSpectrum("Resolution", filename, 0, Qt::blue);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }
}

/**
 * Updates the property manager when the lower guide is moved on the mini plot
 *
 * @param min :: The new value of the lower guide
 */
void ResNorm::minValueChanged(double min) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNorm::updateProperties);
  m_dblManager->setValue(m_properties["EMin"], min);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNorm::updateProperties);
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void ResNorm::maxValueChanged(double max) {
  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNorm::updateProperties);
  m_dblManager->setValue(m_properties["EMax"], max);
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNorm::updateProperties);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void ResNorm::updateProperties(QtProperty *prop, double val) {
  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormERange");

  disconnect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNorm::updateProperties);

  if (prop == m_properties["EMin"]) {
    setRangeSelectorMin(m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  } else if (prop == m_properties["EMax"]) {
    setRangeSelectorMax(m_properties["EMin"], m_properties["EMax"], eRangeSelector, val);
  }
  connect(m_dblManager, &QtDoublePropertyManager::valueChanged, this, &ResNorm::updateProperties);
}

/**
 * Sets a new preview spectrum for the mini plot.
 *
 * @param value workspace index
 */
void ResNorm::previewSpecChanged(int value) {
  m_previewSpec = value;

  m_uiForm.ppPlot->clear();

  // Update vanadium plot
  if (m_uiForm.dsVanadium->isValid())
    try {
      m_uiForm.ppPlot->addSpectrum("Vanadium", m_uiForm.dsVanadium->getCurrentDataName(), m_previewSpec);
      m_uiForm.ppPlot->addSpectrum("Resolution", m_uiForm.dsResolution->getCurrentDataName(), 0, Qt::blue);
    } catch (std::exception const &ex) {
      g_log.warning(ex.what());
    }

  // Update fit plot
  std::string fitWsGroupName(m_pythonExportWsName + "_Fit_Workspaces");
  std::string fitParamsName(m_pythonExportWsName + "_Fit");
  if (AnalysisDataService::Instance().doesExist(fitWsGroupName)) {
    auto const fitWorkspaces = getADSWorkspace<WorkspaceGroup>(fitWsGroupName);
    auto const fitParams = getADSWorkspace<ITableWorkspace>(fitParamsName);
    if (fitWorkspaces && fitParams) {
      Column_const_sptr scaleFactors = fitParams->getColumn("Scaling");
      std::string fitWsName(fitWorkspaces->getItem(m_previewSpec)->getName());
      auto const fitWs = getADSWorkspace(fitWsName);

      auto fit = WorkspaceFactory::Instance().create(fitWs, 1);
      fit->setSharedX(0, fitWs->sharedX(1));
      fit->setSharedY(0, fitWs->sharedY(1));
      fit->setSharedE(0, fitWs->sharedE(1));

      fit->mutableY(0) /= scaleFactors->cell<double>(m_previewSpec);

      try {
        m_uiForm.ppPlot->addSpectrum("Fit", fit, 0, Qt::green);
      } catch (std::exception const &ex) {
        g_log.warning(ex.what());
      }

      AnalysisDataService::Instance().addOrReplace("__" + fitWsGroupName + "_scaled", fit);
    }
  }
}

/**
 * Plot the current spectrum in the miniplot
 */

void ResNorm::plotCurrentPreview() {

  std::vector<std::string> plotWorkspaces;
  std::vector<int> plotIndices;

  if (m_uiForm.ppPlot->hasCurve("Vanadium")) {
    plotWorkspaces.emplace_back(m_uiForm.dsVanadium->getCurrentDataName().toStdString());
    plotIndices.emplace_back(m_previewSpec);
  }
  if (m_uiForm.ppPlot->hasCurve("Resolution")) {
    plotWorkspaces.emplace_back(m_uiForm.dsResolution->getCurrentDataName().toStdString());
    plotIndices.emplace_back(0);
  }
  if (m_uiForm.ppPlot->hasCurve("Fit")) {
    std::string fitWsGroupName(m_pythonExportWsName + "_Fit_Workspaces");

    plotWorkspaces.emplace_back("__" + fitWsGroupName + "_scaled");
    plotIndices.emplace_back(0);
  }
  m_plotter->plotCorrespondingSpectra(
      plotWorkspaces, plotIndices, std::vector<bool>(plotWorkspaces.size(), SettingsHelper::externalPlotErrorBars()));
}

/**
 * Handles saving when button is clicked
 */
void ResNorm::saveClicked() {

  const auto resWsName(m_uiForm.dsResolution->getCurrentDataName());
  const auto outputWsName = getWorkspaceBasename(resWsName.toStdString()) + "_ResNorm";
  addSaveWorkspaceToQueue(outputWsName);

  m_pythonExportWsName = outputWsName;
  // Check workspace exists
  InelasticTab::checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);

  addSaveWorkspaceToQueue(outputWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles plotting when button is clicked
 */
void ResNorm::plotClicked() {
  setPlotResultIsPlotting(true);
  auto const errorBars = SettingsHelper::externalPlotErrorBars();

  QString const plotOptions = m_uiForm.cbPlot->currentText();
  if (plotOptions == "Intensity" || plotOptions == "All")
    m_plotter->plotSpectra(m_pythonExportWsName + "_Intensity", "0", errorBars);
  if (plotOptions == "Stretch" || plotOptions == "All")
    m_plotter->plotSpectra(m_pythonExportWsName + "_Stretch", "0", errorBars);
  setPlotResultIsPlotting(false);
}

void ResNorm::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void ResNorm::setSaveResultEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void ResNorm::setButtonsEnabled(bool enabled) {
  m_runPresenter->setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void ResNorm::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

} // namespace MantidQt::CustomInterfaces
