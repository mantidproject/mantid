// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitAnalysisTab.h"
#include "IndirectSettingsHelper.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QString>
#include <QtCore>

#include <algorithm>
#include <utility>

/// Logger
Mantid::Kernel::Logger g_log("IndirectFitAnalysisTab");

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

WorkspaceGroup_sptr getADSGroupWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(workspaceName);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * @param functionName  The name of the function.
 * @param compositeFunction  The function to search within.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t IndirectFitAnalysisTab::getNumberOfSpecificFunctionContained(const std::string &functionName,
                                                                    const IFunction *compositeFunction) {
  // Whilst this could be a free method it would require its own
  // dll_export in the header, so it's easier to make it static
  assert(compositeFunction);

  if (compositeFunction->nFunctions() == 0) {
    return compositeFunction->name() == functionName ? 1 : 0;
  } else {
    size_t count{0};
    for (size_t i{0}; i < compositeFunction->nFunctions(); i++) {
      count += getNumberOfSpecificFunctionContained(functionName, compositeFunction->getFunction(i).get());
    }
    return count;
  }
}

IndirectFitAnalysisTab::IndirectFitAnalysisTab(IndirectFittingModel *model, QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_fittingModel(model) {}

void IndirectFitAnalysisTab::setup() {
  setupFitTab();
  updateResultOptions();

  connect(m_outOptionsPresenter.get(), SIGNAL(plotSpectra()), this, SLOT(plotSelectedSpectra()));

  connectDataPresenter();
  connectPlotPresenter();
  connectFitPropertyBrowser();
}

void IndirectFitAnalysisTab::connectDataPresenter() {
  connect(m_dataPresenter.get(), SIGNAL(startXChanged(double, WorkspaceID, WorkspaceIndex)), this,
          SLOT(tableStartXChanged(double, WorkspaceID, WorkspaceIndex)));
  connect(m_dataPresenter.get(), SIGNAL(endXChanged(double, WorkspaceID, WorkspaceIndex)), this,
          SLOT(tableEndXChanged(double, WorkspaceID, WorkspaceIndex)));
  connect(m_dataPresenter.get(), SIGNAL(excludeRegionChanged(const std::string &, WorkspaceID, WorkspaceIndex)), this,
          SLOT(tableExcludeChanged(const std::string &, WorkspaceID, WorkspaceIndex)));
  connect(m_dataPresenter.get(), SIGNAL(startXChanged(double)), this, SLOT(startXChanged(double)));
  connect(m_dataPresenter.get(), SIGNAL(endXChanged(double)), this, SLOT(endXChanged(double)));

  connect(m_dataPresenter.get(), SIGNAL(singleResolutionLoaded()), this, SLOT(respondToSingleResolutionLoaded()));
  connect(m_dataPresenter.get(), SIGNAL(dataChanged()), this, SLOT(respondToDataChanged()));
  connect(m_dataPresenter.get(), SIGNAL(dataAdded()), this, SLOT(respondToDataAdded()));
  connect(m_dataPresenter.get(), SIGNAL(dataRemoved()), this, SLOT(respondToDataRemoved()));
}

void IndirectFitAnalysisTab::connectPlotPresenter() {
  connect(m_plotPresenter.get(), SIGNAL(fitSingleSpectrum(WorkspaceID, WorkspaceIndex)), this,
          SLOT(singleFit(WorkspaceID, WorkspaceIndex)));
  connect(m_plotPresenter.get(), SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));
  connect(m_plotPresenter.get(), SIGNAL(startXChanged(double)), this, SLOT(updateDataInTable()));
  connect(m_plotPresenter.get(), SIGNAL(endXChanged(double)), this, SLOT(updateDataInTable()));
  connect(m_plotPresenter.get(), SIGNAL(selectedFitDataChanged(WorkspaceID)), this,
          SLOT(respondToSelectedFitDataChanged(WorkspaceID)));
  connect(m_plotPresenter.get(), SIGNAL(noFitDataSelected()), this, SLOT(respondToNoFitDataSelected()));
  connect(m_plotPresenter.get(), SIGNAL(plotSpectrumChanged()), this, SLOT(respondToPlotSpectrumChanged()));
  connect(m_plotPresenter.get(), SIGNAL(fwhmChanged(double)), this, SLOT(respondToFwhmChanged(double)));
  connect(m_plotPresenter.get(), SIGNAL(backgroundChanged(double)), this, SLOT(respondToBackgroundChanged(double)));
}

