// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitAnalysisTab.h"
#include "ui_ConvFit.h"
#include "ui_IqtFit.h"
#include "ui_JumpFit.h"
#include "ui_MSDFit.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QString>
#include <QtCore>

#include <algorithm>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

WorkspaceGroup_sptr getADSGroupWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      workspaceName);
}

// void updateParameters(
//    IFunction_sptr function,
//    std::unordered_map<std::string, ParameterValue> const &parameters) {
//  if (!function)
//    return;
//  for (auto i = 0u; i < function->nParams(); ++i) {
//    auto const value = parameters.find(function->parameterName(i));
//    if (value != parameters.end()) {
//      function->setParameter(i, value->second.value);
//      if (value->second.error)
//        function->setError(i, *value->second.error);
//    }
//  }
//}

// void updateAttributes(
//    IFunction_sptr function, std::vector<std::string> const &attributeNames,
//    std::unordered_map<std::string, IFunction::Attribute> const &attributes) {
//  for (const auto &attributeName : attributeNames) {
//    auto const value = attributes.find(attributeName);
//    if (value != attributes.end())
//      function->setAttribute(attributeName, value->second);
//  }
//}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitAnalysisTab::IndirectFitAnalysisTab(IndirectFittingModel *model,
                                               QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_fittingModel(model) {}

void IndirectFitAnalysisTab::setup() {
  setupFitTab();
  updateResultOptions();

  connect(m_outOptionsPresenter.get(), SIGNAL(plotSpectra()), this,
          SLOT(plotSelectedSpectra()));

  connectDataPresenter();
  connectPlotPresenter();
  connectFitPropertyBrowser();
  connectSpectrumPresenter();
}

void IndirectFitAnalysisTab::connectDataPresenter() {
  connect(m_dataPresenter.get(),
          SIGNAL(startXChanged(double, DatasetIndex, WorkspaceIndex)), this,
          SLOT(tableStartXChanged(double, DatasetIndex, WorkspaceIndex)));
  connect(m_dataPresenter.get(),
          SIGNAL(endXChanged(double, DatasetIndex, WorkspaceIndex)), this,
          SLOT(tableEndXChanged(double, DatasetIndex, WorkspaceIndex)));
  connect(m_dataPresenter.get(),
          SIGNAL(excludeRegionChanged(const std::string &, DatasetIndex,
                                      WorkspaceIndex)),
          this,
          SLOT(tableExcludeChanged(const std::string &, DatasetIndex,
                                   WorkspaceIndex)));
  connect(m_dataPresenter.get(), SIGNAL(startXChanged(double)), this,
          SLOT(startXChanged(double)));
  connect(m_dataPresenter.get(), SIGNAL(endXChanged(double)), this,
          SLOT(endXChanged(double)));

  connect(m_dataPresenter.get(), SIGNAL(singleResolutionLoaded()), this,
          SLOT(respondToSingleResolutionLoaded()));
  connect(m_dataPresenter.get(), SIGNAL(dataChanged()), this,
          SLOT(respondToDataChanged()));
  connect(m_dataPresenter.get(), SIGNAL(singleDataViewSelected()), this,
          SLOT(respondToSingleDataViewSelected()));
  connect(m_dataPresenter.get(), SIGNAL(multipleDataViewSelected()), this,
          SLOT(respondToMultipleDataViewSelected()));
  connect(m_dataPresenter.get(), SIGNAL(dataAdded()), this,
          SLOT(respondToDataAdded()));
  connect(m_dataPresenter.get(), SIGNAL(dataRemoved()), this,
          SLOT(respondToDataRemoved()));
}

