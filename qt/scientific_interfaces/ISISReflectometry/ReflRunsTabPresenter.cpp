// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

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
} // unnamed namespace

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
      m_searcher(searcher), m_instrumentChanged(false) {
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

  updateViewWhenMonitorStopped();
}

ReflRunsTabPresenter::~ReflRunsTabPresenter() {
  if (m_monitorAlg)
    stopObserving(m_monitorAlg);
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
  case IReflRunsTabPresenter::StartMonitorFlag:
    startMonitor();
    break;
  case IReflRunsTabPresenter::StopMonitorFlag:
    stopMonitor();
    break;
  case IReflRunsTabPresenter::StartMonitorCompleteFlag:
    startMonitorComplete();
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
  auto commands = getTablePresenter(group)->publishCommands();
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

  if (shouldUpdateExistingSearchResults()) {
    m_searchModel->addDataFromTable(*getTransferStrategy(), results,
                                    m_view->getSearchInstrument());
  } else {
    // Create a new search results list and display it on the view
    m_searchModel = boost::make_shared<ReflSearchModel>(
        *getTransferStrategy(), results, m_view->getSearchInstrument());
    m_view->showSearch(m_searchModel);
  }
}

/** Searches ICAT for runs with given instrument and investigation id, transfers
 * runs to table and processes them. Clears any existing table data first.
 */
void ReflRunsTabPresenter::startNewAutoreduction() {

  auto const selected = selectedGroup();

  if (requireNewAutoreduction()) {
    // If starting a brand new autoreduction, delete all rows / groups in
    // existing table first
    // We'll prompt the user to check it's ok to delete existing rows
    auto tablePresenter = getTablePresenter(selected);
    tablePresenter->setPromptUser(false);
    try {
      tablePresenter->notify(DataProcessorPresenter::DeleteAllFlag);
    } catch (const DataProcessorPresenter::DeleteAllRowsCancelledException &) {
      return;
    }
  }

  if (setupNewAutoreduction(selected, m_view->getSearchString()))
    checkForNewRuns();
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
    transfer(rowsToTransfer, autoreductionGroup(), TransferMatch::Strict);
    auto tablePresenter = getTablePresenter(autoreductionGroup());
    tablePresenter->setPromptUser(false);
    tablePresenter->notify(DataProcessorPresenter::ProcessAllFlag);
  } else {
    confirmReductionCompleted(autoreductionGroup());
  }
}

void ReflRunsTabPresenter::pauseAutoreduction() {
  if (isAutoreducing())
    getTablePresenter(autoreductionGroup())
        ->notify(DataProcessorPresenter::PauseFlag);
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
  return getTablePresenter(group)->isProcessing();
}

