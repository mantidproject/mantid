// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsPresenter.h"
#include "Autoreduction.h"
#include "CatalogSearcher.h"
#include "Common/IMessageHandler.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "GUI/RunsTable/RunsTablePresenter.h"
#include "IRunsView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "MantidQtWidgets/Common/ProgressPresenter.h"
#include "SearchModel.h"

#include <QStringList>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#include "Reduction/ValidateRow.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param mainView :: [input] The view we're managing
 * @param progressableView :: [input] The view reporting progress
 * @param makeRunsTablePresenter :: A generator for the child presenters.
 * @param thetaTolerance The tolerance used to determine if two runs should be
 * summed in a reduction.
 * @param instruments The names of the instruments to show as options for the
 * search.
 * @param defaultInstrumentIndex The index of the instrument to have selected by
 * default.
 * @param messageHandler :: A handler to pass messages to the user
 * @param autoreduction :: [input] The autoreduction implementation
 * @param searcher :: [input] The search implementation
 */
RunsPresenter::RunsPresenter(
    IRunsView *mainView, ProgressableView *progressableView,
    const RunsTablePresenterFactory &makeRunsTablePresenter,
    double thetaTolerance, std::vector<std::string> const &instruments,
    int defaultInstrumentIndex, IMessageHandler *messageHandler,
    boost::shared_ptr<IAutoreduction> autoreduction,
    boost::shared_ptr<ISearcher> searcher)
    : m_autoreduction(autoreduction), m_view(mainView),
      m_progressView(progressableView), m_mainPresenter(nullptr),
      m_messageHandler(messageHandler), m_searcher(searcher),
      m_instruments(instruments),
      m_defaultInstrumentIndex(defaultInstrumentIndex),
      m_instrumentChanged(false), m_thetaTolerance(thetaTolerance) {

  assert(m_view != nullptr);
  m_view->subscribe(this);
  m_tablePresenter = makeRunsTablePresenter(m_view->table());
  m_tablePresenter->acceptMainPresenter(this);

  if (!m_autoreduction)
    m_autoreduction.reset(new Autoreduction());

  // If we don't have a searcher yet, use CatalogSearcher
  if (!m_searcher)
    m_searcher.reset(new CatalogSearcher());

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
  // Must do this after setting main presenter or notifications don't get
  // through
  m_view->setInstrumentList(m_instruments, m_defaultInstrumentIndex);
  // Register this presenter as the workspace receiver
  // When doing so, the inner presenters will notify this
  // presenter with the list of commands

  //  tablePresenter()->accept(this);

  // Note this must be done here since notifying the gdpp of its view
  // will cause it to request settings only accessible via the main
  // presenter.
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
  // Start the search algorithm. If it is not started, make sure
  // autoreduction is not left running
  if (!search())
    stopAutoreduction();
}

void RunsPresenter::notifyTimerEvent() { checkForNewRuns(); }

void RunsPresenter::notifyICATSearchComplete() { icatSearchComplete(); }

void RunsPresenter::notifyTransfer() {
  transfer(m_view->getSelectedSearchRows(), TransferMatch::Any);
}

void RunsPresenter::notifyInstrumentChanged() {
  auto const instrumentName = m_view->getSearchInstrument();
  if (m_mainPresenter)
    m_mainPresenter->notifyInstrumentChanged(instrumentName);
}

void RunsPresenter::notifyInstrumentChanged(std::string const &instrumentName) {
  m_mainPresenter->notifyInstrumentChanged(instrumentName);
}

void RunsPresenter::notifyReductionResumed() {
  m_mainPresenter->notifyReductionResumed();
}

void RunsPresenter::notifyReductionPaused() {
  m_mainPresenter->notifyReductionPaused();
}

void RunsPresenter::notifyAutoreductionResumed() {
  m_mainPresenter->notifyAutoreductionResumed();
}

void RunsPresenter::notifyAutoreductionPaused() {
  m_mainPresenter->notifyAutoreductionPaused();
}

void RunsPresenter::notifyStartMonitor() { startMonitor(); }

void RunsPresenter::notifyStopMonitor() { stopMonitor(); }

void RunsPresenter::notifyStartMonitorComplete() { startMonitorComplete(); }

void RunsPresenter::notifyRowStateChanged() {
  tablePresenter()->notifyRowStateChanged();
}

void RunsPresenter::reductionResumed() {
  updateWidgetEnabledState();
  tablePresenter()->reductionResumed();
}

void RunsPresenter::reductionPaused() {
  updateWidgetEnabledState();
  tablePresenter()->reductionPaused();
}

/** Searches ICAT for runs with given instrument and investigation id, transfers
 * runs to table and processes them. Clears any existing table data first.
 */