void IndirectFitAnalysisTab::connectPlotPresenter() {
  connect(m_plotPresenter.get(),
          SIGNAL(fitSingleSpectrum(DatasetIndex, WorkspaceIndex)), this,
          SLOT(singleFit(DatasetIndex, WorkspaceIndex)));
  connect(m_plotPresenter.get(),
          SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));
  connect(m_plotPresenter.get(), SIGNAL(startXChanged(double)), this,
          SLOT(setDataTableStartX(double)));
  connect(m_plotPresenter.get(), SIGNAL(endXChanged(double)), this,
          SLOT(setDataTableEndX(double)));
  connect(m_plotPresenter.get(), SIGNAL(selectedFitDataChanged(DatasetIndex)),
          this, SLOT(respondToSelectedFitDataChanged(DatasetIndex)));
  connect(m_plotPresenter.get(), SIGNAL(noFitDataSelected()), this,
          SLOT(respondToNoFitDataSelected()));
  connect(m_plotPresenter.get(), SIGNAL(plotSpectrumChanged(WorkspaceIndex)),
          this, SLOT(respondToPlotSpectrumChanged(WorkspaceIndex)));
  connect(m_plotPresenter.get(), SIGNAL(fwhmChanged(double)), this,
          SLOT(respondToFwhmChanged(double)));
  connect(m_plotPresenter.get(), SIGNAL(backgroundChanged(double)), this,
          SLOT(respondToBackgroundChanged(double)));
}

void IndirectFitAnalysisTab::connectSpectrumPresenter() {
  connect(m_spectrumPresenter.get(), SIGNAL(spectraChanged(DatasetIndex)), this,
          SLOT(respondToChangeOfSpectraRange(DatasetIndex)));
  connect(m_spectrumPresenter.get(), SIGNAL(maskChanged(const std::string &)),
          this, SLOT(setDataTableExclude(const std::string &)));
}

void IndirectFitAnalysisTab::connectFitPropertyBrowser() {
  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(respondToFunctionChanged()));
}

void IndirectFitAnalysisTab::setFitDataPresenter(
    std::unique_ptr<IndirectFitDataPresenter> presenter) {
  m_dataPresenter = std::move(presenter);
}

void IndirectFitAnalysisTab::setPlotView(IIndirectFitPlotView *view) {
  m_plotPresenter =
      std::make_unique<IndirectFitPlotPresenter>(m_fittingModel.get(), view);
}

void IndirectFitAnalysisTab::setSpectrumSelectionView(
    IndirectSpectrumSelectionView *view) {
  m_spectrumPresenter = std::make_unique<IndirectSpectrumSelectionPresenter>(
      m_fittingModel.get(), view);
}

void IndirectFitAnalysisTab::setOutputOptionsView(
    IIndirectFitOutputOptionsView *view) {
  m_outOptionsPresenter =
      std::make_unique<IndirectFitOutputOptionsPresenter>(view);
}

void IndirectFitAnalysisTab::setFitPropertyBrowser(
    IndirectFitPropertyBrowser *browser) {
  browser->init();
  m_fitPropertyBrowser = browser;
}

void IndirectFitAnalysisTab::loadSettings(const QSettings &settings) {
  m_dataPresenter->loadSettings(settings);
}

void IndirectFitAnalysisTab::setFileExtensionsByName(bool filter) {
  auto const tab = tabName();
  setSampleSuffixes(tab, filter);
  if (hasResolution())
    setResolutionSuffixes(tab, filter);
}

void IndirectFitAnalysisTab::setSampleSuffixes(std::string const &tab,
                                               bool filter) {
  QStringList const noSuffixes{""};
  setSampleWSSuffixes(filter ? getSampleWSSuffixes(tab) : noSuffixes);
  setSampleFBSuffixes(filter ? getSampleFBSuffixes(tab) : getExtensions(tab));
  m_dataPresenter->setMultiInputSampleWSSuffixes();
  m_dataPresenter->setMultiInputSampleFBSuffixes();
}