void IndirectFitAnalysisTab::connectFitPropertyBrowser() {
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this, SLOT(respondToFunctionChanged()));
}

void IndirectFitAnalysisTab::setFitDataPresenter(std::unique_ptr<IndirectFitDataPresenter> presenter) {
  m_dataPresenter = std::move(presenter);
}

void IndirectFitAnalysisTab::setPlotView(IIndirectFitPlotView *view) {
  m_plotPresenter = std::make_unique<IndirectFitPlotPresenter>(m_fittingModel.get(), view);
}

void IndirectFitAnalysisTab::setOutputOptionsView(IIndirectFitOutputOptionsView *view) {
  m_outOptionsPresenter = std::make_unique<IndirectFitOutputOptionsPresenter>(view);
}

void IndirectFitAnalysisTab::setFitPropertyBrowser(IndirectFitPropertyBrowser *browser) {
  browser->init();
  m_fitPropertyBrowser = browser;
}

void IndirectFitAnalysisTab::setFileExtensionsByName(bool filter) {
  auto const tab = getTabName();
  setSampleSuffixes(tab, filter);
  if (hasResolution())
    setResolutionSuffixes(tab, filter);
}

void IndirectFitAnalysisTab::setSampleSuffixes(std::string const &tab, bool filter) {
  QStringList const noSuffixes{""};
  setSampleWSSuffixes(filter ? getSampleWSSuffixes(tab) : noSuffixes);
  setSampleFBSuffixes(filter ? getSampleFBSuffixes(tab) : getExtensions(tab));
}

void IndirectFitAnalysisTab::setResolutionSuffixes(std::string const &tab, bool filter) {
  QStringList const noSuffixes{""};
  setResolutionWSSuffixes(filter ? getResolutionWSSuffixes(tab) : noSuffixes);
  setResolutionFBSuffixes(filter ? getResolutionFBSuffixes(tab) : getExtensions(tab));
}

void IndirectFitAnalysisTab::setSampleWSSuffixes(const QStringList &suffices) {
  m_dataPresenter->setSampleWSSuffices(suffices);
}

void IndirectFitAnalysisTab::setSampleFBSuffixes(const QStringList &suffices) {
  m_dataPresenter->setSampleFBSuffices(suffices);
}

void IndirectFitAnalysisTab::setResolutionWSSuffixes(const QStringList &suffices) {
  m_dataPresenter->setResolutionWSSuffices(suffices);
}

void IndirectFitAnalysisTab::setResolutionFBSuffixes(const QStringList &suffices) {
  m_dataPresenter->setResolutionFBSuffices(suffices);
}

WorkspaceID IndirectFitAnalysisTab::getSelectedDataIndex() const { return m_plotPresenter->getSelectedDataIndex(); }

WorkspaceIndex IndirectFitAnalysisTab::getSelectedSpectrum() const { return m_plotPresenter->getSelectedSpectrum(); }

bool IndirectFitAnalysisTab::isRangeCurrentlySelected(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return m_plotPresenter->isCurrentlySelected(workspaceID, spectrum);
}

IndirectFittingModel *IndirectFitAnalysisTab::getFittingModel() const { return m_fittingModel.get(); }

/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t IndirectFitAnalysisTab::getNumberOfCustomFunctions(const std::string &functionName) const {
  auto fittingFunction = m_fittingModel->getFitFunction();
  if (fittingFunction && fittingFunction->nFunctions() > 0)
    return getNumberOfSpecificFunctionContained(functionName, fittingFunction->getFunction(0).get());
  else
    return 0;
}