void RunsPresenter::autoreductionResumed() {
  if (requireNewAutoreduction()) {
    // If starting a brand new autoreduction, delete all rows / groups in
    // existing table first
    // We'll prompt the user to check it's ok to delete existing rows

    // tablePresenter()->setPromptUser(false);
    // try {
    //  tablePresenter()->notify(DataProcessorPresenter::DeleteAllFlag);
    //} catch (const DataProcessorPresenter::DeleteAllRowsCancelledException &)
    //{
    //  return;
    //}
  }

  if (m_autoreduction->setupNewAutoreduction(m_view->getSearchString()))
    checkForNewRuns();

  updateWidgetEnabledState();
  tablePresenter()->autoreductionResumed();
}

void RunsPresenter::autoreductionPaused() {
  m_view->stopTimer();
  m_autoreduction->stop();
  updateWidgetEnabledState();
  tablePresenter()->autoreductionPaused();
}

void RunsPresenter::instrumentChanged(
    std::string const &instrumentName,
    Mantid::Geometry::Instrument_const_sptr instrument) {
  m_instrumentChanged = true;
  m_view->setSearchInstrument(instrumentName);
  tablePresenter()->instrumentChanged(instrumentName, instrument);
}

void RunsPresenter::settingsChanged() { tablePresenter()->settingsChanged(); }

/** Searches for runs that can be used
 * @return : true if the search algorithm was started successfully, false if
 * there was a problem */
bool RunsPresenter::search() {
  auto const searchString = m_view->getSearchString();
  // Don't bother searching if they're not searching for anything
  if (searchString.empty())
    return false;

  // This is breaking the abstraction provided by ISearcher, but provides a
  // nice usability win
  // If we're not logged into a catalog, prompt the user to do so
  if (CatalogManager::Instance().getActiveSessions().empty()) {
    try {
      // TODO: replace python runner
      // std::stringstream pythonSrc;
      // pythonSrc << "try:\n";
      // pythonSrc << "  algm = CatalogLoginDialog()\n";
      // pythonSrc << "except:\n";
      // pythonSrc << "  pass\n";
      // m_mainPresenter->runPythonAlgorithm(pythonSrc.str());
    } catch (std::runtime_error &e) {
      m_view->loginFailed(e.what());
      return false;
    }
  }
  std::string sessionId;
  // check to see if we have any active sessions for ICAT
  if (!CatalogManager::Instance().getActiveSessions().empty()) {
    // we have an active session, so grab the ID
    sessionId =
        CatalogManager::Instance().getActiveSessions().front()->getSessionId();
  } else {
    // there are no active sessions, we return here to avoid an exception
    m_view->noActiveICatSessions();
    return false;
  }
  auto algSearch = AlgorithmManager::Instance().create("CatalogGetDataFiles");
  algSearch->initialize();
  algSearch->setChild(true);
  algSearch->setLogging(false);
  algSearch->setProperty("OutputWorkspace", "_ReflSearchResults");
  algSearch->setProperty("Session", sessionId);
  algSearch->setProperty("InvestigationId", searchString);
  auto algRunner = m_view->getAlgorithmRunner();
  algRunner->startAlgorithm(algSearch);

  return true;
}

/** Populates the search results table
 * @param searchAlg : [input] The search algorithm
 */
void RunsPresenter::populateSearch(IAlgorithm_sptr searchAlg) {
  if (!searchAlg->isExecuted())
    return;

  // Get the results from the algorithm
  ITableWorkspace_sptr results = searchAlg->getProperty("OutputWorkspace");

  // Update the state and model
  m_instrumentChanged = false;

  if (shouldUpdateExistingSearchResults()) {
    m_searchModel->addDataFromTable(results, m_view->getSearchInstrument());
  } else {
    // Create a new search results list and display it on the view
    m_searchModel =
        boost::make_shared<SearchModel>(results, m_view->getSearchInstrument());
    m_view->showSearch(m_searchModel);
  }
}

/** Determines whether to start a new autoreduction. Starts a new one if the
 * either the search number, transfer method or instrument has changed
 * @return : Boolean on whether to start a new autoreduction
 */
bool RunsPresenter::requireNewAutoreduction() const {
  bool searchNumChanged =
      m_autoreduction->searchStringChanged(m_view->getSearchString());

  return searchNumChanged || m_instrumentChanged;
}

/** Start a single autoreduction process. Called periodially to add and process
 *  any new runs in the table.
 */
void RunsPresenter::checkForNewRuns() {
  // Stop notifications during processing
  m_view->stopTimer();

  // Initially we just need to start an ICat search and the reduction will be
  // run when the search completes
  m_view->startIcatSearch();
}

/** Run an autoreduction process based on the latest search results
 */
void RunsPresenter::autoreduceNewRuns() {

  m_autoreduction->setSearchResultsExist();
  auto rowsToTransfer = m_view->getAllSearchRows();

  if (rowsToTransfer.size() > 0) {
    transfer(rowsToTransfer, TransferMatch::Strict);
  } else {
    m_mainPresenter->notifyAutoreductionCompleted();
  }
}

void RunsPresenter::stopAutoreduction() {
  m_view->stopTimer();
  m_autoreduction->stop();
}

