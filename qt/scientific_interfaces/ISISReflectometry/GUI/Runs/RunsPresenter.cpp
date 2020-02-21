// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsPresenter.h"
#include "CatalogRunNotifier.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "GUI/Common/IMessageHandler.h"
#include "GUI/Common/IPythonRunner.h"
#include "GUI/RunsTable/RunsTablePresenter.h"
#include "IRunsView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/ProgressPresenter.h"
#include "QtCatalogSearcher.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#include "Reduction/ValidateRow.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** Constructor
 * @param mainView :: [input] The view we're managing
 * @param progressableView :: [input] The view reporting progress
 * @param makeRunsTablePresenter :: A generator for the child presenters.
 * @param thetaTolerance The tolerance used to determine if two runs should be
 * summed in a reduction.
 * @param instruments The names of the instruments to show as options for the
 * search.
 * @param messageHandler :: A handler to pass messages to the user
 */
RunsPresenter::RunsPresenter(
    IRunsView *mainView, ProgressableView *progressableView,
    const RunsTablePresenterFactory &makeRunsTablePresenter,
    double thetaTolerance, std::vector<std::string> const &instruments,
    IMessageHandler *messageHandler)
    : m_runNotifier(std::make_unique<CatalogRunNotifier>(mainView)),
      m_searcher(std::make_unique<QtCatalogSearcher>(mainView)),
      m_view(mainView), m_progressView(progressableView),
      m_mainPresenter(nullptr), m_messageHandler(messageHandler),
      m_instruments(instruments), m_thetaTolerance(thetaTolerance) {

  assert(m_view != nullptr);
  m_view->subscribe(this);
  m_tablePresenter = makeRunsTablePresenter(m_view->table());
  m_tablePresenter->acceptMainPresenter(this);
  m_runNotifier->subscribe(this);
  m_searcher->subscribe(this);

  updateViewWhenMonitorStopped();
}

RunsPresenter::~RunsPresenter() {
  if (m_monitorAlg)
    stopObserving(m_monitorAlg);
}

/** Accept a main presenter
 * @param mainPresenter :: [input] A main presenter
 */
void RunsPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

void RunsPresenter::initInstrumentList() {
  m_view->setInstrumentList(m_instruments);
}

RunsTable const &RunsPresenter::runsTable() const {
  return tablePresenter()->runsTable();
}

RunsTable &RunsPresenter::mutableRunsTable() {
  return tablePresenter()->mutableRunsTable();
}

/**
   Used by the view to tell the presenter something has changed
*/

void RunsPresenter::notifySearch() {
  m_searcher->reset();
  updateWidgetEnabledState();
  search(ISearcher::SearchType::MANUAL);
}

void RunsPresenter::notifyCheckForNewRuns() { checkForNewRuns(); }

void RunsPresenter::notifySearchComplete() {
  if (!isAutoreducing())
    m_view->resizeSearchResultsColumnsToContents();

  updateWidgetEnabledState();

  if (isAutoreducing())
    autoreduceNewRuns();
}

void RunsPresenter::notifySearchFailed() {
  if (isAutoreducing()) {
    notifyPauseAutoreductionRequested();
  }
}

void RunsPresenter::notifyTransfer() {
  transfer(m_view->getSelectedSearchRows(), TransferMatch::Any);
  notifyRowStateChanged();
}

void RunsPresenter::notifyChangeInstrumentRequested() {
  auto const instrumentName = m_view->getSearchInstrument();
  m_mainPresenter->notifyChangeInstrumentRequested(instrumentName);
}

void RunsPresenter::notifyChangeInstrumentRequested(
    std::string const &instrumentName) {
  m_mainPresenter->notifyChangeInstrumentRequested(instrumentName);
}

void RunsPresenter::notifyResumeReductionRequested() {
  m_mainPresenter->notifyResumeReductionRequested();
}

void RunsPresenter::notifyPauseReductionRequested() {
  m_mainPresenter->notifyPauseReductionRequested();
}

void RunsPresenter::notifyResumeAutoreductionRequested() {
  m_mainPresenter->notifyResumeAutoreductionRequested();
}

void RunsPresenter::notifyPauseAutoreductionRequested() {
  m_mainPresenter->notifyPauseAutoreductionRequested();
}

void RunsPresenter::notifyStartMonitor() { startMonitor(); }

void RunsPresenter::notifyStopMonitor() { stopMonitor(); }

void RunsPresenter::notifyStartMonitorComplete() { startMonitorComplete(); }

void RunsPresenter::notifyRowStateChanged() {
  tablePresenter()->notifyRowStateChanged();
}

void RunsPresenter::notifyRowStateChanged(boost::optional<Item const &> item) {
  tablePresenter()->notifyRowStateChanged(item);
}