void IndirectFitAnalysisTab::setResolutionSuffixes(std::string const &tab,
                                                   bool filter) {
  QStringList const noSuffixes{""};
  setResolutionWSSuffixes(filter ? getResolutionWSSuffixes(tab) : noSuffixes);
  setResolutionFBSuffixes(filter ? getResolutionFBSuffixes(tab)
                                 : getExtensions(tab));
  m_dataPresenter->setMultiInputResolutionWSSuffixes();
  m_dataPresenter->setMultiInputResolutionFBSuffixes();
}

void IndirectFitAnalysisTab::setSampleWSSuffixes(const QStringList &suffices) {
  m_dataPresenter->setSampleWSSuffices(suffices);
}

void IndirectFitAnalysisTab::setSampleFBSuffixes(const QStringList &suffices) {
  m_dataPresenter->setSampleFBSuffices(suffices);
}

void IndirectFitAnalysisTab::setResolutionWSSuffixes(
    const QStringList &suffices) {
  m_dataPresenter->setResolutionWSSuffices(suffices);
}

void IndirectFitAnalysisTab::setResolutionFBSuffixes(
    const QStringList &suffices) {
  m_dataPresenter->setResolutionFBSuffices(suffices);
}

DatasetIndex IndirectFitAnalysisTab::getSelectedDataIndex() const {
  return m_plotPresenter->getSelectedDataIndex();
}

WorkspaceIndex IndirectFitAnalysisTab::getSelectedSpectrum() const {
  return m_plotPresenter->getSelectedSpectrum();
}

bool IndirectFitAnalysisTab::isRangeCurrentlySelected(
    DatasetIndex dataIndex, WorkspaceIndex spectrum) const {
  return m_plotPresenter->isCurrentlySelected(dataIndex, spectrum);
}

IndirectFittingModel *IndirectFitAnalysisTab::fittingModel() const {
  return m_fittingModel.get();
}

/**
 * @return  The fit type selected in the custom functions combo box, in the fit
 *          property browser.
 */
QString IndirectFitAnalysisTab::selectedFitType() const {
  return m_fitPropertyBrowser->selectedFitType();
}

/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t
IndirectFitAnalysisTab::numberOfCustomFunctions(const std::string &) const {
  return 0;
}

void IndirectFitAnalysisTab::setModelFitFunction() {
  m_fittingModel->setFitFunction(m_fitPropertyBrowser->getFittingFunction());
}

