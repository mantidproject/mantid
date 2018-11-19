// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ResNorm.h"

#include "../General/UserInputValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr
retrieveADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
ResNorm::ResNorm(QWidget *parent) : IndirectBayesTab(parent), m_previewSpec(0) {
  m_uiForm.setupUi(parent);

  // Create range selector
  auto eRangeSelector = m_uiForm.ppPlot->addRangeSelector("ResNormERange");
  connect(eRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minValueChanged(double)));
  connect(eRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxValueChanged(double)));

  // Add the properties browser to the ui form
  m_uiForm.treeSpace->addWidget(m_propTree);

  m_properties["EMin"] = m_dblManager->addProperty("EMin");
  m_properties["EMax"] = m_dblManager->addProperty("EMax");

  m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

  m_propTree->addProperty(m_properties["EMin"]);
  m_propTree->addProperty(m_properties["EMax"]);

  // Connect data selector to handler method
  connect(m_uiForm.dsVanadium, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleVanadiumInputReady(const QString &)));
  connect(m_uiForm.dsResolution, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleResolutionInputReady(const QString &)));

  // Connect the preview spectrum selector
  connect(m_uiForm.spPreviewSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(previewSpecChanged(int)));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(handleAlgorithmComplete(bool)));

  // Post Plot and Save
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbPlotCurrent, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
}

void ResNorm::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool ResNorm::validate() {
  UserInputValidator uiv;
  QString errors("");

  bool const vanValid =
      uiv.checkDataSelectorIsValid("Vanadium", m_uiForm.dsVanadium);
  bool const resValid =
      uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  if (vanValid) {
    // Check vanadium input is _red or _sqw workspace
    QString const vanName = m_uiForm.dsVanadium->getCurrentDataName();
    int const cutIndex = vanName.lastIndexOf("_");
    QString const vanSuffix = vanName.right(vanName.size() - (cutIndex + 1));
    if (vanSuffix.compare("red") != 0 || vanSuffix.compare("sqw") != 0)
      uiv.addErrorMessage("The Vanadium run is not _red or _sqw workspace");

    // Check Res and Vanadium are the same Run
    if (resValid) {
      // Check that Res file is still in ADS if not, load it
      auto const resolutionWs = retrieveADSMatrixWorkspace(
          m_uiForm.dsResolution->getCurrentDataName().toStdString());
      auto const vanadiumWs = retrieveADSMatrixWorkspace(vanName.toStdString());

      int const resRun = resolutionWs->getRunNumber();
      int const vanRun = vanadiumWs->getRunNumber();

      if (resRun != vanRun)
        uiv.addErrorMessage("The provided Vanadium and Resolution do not have "
                            "matching run numbers");
    }
  }

  // check eMin and eMax values
  auto const eMin = m_dblManager->value(m_properties["EMin"]);
  auto const eMax = m_dblManager->value(m_properties["EMax"]);
  if (eMin >= eMax)
    errors.append("EMin must be strictly less than EMax.\n");

  // Create and show error messages
  errors.append(uiv.generateErrorMessage());
  if (!errors.isEmpty()) {
    emit showMessageBox(errors);
    return false;
  }

  return true;
}

/**
 * Run the ResNorm v2 algorithm.
 */
void ResNorm::run() {
  const auto vanWsName(m_uiForm.dsVanadium->getCurrentDataName());
  const auto resWsName(m_uiForm.dsResolution->getCurrentDataName());

  const auto eMin(m_dblManager->value(m_properties["EMin"]));
  const auto eMax(m_dblManager->value(m_properties["EMax"]));

  const auto outputWsName = getWorkspaceBasename(resWsName) + "_ResNorm";

  IAlgorithm_sptr resNorm = AlgorithmManager::Instance().create("ResNorm", 2);
  resNorm->initialize();
  resNorm->setProperty("VanadiumWorkspace", vanWsName.toStdString());
  resNorm->setProperty("ResolutionWorkspace", resWsName.toStdString());
  resNorm->setProperty("EnergyMin", eMin);
  resNorm->setProperty("EnergyMax", eMax);
  resNorm->setProperty("CreateOutput", true);
  resNorm->setProperty("OutputWorkspace", outputWsName.toStdString());
  resNorm->setProperty("OutputWorkspaceTable",
                       (outputWsName + "_Fit").toStdString());
  m_batchAlgoRunner->addAlgorithm(resNorm);
  m_pythonExportWsName = outputWsName.toStdString();
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle completion of the algorithm.
 *
 * @param error If the algorithm failed
 */
void ResNorm::handleAlgorithmComplete(bool error) {
  setRunIsRunning(false);
  if (!error)
    // Update preview plot
    previewSpecChanged(m_previewSpec);
  else {
    setPlotResultEnabled(false);
    setSaveResultEnabled(false);
  }
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
  m_uiForm.ppPlot->addSpectrum("Vanadium", filename, m_previewSpec);

  QPair<double, double> res;
  QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Vanadium");

  MatrixWorkspace_sptr vanWs =
      retrieveADSMatrixWorkspace(filename.toStdString());
  m_uiForm.spPreviewSpectrum->setMaximum(
      static_cast<int>(vanWs->getNumberHistograms()) - 1);

  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormERange");

  // Use the values from the instrument parameter file if we can
  // The the maximum and minimum value of the plot
  if (getResolutionRangeFromWs(filename, res)) {
    // ResNorm resolution should be +/- 10 * the IPF resolution
    res.first = res.first * 10;
    res.second = res.second * 10;

    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                     res);
  } else {
    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                     range);
  }

  setPlotPropertyRange(eRangeSelector, m_properties["EMin"],
                       m_properties["EMax"], range);

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
  // Plot the resolution
  m_uiForm.ppPlot->addSpectrum("Resolution", filename, 0, Qt::blue);
}