bool RunsPresenter::isProcessing() const {
  return m_mainPresenter->isProcessing();
}

bool RunsPresenter::isAutoreducing() const {
  return m_mainPresenter->isAutoreducing();
}

void RunsPresenter::icatSearchComplete() {
  // Populate the search results
  auto algRunner = m_view->getAlgorithmRunner();
  IAlgorithm_sptr searchAlg = algRunner->getAlgorithm();
  populateSearch(searchAlg);

  if (isAutoreducing()) {
    autoreduceNewRuns();
  }
}

IRunsTablePresenter *RunsPresenter::tablePresenter() const {
  return m_tablePresenter.get();
}

bool RunsPresenter::shouldUpdateExistingSearchResults() const {
  // Existing search results should be updated rather than replaced if
  // autoreduction is running and has valid results
  return m_searchModel && isAutoreducing() &&
         m_autoreduction->searchResultsExist();
}

/** Check that the given rows are valid for a transfer and warn the user if not
 * @param rowsToTransfer : a set of row indices to transfer
 * @return : true if valid, false if not
 */
bool RunsPresenter::validateRowsToTransfer(
    const std::set<int> &rowsToTransfer) {
  // Check that we have something to transfer
  if (rowsToTransfer.size() == 0) {
    m_view->missingRunsToTransfer();
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

struct RunDescriptionMetadata {
  std::string groupName;
  std::string theta;
};

RunDescriptionMetadata metadataFromDescription(std::string const &description) {
  static boost::regex descriptionFormatRegex("(.*)(th[:=]([0-9.]+))(.*)");
  boost::smatch matches;
  if (boost::regex_search(description, matches, descriptionFormatRegex)) {
    constexpr auto preThetaGroup = 1;
    constexpr auto thetaValueGroup = 3;
    constexpr auto postThetaGroup = 4;

    const auto theta = matches[thetaValueGroup].str();
    const auto preTheta = matches[preThetaGroup].str();
    const auto postTheta = matches[postThetaGroup].str();

    return RunDescriptionMetadata{preTheta, theta};
  } else {
    return RunDescriptionMetadata{description, ""};
  }
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
      auto &result = m_searchModel->getRowData(rowIndex);
      auto resultMetadata = metadataFromDescription(result.description);
      auto row = validateRowFromRunAndTheta(jobs, result.runNumber,
                                            resultMetadata.theta);
      if (row.is_initialized()) {
        mergeRowIntoGroup(jobs, row.get(), m_thetaTolerance,
                          resultMetadata.groupName);
      } else {
        m_searchModel->setError(rowIndex,
                                "Theta was not specified in the description.");
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
  m_view->setInstrumentComboEnabled(!isProcessing() && !isAutoreducing());
  m_view->setSearchTextEntryEnabled(!isAutoreducing());
  m_view->setSearchButtonEnabled(!isAutoreducing());
  m_view->setAutoreduceButtonEnabled(!isAutoreducing() && !isProcessing());
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
RunsPresenter::liveDataReductionOptions(const std::string &instrument) {
  // Get the properties for the reduction algorithm from the settings tab. We
  // don't have a group associated with live data. This is not ideal but for
  // now just use the first group.
  // int const group = 0;
  OptionsMap options; // TODO replace:
                      // convertOptionsFromQMap(getProcessingOptions(group));
  // Add other required input properties to the live data reduction algorithnm
  options["Instrument"] = QString::fromStdString(instrument);
  options["GetLiveValueAlgorithm"] = "GetLiveInstrumentValue";
  // Convert the properties to a string to pass to the algorithm
  auto const optionsString =
      convertMapToString(options, ';', false).toStdString();
  return optionsString;
}

IAlgorithm_sptr RunsPresenter::setupLiveDataMonitorAlgorithm() {
  auto alg = AlgorithmManager::Instance().create("StartLiveData");
  alg->initialize();
  alg->setChild(true);
  alg->setLogging(false);
  auto instrument = m_view->getSearchInstrument();
  alg->setProperty("Instrument", instrument);
  alg->setProperty("OutputWorkspace", "IvsQ_binned_live");
  alg->setProperty("AccumulationWorkspace", "TOF_live");
  alg->setProperty("AccumulationMethod", "Replace");
  alg->setProperty("UpdateEvery", "20");
  alg->setProperty("PostProcessingAlgorithm", liveDataReductionAlgorithm());
  alg->setProperty("PostProcessingProperties",
                   liveDataReductionOptions(instrument));
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
}

void RunsPresenter::updateViewWhenMonitorStarted() {
  m_view->setStartMonitorButtonEnabled(false);
  m_view->setStopMonitorButtonEnabled(true);
}

void RunsPresenter::updateViewWhenMonitorStopped() {
  m_view->setStartMonitorButtonEnabled(true);
  m_view->setStopMonitorButtonEnabled(false);
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
} // namespace CustomInterfaces
} // namespace MantidQt
