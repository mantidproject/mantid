// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsPresenter.h"
#include "CatalogRunNotifier.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "GUI/Common/IPythonRunner.h"
#include "GUI/Common/IReflMessageHandler.h"
#include "GUI/RunsTable/RunsTablePresenter.h"
#include "IRunsView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/ProgressPresenter.h"
#include "MantidQtWidgets/Common/QtAlgorithmRunner.h"
#include "QtCatalogSearcher.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "Reduction/ValidateRow.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry RunsPresenter");
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

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
RunsPresenter::RunsPresenter(IRunsView *mainView, ProgressableView *progressableView,
                             const RunsTablePresenterFactory &makeRunsTablePresenter, double thetaTolerance,
                             std::vector<std::string> instruments, IReflMessageHandler *messageHandler,
                             IFileHandler *fileHandler)
    : m_runNotifier(std::make_unique<CatalogRunNotifier>(mainView)),
      m_searcher(std::make_unique<QtCatalogSearcher>(mainView)), m_view(mainView), m_progressView(progressableView),
      m_mainPresenter(nullptr), m_messageHandler(messageHandler), m_fileHandler(fileHandler),
      m_instruments(std::move(instruments)), m_thetaTolerance(thetaTolerance), m_tableUnsaved{false} {

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
void RunsPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

/** Initialise the list of available instruments.
 * @param selectedInstrument : Optional name of an instrument to try and select from the list.
 * @returns The name of the instrument that is selected.
 */
std::string RunsPresenter::initInstrumentList(const std::string &selectedInstrument) {
  m_view->setInstrumentList(m_instruments, selectedInstrument);
  return m_view->getSearchInstrument();
}

RunsTable const &RunsPresenter::runsTable() const { return tablePresenter()->runsTable(); }

RunsTable &RunsPresenter::mutableRunsTable() { return tablePresenter()->mutableRunsTable(); }

/** Returns true if performing a new search i.e. with different criteria to any
 * previous search
 */
bool RunsPresenter::newSearchCriteria() const { return searchCriteria() != m_searcher->searchCriteria(); }

void RunsPresenter::notifySearch() {
  updateWidgetEnabledState();

  // Don't bother searching if they're not searching for anything
  if (m_view->getSearchString().empty())
    return;

  // Clear existing results if performing a different search
  if (searchCriteria() != m_searcher->searchCriteria()) {
    if (overwriteSearchResultsPrevented()) {
      return;
    }
    m_searcher->reset();
  }

  search();
}

void RunsPresenter::notifyCheckForNewRuns() { checkForNewRuns(); }

void RunsPresenter::notifySearchComplete() {
  if (!isAutoreducing())
    resizeSearchResultsColumns();

  updateWidgetEnabledState();

  if (isAutoreducing())
    autoreduceNewRuns();
}

void RunsPresenter::notifySearchFailed() {
  if (isAutoreducing()) {
    notifyPauseAutoreductionRequested();
  }
}

void RunsPresenter::notifyTransfer() { transfer(m_view->getSelectedSearchRows(), TransferMatch::Any); }

// Notification from our own view that the instrument should be changed
void RunsPresenter::notifyChangeInstrumentRequested() {
  auto const newName = m_view->getSearchInstrument();

  // If the instrument cannot be changed, revert it on the view and quit
  if (changeInstrumentPrevented(newName))
    m_view->setSearchInstrument(instrumentName());
  else
    m_mainPresenter->notifyChangeInstrumentRequested(newName);
}

void RunsPresenter::notifyExportSearchResults() const {
  auto csv = m_searcher->getSearchResultsCSV();
  if (csv.empty()) {
    m_messageHandler->giveUserCritical(
        "No search results loaded. Enter an Investigation ID (and a cycle if using) to load results.", "Error");
    return;
  }

  auto filename = m_messageHandler->askUserForSaveFileName("CSV (*.csv)");
  if (filename.empty()) {
    return;
  }

  // Append a .csv extension if the user didn't add one manually.
  if (filename.find_last_of('.') == std::string::npos ||
      filename.substr(filename.find_last_of('.') + 1) != std::string("csv")) {
    filename += ".csv";
  }

  try {
    m_fileHandler->saveCSVToFile(filename, csv);
  } catch (std::runtime_error &e) {
    m_messageHandler->giveUserCritical(e.what(), "Error");
  }
}

// Notification from a child presenter that the instrument needs to be changed
// Returns true and continues to change the instrument if possible; returns
// false if not
bool RunsPresenter::notifyChangeInstrumentRequested(std::string const &instrumentName) {
  if (changeInstrumentPrevented(instrumentName))
    return false;

  m_mainPresenter->notifyChangeInstrumentRequested(instrumentName);
  return true;
}

void RunsPresenter::notifyResumeReductionRequested() { m_mainPresenter->notifyResumeReductionRequested(); }

void RunsPresenter::notifyPauseReductionRequested() { m_mainPresenter->notifyPauseReductionRequested(); }

void RunsPresenter::notifyResumeAutoreductionRequested() { m_mainPresenter->notifyResumeAutoreductionRequested(); }

void RunsPresenter::notifyPauseAutoreductionRequested() { m_mainPresenter->notifyPauseAutoreductionRequested(); }

void RunsPresenter::notifyStartMonitor() { startMonitor(); }

void RunsPresenter::notifyStopMonitor() { stopMonitor(); }

void RunsPresenter::notifyStartMonitorComplete() { startMonitorComplete(); }

void RunsPresenter::notifyRowStateChanged() { tablePresenter()->notifyRowStateChanged(); }

void RunsPresenter::notifyRowStateChanged(boost::optional<Item const &> item) {
  tablePresenter()->notifyRowStateChanged(item);
}

void RunsPresenter::notifyRowModelChanged() {
  tablePresenter()->notifyRowModelChanged();
  tablePresenter()->notifyRowStateChanged();
}

void RunsPresenter::notifyRowModelChanged(boost::optional<Item const &> item) {
  tablePresenter()->notifyRowModelChanged(item);
  tablePresenter()->notifyRowStateChanged(item);
}

void RunsPresenter::notifyBatchLoaded() { m_tablePresenter->notifyBatchLoaded(); }

void RunsPresenter::notifyReductionResumed() {
  updateWidgetEnabledState();
  tablePresenter()->notifyReductionResumed();
  notifyRowStateChanged();
}

void RunsPresenter::notifyReductionPaused() {
  updateWidgetEnabledState();
  tablePresenter()->notifyReductionPaused();
  notifyRowStateChanged();
}

/** Returns true if performing a new autoreduction search i.e. with different
 * criteria to any previous autoreduction
 */
bool RunsPresenter::newAutoreductionCriteria() const { return searchCriteria() != m_lastAutoreductionSearch; }

/** Return true if starting a new autoreduction (with new criteria) is
 * prevented e.g. if the user does not want to discard changes
 */
bool RunsPresenter::autoreductionPrevented() const {
  // There's slight duplication in the checks here to ensure the user gets an
  // informative warning message
  if (newAutoreductionCriteria() && newSearchCriteria() && m_tableUnsaved && m_searcher->hasUnsavedChanges())
    return overwriteSearchResultsAndTablePrevented();
  else if (newAutoreductionCriteria() && m_tableUnsaved)
    return overwriteTablePrevented();
  else if (newSearchCriteria() && m_searcher->hasUnsavedChanges())
    return overwriteSearchResultsPrevented();

  return false;
}

/** Resume autoreduction. Clears any existing table data first and then
 * starts a search to check if there are new runs.
 */
bool RunsPresenter::resumeAutoreduction() {
  if (m_view->getSearchString().empty()) {
    m_messageHandler->giveUserInfo("Search field is empty", "Search Issue");
    return false;
  }

  if (autoreductionPrevented())
    return false;

  // Clear the search results if it's a new search
  if (newSearchCriteria())
    m_searcher->reset();

  // Clear the main table if it's a new autoreduction
  if (newAutoreductionCriteria()) {
    m_lastAutoreductionSearch = searchCriteria();
    tablePresenter()->notifyRemoveAllRowsAndGroupsRequested();
    m_tableUnsaved = false;
  }

  checkForNewRuns();
  return true;
}

void RunsPresenter::notifyAutoreductionResumed() {
  updateWidgetEnabledState();
  tablePresenter()->notifyAutoreductionResumed();
  m_progressView->setAsEndlessIndicator();
  notifyRowStateChanged();
}

void RunsPresenter::notifyAutoreductionPaused() {
  m_runNotifier->stopPolling();
  m_progressView->setAsPercentageIndicator();
  updateWidgetEnabledState();
  tablePresenter()->notifyAutoreductionPaused();
  notifyRowStateChanged();
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

std::string RunsPresenter::instrumentName() const { return m_mainPresenter->instrumentName(); }

void RunsPresenter::notifyTableChanged() { m_tableUnsaved = true; }

void RunsPresenter::notifyRowContentChanged(Row &changedRow) { m_mainPresenter->notifyRowContentChanged(changedRow); }

void RunsPresenter::notifyGroupNameChanged(Group &changedGroup) {
  m_mainPresenter->notifyGroupNameChanged(changedGroup);
}

void RunsPresenter::settingsChanged() { tablePresenter()->settingsChanged(); }

void RunsPresenter::notifyChangesSaved() {
  m_searcher->setSaved();
  m_tableUnsaved = false;
}

/** Searches for runs that can be used
 * @return : true if the search algorithm was started successfully, false if
 * there was a problem */
bool RunsPresenter::search() {
  if (!m_searcher->startSearchAsync(searchCriteria())) {
    m_messageHandler->giveUserCritical("Error starting search", "Error");
    return false;
  }

  return true;
}

/** Resize the search results table columns to something sensible
 */
void RunsPresenter::resizeSearchResultsColumns() {
  // Resize to content
  m_view->resizeSearchResultsColumnsToContents();

  // Limit columns' widths to a sensible maximum, based on a % of the table
  // width
  static auto constexpr numColumns = static_cast<int>(ISearchModel::Column::NUM_COLUMNS);
  auto const factor = 0.4;
  auto const maxWidth = static_cast<int>(m_view->getSearchResultsTableWidth() * factor);
  for (auto column = 0; column < numColumns; ++column) {
    if (m_view->getSearchResultsColumnWidth(column) > maxWidth)
      m_view->setSearchResultsColumnWidth(column, maxWidth);
  }
}

/** Start a single autoreduction process. Called periodially to add and process
 *  any new runs in the table.
 */
void RunsPresenter::checkForNewRuns() {
  // Stop notifications during processing
  m_runNotifier->stopPolling();

  // Initially we just need to start an ICat search and the reduction will be
  // run when the search completes
  search();
}

/** Run an autoreduction process based on the latest search results
 */
void RunsPresenter::autoreduceNewRuns() {

  auto rowsToTransfer = m_view->getAllSearchRows();

  if (rowsToTransfer.size() > 0)
    transfer(rowsToTransfer, TransferMatch::Strict);

  m_mainPresenter->notifyResumeReductionRequested();
}

bool RunsPresenter::isProcessing() const { return m_mainPresenter->isProcessing(); }

bool RunsPresenter::isAutoreducing() const { return m_mainPresenter->isAutoreducing(); }

bool RunsPresenter::isAnyBatchProcessing() const { return m_mainPresenter->isAnyBatchProcessing(); }

bool RunsPresenter::isAnyBatchAutoreducing() const { return m_mainPresenter->isAnyBatchAutoreducing(); }

bool RunsPresenter::changeInstrumentPrevented(std::string const &newName) const {
  return newName != instrumentName() && overwriteSearchResultsPrevented();
}

bool RunsPresenter::hasUnsavedChanges() const { return m_tableUnsaved || m_searcher->hasUnsavedChanges(); }

bool RunsPresenter::overwriteSearchResultsAndTablePrevented() const {
  return hasUnsavedChanges() &&
         !m_mainPresenter->discardChanges("This will cause unsaved changes in the search results "
                                          "and main table to be lost. Continue?");
}

bool RunsPresenter::overwriteTablePrevented() const {
  return m_tableUnsaved && !m_mainPresenter->discardChanges("This will cause unsaved changes in "
                                                            "the table to be lost. Continue?");
}

bool RunsPresenter::overwriteSearchResultsPrevented() const {
  return m_searcher->hasUnsavedChanges() &&
         !m_mainPresenter->discardChanges("This will cause unsaved changes in the search results to be "
                                          "lost. Continue?");
}

bool RunsPresenter::searchInProgress() const { return m_searcher->searchInProgress(); }

SearchCriteria RunsPresenter::searchCriteria() const {
  return SearchCriteria{m_view->getSearchInstrument(), m_view->getSearchCycle(), m_view->getSearchString()};
}

int RunsPresenter::percentComplete() const {
  if (!m_mainPresenter)
    return 0;
  return m_mainPresenter->percentComplete();
}

void RunsPresenter::setRoundPrecision(int &precision) { m_tablePresenter->setTablePrecision(precision); }

void RunsPresenter::resetRoundPrecision() { m_tablePresenter->resetTablePrecision(); }

IRunsTablePresenter *RunsPresenter::tablePresenter() const { return m_tablePresenter.get(); }

/** Check that the given rows are valid for a transfer and warn the user if not
 * @param rowsToTransfer : a set of row indices to transfer
 * @return : true if valid, false if not
 */
bool RunsPresenter::validateRowsToTransfer(const std::set<int> &rowsToTransfer) {
  // Check that we have something to transfer
  if (rowsToTransfer.size() == 0) {
    m_messageHandler->giveUserCritical("Please select at least one run to transfer.", "No runs selected");
    return false;
  }
  return true;
}

/** Set up the progress bar
 */
ProgressPresenter RunsPresenter::setupProgressBar(const std::set<int> &rowsToTransfer) {

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
void RunsPresenter::transfer(const std::set<int> &rowsToTransfer, const TransferMatch matchType) {
  UNUSED_ARG(matchType);
  if (validateRowsToTransfer(rowsToTransfer)) {
    auto progress = setupProgressBar(rowsToTransfer);
    auto jobs = runsTable().reductionJobs();

    for (auto rowIndex : rowsToTransfer) {
      auto const &result = m_searcher->getSearchResult(rowIndex);
      if (result.hasError() || result.exclude())
        continue;
      auto row = validateRowFromRunAndTheta(result.runNumber(), result.theta());
      assert(row.is_initialized());
      mergeRowIntoGroup(jobs, row.get(), m_thetaTolerance, result.groupName());
    }

    tablePresenter()->mergeAdditionalJobs(jobs);
    m_mainPresenter->notifyRunsTransferred();
  }
}

/** Tells the view to update the enabled/disabled state of all relevant
 * widgets based on whether processing is in progress or not.
 */
void RunsPresenter::updateWidgetEnabledState() const {
  // Update the menus
  m_view->updateMenuEnabledState(isProcessing());

  // Update components
  m_view->setInstrumentComboEnabled(!isAnyBatchProcessing() && !isAnyBatchAutoreducing());
  m_view->setSearchTextEntryEnabled(!isAutoreducing() && !searchInProgress());
  m_view->setSearchButtonEnabled(!isAutoreducing() && !searchInProgress());
  m_view->setSearchResultsEnabled(!isAutoreducing() && !searchInProgress());
  m_view->setAutoreduceButtonEnabled(!isAnyBatchAutoreducing() && !isProcessing() && !searchInProgress());
  m_view->setAutoreducePauseButtonEnabled(isAutoreducing());
  m_view->setTransferButtonEnabled(!isProcessing() && !isAutoreducing());
}

void RunsPresenter::handleError(const std::string &message, const std::exception &e) {
  m_messageHandler->giveUserCritical(message + ": " + std::string(e.what()), "Error");
}

void RunsPresenter::handleError(const std::string &message) { m_messageHandler->giveUserCritical(message, "Error"); }

std::string RunsPresenter::liveDataReductionAlgorithm() { return "ReflectometryReductionOneLiveData"; }

std::string RunsPresenter::liveDataReductionOptions(const std::string &inputWorkspace, const std::string &instrument) {
  // Get the properties for the reduction algorithm from the settings tabs
  g_log.warning("Note that lookup of experiment settings by angle/title is not supported for live data.");
  auto options = m_mainPresenter->rowProcessingProperties();
  // Add other required input properties to the live data reduction algorithnm
  options->setPropertyValue("InputWorkspace", inputWorkspace);
  options->setPropertyValue("Instrument", instrument);
  options->setPropertyValue("GetLiveValueAlgorithm", "GetLiveInstrumentValue");

  return convertAlgPropsToString(*options);
}

IAlgorithm_sptr RunsPresenter::setupLiveDataMonitorAlgorithm() {
  auto alg = AlgorithmManager::Instance().create("StartLiveData");
  alg->initialize();
  alg->setChild(false);
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
  alg->setProperty("PostProcessingProperties", liveDataReductionOptions(inputWorkspace, instrument));
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
  g_log.warning("Live data monitor stopped; re-starting the monitor.");
  startMonitor();
}

/** Handler called when the monitor algorithm errors
 */
void RunsPresenter::errorHandle(const IAlgorithm *alg, const std::string &what) {
  UNUSED_ARG(alg);
  UNUSED_ARG(what);
  stopObserving(m_monitorAlg);
  m_monitorAlg.reset();
  updateViewWhenMonitorStopped();
  if (what != "Algorithm terminated") {
    g_log.warning("Live data error: " + what + "; re-starting the monitor.");
    startMonitor();
  }
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