/**
 * Updates the property manager when the lower guide is moved on the mini plot
 *
 * @param min :: The new value of the lower guide
 */
void ResNorm::minValueChanged(double min) {
  m_dblManager->setValue(m_properties["EMin"], min);
}

/**
 * Updates the property manager when the upper guide is moved on the mini plot
 *
 * @param max :: The new value of the upper guide
 */
void ResNorm::maxValueChanged(double max) {
  m_dblManager->setValue(m_properties["EMax"], max);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void ResNorm::updateProperties(QtProperty *prop, double val) {
  UNUSED_ARG(val);

  auto eRangeSelector = m_uiForm.ppPlot->getRangeSelector("ResNormERange");

  if (prop == m_properties["EMin"] || prop == m_properties["EMax"]) {
    auto bounds = qMakePair(m_dblManager->value(m_properties["EMin"]),
                            m_dblManager->value(m_properties["EMax"]));
    setRangeSelector(eRangeSelector, m_properties["EMin"], m_properties["EMax"],
                     bounds);
  }
}

/**
 * Sets a new preview spectrum for the mini plot.
 *
 * @param value workspace index
 */
void ResNorm::previewSpecChanged(int value) {
  m_previewSpec = value;

  // Update vanadium plot
  if (m_uiForm.dsVanadium->isValid())
    m_uiForm.ppPlot->addSpectrum(
        "Vanadium", m_uiForm.dsVanadium->getCurrentDataName(), m_previewSpec);

  // Update fit plot
  std::string fitWsGroupName(m_pythonExportWsName + "_Fit_Workspaces");
  std::string fitParamsName(m_pythonExportWsName + "_Fit");
  if (AnalysisDataService::Instance().doesExist(fitWsGroupName)) {
    WorkspaceGroup_sptr fitWorkspaces =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            fitWsGroupName);
    ITableWorkspace_sptr fitParams =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            fitParamsName);
    if (fitWorkspaces && fitParams) {
      Column_const_sptr scaleFactors = fitParams->getColumn("Scaling");
      std::string fitWsName(fitWorkspaces->getItem(m_previewSpec)->getName());
      MatrixWorkspace_const_sptr fitWs = retrieveADSMatrixWorkspace(fitWsName);

      MatrixWorkspace_sptr fit = WorkspaceFactory::Instance().create(fitWs, 1);
      fit->setSharedX(0, fitWs->sharedX(1));
      fit->setSharedY(0, fitWs->sharedY(1));
      fit->setSharedE(0, fitWs->sharedE(1));

      fit->mutableY(0) /= scaleFactors->cell<double>(m_previewSpec);

      m_uiForm.ppPlot->addSpectrum("Fit", fit, 0, Qt::red);

      AnalysisDataService::Instance().addOrReplace(
          "__" + fitWsGroupName + "_scaled", fit);
    }
  }
}

/**
 * Plot the current spectrum in the miniplot
 */

void ResNorm::plotCurrentPreview() {

  QStringList plotWorkspaces;
  std::vector<int> plotIndices;

  if (m_uiForm.ppPlot->hasCurve("Vanadium")) {
    plotWorkspaces << m_uiForm.dsVanadium->getCurrentDataName();
    plotIndices.push_back(m_previewSpec);
  }
  if (m_uiForm.ppPlot->hasCurve("Resolution")) {
    plotWorkspaces << m_uiForm.dsResolution->getCurrentDataName();
    plotIndices.push_back(0);
  }
  if (m_uiForm.ppPlot->hasCurve("Fit")) {
    std::string fitWsGroupName(m_pythonExportWsName + "_Fit_Workspaces");

    plotWorkspaces << QString::fromStdString("__" + fitWsGroupName + "_scaled");
    plotIndices.push_back(0);
  }
  plotMultipleSpectra(plotWorkspaces, plotIndices);
}

void ResNorm::runClicked() {
  if (validateTab()) {
    setRunIsRunning(true);
    runTab();
  }
}

/**
 * Handles saving when button is clicked
 */
void ResNorm::saveClicked() {

  const auto resWsName(m_uiForm.dsResolution->getCurrentDataName());
  const auto outputWsName = getWorkspaceBasename(resWsName) + "_ResNorm";
  addSaveWorkspaceToQueue(outputWsName);

  m_pythonExportWsName = outputWsName.toStdString();
  // Check workspace exists
  IndirectTab::checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);

  addSaveWorkspaceToQueue(outputWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles plotting when button is clicked
 */
void ResNorm::plotClicked() {
  setPlotResultIsPlotting(true);

  QString const plotOptions = m_uiForm.cbPlot->currentText();
  if (plotOptions == "Intensity" || plotOptions == "All")
    plotSpectrum(QString::fromStdString(m_pythonExportWsName) + "_Intensity");
  if (plotOptions == "Stretch" || plotOptions == "All")
    plotSpectrum(QString::fromStdString(m_pythonExportWsName) + "_Stretch");

  setPlotResultIsPlotting(false);
}

void ResNorm::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void ResNorm::setPlotResultEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlot->setEnabled(enabled);
}

void ResNorm::setSaveResultEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void ResNorm::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotResultEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void ResNorm::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void ResNorm::setPlotResultIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

} // namespace CustomInterfaces
} // namespace MantidQt
