#include "ReflRunsTabPresenter.h"
#include "IReflMainWindowPresenter.h"
#include "IReflRunsTabView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "MantidQtWidgets/Common/ProgressPresenter.h"
#include "ReflCatalogSearcher.h"
#include "ReflFromStdStringMap.h"
#include "ReflNexusMeasurementItemSource.h"
#include "ReflSearchModel.h"

#include <QStringList>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#include "Reduction/ValidateRow.h"
#include "Reduction/WorkspaceNamesFactory.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");
}

/** Constructor
* @param mainView :: [input] The view we're managing
* @param progressableView :: [input] The view reporting progress
* @param makeRunsTablePresenter :: A generator for the child presenters.
* @param workspaceNamesFactory :: A generator for the workspace names used in
* the reduction.
* @param thetaTolerance The tolerance used to determine if two runs should be
* summed in a reduction.
* @param instruments The names of the instruments to show as options for the
* search.
* @param defaultInstrumentIndex The index of the instrument to have selected by
* default.
* @param searcher :: [input] The search implementation
*/
ReflRunsTabPresenter::ReflRunsTabPresenter(
    IReflRunsTabView *mainView, ProgressableView *progressableView,
    RunsTablePresenterFactory makeRunsTablePresenter,
    WorkspaceNamesFactory workspaceNamesFactory, double thetaTolerance,
    std::vector<std::string> const &instruments, int defaultInstrumentIndex,
    boost::shared_ptr<IReflSearcher> searcher)
    : m_view(mainView), m_progressView(progressableView),
      m_makeRunsTablePresenter(makeRunsTablePresenter),
      m_workspaceNamesFactory(workspaceNamesFactory), m_mainPresenter(nullptr),
      m_searcher(searcher), m_instrumentChanged(false),
      m_thetaTolerance(thetaTolerance) {

  assert(m_view != nullptr);
  m_view->subscribe(this);
  m_tablePresenter = m_makeRunsTablePresenter(m_view->table());

  m_view->setInstrumentList(instruments, defaultInstrumentIndex);

  // If we don't have a searcher yet, use ReflCatalogSearcher
  if (!m_searcher)
    m_searcher.reset(new ReflCatalogSearcher());
}

/** Accept a main presenter
 * @param mainPresenter :: [input] A main presenter
 */
void ReflRunsTabPresenter::acceptMainPresenter(
    IReflBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
  // Register this presenter as the workspace receiver
  // When doing so, the inner presenters will notify this
  // presenter with the list of commands

  // for (const auto &presenter : m_tablePresenters)
  //  presenter->accept(this);

  // Note this must be done here since notifying the gdpp of its view
  // will cause it to request settings only accessible via the main
  // presenter.
}

void ReflRunsTabPresenter::settingsChanged() {
  // assert(static_cast<std::size_t>(group) < m_tablePresenters.size());
  // m_tablePresenters[group]->settingsChanged();
}

