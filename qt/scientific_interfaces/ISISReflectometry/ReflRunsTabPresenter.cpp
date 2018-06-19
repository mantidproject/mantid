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
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>

#include "Reduction/WorkspaceNamesFactory.h"
#include "ValidateRow.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");

QStringList fromStdStringVector(std::vector<std::string> const &inVec) {
  QStringList outVec;
  std::transform(inVec.begin(), inVec.end(), std::back_inserter(outVec),
                 &QString::fromStdString);
  return outVec;
}
}

/** Constructor
* @param mainView :: [input] The view we're managing
* @param progressableView :: [input] The view reporting progress
* @param tablePresenters :: [input] The data processor presenters
* @param searcher :: [input] The search implementation
*/
ReflRunsTabPresenter::ReflRunsTabPresenter(
    IReflRunsTabView *mainView, ProgressableView *progressableView,
    BatchPresenterFactory makeBatchPresenter,
    WorkspaceNamesFactory workspaceNamesFactory, double thetaTolerance,
    std::vector<std::string> const &instruments, int defaultInstrumentIndex,
    boost::shared_ptr<IReflSearcher> searcher)
    : m_view(mainView), m_progressView(progressableView),
      m_makeBatchPresenter(makeBatchPresenter),
      m_workspaceNamesFactory(workspaceNamesFactory), m_mainPresenter(nullptr),
      m_searcher(searcher), m_instrumentChanged(false),
      m_thetaTolerance(thetaTolerance) {

  assert(m_view != nullptr);
  m_view->subscribe(this);
  for (const auto &tableView : m_view->tableViews())
    m_tablePresenters.emplace_back(m_makeBatchPresenter(tableView));

  m_view->setInstrumentList(instruments, defaultInstrumentIndex);

  // If we don't have a searcher yet, use ReflCatalogSearcher
  if (!m_searcher)
    m_searcher.reset(new ReflCatalogSearcher());
}

/** Accept a main presenter
* @param mainPresenter :: [input] A main presenter
*/
void ReflRunsTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
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

