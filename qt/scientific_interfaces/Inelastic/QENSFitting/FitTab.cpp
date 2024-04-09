// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitTab.h"
#include "Common/SettingsHelper.h"
#include "MantidQtWidgets/Common/InterfaceUtils.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

#include "FitPlotView.h"
#include "FitTabConstants.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/PropertyHandler.h"

#include <QSignalBlocker>
#include <QString>
#include <QtCore>

#include <algorithm>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
/// Logger
Mantid::Kernel::Logger g_log("QENS Fitting");
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

/**
 * @param functionName  The name of the function.
 * @param compositeFunction  The function to search within.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t FitTab::getNumberOfSpecificFunctionContained(const std::string &functionName,
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

FitTab::FitTab(std::string const &tabName, bool const hasResolution, QWidget *parent)
    : IndirectTab(parent), m_uiForm(new Ui::FitTab), m_tabName(tabName), m_hasResolution(hasResolution) {
  m_uiForm->setupUi(parent);
}

void FitTab::setup() {
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runTab()));
  updateResultOptions();

  connectFitPropertyBrowser();
}

void FitTab::connectFitPropertyBrowser() {
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this, SLOT(respondToFunctionChanged()));
}

void FitTab::subscribeFitBrowserToDataPresenter() {
  m_dataPresenter->subscribeFitPropertyBrowser(m_fitPropertyBrowser);
}

void FitTab::setupOutputOptionsPresenter(bool const editResults) {
  auto model = std::make_unique<FitOutputOptionsModel>();
  m_outOptionsPresenter =
      std::make_unique<FitOutputOptionsPresenter>(this, m_uiForm->ovOutputOptionsView, std::move(model));
  m_outOptionsPresenter->setEditResultVisible(editResults);
}

void FitTab::setupPlotView(std::optional<std::pair<double, double>> const &xPlotBounds) {
  auto model = std::make_unique<FitPlotModel>();
  m_plotPresenter = std::make_unique<FitPlotPresenter>(this, m_uiForm->dockArea->m_fitPlotView, std::move(model));
  m_plotPresenter->setFittingData(m_dataPresenter->getFittingData());
  m_plotPresenter->setFitOutput(m_fittingModel->getFitOutput());
  if (xPlotBounds) {
    m_plotPresenter->setXBounds(*xPlotBounds);
  }
  m_plotPresenter->updatePlots();
}

void FitTab::setRunIsRunning(bool running) { m_uiForm->pbRun->setText(running ? "Running..." : "Run"); }

void FitTab::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

void FitTab::setFileExtensionsByName(bool filter) {
  auto const tab = getTabName();
  setSampleSuffixes(tab, filter);
  if (hasResolution())
    setResolutionSuffixes(tab, filter);
}

void FitTab::setSampleSuffixes(std::string const &tab, bool filter) {
  QStringList const noSuffixes{""};
  setSampleWSSuffixes(filter ? InterfaceUtils::getSampleWSSuffixes(tab) : noSuffixes);
  setSampleFBSuffixes(filter ? InterfaceUtils::getSampleFBSuffixes(tab) : InterfaceUtils::getExtensions(tab));
}

void FitTab::setResolutionSuffixes(std::string const &tab, bool filter) {
  QStringList const noSuffixes{""};
  setResolutionWSSuffixes(filter ? InterfaceUtils::getResolutionWSSuffixes(tab) : noSuffixes);
  setResolutionFBSuffixes(filter ? InterfaceUtils::getResolutionFBSuffixes(tab) : InterfaceUtils::getExtensions(tab));
}

void FitTab::setSampleWSSuffixes(const QStringList &suffices) { m_dataPresenter->setSampleWSSuffices(suffices); }

void FitTab::setSampleFBSuffixes(const QStringList &suffices) { m_dataPresenter->setSampleFBSuffices(suffices); }

void FitTab::setResolutionWSSuffixes(const QStringList &suffices) {
  m_dataPresenter->setResolutionWSSuffices(suffices);
}

void FitTab::setResolutionFBSuffixes(const QStringList &suffices) {
  m_dataPresenter->setResolutionFBSuffices(suffices);
}

WorkspaceID FitTab::getSelectedDataIndex() const { return m_plotPresenter->getActiveWorkspaceID(); }

WorkspaceIndex FitTab::getSelectedSpectrum() const { return m_plotPresenter->getActiveWorkspaceIndex(); }

bool FitTab::isRangeCurrentlySelected(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return m_plotPresenter->isCurrentlySelected(workspaceID, spectrum);
}

FittingModel *FitTab::getFittingModel() const { return m_fittingModel.get(); }

/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t FitTab::getNumberOfCustomFunctions(const std::string &functionName) const {
  auto fittingFunction = m_fittingModel->getFitFunction();
  if (fittingFunction && fittingFunction->nFunctions() > 0)
    return getNumberOfSpecificFunctionContained(functionName, fittingFunction->getFunction(0).get());
  else
    return 0;
}

void FitTab::setModelFitFunction() {
  auto func = m_fitPropertyBrowser->getFitFunction();
  m_plotPresenter->setFitFunction(func);
  m_fittingModel->setFitFunction(func);
}

void FitTab::setModelStartX(double startX) {
  const auto dataIndex = getSelectedDataIndex();
  m_dataPresenter->setStartX(startX, dataIndex, getSelectedSpectrum());
}

void FitTab::setModelEndX(double endX) {
  const auto dataIndex = getSelectedDataIndex();
  m_dataPresenter->setStartX(endX, dataIndex, getSelectedSpectrum());
}

void FitTab::handleTableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (isRangeCurrentlySelected(workspaceID, spectrum)) {
    m_plotPresenter->setStartX(startX);
    m_plotPresenter->updateGuess();
  }
}

void FitTab::handleTableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (isRangeCurrentlySelected(workspaceID, spectrum)) {
    m_plotPresenter->setEndX(endX);
    m_plotPresenter->updateGuess();
  }
}

void FitTab::handleStartXChanged(double startX) {
  m_plotPresenter->setStartX(startX);
  m_dataPresenter->setStartX(startX, m_plotPresenter->getActiveWorkspaceID());
  updateParameterEstimationData();
  m_plotPresenter->updateGuess();
  m_dataPresenter->updateTableFromModel();
}

void FitTab::handleEndXChanged(double endX) {
  m_plotPresenter->setEndX(endX);
  m_dataPresenter->setEndX(endX, m_plotPresenter->getActiveWorkspaceID());
  updateParameterEstimationData();
  m_plotPresenter->updateGuess();
  m_dataPresenter->updateTableFromModel();
}

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveMembers If true, members are to be convolved.
 */