void RunsPresenter::notifyRowOutputsChanged() {
  tablePresenter()->notifyRowOutputsChanged();
}

void RunsPresenter::notifyRowOutputsChanged(
    boost::optional<Item const &> item) {
  tablePresenter()->notifyRowOutputsChanged(item);
}

void RunsPresenter::notifyReductionResumed() {
  updateWidgetEnabledState();
  tablePresenter()->notifyReductionResumed();
  notifyRowStateChanged();
}

void RunsPresenter::notifyReductionPaused() {
  updateWidgetEnabledState();
  tablePresenter()->notifyReductionPaused();
}

/** Resume autoreduction. Clears any existing table data first and then
 * starts a search to check if there are new runs.
 */
bool RunsPresenter::resumeAutoreduction() {
  auto const searchString = m_view->getSearchString();
  auto const instrument = m_view->getSearchInstrument();

  if (searchString == "") {
    m_messageHandler->giveUserInfo("Search field is empty", "Search Issue");
    return false;
  }

  // Check if starting an autoreduction with new settings, reset the previous
  // search results and clear the main table
  if (m_searcher->searchSettingsChanged(searchString, instrument,
                                        ISearcher::SearchType::AUTO)) {
    // If there are unsaved changes, ask the user first
    if (isOverwritingTablePrevented()) {
      return false;
    }
    m_searcher->reset();
    tablePresenter()->notifyRemoveAllRowsAndGroupsRequested();
  }

  checkForNewRuns();
  return true;
}

void RunsPresenter::notifyAutoreductionResumed() {
  updateWidgetEnabledState();
  tablePresenter()->notifyAutoreductionResumed();
  m_progressView->setAsEndlessIndicator();
}

void RunsPresenter::notifyAutoreductionPaused() {
  m_runNotifier->stopPolling();
  m_progressView->setAsPercentageIndicator();
  updateWidgetEnabledState();
  tablePresenter()->notifyAutoreductionPaused();
}

void RunsPresenter::notifyAnyBatchReductionResumed() {
  updateWidgetEnabledState();
  tablePresenter()->notifyAnyBatchReductionResumed();
}

void RunsPresenter::notifyAnyBatchReductionPaused() {
  updateWidgetEnabledState();
  tablePresenter()->notifyAnyBatchReductionPaused();
}

void RunsPresenter::notifyAnyBatchAutoreductionResumed() {
  updateWidgetEnabledState();
  tablePresenter()->notifyAnyBatchAutoreductionResumed();
}

void RunsPresenter::notifyAnyBatchAutoreductionPaused() {
  updateWidgetEnabledState();
  tablePresenter()->notifyAnyBatchAutoreductionPaused();
}

void RunsPresenter::autoreductionCompleted() {
  // Return to polling state
  m_runNotifier->startPolling();
  updateWidgetEnabledState();
}

void RunsPresenter::notifyInstrumentChanged(std::string const &instrumentName) {
  m_searcher->reset();
  m_view->setSearchInstrument(instrumentName);
  tablePresenter()->notifyInstrumentChanged(instrumentName);
}

void RunsPresenter::notifyTableChanged() { m_mainPresenter->setBatchUnsaved(); }

void RunsPresenter::settingsChanged() { tablePresenter()->settingsChanged(); }

/** Searches for runs that can be used
 * @return : true if the search algorithm was started successfully, false if
 * there was a problem */
bool RunsPresenter::search(ISearcher::SearchType searchType) {
  auto const searchString = m_view->getSearchString();
  // Don't bother searching if they're not searching for anything
  if (searchString.empty())
    return false;

  if (!m_searcher->startSearchAsync(searchString, m_view->getSearchInstrument(),
                                    searchType)) {
    m_messageHandler->giveUserCritical("Catalog login failed", "Error");
    return false;
  }

  return true;
}

/** Start a single autoreduction process. Called periodially to add and process
 *  any new runs in the table.
 */
void RunsPresenter::checkForNewRuns() {
  // Stop notifications during processing
  m_runNotifier->stopPolling();

  // Initially we just need to start an ICat search and the reduction will be
  // run when the search completes
  search(ISearcher::SearchType::AUTO);
}

/** Run an autoreduction process based on the latest search results
 */
void RunsPresenter::autoreduceNewRuns() {

  auto rowsToTransfer = m_view->getAllSearchRows();

  if (rowsToTransfer.size() > 0)
    transfer(rowsToTransfer, TransferMatch::Strict);

  m_mainPresenter->notifyResumeReductionRequested();
}

bool RunsPresenter::isProcessing() const {
  return m_mainPresenter->isProcessing();
}

bool RunsPresenter::isAutoreducing() const {
  return m_mainPresenter->isAutoreducing();
}