/**
Used by the view to tell the presenter something has changed
*/
void ReflRunsTabPresenter::notify(IReflRunsTabPresenter::Flag flag) {
  switch (flag) {
  case IReflRunsTabPresenter::SearchFlag:
    // Start the search algorithm. If it is not started, make sure
    // autoreduction is not left running
    if (!search())
      stopAutoreduction();
    break;
  case IReflRunsTabPresenter::StartAutoreductionFlag:
    startNewAutoreduction();
    break;
  case IReflRunsTabPresenter::PauseAutoreductionFlag:
    pauseAutoreduction();
    break;
  case IReflRunsTabPresenter::TimerEventFlag:
    checkForNewRuns();
    break;
  case IReflRunsTabPresenter::ICATSearchCompleteFlag: {
    icatSearchComplete();
    break;
  }
  case IReflRunsTabPresenter::TransferFlag:
    transfer(m_view->getSelectedSearchRows(), TransferMatch::Any);
    break;
  case IReflRunsTabPresenter::InstrumentChangedFlag:
    changeInstrument();
    break;
  case IReflRunsTabPresenter::GroupChangedFlag:
    changeGroup();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/** Searches for runs that can be used
 * @return : true if the search algorithm was started successfully, false if
 * there was a problem */
bool ReflRunsTabPresenter::search() {
  auto const searchString = m_view->getSearchString();
  // Don't bother searching if they're not searching for anything
  if (searchString.empty())
    return false;

  // This is breaking the abstraction provided by IReflSearcher, but provides a
  // nice usability win
  // If we're not logged into a catalog, prompt the user to do so
  if (CatalogManager::Instance().getActiveSessions().empty()) {
    try {
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
void ReflRunsTabPresenter::populateSearch(IAlgorithm_sptr searchAlg) {
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
    m_searchModel = boost::make_shared<ReflSearchModel>(
        results, m_view->getSearchInstrument());
    m_view->showSearch(m_searchModel);
  }
}

/** Searches ICAT for runs with given instrument and investigation id, transfers
 * runs to table and processes them. Clears any existing table data first.
 */
void ReflRunsTabPresenter::startNewAutoreduction() {
  // if (requireNewAutoreduction()) {
  //   // If starting a brand new autoreduction, delete all rows / groups in
  //   // existing table first
  //   // We'll prompt the user to check it's ok to delete existing rows
  //   auto tablePresenter = getTablePresenter(group);
  //   tablePresenter->setPromptUser(false);
  //   try {
  //     tablePresenter->notify(DataProcessorPresenter::DeleteAllFlag);
  //   } catch (const DataProcessorPresenter::DeleteAllRowsCancelledException &)
  //   {
  //     return;
  //   }
  // }

  if (setupNewAutoreduction(m_view->getSearchString()))
    checkForNewRuns();
}

/** Determines whether to start a new autoreduction. Starts a new one if the
 * either the search number, transfer method or instrument has changed
 * @return : Boolean on whether to start a new autoreduction
 */
bool ReflRunsTabPresenter::requireNewAutoreduction() const {
  bool searchNumChanged =
      m_autoreduction.searchStringChanged(m_view->getSearchString());

  return searchNumChanged || m_instrumentChanged;
}

bool ReflRunsTabPresenter::setupNewAutoreduction(
     const std::string &searchString) {
  return m_autoreduction.setupNewAutoreduction(searchString);
}

/** Start a single autoreduction process. Called periodially to add and process
 *  any new runs in the table.
 */
void ReflRunsTabPresenter::checkForNewRuns() {
  // Stop notifications during processing
  m_view->stopTimer();

  // Initially we just need to start an ICat search and the reduction will be
  // run when the search completes
  m_view->startIcatSearch();
}

/** Run an autoreduction process based on the latest search results
 */
void ReflRunsTabPresenter::autoreduceNewRuns() {

  m_autoreduction.setSearchResultsExist();
  auto rowsToTransfer = m_view->getAllSearchRows();

  if (rowsToTransfer.size() > 0) {
    // transfer(rowsToTransfer, autoreductionGroup(), TransferMatch::Strict);
    //    auto tablePresenter = getTablePresenter(autoreductionGroup());
    //    tablePresenter->setPromptUser(false);
    //    tablePresenter->notify(DataProcessorPresenter::ProcessAllFlag);
  } else {
    //confirmReductionCompleted();
  }
}

void ReflRunsTabPresenter::pauseAutoreduction() {
  //  if (isAutoreducing())
  //    getTablePresenter(autoreductionGroup())
  //        ->notify(DataProcessorPresenter::PauseFlag);
}

void ReflRunsTabPresenter::stopAutoreduction() {
  m_view->stopTimer();
  m_autoreduction.stop();
}

bool ReflRunsTabPresenter::isAutoreducing() const {
  return m_autoreduction.running();
}


bool ReflRunsTabPresenter::isProcessing() const {
  // TODO define this properly.
  return false;
}

void ReflRunsTabPresenter::icatSearchComplete() {
  // Populate the search results
  auto algRunner = m_view->getAlgorithmRunner();
  IAlgorithm_sptr searchAlg = algRunner->getAlgorithm();
  populateSearch(searchAlg);

  if (isAutoreducing()) {
    autoreduceNewRuns();
  }
}

RunsTablePresenter *ReflRunsTabPresenter::tablePresenter() const {
  return m_tablePresenter.get();
}

bool ReflRunsTabPresenter::shouldUpdateExistingSearchResults() const {
  // Existing search results should be updated rather than replaced if
  // autoreduction is running and has valid results
  return m_searchModel && isAutoreducing() &&
         m_autoreduction.searchResultsExist();
}

/** Check that the given rows are valid for a transfer and warn the user if not
 * @param rowsToTransfer : a set of row indices to transfer
 * @return : true if valid, false if not
 */
bool ReflRunsTabPresenter::validateRowsToTransfer(
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
ReflRunsTabPresenter::setupProgressBar(const std::set<int> &rowsToTransfer) {

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
 * @param group : the group number of the table to transfer to
 * @param matchType : an enum specifying how strictly to match runs against
 * the transfer criteria
 * @return : The runs to transfer as a vector of maps
 */
void ReflRunsTabPresenter::transfer(const std::set<int> &rowsToTransfer,
                                    const TransferMatch) {
  if (validateRowsToTransfer(rowsToTransfer)) {
    auto progress = setupProgressBar(rowsToTransfer);
    auto jobs = newJobsWithSlicingFrom(tablePresenter()->reductionJobs());

    for (auto rowIndex : rowsToTransfer) {
      auto &result = (*m_searchModel)[rowIndex];
      auto resultMetadata = metadataFromDescription(result.description);
      auto row =
          validateRowFromRunAndTheta(jobs, m_workspaceNamesFactory,
                                     result.runNumber, resultMetadata.theta);
      if (row.is_initialized()) {
        mergeRowIntoGroup(jobs, row.get(), m_thetaTolerance,
                          resultMetadata.groupName, m_workspaceNamesFactory);
      } else {
        m_searchModel->setError(rowIndex,
                                "Theta was not specified in the description.");
      }
    }

    tablePresenter()->mergeAdditionalJobs(jobs);
  }
}

/** Tells the view to update the enabled/disabled state of all relevant
 *widgets
 * based on whether processing is in progress or not.
 *
 */
void ReflRunsTabPresenter::updateWidgetEnabledState() const {
  auto const processing = isProcessing();
  auto const autoreducing = isAutoreducing();

  // Update the menus
  m_view->updateMenuEnabledState(processing);

  // Update components
  m_view->setTransferButtonEnabled(!processing && !autoreducing);
  m_view->setInstrumentComboEnabled(!processing && !autoreducing);
  m_view->setAutoreducePauseButtonEnabled(autoreducing);
  m_view->setSearchTextEntryEnabled(!autoreducing);
  m_view->setSearchButtonEnabled(!autoreducing);
  m_view->setAutoreduceButtonEnabled(!autoreducing && !processing);
}

/** Changes the current instrument in the data processor widget. Also clears
* the
* and the table selection model and updates the config service, printing an
* information message
*/
void ReflRunsTabPresenter::changeInstrument() {
  auto const instrument = m_view->getSearchInstrument();
  m_mainPresenter->setInstrumentName(instrument);
  Mantid::Kernel::ConfigService::Instance().setString("default.instrument",
                                                      instrument);
  g_log.information() << "Instrument changed to " << instrument;
  m_instrumentChanged = true;
}

void ReflRunsTabPresenter::changeGroup() { updateWidgetEnabledState(); }
}
}