void IndirectFitAnalysisTab::setModelFitFunction() {
  auto func = m_fitPropertyBrowser->getFitFunction();
  m_fittingModel->setFitFunction(func);
}

void IndirectFitAnalysisTab::setModelStartX(double startX) {
  const auto dataIndex = getSelectedDataIndex();
  if (m_fittingModel->getNumberOfWorkspaces() > dataIndex) {
    m_fittingModel->setStartX(startX, dataIndex, getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::setModelEndX(double endX) {
  const auto dataIndex = getSelectedDataIndex();
  if (m_fittingModel->getNumberOfWorkspaces() > dataIndex) {
    m_fittingModel->setEndX(endX, dataIndex, getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::updateDataInTable() { m_dataPresenter->updateDataInTable(); }

void IndirectFitAnalysisTab::tableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (isRangeCurrentlySelected(workspaceID, spectrum)) {
    m_plotPresenter->setStartX(startX);
    m_plotPresenter->updateGuess();
    m_fittingModel->setStartX(startX, m_plotPresenter->getSelectedDataIndex(), m_plotPresenter->getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::tableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (isRangeCurrentlySelected(workspaceID, spectrum)) {
    m_plotPresenter->setEndX(endX);
    m_plotPresenter->updateGuess();
    m_fittingModel->setEndX(endX, m_plotPresenter->getSelectedDataIndex(), m_plotPresenter->getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::startXChanged(double startX) {
  m_plotPresenter->setStartX(startX);
  m_fittingModel->setStartX(startX, m_plotPresenter->getSelectedDataIndex());
  updateParameterEstimationData();
  m_plotPresenter->updateGuess();
}

void IndirectFitAnalysisTab::endXChanged(double endX) {
  m_plotPresenter->setEndX(endX);
  m_fittingModel->setEndX(endX, m_plotPresenter->getSelectedDataIndex());
  updateParameterEstimationData();
  m_plotPresenter->updateGuess();
}

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveMembers If true, members are to be convolved.
 */
void IndirectFitAnalysisTab::setConvolveMembers(bool convolveMembers) {
  m_fitPropertyBrowser->setConvolveMembers(convolveMembers);
  // if convolve members is on, output members should also be on
  if (convolveMembers)
    m_fitPropertyBrowser->setOutputCompositeMembers(true);
}

void IndirectFitAnalysisTab::updateFitOutput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(updateFitOutput(bool)));

  if (error) {
    m_fittingModel->cleanFailedRun(m_fittingAlgorithm);
    m_fittingAlgorithm.reset();
  } else {
    m_fittingModel->addOutput(m_fittingAlgorithm);
  }
}

void IndirectFitAnalysisTab::updateSingleFitOutput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(updateSingleFitOutput(bool)));

  if (error) {
    m_fittingModel->cleanFailedSingleRun(m_fittingAlgorithm, m_activeWorkspaceID);
    m_fittingAlgorithm.reset();
  } else
    m_fittingModel->addSingleFitOutput(m_fittingAlgorithm, m_activeWorkspaceID, m_activeSpectrumIndex);
}

/**
 * Performs necessary state changes when the fit algorithm was run
 * and completed within this interface.
 */
void IndirectFitAnalysisTab::fitAlgorithmComplete(bool error) {
  setRunIsRunning(false);
  m_plotPresenter->setFitSingleSpectrumIsFitting(false);
  enableFitButtons(true);
  enableOutputOptions(!error);
  m_fitPropertyBrowser->setErrorsEnabled(!error);
  if (!error) {
    updateFitBrowserParameterValuesFromAlg();
    updateFitStatus();
    setModelFitFunction();
  }
  m_plotPresenter->updatePlots();
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(fitAlgorithmComplete(bool)));
}

/**
 * Updates the parameter values and errors in the fit property browser.
 */
void IndirectFitAnalysisTab::updateParameterValues() {
  updateParameterValues(m_fittingModel->getParameterValues(getSelectedDataIndex(), getSelectedSpectrum()));
}

/**
 * Updates the parameter values and errors in the fit property browser.
 *
 * @param parameters  The parameter values to update the browser with.
 */
void IndirectFitAnalysisTab::updateParameterValues(const std::unordered_map<std::string, ParameterValue> &params) {
  try {
    updateFitBrowserParameterValues(params);
  } catch (const std::out_of_range &) {
    g_log.warning("Warning issue updating parameter values in fit property browser");
  } catch (const std::invalid_argument &) {
    g_log.warning("Warning issue updating parameter values in fit property browser");
  }
}

void IndirectFitAnalysisTab::updateFitBrowserParameterValues(std::unordered_map<std::string, ParameterValue> params) {
  IFunction_sptr fun = m_fittingModel->getFitFunction();
  if (fun) {
    for (auto pair : params) {
      fun->setParameter(pair.first, pair.second.value);
    }
    if (fun->getNumberDomains() > 1) {
      m_fitPropertyBrowser->updateMultiDatasetParameters(*fun);
    } else {
      m_fitPropertyBrowser->updateParameters(*fun);
    }
  }
}

void IndirectFitAnalysisTab::updateFitBrowserParameterValuesFromAlg() {
  try {
    updateFitBrowserParameterValues();
    if (m_fittingAlgorithm) {
      MantidQt::API::SignalBlocker blocker(m_fitPropertyBrowser);
      if (m_fittingModel->getFittingMode() == FittingMode::SEQUENTIAL) {
        auto const paramWsName = m_fittingAlgorithm->getPropertyValue("OutputParameterWorkspace");
        auto paramWs = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(paramWsName);
        auto rowCount = static_cast<int>(paramWs->rowCount());
        if (rowCount == static_cast<int>(m_fittingModel->getNumberOfDomains()))
          m_fitPropertyBrowser->updateMultiDatasetParameters(*paramWs);
      } else {
        IFunction_sptr fun = m_fittingAlgorithm->getProperty("Function");
        if (fun->getNumberDomains() > 1)
          m_fitPropertyBrowser->updateMultiDatasetParameters(*fun);
        else
          m_fitPropertyBrowser->updateParameters(*fun);
      }
    }
  } catch (const std::out_of_range &) {
    g_log.warning("Warning issue updating parameter values in fit property browser");
  } catch (const std::invalid_argument &) {
    g_log.warning("Warning issue updating parameter values in fit property browser");
  }
}
/**
 * Updates the fit output status
 */
void IndirectFitAnalysisTab::updateFitStatus() {

  if (m_fittingModel->getFittingMode() == FittingMode::SIMULTANEOUS) {
    std::string fit_status = m_fittingAlgorithm->getProperty("OutputStatus");
    double chi2 = m_fittingAlgorithm->getProperty("OutputChiSquared");
    const std::vector<std::string> status(m_fittingModel->getNumberOfDomains(), fit_status);
    const std::vector<double> chiSquared(m_fittingModel->getNumberOfDomains(), chi2);
    m_fitPropertyBrowser->updateFitStatusData(status, chiSquared);
  } else {
    const std::vector<std::string> status = m_fittingAlgorithm->getProperty("OutputStatus");
    const std::vector<double> chiSquared = m_fittingAlgorithm->getProperty("OutputChiSquared");
    m_fitPropertyBrowser->updateFitStatusData(status, chiSquared);
  }
}
/**
 * Plots the spectra corresponding to the selected parameters
 */
void IndirectFitAnalysisTab::plotSelectedSpectra() {
  enableFitButtons(false);
  plotSelectedSpectra(m_outOptionsPresenter->getSpectraToPlot());
  enableFitButtons(true);
  m_outOptionsPresenter->setPlotting(false);
}

/**
 * Plots the spectra corresponding to the selected parameters
 * @param spectra :: a vector of spectra to plot from a group workspace
 */
void IndirectFitAnalysisTab::plotSelectedSpectra(std::vector<SpectrumToPlot> const &spectra) {
  for (auto const &spectrum : spectra)
    plotSpectrum(spectrum.first, spectrum.second);
  m_outOptionsPresenter->clearSpectraToPlot();
}

/**
 * Plots a spectrum with the specified index in a workspace
 * @workspaceName :: the workspace containing the spectrum to plot
 * @index :: the index in the workspace
 * @errorBars :: true if you want error bars to be plotted
 */
void IndirectFitAnalysisTab::plotSpectrum(std::string const &workspaceName, std::size_t const &index) {
  m_plotter->plotSpectra(workspaceName, std::to_string(index), IndirectSettingsHelper::externalPlotErrorBars());
}

/**
 * Gets the name used for the base of the result workspaces
 */
std::string IndirectFitAnalysisTab::getOutputBasename() const { return m_fittingModel->getOutputBasename(); }

/**
 * Gets the Result workspace from a fit
 */
WorkspaceGroup_sptr IndirectFitAnalysisTab::getResultWorkspace() const { return m_fittingModel->getResultWorkspace(); }

/**
 * Gets the names of the Fit Parameters
 */
std::vector<std::string> IndirectFitAnalysisTab::getFitParameterNames() const {
  return m_fittingModel->getFitParameterNames();
}

/**
 * Executes the single fit algorithm defined in this indirect fit analysis tab.
 */
void IndirectFitAnalysisTab::singleFit() { singleFit(getSelectedDataIndex(), getSelectedSpectrum()); }

void IndirectFitAnalysisTab::singleFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (validate()) {
    m_activeSpectrumIndex = spectrum;
    m_plotPresenter->setFitSingleSpectrumIsFitting(true);
    enableFitButtons(false);
    enableOutputOptions(false);
    m_fittingModel->setFittingMode(FittingMode::SIMULTANEOUS);
    m_activeWorkspaceID = workspaceID;
    runSingleFit(m_fittingModel->getSingleFit(workspaceID, spectrum));
  }
}

/**
 * Executes the sequential fit algorithm defined in this indirect fit analysis
 * tab.
 */
void IndirectFitAnalysisTab::executeFit() {
  if (validate()) {
    setRunIsRunning(true);
    enableFitButtons(false);
    enableOutputOptions(false);
    runFitAlgorithm(m_fittingModel->getFittingAlgorithm());
  }
}

bool IndirectFitAnalysisTab::validate() {
  UserInputValidator validator;
  m_dataPresenter->validate(validator);

  const auto invalidFunction = m_fittingModel->isInvalidFunction();
  if (invalidFunction)
    validator.addErrorMessage(QString::fromStdString(*invalidFunction));
  if (m_fittingModel->getNumberOfWorkspaces() == WorkspaceID{0})
    validator.addErrorMessage(QString::fromStdString("No data has been selected for a fit."));

  const auto error = validator.generateErrorMessage();
  emit showMessageBox(error);
  return error.isEmpty();
}

/**
 * Called when the 'Run' button is called in the IndirectTab.
 */
void IndirectFitAnalysisTab::run() {
  setRunIsRunning(true);
  enableFitButtons(false);
  enableOutputOptions(false);
  m_fittingModel->setFittingMode(m_fitPropertyBrowser->getFittingMode());
  runFitAlgorithm(m_fittingModel->getFittingAlgorithm());
}

/**
 * Enables or disables the 'Run', 'Fit Single Spectrum' and other related
 * buttons
 * @param enable :: true to enable buttons
 */
void IndirectFitAnalysisTab::enableFitButtons(bool enable) {
  setRunEnabled(enable);
  m_plotPresenter->setFitSingleSpectrumEnabled(enable);
  m_fitPropertyBrowser->setFitEnabled(enable);
}

/**
 * Enables or disables the output options. It also sets the current result and
 * PDF workspaces to be plotted
 * @param enable :: true to enable buttons
 */
void IndirectFitAnalysisTab::enableOutputOptions(bool enable) {
  if (enable) {
    m_outOptionsPresenter->setResultWorkspace(getResultWorkspace());
    setPDFWorkspace(getOutputBasename() + "_PDFs");
    m_outOptionsPresenter->setPlotTypes("Result Group");
  } else
    m_outOptionsPresenter->setMultiWorkspaceOptionsVisible(enable);

  m_outOptionsPresenter->setPlotEnabled(enable && m_outOptionsPresenter->isSelectedGroupPlottable());
  m_outOptionsPresenter->setEditResultEnabled(enable);
  m_outOptionsPresenter->setSaveEnabled(enable);
}

/**
 * Sets the active PDF workspace within the output options if one exists for the
 * current run
 * @param workspaceName :: the name of the PDF workspace if it exists
 */
void IndirectFitAnalysisTab::setPDFWorkspace(std::string const &workspaceName) {
  auto const fabMinimizer = m_fitPropertyBrowser->minimizer() == "FABADA";
  auto const enablePDFOptions = doesExistInADS(workspaceName) && fabMinimizer;

  if (enablePDFOptions) {
    m_outOptionsPresenter->setPDFWorkspace(getADSGroupWorkspace(workspaceName));
    m_outOptionsPresenter->setPlotWorkspaces();
  } else
    m_outOptionsPresenter->removePDFWorkspace();
  m_outOptionsPresenter->setMultiWorkspaceOptionsVisible(enablePDFOptions);
}

void IndirectFitAnalysisTab::updateParameterEstimationData() {
  m_fitPropertyBrowser->updateParameterEstimationData(
      m_dataPresenter->getDataForParameterEstimation(getEstimationDataSelector()));
  const bool isFit = m_fittingModel->isPreviouslyFit(getSelectedDataIndex(), getSelectedSpectrum());
  // If we haven't fit the data yet we may update the guess
  if (!isFit) {
    m_fitPropertyBrowser->estimateFunctionParameters();
  }
}

/**
 * Sets the visiblity of the output options Edit Result button
 * @param visible :: true to make the edit result button visible
 */
void IndirectFitAnalysisTab::setEditResultVisible(bool visible) {
  m_outOptionsPresenter->setEditResultVisible(visible);
}

void IndirectFitAnalysisTab::setAlgorithmProperties(const IAlgorithm_sptr &fitAlgorithm) const {
  fitAlgorithm->setProperty("Minimizer", m_fitPropertyBrowser->minimizer(true));
  fitAlgorithm->setProperty("MaxIterations", m_fitPropertyBrowser->maxIterations());
  fitAlgorithm->setProperty("PeakRadius", m_fitPropertyBrowser->getPeakRadius());
  fitAlgorithm->setProperty("CostFunction", m_fitPropertyBrowser->costFunction());
  fitAlgorithm->setProperty("IgnoreInvalidData", m_fitPropertyBrowser->ignoreInvalidData());
  fitAlgorithm->setProperty("EvaluationType", m_fitPropertyBrowser->fitEvaluationType());
  fitAlgorithm->setProperty("PeakRadius", m_fitPropertyBrowser->getPeakRadius());
  if (m_fitPropertyBrowser->convolveMembers()) {
    fitAlgorithm->setProperty("ConvolveMembers", true);
    fitAlgorithm->setProperty("OutputCompositeMembers", true);
  } else {
    fitAlgorithm->setProperty("OutputCompositeMembers", m_fitPropertyBrowser->outputCompositeMembers());
  }

  if (m_fittingModel->getFittingMode() == FittingMode::SEQUENTIAL) {
    fitAlgorithm->setProperty("FitType", m_fitPropertyBrowser->fitType());
  }
  fitAlgorithm->setProperty("OutputFitStatus", true);
}

/*
 * Runs the specified fit algorithm and calls the algorithmComplete
 * method of this fit analysis tab once completed.
 *
 * @param fitAlgorithm      The fit algorithm to run.
 */
void IndirectFitAnalysisTab::runFitAlgorithm(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(updateFitOutput(bool)));
  setupFit(std::move(fitAlgorithm));
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectFitAnalysisTab::runSingleFit(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(updateSingleFitOutput(bool)));
  setupFit(std::move(fitAlgorithm));
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectFitAnalysisTab::setupFit(IAlgorithm_sptr fitAlgorithm) {
  setAlgorithmProperties(fitAlgorithm);
  m_fittingAlgorithm = fitAlgorithm;
  m_batchAlgoRunner->addAlgorithm(fitAlgorithm);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(fitAlgorithmComplete(bool)));
}

QList<FunctionModelDataset> IndirectFitAnalysisTab::getDatasets() const {
  QList<FunctionModelDataset> datasets;

  for (auto i = 0u; i < m_fittingModel->getNumberOfWorkspaces().value; ++i) {
    WorkspaceID workspaceID{i};

    auto const name = m_fittingModel->getWorkspace(workspaceID)->getName();
    datasets.append(FunctionModelDataset(QString::fromStdString(name), m_fittingModel->getSpectra(workspaceID)));
  }
  return datasets;
}

void IndirectFitAnalysisTab::updateDataReferences() {
  m_fitPropertyBrowser->updateFunctionBrowserData(static_cast<int>(m_fittingModel->getNumberOfDomains()), getDatasets(),
                                                  m_fittingModel->getQValuesForData(),
                                                  m_fittingModel->getResolutionsForFit());
  m_fittingModel->setFitFunction(m_fitPropertyBrowser->getFitFunction());
}

/**
 * Updates whether the options for plotting and saving fit results are
 * enabled/disabled.
 */
void IndirectFitAnalysisTab::updateResultOptions() {
  const bool isFit = m_fittingModel->isPreviouslyFit(getSelectedDataIndex(), getSelectedSpectrum());
  if (isFit)
    m_outOptionsPresenter->setResultWorkspace(getResultWorkspace());
  m_outOptionsPresenter->setPlotEnabled(isFit);
  m_outOptionsPresenter->setEditResultEnabled(isFit);
  m_outOptionsPresenter->setSaveEnabled(isFit);
}

void IndirectFitAnalysisTab::respondToSingleResolutionLoaded() {
  setModelFitFunction();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateGuessAvailability();
}

void IndirectFitAnalysisTab::respondToDataChanged() {
  updateDataReferences();
  m_fittingModel->removeFittingData();
  m_plotPresenter->updateAvailableSpectra();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateGuessAvailability();
  updateParameterEstimationData();
  updateResultOptions();
}

void IndirectFitAnalysisTab::respondToDataAdded() {
  updateDataReferences();
  m_plotPresenter->appendLastDataToSelection();
  updateParameterEstimationData();
}

void IndirectFitAnalysisTab::respondToDataRemoved() {
  updateDataReferences();
  m_plotPresenter->updateDataSelection();
  updateParameterEstimationData();
}

void IndirectFitAnalysisTab::respondToPlotSpectrumChanged() {
  auto const index = m_plotPresenter->getSelectedDomainIndex();
  m_fitPropertyBrowser->setCurrentDataset(index);
}

void IndirectFitAnalysisTab::respondToFwhmChanged(double) {
  updateFitBrowserParameterValues();
  m_plotPresenter->updateGuess();
}

void IndirectFitAnalysisTab::respondToBackgroundChanged(double value) {
  m_fitPropertyBrowser->setBackgroundA0(value);
  setModelFitFunction();
  m_plotPresenter->updateGuess();
}

void IndirectFitAnalysisTab::respondToFunctionChanged() {
  setModelFitFunction();
  m_fittingModel->removeFittingData();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateFit();
  emit functionChanged();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