bool RunsPresenter::isAnyBatchProcessing() const {
  return m_mainPresenter->isAnyBatchProcessing();
}

bool RunsPresenter::isAnyBatchAutoreducing() const {
  return m_mainPresenter->isAnyBatchAutoreducing();
}

bool RunsPresenter::isOverwritingTablePrevented() const {
  return m_mainPresenter->isBatchUnsaved() && isOverwriteBatchPrevented();
}

bool RunsPresenter::isOverwriteBatchPrevented() const {
  return m_mainPresenter->isWarnDiscardChangesChecked() &&
         !m_messageHandler->askUserDiscardChanges();
}

bool RunsPresenter::searchInProgress() const {
  return m_searcher->searchInProgress();
}

int RunsPresenter::percentComplete() const {
  if (!m_mainPresenter)
    return 0;
  return m_mainPresenter->percentComplete();
}

void RunsPresenter::setRoundPrecision(int &precision) {
  m_tablePresenter->setTablePrecision(precision);
}

void RunsPresenter::resetRoundPrecision() {
  m_tablePresenter->resetTablePrecision();
}

IRunsTablePresenter *RunsPresenter::tablePresenter() const {
  return m_tablePresenter.get();
}

/** Check that the given rows are valid for a transfer and warn the user if not
 * @param rowsToTransfer : a set of row indices to transfer
 * @return : true if valid, false if not
 */
bool RunsPresenter::validateRowsToTransfer(
    const std::set<int> &rowsToTransfer) {
  // Check that we have something to transfer
  if (rowsToTransfer.size() == 0) {
    m_messageHandler->giveUserCritical(
        "Please select at least one run to transfer.", "No runs selected");
    return false;
  }
  return true;
}

/** Set up the progress bar
 */
ProgressPresenter
RunsPresenter::setupProgressBar(const std::set<int> &rowsToTransfer) {

  auto start = double(0.0);
  auto end = static_cast<double>(rowsToTransfer.size());
  auto nsteps = static_cast<int64_t>(rowsToTransfer.size());
  auto progress = ProgressPresenter(start, end, nsteps, this->m_progressView);

  if (isAutoreducing())
    progress.setAsEndlessIndicator();
  else
    progress.setAsPercentageIndicator();

  return progress;
}

/** Transfers the selected runs in the search results to the processing table
 * @param rowsToTransfer : a set of row indices in the search results to
 * transfer
 * @param matchType : an enum specifying how strictly to match runs against
 * the transfer criteria
 * @return : The runs to transfer as a vector of maps
 */
void RunsPresenter::transfer(const std::set<int> &rowsToTransfer,
                             const TransferMatch matchType) {
  UNUSED_ARG(matchType);
  if (validateRowsToTransfer(rowsToTransfer)) {
    auto progress = setupProgressBar(rowsToTransfer);
    auto jobs = runsTable().reductionJobs();

    for (auto rowIndex : rowsToTransfer) {
      auto const &result = m_searcher->getSearchResult(rowIndex);
      auto row = validateRowFromRunAndTheta(result.runNumber(), result.theta());
      if (row.is_initialized()) {
        mergeRowIntoGroup(jobs, row.get(), m_thetaTolerance,
                          result.groupName());
      } else {
        m_searcher->setSearchResultError(
            rowIndex, "Theta was not specified in the description.");
      }
    }

    tablePresenter()->mergeAdditionalJobs(jobs);
  }
}

/** Tells the view to update the enabled/disabled state of all relevant
 * widgets based on whether processing is in progress or not.
 */
void RunsPresenter::updateWidgetEnabledState() const {
  // Update the menus
  m_view->updateMenuEnabledState(isProcessing());

  // Update components
  m_view->setInstrumentComboEnabled(!isAnyBatchProcessing() &&
                                    !isAnyBatchAutoreducing());
  m_view->setSearchTextEntryEnabled(!isAutoreducing() && !searchInProgress());
  m_view->setSearchButtonEnabled(!isAutoreducing() && !searchInProgress());
  m_view->setAutoreduceButtonEnabled(!isAnyBatchAutoreducing() &&
                                     !isProcessing() && !searchInProgress());
  m_view->setAutoreducePauseButtonEnabled(isAutoreducing());
  m_view->setTransferButtonEnabled(!isProcessing() && !isAutoreducing());
}

void RunsPresenter::handleError(const std::string &message,
                                const std::exception &e) {
  m_messageHandler->giveUserCritical(message + ": " + std::string(e.what()),
                                     "Error");
}

void RunsPresenter::handleError(const std::string &message) {
  m_messageHandler->giveUserCritical(message, "Error");
}