void FitTab::setConvolveMembers(bool convolveMembers) {
  m_fitPropertyBrowser->setConvolveMembers(convolveMembers);
  // if convolve members is on, output members should also be on
  if (convolveMembers)
    m_fitPropertyBrowser->setOutputCompositeMembers(true);
}

void FitTab::updateFitOutput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(updateFitOutput(bool)));

  if (error) {
    m_fittingModel->cleanFailedRun(m_fittingAlgorithm);
    m_fittingAlgorithm.reset();
  } else {
    m_fittingModel->addOutput(m_fittingAlgorithm);
  }
}

void FitTab::updateSingleFitOutput(bool error) {
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
void FitTab::fitAlgorithmComplete(bool error) {
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
void FitTab::updateParameterValues() {
  updateParameterValues(m_fittingModel->getParameterValues(getSelectedDataIndex(), getSelectedSpectrum()));
}

/**
 * Updates the parameter values and errors in the fit property browser.
 *
 * @param parameters  The parameter values to update the browser with.
 */
void FitTab::updateParameterValues(const std::unordered_map<std::string, ParameterValue> &params) {
  try {
    updateFitBrowserParameterValues(params);
  } catch (const std::out_of_range &) {
    g_log.warning("Warning issue updating parameter values in fit property browser");
  } catch (const std::invalid_argument &) {
    g_log.warning("Warning issue updating parameter values in fit property browser");
  }
}

void FitTab::updateFitBrowserParameterValues(const std::unordered_map<std::string, ParameterValue> &params) {
  IFunction_sptr fun = m_fittingModel->getFitFunction();
  if (fun) {
    for (auto const &pair : params) {
      fun->setParameter(pair.first, pair.second.value);
    }
    if (fun->getNumberDomains() > 1) {
      m_fitPropertyBrowser->updateMultiDatasetParameters(*fun);
    } else {
      m_fitPropertyBrowser->updateParameters(*fun);
    }
  }
}

void FitTab::updateFitBrowserParameterValuesFromAlg() {
  try {
    updateFitBrowserParameterValues();
    if (m_fittingAlgorithm) {
      QSignalBlocker blocker(m_fitPropertyBrowser);
      if (m_fittingModel->getFittingMode() == FittingMode::SEQUENTIAL) {
        auto const paramWsName = m_fittingAlgorithm->getPropertyValue("OutputParameterWorkspace");
        auto paramWs = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(paramWsName);
        auto rowCount = static_cast<int>(paramWs->rowCount());
        if (rowCount == static_cast<int>(m_dataPresenter->getNumberOfDomains()))
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
void FitTab::updateFitStatus() {

  if (m_fittingModel->getFittingMode() == FittingMode::SIMULTANEOUS) {
    std::string fit_status = m_fittingAlgorithm->getProperty("OutputStatus");
    double chi2 = m_fittingAlgorithm->getProperty("OutputChiSquared");
    const std::vector<std::string> status(m_dataPresenter->getNumberOfDomains(), fit_status);
    const std::vector<double> chiSquared(m_dataPresenter->getNumberOfDomains(), chi2);
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
void FitTab::handlePlotSelectedSpectra() {
  enableFitButtons(false);
  plotSelectedSpectra(m_outOptionsPresenter->getSpectraToPlot());
  enableFitButtons(true);
  m_outOptionsPresenter->setPlotting(false);
}

/**
 * Plots the spectra corresponding to the selected parameters
 * @param spectra :: a vector of spectra to plot from a group workspace
 */
void FitTab::plotSelectedSpectra(std::vector<SpectrumToPlot> const &spectra) {
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
void FitTab::plotSpectrum(std::string const &workspaceName, std::size_t const &index) {
  m_plotter->plotSpectra(workspaceName, std::to_string(index), SettingsHelper::externalPlotErrorBars());
}

/**
 * Gets the name used for the base of the result workspaces
 */
std::string FitTab::getOutputBasename() const { return m_fittingModel->getOutputBasename(); }

/**
 * Gets the Result workspace from a fit
 */
WorkspaceGroup_sptr FitTab::getResultWorkspace() const { return m_fittingModel->getResultWorkspace(); }

/**
 * Gets the names of the Fit Parameters
 */
std::vector<std::string> FitTab::getFitParameterNames() const { return m_fittingModel->getFitParameterNames(); }

/**
 * Executes the single fit algorithm defined in this indirect fit analysis tab.
 */
void FitTab::singleFit() { handleSingleFitClicked(getSelectedDataIndex(), getSelectedSpectrum()); }

void FitTab::handleSingleFitClicked(WorkspaceID workspaceID, WorkspaceIndex spectrum) {
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
void FitTab::executeFit() {
  if (validate()) {
    setRunIsRunning(true);
    enableFitButtons(false);
    enableOutputOptions(false);
    runFitAlgorithm(m_fittingModel->getFittingAlgorithm(m_fittingModel->getFittingMode()));
  }
}

bool FitTab::validate() {
  UserInputValidator validator;
  m_dataPresenter->validate(validator);

  const auto invalidFunction = m_fittingModel->isInvalidFunction();
  if (invalidFunction)
    validator.addErrorMessage(QString::fromStdString(*invalidFunction));

  const auto error = validator.generateErrorMessage();
  emit showMessageBox(error);
  return error.isEmpty();
}

/**
 * Called when the 'Run' button is called in the IndirectTab.
 */
void FitTab::run() {
  setRunIsRunning(true);
  enableFitButtons(false);
  enableOutputOptions(false);
  m_fittingModel->setFittingMode(m_fitPropertyBrowser->getFittingMode());
  runFitAlgorithm(m_fittingModel->getFittingAlgorithm(m_fittingModel->getFittingMode()));
}

/**
 * Enables or disables the 'Run', 'Fit Single Spectrum' and other related
 * buttons
 * @param enable :: true to enable buttons
 */
void FitTab::enableFitButtons(bool enable) {
  setRunEnabled(enable);
  m_plotPresenter->setFitSingleSpectrumEnabled(enable);
  m_fitPropertyBrowser->setFitEnabled(enable);
}

/**
 * Enables or disables the output options. It also sets the current result and
 * PDF workspaces to be plotted
 * @param enable :: true to enable buttons
 */
void FitTab::enableOutputOptions(bool enable) {
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
void FitTab::setPDFWorkspace(std::string const &workspaceName) {
  auto const fabMinimizer = m_fitPropertyBrowser->minimizer() == "FABADA";
  auto const enablePDFOptions = WorkspaceUtils::doesExistInADS(workspaceName) && fabMinimizer;

  if (enablePDFOptions) {
    m_outOptionsPresenter->setPDFWorkspace(WorkspaceUtils::getADSWorkspace<WorkspaceGroup>(workspaceName));
    m_outOptionsPresenter->setPlotWorkspaces();
  } else
    m_outOptionsPresenter->removePDFWorkspace();
  m_outOptionsPresenter->setMultiWorkspaceOptionsVisible(enablePDFOptions);
}

void FitTab::updateParameterEstimationData() {
  m_fitPropertyBrowser->updateParameterEstimationData(
      m_dataPresenter->getDataForParameterEstimation(m_fitPropertyBrowser->getEstimationDataSelector()));
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
void FitTab::setEditResultVisible(bool visible) { m_outOptionsPresenter->setEditResultVisible(visible); }

void FitTab::setAlgorithmProperties(const IAlgorithm_sptr &fitAlgorithm) const {
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
void FitTab::runFitAlgorithm(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(updateFitOutput(bool)));
  setupFit(std::move(fitAlgorithm));
  m_batchAlgoRunner->executeBatchAsync();
}

void FitTab::runSingleFit(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(updateSingleFitOutput(bool)));
  setupFit(std::move(fitAlgorithm));
  m_batchAlgoRunner->executeBatchAsync();
}

void FitTab::setupFit(IAlgorithm_sptr fitAlgorithm) {
  setAlgorithmProperties(fitAlgorithm);
  m_fittingAlgorithm = fitAlgorithm;
  m_batchAlgoRunner->addAlgorithm(fitAlgorithm);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(fitAlgorithmComplete(bool)));
}

QList<FunctionModelDataset> FitTab::getDatasets() const {
  QList<FunctionModelDataset> datasets;

  for (auto i = 0u; i < m_dataPresenter->getNumberOfWorkspaces().value; ++i) {
    WorkspaceID workspaceID{i};

    auto const name = m_fittingModel->getWorkspace(workspaceID)->getName();
    datasets.append(FunctionModelDataset(QString::fromStdString(name), m_dataPresenter->getSpectra(workspaceID)));
  }
  return datasets;
}

void FitTab::updateDataReferences() {
  m_fitPropertyBrowser->updateFunctionBrowserData(static_cast<int>(m_dataPresenter->getNumberOfDomains()),
                                                  getDatasets(), m_dataPresenter->getQValuesForData(),
                                                  m_dataPresenter->getResolutionsForFit());
  setModelFitFunction();
}

/**
 * Updates whether the options for plotting and saving fit results are
 * enabled/disabled.
 */
void FitTab::updateResultOptions() {
  const bool isFit = m_fittingModel->isPreviouslyFit(getSelectedDataIndex(), getSelectedSpectrum());
  if (isFit)
    m_outOptionsPresenter->setResultWorkspace(getResultWorkspace());
  m_outOptionsPresenter->setPlotEnabled(isFit);
  m_outOptionsPresenter->setEditResultEnabled(isFit);
  m_outOptionsPresenter->setSaveEnabled(isFit);
}

void FitTab::handleDataChanged() {
  updateDataReferences();
  m_fittingModel->removeFittingData();
  m_plotPresenter->updateAvailableSpectra();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateGuessAvailability();
  updateParameterEstimationData();
  updateResultOptions();
}

void FitTab::handleDataAdded(IAddWorkspaceDialog const *dialog) {
  if (m_dataPresenter->addWorkspaceFromDialog(dialog)) {
    m_fittingModel->addDefaultParameters();
  }
  updateDataReferences();
  m_plotPresenter->appendLastDataToSelection(m_dataPresenter->createDisplayNames());
  updateParameterEstimationData();
}

void FitTab::handleDataRemoved() {
  m_fittingModel->removeDefaultParameters();
  updateDataReferences();
  m_plotPresenter->updateDataSelection(m_dataPresenter->createDisplayNames());
  updateParameterEstimationData();
}

void FitTab::handlePlotSpectrumChanged() {
  auto const index = m_plotPresenter->getSelectedDomainIndex();
  m_fitPropertyBrowser->setCurrentDataset(index);
}

void FitTab::handleFwhmChanged(double fwhm) {
  m_fittingModel->setFWHM(fwhm, m_plotPresenter->getActiveWorkspaceID());
  updateFitBrowserParameterValues();
  m_plotPresenter->updateGuess();
}

void FitTab::handleBackgroundChanged(double value) {
  m_fittingModel->setBackground(value, m_plotPresenter->getActiveWorkspaceID());
  m_fitPropertyBrowser->setBackgroundA0(value);
  setModelFitFunction();
  m_plotPresenter->updateGuess();
}

void FitTab::respondToFunctionChanged() {
  setModelFitFunction();
  m_fittingModel->removeFittingData();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateFit();
  m_fittingModel->setFitTypeString(getFitTypeString());
}

std::string FitTab::getFitTypeString() const {
  auto const multiDomainFunction = m_fittingModel->getFitFunction();
  if (!multiDomainFunction || multiDomainFunction->nFunctions() == 0) {
    return "NoCurrentFunction";
  }

  std::string fitType{""};
  for (auto const &fitFunctionName : FUNCTION_STRINGS) {
    auto occurances = getNumberOfCustomFunctions(fitFunctionName.first);
    if (occurances > 0) {
      fitType += std::to_string(occurances) + fitFunctionName.second;
    }
  }

  if (getNumberOfCustomFunctions("DeltaFunction") > 0) {
    fitType += "Delta";
  }

  return fitType;
}

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