bool ReflRunsTabPresenter::isProcessing() const {
  auto const numberOfGroups = static_cast<int>(m_tablePresenters.size());
  for (int groupIndex = 0; groupIndex < numberOfGroups; ++groupIndex) {
    if (isProcessing(groupIndex))
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

DataProcessorPresenter *
ReflRunsTabPresenter::getTablePresenter(int group) const {
  if (group < 0 || group > static_cast<int>(m_tablePresenters.size()))
    throw std::runtime_error("Invalid group number " + std::to_string(group));

  return m_tablePresenters.at(group);
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

  SearchResultMap foundRunDetails;
  for (const auto &row : rowsToTransfer) {
    const auto foundRun = searchModelData(row, 0);
    const auto description = searchModelData(row, 1);
    const auto location = searchModelData(row, 2);
    foundRunDetails[foundRun] = SearchResult{description, location};
  }

  return foundRunDetails;
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

  if (isAutoreducing())
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
  auto transferableRunDetails = getSearchResultRunDetails(rowsToTransfer);

  // Apply the transfer strategy
  TransferResults transferDetails = getTransferStrategy()->transferRuns(
      transferableRunDetails, progress, matchType);

  // Handle any runs that cannot be transferred
  updateErrorStateInSearchModel(rowsToTransfer, transferDetails.getErrorRuns());

  // Do the transfer
  getTablePresenter(group)->transfer(
      ::MantidQt::CustomInterfaces::fromStdStringVectorMap(
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
  m_view->setTransferMethodComboEnabled(!autoreducing);
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

  // If processing has already finished, confirm reduction is paused; otherwise
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
  if (!isAutoreducing(group))
    getTablePresenter(group)->confirmReductionPaused();
}

/** Notifies main presenter that data reduction is confirmed to be resumed
 */
void ReflRunsTabPresenter::confirmReductionResumed(int group) {
  updateWidgetEnabledState();
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

void ReflRunsTabPresenter::changeGroup() {
  updateWidgetEnabledState();
  // Update the current menu commands based on the current group
  pushCommands(selectedGroup());
}

void ReflRunsTabPresenter::handleError(const std::string &message,
                                       const std::exception &e) {
  m_mainPresenter->giveUserCritical(message + ": " + std::string(e.what()),
                                    "Error");
}

void ReflRunsTabPresenter::handleError(const std::string &message) {
  m_mainPresenter->giveUserCritical(message, "Error");
}

std::string ReflRunsTabPresenter::liveDataReductionAlgorithm() {
  return "ReflectometryReductionOneLiveData";
}

std::string
ReflRunsTabPresenter::liveDataReductionOptions(const std::string &instrument) {
  // Get the properties for the reduction algorithm from the settings tab. We
  // don't have a group associated with live data. This is not ideal but for
  // now just use the first group.
  auto options = convertOptionsFromQMap(getProcessingOptions(0));
  // Add other required input properties to the live data reduction algorithnm
  options["Instrument"] = QString::fromStdString(instrument);
  options["GetLiveValueAlgorithm"] = "GetLiveInstrumentValue";
  // Convert the properties to a string to pass to the algorithm
  auto const optionsString =
      convertMapToString(options, ';', false).toStdString();
  return optionsString;
}

IAlgorithm_sptr ReflRunsTabPresenter::setupLiveDataMonitorAlgorithm() {
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

void ReflRunsTabPresenter::updateViewWhenMonitorStarting() {
  m_view->setStartMonitorButtonEnabled(false);
  m_view->setStopMonitorButtonEnabled(false);
}

void ReflRunsTabPresenter::updateViewWhenMonitorStarted() {
  m_view->setStartMonitorButtonEnabled(false);
  m_view->setStopMonitorButtonEnabled(true);
}

void ReflRunsTabPresenter::updateViewWhenMonitorStopped() {
  m_view->setStartMonitorButtonEnabled(true);
  m_view->setStopMonitorButtonEnabled(false);
}

/** Start live data monitoring
 */
void ReflRunsTabPresenter::startMonitor() {
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
void ReflRunsTabPresenter::startMonitorComplete() {
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
void ReflRunsTabPresenter::stopMonitor() {
  if (!m_monitorAlg)
    return;

  stopObserving(m_monitorAlg);
  m_monitorAlg->cancel();
  m_monitorAlg.reset();
  updateViewWhenMonitorStopped();
}

/** Handler called when the monitor algorithm finishes
 */
void ReflRunsTabPresenter::finishHandle(const IAlgorithm *alg) {
  UNUSED_ARG(alg);
  stopObserving(m_monitorAlg);
  m_monitorAlg.reset();
  updateViewWhenMonitorStopped();
}

/** Handler called when the monitor algorithm errors
 */
void ReflRunsTabPresenter::errorHandle(const IAlgorithm *alg,
                                       const std::string &what) {
  UNUSED_ARG(alg);
  UNUSED_ARG(what);
  stopObserving(m_monitorAlg);
  m_monitorAlg.reset();
  updateViewWhenMonitorStopped();
}

const std::string ReflRunsTabPresenter::MeasureTransferMethod = "Measurement";
const std::string ReflRunsTabPresenter::LegacyTransferMethod = "Description";
} // namespace CustomInterfaces
} // namespace MantidQt