void ReflRunsTabPresenter::settingsChanged(int group) {
  assert(static_cast<std::size_t>(group) < m_tablePresenters.size());
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
    transfer(m_view->getSelectedSearchRows(), selectedGroup(),
             TransferMatch::Any);
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

void ReflRunsTabPresenter::completedGroupReductionSuccessfully(
    GroupData const &group, std::string const &workspaceName) {
  m_mainPresenter->completedGroupReductionSuccessfully(group, workspaceName);
}

void ReflRunsTabPresenter::completedRowReductionSuccessfully(
    GroupData const &group, std::string const &workspaceNames) {
  m_mainPresenter->completedRowReductionSuccessfully(group, workspaceNames);
}

/** Pushes the list of commands (actions) */
void ReflRunsTabPresenter::pushCommands(int) {

  //  m_view->clearCommands();
  //
  //  // The expected number of commands
  //  const size_t nCommands = 31;
  //  //auto commands = getTablePresenter(group)->publishCommands();
  //  if (commands.size() != nCommands) {
  //    throw std::runtime_error("Invalid list of commands");
  //  }
  //  // The index at which "row" commands start
  //  const size_t rowCommStart = 10u;
  //  // We want to have two menus
  //  // Populate the "Reflectometry" menu
  //  std::vector<MantidWidgets::DataProcessor::Command_uptr> tableCommands;
  //  for (size_t i = 0; i < rowCommStart; i++)
  //    tableCommands.push_back(std::move(commands[i]));
  //  m_view->setTableCommands(std::move(tableCommands));
  //  // Populate the "Edit" menu
  //  std::vector<MantidWidgets::DataProcessor::Command_uptr> rowCommands;
  //  for (size_t i = rowCommStart; i < nCommands; i++)
  //    rowCommands.push_back(std::move(commands[i]));
  //  m_view->setRowCommands(std::move(rowCommands));
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
      std::stringstream pythonSrc;
      pythonSrc << "try:\n";
      pythonSrc << "  algm = CatalogLoginDialog()\n";
      pythonSrc << "except:\n";
      pythonSrc << "  pass\n";
      m_mainPresenter->runPythonAlgorithm(pythonSrc.str());
    } catch (std::runtime_error &e) {
      m_mainPresenter->giveUserCritical(
          "Error Logging in:\n" + std::string(e.what()), "login failed");
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
    m_mainPresenter->giveUserInfo(
        "Error Logging in: Please press 'Search' to try again.",
        "Login Failed");
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

  auto const group = selectedGroup();

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

  if (setupNewAutoreduction(group, m_view->getSearchString()))
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
    int group, const std::string &searchString) {
  return m_autoreduction.setupNewAutoreduction(group, searchString);
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
    confirmReductionCompleted(autoreductionGroup());
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

bool ReflRunsTabPresenter::isAutoreducing(int group) const {
  return isAutoreducing() && m_autoreduction.group() == group;
}

int ReflRunsTabPresenter::autoreductionGroup() const {
  return m_autoreduction.group();
}

bool ReflRunsTabPresenter::isProcessing(int group) const {
  //  return getTablePresenter(group)->isProcessing();
  return false;
}

bool ReflRunsTabPresenter::isProcessing() const {
  auto const numberOfGroups = static_cast<int>(m_tablePresenters.size());
  for (int group = 0; group < numberOfGroups; ++group) {
    if (isProcessing(group))
      return true;
  }
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

BatchPresenter *ReflRunsTabPresenter::getTablePresenter(int group) const {
  if (group < 0 || group > static_cast<int>(m_tablePresenters.size()))
    throw std::runtime_error("Invalid group number " + std::to_string(group));

  return m_tablePresenters.at(group).get();
}

int ReflRunsTabPresenter::selectedGroup() const {
  return m_view->getSelectedGroup();
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
    m_mainPresenter->giveUserCritical(
        "Error: Please select at least one run to transfer.",
        "No runs selected");
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
                                    int group, const TransferMatch matchType) {
  if (validateRowsToTransfer(rowsToTransfer)) {
    auto progress = setupProgressBar(rowsToTransfer);
    auto jobs = newJobsWithSlicingFrom(getTablePresenter(0)->reductionJobs());

    for (auto rowIndex : rowsToTransfer) {
      auto &result = (*m_searchModel)[rowIndex];
      auto resultMetadata = metadataFromDescription(result.description);
      auto row =
          validateRowFromRunAndTheta(jobs, m_workspaceNamesFactory,
                                     result.runNumber, resultMetadata.theta);
      if (row.is_initialized()) {
        mergeRowIntoGroup(jobs, row.get(), 0.001, resultMetadata.groupName,
                          m_workspaceNamesFactory);
      } else {
        m_searchModel->setError(rowIndex,
                                "Theta was not specified in the description.");
      }
    }

    getTablePresenter(0)->mergeAdditionalJobs(jobs);
  }
}

/** Used to tell the presenter something has changed in the ADS
*
* @param workspaceList :: the list of table workspaces in the ADS that could
*be
* loaded into the interface
* @param group :: the group that the notification came from
*/
void ReflRunsTabPresenter::notifyADSChanged(const QSet<QString> &workspaceList,
                                            int group) {

  UNUSED_ARG(workspaceList);

  // All groups pass on notifications about ADS changes. We only push commands
  // for the active group.
  if (group == selectedGroup())
    pushCommands(group);

  m_view->updateMenuEnabledState(isProcessing(group));
}

/** Requests global pre-processing options. Options are supplied by
  * the main presenter and there can be multiple sets of options for different
  * columns that need to be preprocessed.
  * @return :: A map of the column name to the global pre-processing options
  * for that column
  * the main presenter.
  * @return :: Global pre-processing options
  */
ColumnOptionsQMap
ReflRunsTabPresenter::getPreprocessingOptions(int group) const {
  ColumnOptionsQMap result;
  assert(m_mainPresenter != nullptr &&
         "The main presenter must be set with acceptMainPresenter.");

  // Note that there are no options for the Run(s) column so just add
  // Transmission Run(s)
  auto transmissionOptions =
      OptionsQMap(m_mainPresenter->getTransmissionOptions(group));
  result["Transmission Run(s)"] = transmissionOptions;

  return result;
}

/** Requests global processing options. Options are supplied by the main
* presenter
* @return :: Global processing options
*/
OptionsQMap ReflRunsTabPresenter::getProcessingOptions(int group) const {
  assert(m_mainPresenter != nullptr &&
         "The main presenter must be set with acceptMainPresenter.");
  return m_mainPresenter->getReductionOptions(group);
}

/** Requests global post-processing options as a string. Options are supplied
* by the main presenter
* @return :: Global post-processing options as a string
*/
QString
ReflRunsTabPresenter::getPostprocessingOptionsAsString(int group) const {
  return QString::fromStdString(m_mainPresenter->getStitchOptions(group));
}

/** Requests time-slicing values. Values are supplied by the main presenter
* @return :: Time-slicing values
*/
QString ReflRunsTabPresenter::getTimeSlicingValues(int group) const {
  return QString::fromStdString(m_mainPresenter->getTimeSlicingValues(group));
}

/** Requests time-slicing type. Type is supplied by the main presenter
* @return :: Time-slicing values
*/
QString ReflRunsTabPresenter::getTimeSlicingType(int group) const {
  return QString::fromStdString(m_mainPresenter->getTimeSlicingType(group));
}

/** Requests transmission runs for a particular run angle. Values are supplied
* by the main presenter
* @return :: Transmission run(s) as a comma-separated list
*/
OptionsQMap ReflRunsTabPresenter::getOptionsForAngle(const double angle,
                                                     int group) const {
  return m_mainPresenter->getOptionsForAngle(group, angle);
}

/** Check whether there are per-angle transmission runs in the settings
 * @return :: true if there are per-angle transmission runs
 */
bool ReflRunsTabPresenter::hasPerAngleOptions(int group) const {
  return m_mainPresenter->hasPerAngleOptions(group);
}

/** Tells the view to update the enabled/disabled state of all relevant
 *widgets
 * based on whether processing is in progress or not.
 *
 */
void ReflRunsTabPresenter::updateWidgetEnabledState() const {
  auto const processing = isProcessing();
  auto const autoreducing = isAutoreducing();
  auto const processingActiveGroup = isProcessing(selectedGroup());

  // Update the menus
  m_view->updateMenuEnabledState(processing);

  // Update components
  m_view->setTransferButtonEnabled(!processing && !autoreducing);
  m_view->setInstrumentComboEnabled(!processing && !autoreducing);
  m_view->setAutoreducePauseButtonEnabled(autoreducing);
  m_view->setSearchTextEntryEnabled(!autoreducing);
  m_view->setSearchButtonEnabled(!autoreducing);
  m_view->setAutoreduceButtonEnabled(!autoreducing && !processingActiveGroup);
}

/** Tells view to update the enabled/disabled state of all relevant widgets
 * based on the fact that processing is not in progress
*/
void ReflRunsTabPresenter::pause(int group) {
  if (m_autoreduction.pause(group)) {
    m_view->stopTimer();
    m_progressView->setAsPercentageIndicator();
  }

  // If processing has already finished, confirm reduction is paused;
  // otherwise
  // leave it to finish
  if (!isProcessing(group))
    confirmReductionPaused(group);
}

void ReflRunsTabPresenter::resume(int group) const { UNUSED_ARG(group); }

/** Notifies main presenter that data reduction is confirmed to be finished
* i.e. after all rows have been reduced
*/
void ReflRunsTabPresenter::confirmReductionCompleted(int group) {
  UNUSED_ARG(group);
  m_view->startTimer(10000);
}

/** Notifies main presenter that data reduction is confirmed to be paused
* via a user command to pause reduction
*/
void ReflRunsTabPresenter::confirmReductionPaused(int group) {
  updateWidgetEnabledState();
  m_mainPresenter->notifyReductionPaused(group);

  // We need to notify back to the table presenter to update the widget
  // state. This must be done from here otherwise there is no notification to
  // the table to update when autoprocessing is paused.

  //  if (!isAutoreducing(group))
  //    getTablePresenter(group)->confirmReductionPaused();
}

/** Notifies main presenter that data reduction is confirmed to be resumed
*/
void ReflRunsTabPresenter::confirmReductionResumed(int group) {
  updateWidgetEnabledState();
  m_mainPresenter->notifyReductionResumed(group);
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

void ReflRunsTabPresenter::changeGroup() {
  updateWidgetEnabledState();
  // Update the current menu commands based on the current group
  pushCommands(selectedGroup());
}
}
}