void IndirectFitAnalysisTab::setModelStartX(double startX) {
  const auto dataIndex = getSelectedDataIndex();
  if (m_fittingModel->numberOfWorkspaces() > dataIndex) {
    m_fittingModel->setStartX(startX, dataIndex, getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::setModelEndX(double endX) {
  const auto dataIndex = getSelectedDataIndex();
  if (m_fittingModel->numberOfWorkspaces() > dataIndex) {
    m_fittingModel->setEndX(endX, dataIndex, getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::setDataTableStartX(double startX) {
  m_dataPresenter->setStartX(startX, m_plotPresenter->getSelectedDataIndex());
}

void IndirectFitAnalysisTab::setDataTableEndX(double endX) {
  m_dataPresenter->setEndX(endX, m_plotPresenter->getSelectedDataIndex());
}

void IndirectFitAnalysisTab::setDataTableExclude(const std::string &exclude) {
  m_dataPresenter->setExclude(exclude, m_plotPresenter->getSelectedDataIndex(),
                              m_plotPresenter->getSelectedSpectrum());
}

void IndirectFitAnalysisTab::tableStartXChanged(double startX,
                                                DatasetIndex dataIndex,
                                                WorkspaceIndex spectrum) {
  if (isRangeCurrentlySelected(dataIndex, spectrum)) {
    m_plotPresenter->setStartX(startX);
    m_plotPresenter->updateGuess();
    m_fittingModel->setStartX(startX, m_plotPresenter->getSelectedDataIndex(),
                              m_plotPresenter->getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::tableEndXChanged(double endX,
                                              DatasetIndex dataIndex,
                                              WorkspaceIndex spectrum) {
  if (isRangeCurrentlySelected(dataIndex, spectrum)) {
    m_plotPresenter->setEndX(endX);
    m_plotPresenter->updateGuess();
    m_fittingModel->setEndX(endX, m_plotPresenter->getSelectedDataIndex(),
                            m_plotPresenter->getSelectedSpectrum());
  }
}

void IndirectFitAnalysisTab::tableExcludeChanged(const std::string & /*unused*/,
                                                 DatasetIndex dataIndex,
                                                 WorkspaceIndex spectrum) {
  if (isRangeCurrentlySelected(dataIndex, spectrum))
    m_spectrumPresenter->displayBinMask();
}

void IndirectFitAnalysisTab::startXChanged(double startX) {
  m_plotPresenter->setStartX(startX);
  m_plotPresenter->updateGuess();
  m_fittingModel->setStartX(startX, m_plotPresenter->getSelectedDataIndex());
}

void IndirectFitAnalysisTab::endXChanged(double endX) {
  m_plotPresenter->setEndX(endX);
  m_plotPresenter->updateGuess();
  m_fittingModel->setEndX(endX, m_plotPresenter->getSelectedDataIndex());
}

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveMembers If true, members are to be convolved.
 */
void IndirectFitAnalysisTab::setConvolveMembers(bool convolveMembers) {
  m_fitPropertyBrowser->setConvolveMembers(convolveMembers);
}

void IndirectFitAnalysisTab::updateFitOutput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(updateFitOutput(bool)));

  if (error) {
    m_fittingModel->cleanFailedRun(m_fittingAlgorithm);
    m_fittingAlgorithm.reset();
  } else
    m_fittingModel->addOutput(m_fittingAlgorithm);
}

void IndirectFitAnalysisTab::updateSingleFitOutput(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(updateSingleFitOutput(bool)));

  if (error) {
    m_fittingModel->cleanFailedSingleRun(m_fittingAlgorithm, DatasetIndex{0});
    m_fittingAlgorithm.reset();
  } else
    m_fittingModel->addSingleFitOutput(m_fittingAlgorithm, DatasetIndex{0});
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
    updateParameterValues();
    setModelFitFunction();
  }
  m_spectrumPresenter->enableView();
  m_plotPresenter->updatePlots();
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(fitAlgorithmComplete(bool)));
}

/**
 * Updates the parameter values and errors in the fit property browser.
 */
void IndirectFitAnalysisTab::updateParameterValues() {
  updateParameterValues(m_fittingModel->getParameterValues(
      getSelectedDataIndex(), getSelectedSpectrum()));
}

/**
 * Updates the parameter values and errors in the fit property browser.
 *
 * @param parameters  The parameter values to update the browser with.
 */
void IndirectFitAnalysisTab::updateParameterValues(
    const std::unordered_map<std::string, ParameterValue> &) {
  try {
    updateFitBrowserParameterValues();
  } catch (const std::out_of_range &) {
  } catch (const std::invalid_argument &) {
  }
}

void IndirectFitAnalysisTab::updateFitBrowserParameterValues() {
  if (m_fittingAlgorithm) {
    MantidQt::API::SignalBlocker blocker(m_fitPropertyBrowser);
    if (m_fittingModel->getFittingMode() == FittingMode::SEQUENTIAL) {
      auto const paramWsName =
          m_fittingAlgorithm->getPropertyValue("OutputParameterWorkspace");
      auto paramWs =
          AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
              paramWsName);
      m_fitPropertyBrowser->updateMultiDatasetParameters(*paramWs);
    } else {
      IFunction_sptr fun = m_fittingAlgorithm->getProperty("Function");
      if (fun->getNumberDomains() > 1)
        m_fitPropertyBrowser->updateMultiDatasetParameters(*fun);
      else
        m_fitPropertyBrowser->updateParameters(*fun);
    }
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
void IndirectFitAnalysisTab::plotSelectedSpectra(
    std::vector<SpectrumToPlot> const &spectra) {
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
void IndirectFitAnalysisTab::plotSpectrum(std::string const &workspaceName,
                                          std::size_t const &index) {
  IndirectTab::plotSpectrum(QString::fromStdString(workspaceName),
                            static_cast<int>(index));
}

/**
 * Gets the name used for the base of the result workspaces
 */
std::string IndirectFitAnalysisTab::getOutputBasename() const {
  return m_fittingModel->getOutputBasename();
}

/**
 * Gets the Result workspace from a fit
 */
WorkspaceGroup_sptr IndirectFitAnalysisTab::getResultWorkspace() const {
  return m_fittingModel->getResultWorkspace();
}

/**
 * Gets the names of the Fit Parameters
 */
std::vector<std::string> IndirectFitAnalysisTab::getFitParameterNames() const {
  return m_fittingModel->getFitParameterNames();
}

/**
 * Executes the single fit algorithm defined in this indirect fit analysis tab.
 */
void IndirectFitAnalysisTab::singleFit() {
  singleFit(getSelectedDataIndex(), getSelectedSpectrum());
}

void IndirectFitAnalysisTab::singleFit(DatasetIndex dataIndex,
                                       WorkspaceIndex spectrum) {
  if (validate()) {
    m_plotPresenter->setFitSingleSpectrumIsFitting(true);
    enableFitButtons(false);
    enableOutputOptions(false);
    runSingleFit(m_fittingModel->getSingleFit(dataIndex, spectrum));
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
  m_spectrumPresenter->validate(validator);

  const auto invalidFunction = m_fittingModel->isInvalidFunction();
  if (invalidFunction)
    validator.addErrorMessage(QString::fromStdString(*invalidFunction));
  if (m_fittingModel->numberOfWorkspaces() == DatasetIndex{0})
    validator.addErrorMessage(
        QString::fromStdString("No data has been selected for a fit."));

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
  auto const fitType = m_fitPropertyBrowser->selectedFitType();
  if (fitType == "Simultaneous") {
    m_fittingModel->setFittingMode(FittingMode::SIMULTANEOUS);
  } else {
    m_fittingModel->setFittingMode(FittingMode::SEQUENTIAL);
  }
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

  m_outOptionsPresenter->setPlotEnabled(
      enable && m_outOptionsPresenter->isSelectedGroupPlottable());
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
      m_dataPresenter->getDataForParameterEstimation(
          getEstimationDataSelector()));
}

/**
 * Sets the visiblity of the output options Edit Result button
 * @param visible :: true to make the edit result button visible
 */
void IndirectFitAnalysisTab::setEditResultVisible(bool visible) {
  m_outOptionsPresenter->setEditResultVisible(visible);
}

void IndirectFitAnalysisTab::setAlgorithmProperties(
    IAlgorithm_sptr fitAlgorithm) const {
  fitAlgorithm->setProperty("Minimizer", m_fitPropertyBrowser->minimizer(true));
  fitAlgorithm->setProperty("MaxIterations",
                            m_fitPropertyBrowser->maxIterations());
  fitAlgorithm->setProperty("ConvolveMembers",
                            m_fitPropertyBrowser->convolveMembers());
  fitAlgorithm->setProperty("PeakRadius",
                            m_fitPropertyBrowser->getPeakRadius());
  fitAlgorithm->setProperty("CostFunction",
                            m_fitPropertyBrowser->costFunction());
  fitAlgorithm->setProperty("IgnoreInvalidData",
                            m_fitPropertyBrowser->ignoreInvalidData());

  if (m_fitPropertyBrowser->isHistogramFit())
    fitAlgorithm->setProperty("EvaluationType", "Histogram");
}

/*
 * Runs the specified fit algorithm and calls the algorithmComplete
 * method of this fit analysis tab once completed.
 *
 * @param fitAlgorithm      The fit algorithm to run.
 */
void IndirectFitAnalysisTab::runFitAlgorithm(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(updateFitOutput(bool)));
  setupFit(fitAlgorithm);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectFitAnalysisTab::runSingleFit(IAlgorithm_sptr fitAlgorithm) {
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(updateSingleFitOutput(bool)));
  setupFit(fitAlgorithm);
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectFitAnalysisTab::setupFit(IAlgorithm_sptr fitAlgorithm) {
  setAlgorithmProperties(fitAlgorithm);
  m_fittingAlgorithm = fitAlgorithm;
  m_spectrumPresenter->disableView();
  m_batchAlgoRunner->addAlgorithm(fitAlgorithm);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(fitAlgorithmComplete(bool)));
}

QStringList IndirectFitAnalysisTab::getDatasetNames() const {
  QStringList datasetNames;
  auto const numberWorkspaces = m_fittingModel->numberOfWorkspaces();
  for (DatasetIndex i{0}; i < numberWorkspaces; ++i) {
    auto const name =
        QString::fromStdString(m_fittingModel->getWorkspace(i)->getName());
    auto const numberSpectra = m_fittingModel->getNumberOfSpectra(i);
    for (SpectrumRowIndex j{0}; j < numberSpectra; ++j) {
      datasetNames << name + " (" + QString::number(j.value) + ")";
    }
  }
  return datasetNames;
}

void IndirectFitAnalysisTab::updateDataReferences() {
  m_fitPropertyBrowser->updateFunctionBrowserData(
      m_fittingModel->getNumberOfDomains(), getDatasetNames());
  m_fittingModel->setFitFunction(m_fitPropertyBrowser->getFittingFunction());
}

/**
 * Updates whether the options for plotting and saving fit results are
 * enabled/disabled.
 */
void IndirectFitAnalysisTab::updateResultOptions() {
  const bool isFit = m_fittingModel->isPreviouslyFit(getSelectedDataIndex(),
                                                     getSelectedSpectrum());
  if (isFit)
    m_outOptionsPresenter->setResultWorkspace(getResultWorkspace());
  m_outOptionsPresenter->setPlotEnabled(isFit);
  m_outOptionsPresenter->setEditResultEnabled(isFit);
  m_outOptionsPresenter->setSaveEnabled(isFit);
}

void IndirectFitAnalysisTab::respondToChangeOfSpectraRange(DatasetIndex i) {
  m_plotPresenter->updateSelectedDataName();
  m_plotPresenter->updateAvailableSpectra();
  m_dataPresenter->updateSpectraInTable(i);
  m_fitPropertyBrowser->updateFunctionBrowserData(
      m_fittingModel->getNumberOfDomains(), getDatasetNames());
  setModelFitFunction();
  updateParameterEstimationData();
}

void IndirectFitAnalysisTab::respondToSingleResolutionLoaded() {
  setModelFitFunction();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateGuess();
}

void IndirectFitAnalysisTab::respondToDataChanged() {
  updateResultOptions();
  updateDataReferences();
  m_spectrumPresenter->updateSpectra();
  m_plotPresenter->updateAvailableSpectra();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateGuess();
  updateParameterEstimationData();
}

void IndirectFitAnalysisTab::respondToSingleDataViewSelected() {
  m_spectrumPresenter->setActiveIndexToZero();
  m_plotPresenter->hideMultipleDataSelection();
}

void IndirectFitAnalysisTab::respondToMultipleDataViewSelected() {
  m_plotPresenter->showMultipleDataSelection();
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

void IndirectFitAnalysisTab::respondToSelectedFitDataChanged(DatasetIndex i) {
  m_spectrumPresenter->setActiveModelIndex(i);
  updateParameterValues();
}

void IndirectFitAnalysisTab::respondToNoFitDataSelected() {
  m_spectrumPresenter->disableView();
}

void IndirectFitAnalysisTab::respondToPlotSpectrumChanged(WorkspaceIndex) {
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
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateGuess();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
