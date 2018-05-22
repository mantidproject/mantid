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
#include "ReflLegacyTransferStrategy.h"
#include "ReflMeasureTransferStrategy.h"
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

/** Get the error message associated with the given run
 * @param run : the run number as a string
 * @param invalidRuns : the list of invalid runs as a map of description
 * to error message, where the description may contain a list of run numbers
 * separated by a '+' character
 */
std::string getRunErrorMessage(
    const std::string &searchRun,
    const std::vector<TransferResults::COLUMN_MAP_TYPE> &invalidRuns) {

  // Loop through the list of invalid rows
  for (auto row : invalidRuns) {
    // Loop through all entries in the error map for this row
    for (auto errorPair : row) {
      // Extract the run numbers for this row
      auto const runNumbers = errorPair.first;
      StringTokenizer tokenizer(runNumbers, "+", StringTokenizer::TOK_TRIM);
      auto const runList = tokenizer.asVector();

      // If the requested run is in the list, return the error message
      if (std::find(runList.begin(), runList.end(), searchRun) != runList.end())
        return errorPair.second;
    }
  }

  return std::string();
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
    std::vector<DataProcessorPresenter *> tablePresenters,
    boost::shared_ptr<IReflSearcher> searcher)
    : m_view(mainView), m_progressView(progressableView),
      m_tablePresenters(tablePresenters), m_mainPresenter(nullptr),
      m_searcher(searcher), m_instrumentChanged(false), m_autoreduction() {
  assert(m_view != nullptr);

  // If we don't have a searcher yet, use ReflCatalogSearcher
  if (!m_searcher)
    m_searcher.reset(new ReflCatalogSearcher());

  // Set the possible tranfer methods
  std::set<std::string> methods;
  methods.insert(LegacyTransferMethod);
  methods.insert(MeasureTransferMethod);
  m_view->setTransferMethods(methods);

  // Set current transfer method
  m_currentTransferMethod = m_view->getTransferMethod();

  // Set up the instrument selectors
  std::vector<std::string> instruments;
  instruments.emplace_back("INTER");
  instruments.emplace_back("SURF");
  instruments.emplace_back("CRISP");
  instruments.emplace_back("POLREF");
  instruments.emplace_back("OFFSPEC");

  // If the user's configured default instrument is in this list, set it as the
  // default, otherwise use INTER
  const std::string defaultInst =
      Mantid::Kernel::ConfigService::Instance().getString("default.instrument");
  if (std::find(instruments.begin(), instruments.end(), defaultInst) !=
      instruments.end()) {
    m_view->setInstrumentList(instruments, defaultInst);
    for (const auto &presenter : m_tablePresenters)
      presenter->setInstrumentList(fromStdStringVector(instruments),
                                   QString::fromStdString(defaultInst));
  } else {
    m_view->setInstrumentList(instruments, "INTER");
    for (const auto &presenter : m_tablePresenters)
      presenter->setInstrumentList(fromStdStringVector(instruments), "INTER");
  }
}

ReflRunsTabPresenter::~ReflRunsTabPresenter() {}

/** Accept a main presenter
* @param mainPresenter :: [input] A main presenter
*/
void ReflRunsTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
  // Register this presenter as the workspace receiver
  // When doing so, the inner presenters will notify this
  // presenter with the list of commands

  for (const auto &presenter : m_tablePresenters)
    presenter->accept(this);
  // Note this must be done here since notifying the gdpp of its view
  // will cause it to request settings only accessible via the main
  // presenter.
}

