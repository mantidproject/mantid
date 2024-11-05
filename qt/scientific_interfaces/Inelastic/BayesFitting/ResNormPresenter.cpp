// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ResNormPresenter.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <QColor>
#include <qnamespace.h>
#include <string>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
Mantid::Kernel::Logger g_log("ResNormPresenter");
} // namespace

namespace MantidQt::CustomInterfaces {
ResNormPresenter::ResNormPresenter(QWidget *parent, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner,
                                   std::unique_ptr<IResNormModel> model, IResNormView *view)
    : BayesFittingTab(parent, std::move(algorithmRunner)), m_model(std::move(model)), m_view(view), m_previewSpec(0) {
  m_view->subscribePresenter(this);

  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
}

void ResNormPresenter::handleValidation(IUserInputValidator *validator) const {
  bool const vanValid = validator->checkDataSelectorIsValid("Vanadium", m_view->getDataSelector("Vanadium"));
  bool const resValid = validator->checkDataSelectorIsValid("Resolution", m_view->getDataSelector("Resolution"));

  if (vanValid) {
    // Check vanadium input is _red or _sqw workspace
    auto const vanName = m_view->getCurrentDataName("Vanadium");

    auto const cutIndex = vanName.find_last_of("_");
    auto const vanSuffix = vanName.substr(cutIndex + 1);
    if (vanSuffix.compare("red") != 0 && vanSuffix.compare("sqw") != 0)
      validator->addErrorMessage("The Vanadium run is not _red or _sqw workspace");

    // Check Res and Vanadium are the same Run
    if (resValid) {
      // Check that Res file is still in ADS if not, load it
      auto const resolutionWs = getADSWorkspace(m_view->getCurrentDataName("Vanadium"));
      auto const vanadiumWs = getADSWorkspace(vanName);

      int const resRun = resolutionWs->getRunNumber();
      int const vanRun = vanadiumWs->getRunNumber();

      if (resRun != vanRun)
        validator->addErrorMessage("The provided Vanadium and Resolution do not have "
                                   "matching run numbers");
    }
  }

  // check eMin and eMax values
  auto const eMin = m_model->eMin();
  auto const eMax = m_model->eMax();
  if (eMin >= eMax)
    validator->addErrorMessage("EMin must be strictly less than EMax.\n");
}

void ResNormPresenter::handleRun() {
  m_view->watchADS(false);

  auto const vanWsName(m_view->getCurrentDataName("Vanadium"));
  auto const resWsName(m_view->getCurrentDataName("Resolution"));
  auto const outputWsName = getWorkspaceBasename(resWsName) + "_ResNorm";

  auto resAlgo = m_model->setupResNormAlgorithm(outputWsName, vanWsName, resWsName);
  m_pythonExportWsName = outputWsName;
  m_algorithmRunner->execute((std::move(resAlgo)));
}

/**
 * Handle completion of the algorithm.
 *
 * @param error If the algorithm failed
 */
void ResNormPresenter::runComplete(IAlgorithm_sptr const &algorithm, bool const error) {
  (void)algorithm;
  m_view->setPlotResultEnabled(!error);
  m_view->setSaveResultEnabled(!error);
  if (!error) {
    // Update preview plot
    handlePreviewSpecChanged(m_previewSpec);
    // Copy and add sample logs to result workspaces
    setSampleLogs();
    m_view->watchADS(true);
  }
}

void ResNormPresenter::setFileExtensionsByName(bool filter) { m_view->setSuffixes(filter); }

void ResNormPresenter::setLoadHistory(bool doLoadHistory) { m_view->setLoadHistory(doLoadHistory); }

void ResNormPresenter::setSampleLogs() const {
  auto const resWsName = m_view->getCurrentDataName("Resolution");
  auto const vanWsName = m_view->getCurrentDataName("Vanadium");
  auto const outputWsName = getWorkspaceBasename(resWsName) + "_ResNorm";
  m_model->processLogs(vanWsName, resWsName, outputWsName);
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The current settings
 */
void ResNormPresenter::loadSettings(const QSettings &settings) { m_view->loadSettings(settings); }

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void ResNormPresenter::handleVanadiumInputReady(std::string const &filename) {

  if (!m_view->plotHasCurve("Resolution")) {
    m_view->clearPlot();
  }
  m_view->updatePlot("Vanadium", m_previewSpec);

  auto const vanWs = getADSWorkspace(filename);
  if (vanWs) {
    m_view->setMaximumSpectrum(static_cast<int>(vanWs->getNumberHistograms()) - 1);
  }
  m_view->updateSelectorRange(filename);
}

void ResNormPresenter::handleResolutionInputReady() {
  if (!m_view->plotHasCurve("Vanadium")) {
    m_view->clearPlot();
  }
  m_view->updatePlot("Resolution", 0, "", Qt::blue);
}

/**
 * Sets a new preview spectrum for the mini plot.
 *
 * @param value workspace index
 */
void ResNormPresenter::handlePreviewSpecChanged(int value) {
  m_previewSpec = value;
  m_view->clearPlot();

  // Update vanadium plot
  if (m_view->getDataSelector("Vanadium")->isValid()) {
    m_view->updatePlot("Vanadium", m_previewSpec);
    m_view->updatePlot("Resolution", 0, "", Qt::blue);
  }
  updateFitPlot();
}

void ResNormPresenter::updateFitPlot() const {
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

      m_view->updatePlot("Fit", 0, fitWsName, Qt::green);

      AnalysisDataService::Instance().addOrReplace("__" + fitWsGroupName + "_scaled", fit);
    }
  }
}

/**
 * Plot the current spectrum in the miniplot
 */

void ResNormPresenter::handlePlotCurrentPreview() {

  std::vector<std::string> plotWorkspaces;
  std::vector<int> plotIndices;

  if (m_view->plotHasCurve("Vanadium")) {
    plotWorkspaces.emplace_back(m_view->getCurrentDataName("Vanadium"));
    plotIndices.emplace_back(m_previewSpec);
  }
  if (m_view->plotHasCurve("Resolution")) {
    plotWorkspaces.emplace_back(m_view->getCurrentDataName("Resolution"));
    plotIndices.emplace_back(0);
  }
  if (m_view->plotHasCurve("Fit")) {
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
void ResNormPresenter::handleSaveClicked() {
  const auto resWsName(m_view->getCurrentDataName("Resolution"));
  const auto outputWsName = getWorkspaceBasename(resWsName) + "_ResNorm";

  m_pythonExportWsName = outputWsName;
  // Check workspace exists
  InelasticTab::checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);

  auto const saveAlgo = m_model->setupSaveAlgorithm(outputWsName);
  m_algorithmRunner->execute(std::move(saveAlgo));
}

void ResNormPresenter::handleDoubleValueChanged(std::string const &propertyName, double value) {
  if (propertyName == "EMax") {
    m_model->setEMax(value);
  } else if (propertyName == "EMin") {
    m_model->setEMin(value);
  }
}

/**
 * Handles plotting when button is clicked
 */
void ResNormPresenter::handlePlotClicked(std::string const &plotOptions) {
  m_view->setPlotResultIsPlotting(true);
  auto const errorBars = SettingsHelper::externalPlotErrorBars();

  if (plotOptions == "Intensity" || plotOptions == "All")
    m_plotter->plotSpectra(m_pythonExportWsName + "_Intensity", "0", errorBars);
  if (plotOptions == "Stretch" || plotOptions == "All")
    m_plotter->plotSpectra(m_pythonExportWsName + "_Stretch", "0", errorBars);
  m_view->setPlotResultIsPlotting(false);
}
} // namespace MantidQt::CustomInterfaces