std::string RunsPresenter::liveDataReductionAlgorithm() {
  return "ReflectometryReductionOneLiveData";
}

std::string
RunsPresenter::liveDataReductionOptions(const std::string &inputWorkspace,
                                        const std::string &instrument) {
  // Get the properties for the reduction algorithm from the settings tabs
  AlgorithmRuntimeProps options = m_mainPresenter->rowProcessingProperties();
  // Add other required input properties to the live data reduction algorithnm
  options["InputWorkspace"] = inputWorkspace;
  options["Instrument"] = instrument;
  options["GetLiveValueAlgorithm"] = "GetLiveInstrumentValue";
  // Convert the properties to a string to pass to the algorithm
  auto const optionsString = convertMapToString(options, ';', false);
  return optionsString;
}

IAlgorithm_sptr RunsPresenter::setupLiveDataMonitorAlgorithm() {
  auto alg = AlgorithmManager::Instance().create("StartLiveData");
  alg->initialize();
  alg->setChild(true);
  alg->setLogging(false);
  auto const instrument = m_view->getSearchInstrument();
  auto const inputWorkspace = "TOF_live";
  auto const updateInterval = m_view->getLiveDataUpdateInterval();
  alg->setProperty("Instrument", instrument);
  alg->setProperty("OutputWorkspace", "IvsQ_binned_live");
  alg->setProperty("AccumulationWorkspace", inputWorkspace);
  alg->setProperty("AccumulationMethod", "Replace");
  alg->setProperty("UpdateEvery", static_cast<double>(updateInterval));
  alg->setProperty("PostProcessingAlgorithm", liveDataReductionAlgorithm());
  alg->setProperty("PostProcessingProperties",
                   liveDataReductionOptions(inputWorkspace, instrument));
  alg->setProperty("RunTransitionBehavior", "Restart");
  auto errorMap = alg->validateInputs();
  if (!errorMap.empty()) {
    std::string errorString;
    for (auto &kvp : errorMap)
      errorString.append(kvp.first + ":" + kvp.second);
    handleError(errorString);
    return nullptr;
  }
  return alg;
}

void RunsPresenter::updateViewWhenMonitorStarting() {
  m_view->setStartMonitorButtonEnabled(false);
  m_view->setStopMonitorButtonEnabled(false);
  m_view->setUpdateIntervalSpinBoxEnabled(false);
}

void RunsPresenter::updateViewWhenMonitorStarted() {
  m_view->setStartMonitorButtonEnabled(false);
  m_view->setStopMonitorButtonEnabled(true);
  m_view->setUpdateIntervalSpinBoxEnabled(false);
}

void RunsPresenter::updateViewWhenMonitorStopped() {
  m_view->setStartMonitorButtonEnabled(true);
  m_view->setStopMonitorButtonEnabled(false);
  m_view->setUpdateIntervalSpinBoxEnabled(true);
}

/** Start live data monitoring
 */
void RunsPresenter::startMonitor() {
  try {
    auto alg = setupLiveDataMonitorAlgorithm();
    if (!alg)
      return;
    auto algRunner = m_view->getMonitorAlgorithmRunner();
    algRunner->startAlgorithm(alg);
    updateViewWhenMonitorStarting();
  } catch (std::exception &e) {
    handleError("Error starting live data", e);
  } catch (...) {
    handleError("Error starting live data");
  }
}

/** Callback called when the monitor algorithm has been started
 */
void RunsPresenter::startMonitorComplete() {
  auto algRunner = m_view->getMonitorAlgorithmRunner();
  m_monitorAlg = algRunner->getAlgorithm()->getProperty("MonitorLiveData");
  if (m_monitorAlg) {
    observeError(m_monitorAlg);
    updateViewWhenMonitorStarted();
  } else {
    updateViewWhenMonitorStopped();
  }
}

/** Stop live data monitoring
 */
void RunsPresenter::stopMonitor() {
  if (!m_monitorAlg)
    return;

  stopObserving(m_monitorAlg);
  m_monitorAlg->cancel();
  m_monitorAlg.reset();
  updateViewWhenMonitorStopped();
}

/** Handler called when the monitor algorithm finishes
 */
void RunsPresenter::finishHandle(const IAlgorithm *alg) {
  UNUSED_ARG(alg);
  stopObserving(m_monitorAlg);
  m_monitorAlg.reset();
  updateViewWhenMonitorStopped();
}

/** Handler called when the monitor algorithm errors
 */
void RunsPresenter::errorHandle(const IAlgorithm *alg,
                                const std::string &what) {
  UNUSED_ARG(alg);
  UNUSED_ARG(what);
  stopObserving(m_monitorAlg);
  m_monitorAlg.reset();
  updateViewWhenMonitorStopped();
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