void ReflRunsTabPresenter::settingsChanged(int group) {
  assert(static_cast<std::size_t>(group) < m_tablePresenters.size());
  m_tablePresenters[group]->settingsChanged();
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
      m_autoreduction.stop();
    break;
  case IReflRunsTabPresenter::StartAutoreductionFlag:
    startNewAutoreduction();
    break;
  case IReflRunsTabPresenter::PauseAutoreductionFlag:
    pauseAutoreduction();
    break;
  case IReflRunsTabPresenter::TimerEventFlag:
    startAutoreduction();
    break;
  case IReflRunsTabPresenter::ICATSearchCompleteFlag: {
    icatSearchComplete();
    break;
  }
  case IReflRunsTabPresenter::TransferFlag:
    transfer(m_view->getSelectedSearchRows(), m_view->getSelectedGroup(),
             TransferMatch::Any);
    break;
  case IReflRunsTabPresenter::InstrumentChangedFlag:
    changeInstrument();
    break;
  case IReflRunsTabPresenter::GroupChangedFlag:
    pushCommands(m_view->getSelectedGroup());
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
void ReflRunsTabPresenter::pushCommands(int group) {

  m_view->clearCommands();

  // The expected number of commands
  const size_t nCommands = 31;
  auto commands = m_tablePresenters.at(group)->publishCommands();
  if (commands.size() != nCommands) {
    throw std::runtime_error("Invalid list of commands");
  }
  // The index at which "row" commands start
  const size_t rowCommStart = 10u;
  // We want to have two menus
  // Populate the "Reflectometry" menu
  std::vector<MantidWidgets::DataProcessor::Command_uptr> tableCommands;
  for (size_t i = 0; i < rowCommStart; i++)
    tableCommands.push_back(std::move(commands[i]));
  m_view->setTableCommands(std::move(tableCommands));
  // Populate the "Edit" menu
  std::vector<MantidWidgets::DataProcessor::Command_uptr> rowCommands;
  for (size_t i = rowCommStart; i < nCommands; i++)
    rowCommands.push_back(std::move(commands[i]));
  m_view->setRowCommands(std::move(rowCommands));
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
  m_currentTransferMethod = m_view->getTransferMethod();

  if (m_searchModel && autoreductionRunning() &&
      m_autoreduction.searchResultsExist()) {
    // We're continuing an existing autoreduction process. Just update the
    // existing search results list with any new runs
    m_searchModel->addDataFromTable(*getTransferStrategy(), results,
                                    m_view->getSearchInstrument());
  } else {
    // Create a new search results list and display it on the view
    m_searchModel = ReflSearchModel_sptr(new ReflSearchModel(
        *getTransferStrategy(), results, m_view->getSearchInstrument()));
    m_view->showSearch(m_searchModel);
  }
}

/** Searches ICAT for runs with given instrument and investigation id, transfers
* runs to table and processes them. Clears any existing table data first.
*/
void ReflRunsTabPresenter::startNewAutoreduction() {

  if (requireNewAutoreduction()) {
    // If starting a brand new autoreduction, delete all rows / groups in
    // existing table first
    // We'll prompt the user to check it's ok to delete existing rows
    auto tablePresenter = m_tablePresenters.at(m_view->getSelectedGroup());
    tablePresenter->setPromptUser(false);
    try {
      tablePresenter->notify(DataProcessorPresenter::DeleteAllFlag);
    } catch (const std::runtime_error &e) {
      // If the user cancelled the deletion, don't start autoreduction
      return;
    }
  }

  if (m_autoreduction.start(m_view->getSelectedGroup(),
                            m_view->getSearchString()))
    startAutoreduction();
}

/** Start a single autoreduction process. Called periodially to add and process
 *  any new runs in the table.
 */
void ReflRunsTabPresenter::startAutoreduction() {

  // Stop any more notifications during processing
  m_view->stopTimer();

  // Initially we just need to start an ICat search and the reduction will be
  // run when the search completes
  m_view->startIcatSearch();
}

/** Called when the user clicks the pause-autoreduction button
 */
void ReflRunsTabPresenter::pauseAutoreduction() {
  // The pause-autoprocess button does exactly the same as the pause button on
  // the data processor, so we just notify the data processor to pause. We
  // allow this button to be used to pause processing started manually as well
  // as auto-processing - we use the active group to pause manual processing.
  int group = 0;
  if (autoreductionRunning())
    group = m_autoreduction.group();
  else
    group = m_view->getSelectedGroup();

  m_tablePresenters.at(group)->notify(DataProcessorPresenter::PauseFlag);
}

void ReflRunsTabPresenter::icatSearchComplete() {
  // Populate the search results
  auto algRunner = m_view->getAlgorithmRunner();
  IAlgorithm_sptr searchAlg = algRunner->getAlgorithm();
  populateSearch(searchAlg);

  // If autoreduction is running, perform the next reduction using the new
  // search results
  if (autoreductionRunning()) {
    m_autoreduction.setSearchResultsExist();
    runAutoreduction();
  }
}

/** Run an autoreduction process based on the latest search results
 */
void ReflRunsTabPresenter::runAutoreduction() {
  // Transfer all of the search results to the table (this excludes any that
  // already exist so will only add new ones)
  auto rowsToTransfer = m_view->getAllSearchRows();
  const auto group = m_autoreduction.group();
  auto tablePresenter = m_tablePresenters.at(group);

  if (rowsToTransfer.size() > 0) {
    transfer(rowsToTransfer, m_autoreduction.group(), TransferMatch::Strict);
  }

  // Don't prompt the user on errors such as an empty table
  tablePresenter->setPromptUser(false);

  // Process all rows in the table
  tablePresenter->notify(DataProcessorPresenter::ProcessAllFlag);
}

/** Check whether autoreduction is running for any group
 */
bool ReflRunsTabPresenter::autoreductionRunning() const {
  return m_autoreduction.running();
}

/** Check whether autoreduction is running for a specific group
 */
bool ReflRunsTabPresenter::autoreductionRunning(int group) const {
  return autoreductionRunning() && m_autoreduction.group() == group;
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

  // Check that the transfer method matches the one used for populating the
  // search
  if (m_currentTransferMethod != m_view->getTransferMethod()) {
    m_mainPresenter->giveUserCritical(
        "Error: Method selected for transferring runs (" +
            m_view->getTransferMethod() +
            ") must match the method used for searching runs (" +
            m_currentTransferMethod + ").",
        "Transfer method mismatch");
    return false;
  }

  return true;
}

/** Get the data for a cell in the search results model as a string
 */
std::string ReflRunsTabPresenter::searchModelData(const int row,
                                                  const int column) {
  return m_searchModel->data(m_searchModel->index(row, column))
      .toString()
      .toStdString();
}

/** Get the details of runs to transfer from the search results table
 * @param rowsToTransfer : a set of row indices
 * @return : a map of run name to a SearchResult struct containing details
 * for that run
 */
SearchResultMap ReflRunsTabPresenter::getSearchResultRunDetails(
    const std::set<int> &rowsToTransfer) {

  SearchResultMap runDetails;
  for (const auto &row : rowsToTransfer) {
    const auto run = searchModelData(row, 0);
    const auto description = searchModelData(row, 1);
    const auto location = searchModelData(row, 2);
    runDetails[run] = SearchResult{description, location};
  }

  return runDetails;
}

/** Iterate through the rows to transfer and set/clear the error state
 * in the search results model
 * @param rowsToTransfer : row indices of all rows to transfer
 * @param invalidRuns : details of runs that are invalid
 */
void ReflRunsTabPresenter::updateErrorStateInSearchModel(
    const std::set<int> &rowsToTransfer,
    const std::vector<TransferResults::COLUMN_MAP_TYPE> &invalidRuns) {

  // The run number is in column 0 in the search results table
  int const columnIndex = 0;

  // Loop through all the rows we want to transfer
  for (auto rowIndex : rowsToTransfer) {
    auto const runToTransfer = searchModelData(rowIndex, columnIndex);
    auto const errorMessage = getRunErrorMessage(runToTransfer, invalidRuns);

    // Set or clear the error in the model for this run
    if (errorMessage.empty())
      m_searchModel->clearError(runToTransfer);
    else
      m_searchModel->addError(runToTransfer, errorMessage);
  }
}

/** Set up the progress bar
 */
ProgressPresenter
ReflRunsTabPresenter::setupProgressBar(const std::set<int> &rowsToTransfer) {

  auto start = double(0.0);
  auto end = static_cast<double>(rowsToTransfer.size());
  auto nsteps = static_cast<int64_t>(rowsToTransfer.size());
  auto progress = ProgressPresenter(start, end, nsteps, this->m_progressView);

  if (autoreductionRunning())
    progress.setAsEndlessIndicator();
  else
    progress.setAsPercentageIndicator();

  return progress;
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
  if (!validateRowsToTransfer(rowsToTransfer))
    return;

  auto progress = setupProgressBar(rowsToTransfer);

  // Extract details of runs to transfer
  auto runDetails = getSearchResultRunDetails(rowsToTransfer);

  // Apply the transfer strategy
  TransferResults transferDetails =
      getTransferStrategy()->transferRuns(runDetails, progress, matchType);

  // Handle any runs that cannot be transferred
  updateErrorStateInSearchModel(rowsToTransfer, transferDetails.getErrorRuns());

  // Do the transfer
  m_tablePresenters.at(group)
      ->transfer(::MantidQt::CustomInterfaces::fromStdStringVectorMap(
          transferDetails.getTransferRuns()));
}

/**
* Select and make a transfer strategy on demand based. Pick up the
* user-provided transfer strategy to do this.
* @return new TransferStrategy
*/
std::unique_ptr<ReflTransferStrategy>
ReflRunsTabPresenter::getTransferStrategy() {
  std::unique_ptr<ReflTransferStrategy> rtnStrategy;
  if (m_currentTransferMethod == MeasureTransferMethod) {

    // We need catalog info overrides from the user-based config service
    std::unique_ptr<CatalogConfigService> catConfigService(
        makeCatalogConfigServiceAdapter(ConfigService::Instance()));

    // We make a user-based Catalog Info object for the transfer
    std::unique_ptr<ICatalogInfo> catInfo = make_unique<UserCatalogInfo>(
        ConfigService::Instance().getFacility().catalogInfo(),
        *catConfigService);

    // We are going to load from disk to pick up the meta data, so provide the
    // right repository to do this.
    std::unique_ptr<ReflMeasurementItemSource> source =
        make_unique<ReflNexusMeasurementItemSource>();

    // Finally make and return the Measure based transfer strategy.
    rtnStrategy = Mantid::Kernel::make_unique<ReflMeasureTransferStrategy>(
        std::move(catInfo), std::move(source));
    return rtnStrategy;
  } else if (m_currentTransferMethod == LegacyTransferMethod) {
    rtnStrategy = make_unique<ReflLegacyTransferStrategy>();
    return rtnStrategy;
  } else {
    throw std::runtime_error("Unknown tranfer method selected: " +
                             m_currentTransferMethod);
  }
}

/** Used to tell the presenter something has changed in the ADS
*
* @param workspaceList :: the list of table workspaces in the ADS that could be
* loaded into the interface
* @param group :: the group that the notification came from
*/
void ReflRunsTabPresenter::notifyADSChanged(const QSet<QString> &workspaceList,
                                            int group) {

  UNUSED_ARG(workspaceList);
  pushCommands(group);
  m_view->updateMenuEnabledState(m_tablePresenters.at(group)->isProcessing());
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

/** Requests global post-processing options as a string. Options are supplied by
* the main
* presenter
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

/** Tells the view to update the enabled/disabled state of all relevant widgets
 * based on whether processing is in progress or not.
 * @param isProcessing :: true if processing is in progress
 *
 */
void ReflRunsTabPresenter::updateWidgetEnabledState(
    const bool isProcessing) const {
  // Update the menus
  m_view->updateMenuEnabledState(isProcessing);

  // Update specific buttons
  m_view->setAutoreduceButtonEnabled(!isProcessing);
  m_view->setAutoreducePauseButtonEnabled(isProcessing);
  m_view->setTransferButtonEnabled(!isProcessing);
  m_view->setInstrumentComboEnabled(!isProcessing);

  // These components are always enabled unless autoreduction is running
  m_view->setTransferMethodComboEnabled(!autoreductionRunning());
  m_view->setSearchTextEntryEnabled(!autoreductionRunning());
  m_view->setSearchButtonEnabled(!autoreductionRunning());
}

/** Tells view to update the enabled/disabled state of all relevant widgets
 * based on the fact that processing is not in progress
*/
void ReflRunsTabPresenter::pause(int group) {
  if (!m_autoreduction.stop(group))
    return;

  m_view->stopTimer();
  updateWidgetEnabledState(false);
  m_progressView->setAsPercentageIndicator();

  // We get here in two scenarios: processing is still running, in which case
  // do not confirm reduction has paused yet (confirmReductionPaused will be
  // called when reduction is finished); and when processing is finished but
  // autoreduction is in progress, in which case we need to confirm reduction
  // has paused now because confirmReductionPaused will not be called.
  if (!m_mainPresenter->checkIfProcessing(group))
    m_mainPresenter->notifyReductionPaused(group);
}

/** Tells view to update the enabled/disabled state of all relevant widgets
 * based on the fact that processing is in progress
*/
void ReflRunsTabPresenter::resume(int group) const {
  UNUSED_ARG(group);
  updateWidgetEnabledState(true);
}

/** Determines whether to start a new autoreduction. Starts a new one if the
* either the search number, transfer method or instrument has changed
* @return : Boolean on whether to start a new autoreduction
*/
bool ReflRunsTabPresenter::requireNewAutoreduction() const {
  bool searchNumChanged =
      m_autoreduction.searchStringChanged(m_view->getSearchString());
  bool transferMethodChanged =
      m_currentTransferMethod != m_view->getTransferMethod();

  return searchNumChanged || transferMethodChanged || m_instrumentChanged;
}

/** Notifies main presenter that data reduction is confirmed to be finished
* i.e. after all rows have been reduced
*/
void ReflRunsTabPresenter::confirmReductionFinished(int group) {
  m_mainPresenter->notifyReductionFinished(group);

  // Start a timer to re-run autoreduction periodically
  m_view->startTimer(1000);
}

/** Notifies main presenter that data reduction is confirmed to be paused
* via a user command to pause reduction
*/
void ReflRunsTabPresenter::confirmReductionPaused(int group) {
  m_mainPresenter->notifyReductionPaused(group);
}

/** Notifies main presenter that data reduction is confirmed to be resumed
*/
void ReflRunsTabPresenter::confirmReductionResumed(int group) {
  m_mainPresenter->notifyReductionResumed(group);
}

/** Changes the current instrument in the data processor widget. Also clears the
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

const std::string ReflRunsTabPresenter::MeasureTransferMethod = "Measurement";
const std::string ReflRunsTabPresenter::LegacyTransferMethod = "Description";
}
}
