#include "MantidQtCustomInterfaces/Reflectometry/ReflRunsTabPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCatalogSearcher.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflFromStdStringMap.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflLegacyTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflNexusMeasurementItemSource.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"

#include <QStringList>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {

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
    std::vector<DataProcessorPresenter *> tablePresenters,
    boost::shared_ptr<IReflSearcher> searcher)
    : m_view(mainView), m_progressView(progressableView),
      m_tablePresenters(tablePresenters), m_mainPresenter(),
      m_searcher(searcher), m_instrumentChanged(false) {

  // Register this presenter as the workspace receiver
  // When doing so, the inner presenters will notify this
  // presenter with the list of commands
  for (const auto &presenter : m_tablePresenters)
    presenter->accept(this);

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
  std::vector<std::string> instruments{
      {"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}};

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

/** Accept a main presenter
* @param mainPresenter :: [input] A main presenter
*/
void ReflRunsTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {

  m_mainPresenter = mainPresenter;
}

/**
Used by the view to tell the presenter something has changed
*/
void ReflRunsTabPresenter::notify(IReflRunsTabPresenter::Flag flag) {
  switch (flag) {
  case IReflRunsTabPresenter::SearchFlag:
    search();
    break;
  case IReflRunsTabPresenter::NewAutoreductionFlag:
    autoreduce(true);
    break;
  case IReflRunsTabPresenter::ResumeAutoreductionFlag:
    autoreduce(false);
    break;
  case IReflRunsTabPresenter::ICATSearchCompleteFlag: {
    auto algRunner = m_view->getAlgorithmRunner();
    IAlgorithm_sptr searchAlg = algRunner->getAlgorithm();
    populateSearch(searchAlg);
    break;
  }
  case IReflRunsTabPresenter::TransferFlag:
    transfer();
    break;
  case IReflRunsTabPresenter::InstrumentChangedFlag:
    changeInstrument();
    break;
  case IReflRunsTabPresenter::GroupChangedFlag:
    pushCommands();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/** Pushes the list of commands (actions) */
void ReflRunsTabPresenter::pushCommands() {

  m_view->clearCommands();

  // The expected number of commands
  const constexpr auto nCommands = 31u;
  auto commands =
      m_tablePresenters.at(m_view->getSelectedGroup())->publishCommands();
  if (commands.size() == nCommands) {
    // The index at which "row" commands start
    const constexpr auto rowCommStart = 10u;
    // We want to have two menus
    // Populate the "Reflectometry" menu
    std::vector<DataProcessorCommand_uptr> tableCommands;
    for (auto i = 0u; i < rowCommStart; i++)
      tableCommands.push_back(std::move(commands[i]));
    m_view->setTableCommands(std::move(tableCommands));
    // Populate the "Edit" menu
    std::vector<DataProcessorCommand_uptr> rowCommands;
    for (auto i = rowCommStart; i < nCommands; i++)
      rowCommands.push_back(std::move(commands[i]));
    m_view->setRowCommands(std::move(rowCommands));
  } else {
    throw std::runtime_error("Invalid list of commands");
  }
}

/** Searches for runs that can be used */
void ReflRunsTabPresenter::search() {
  auto const searchString = m_view->getSearchString();
  // Don't bother searching if they're not searching for anything
  if (searchString.empty())
    return;

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
    return;
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
}

/** Populates the search results table
* @param searchAlg : [input] The search algorithm
*/
void ReflRunsTabPresenter::populateSearch(IAlgorithm_sptr searchAlg) {
  if (searchAlg->isExecuted()) {
    ITableWorkspace_sptr results = searchAlg->getProperty("OutputWorkspace");
    m_instrumentChanged = false;
    m_currentTransferMethod = m_view->getTransferMethod();
    m_searchModel = ReflSearchModel_sptr(new ReflSearchModel(
        *getTransferStrategy(), results, m_view->getSearchInstrument()));
    m_view->showSearch(m_searchModel);
  }
}

/** Searches ICAT for runs with given instrument and investigation id, transfers
* runs to table and processes them
* @param startNew : Boolean on whether to start a new autoreduction
*/
void ReflRunsTabPresenter::autoreduce(bool startNew) {
  m_autoSearchString = m_view->getSearchString();
  auto tablePresenter = m_tablePresenters.at(m_view->getSelectedGroup());

  // If a new autoreduction is being made, we must remove all existing rows and
  // transfer the new ones (obtained by ICAT search) in
  if (startNew) {
    notify(IReflRunsTabPresenter::ICATSearchCompleteFlag);

    // Select all rows / groups in existing table and delete them
    tablePresenter->notify(DataProcessorPresenter::SelectAllFlag);
    tablePresenter->notify(DataProcessorPresenter::DeleteGroupFlag);

    // Select and transfer all rows to the table
    m_view->setAllSearchRowsSelected();
    if (m_view->getSelectedSearchRows().size() > 0)
      transfer();
  }

  tablePresenter->notify(DataProcessorPresenter::SelectAllFlag);
  if (tablePresenter->selectedParents().size() > 0)
    tablePresenter->notify(DataProcessorPresenter::ProcessFlag);
}

SearchResultMap ReflRunsTabPresenter::querySelectedRunsToTransfer(
    std::set<int> const &selectedRows) const {
  SearchResultMap runs;
  for (auto &&row : selectedRows) {
    const auto run = m_searchModel->data(m_searchModel->index(row, 0))
                         .toString()
                         .toStdString();
    auto resultDescription = m_searchModel->data(m_searchModel->index(row, 1))
                                 .toString()
                                 .toStdString();

    auto resultLocation = m_searchModel->data(m_searchModel->index(row, 2))
                              .toString()
                              .toStdString();
    runs[run] = SearchResult(resultDescription, resultLocation);
  }
  return runs;
}

/** Transfers the selected runs in the search results to the processing table
* @return : The runs to transfer as a vector of maps
*/
void ReflRunsTabPresenter::transfer() {
  // Build the input for the transfer strategy
  auto selectedRows = m_view->getSelectedSearchRows();

  // Do not begin transfer if nothing is selected or if the transfer method does
  // not match the one used for populating search
  if (selectedRows.size() == 0) {
    m_mainPresenter->giveUserCritical(
        "Error: Please select at least one run to transfer.",
        "No runs selected");
  } else if (m_currentTransferMethod != m_view->getTransferMethod()) {
    m_mainPresenter->giveUserCritical(
        "Error: Method selected for transferring runs (" +
            m_view->getTransferMethod() +
            ") must match the method used for searching runs (" +
            m_currentTransferMethod + ").",
        "Transfer method mismatch");
  } else {
    auto runs = querySelectedRunsToTransfer(selectedRows);
    ProgressPresenter progress(0, static_cast<double>(selectedRows.size()),
                               static_cast<int64_t>(selectedRows.size()),
                               this->m_progressView);
    TransferResults results =
        getTransferStrategy()->transferRuns(runs, progress);

    auto invalidRuns =
        results.getErrorRuns(); // grab our invalid runs from the transfer

    // iterate through invalidRuns to set the 'invalid transfers' in the search
    // model
    for (auto &error : invalidRuns) {
      // iterate over row containing run number and reason why it's invalid
      for (const auto &errorRowIt : error) {
        const auto &runNumber = errorRowIt.first; // grab run number

        // iterate over rows that are selected in the search table
        for (const auto row : selectedRows) {
          // get the run number from that selected row
          const auto searchRun =
              m_searchModel->data(m_searchModel->index(row, 0))
                  .toString()
                  .toStdString();
          if (searchRun == runNumber) { // if search run number is the same as
                                        // our invalid run number

            // add this error to the member of m_searchModel that holds errors.
            m_searchModel->m_errors.push_back(error);
          }
        }
      }
    }

    m_tablePresenters.at(m_view->getSelectedGroup())
        ->transfer(::MantidQt::CustomInterfaces::fromStdStringVectorMap(
            results.getTransferRuns()));
  }
}

/**
* Select and make a transfer strategy on demand based. Pick up the
* user-provided transfer strategy to do this.
* @return new TransferStrategy
*/
std::unique_ptr<ReflTransferStrategy>
ReflRunsTabPresenter::getTransferStrategy() {
  if (m_currentTransferMethod == MeasureTransferMethod) {

    // We need catalog info overrides from the user-based config service
    auto catalogConfigService =
        makeCatalogConfigServiceAdapter(ConfigService::Instance());

    // We make a user-based Catalog Info object for the transfer
    auto catalogInfo = make_unique<UserCatalogInfo>(
        ConfigService::Instance().getFacility().catalogInfo(),
        *catalogConfigService);

    // We are going to load from disk to pick up the meta data, so provide the
    // right repository to do this.
    auto source = make_unique<ReflNexusMeasurementItemSource>();

    // Finally make and return the Measure based transfer strategy.
    return Mantid::Kernel::make_unique<ReflMeasureTransferStrategy>(
        std::move(catalogInfo), std::move(source));
  } else if (m_currentTransferMethod == LegacyTransferMethod) {
    return make_unique<ReflLegacyTransferStrategy>();
  } else {
    throw std::runtime_error("Unknown tranfer method selected: " +
                             m_currentTransferMethod);
  }
}

/** Used to tell the presenter something has changed in the ADS
*
* @param workspaceList :: the list of table workspaces in the ADS that could be
* loaded into the interface
*/
void ReflRunsTabPresenter::notifyADSChanged(
    const QSet<QString> &workspaceList) {

  UNUSED_ARG(workspaceList);
  pushCommands();
}

/** Requests property names associated with pre-processing values.
* @return :: Pre-processing property names.
*/
QString ReflRunsTabPresenter::getPreprocessingProperties() const {
  return QString(
      "Transmission Run(s):FirstTransmissionRun,SecondTransmissionRun");
}

/** Requests global pre-processing options as a string. Options are supplied by
  * the main presenter.
  * @return :: Global pre-processing options
  */
QString ReflRunsTabPresenter::getPreprocessingOptionsAsString() const {

  return QString("Transmission Run(s),") +
         QString::fromStdString(
             m_mainPresenter->getTransmissionRuns(m_view->getSelectedGroup()));
}

/** Requests global processing options. Options are supplied by the main
* presenter
* @return :: Global processing options
*/
QString ReflRunsTabPresenter::getProcessingOptions() const {

  return QString::fromStdString(
      m_mainPresenter->getReductionOptions(m_view->getSelectedGroup()));
}

/** Requests global post-processing options. Options are supplied by the main
* presenter
* @return :: Global post-processing options
*/
QString ReflRunsTabPresenter::getPostprocessingOptions() const {

  return QString::fromStdString(
      m_mainPresenter->getStitchOptions(m_view->getSelectedGroup()));
}

/** Requests time-slicing values. Values are supplied by the main presenter
* @return :: Time-slicing values
*/
QString ReflRunsTabPresenter::getTimeSlicingValues() const {
  return QString::fromStdString(
      m_mainPresenter->getTimeSlicingValues(m_view->getSelectedGroup()));
}

/** Requests time-slicing type. Type is supplied by the main presenter
* @return :: Time-slicing values
*/
QString ReflRunsTabPresenter::getTimeSlicingType() const {
  return QString::fromStdString(
      m_mainPresenter->getTimeSlicingType(m_view->getSelectedGroup()));
}

/** Tells view to enable all 'process' buttons and disable the 'pause' button
* when data reduction is paused
*/
void ReflRunsTabPresenter::pause() {
  m_view->disableEditMenuAction(DataProcessorAction::PAUSE);
  m_view->autoreduceCannotBePressed();
}

/** Disables the 'process' button and enables the 'pause' button when data
 * reduction is resumed. Also notifies main presenter that data reduction is
 * confirmed to be resumed.
*/
void ReflRunsTabPresenter::resume() {
  m_view->disableEditMenuAction(DataProcessorAction::PROCESS);
  m_view->autoreduceWillPause();
  preventTableModification();

  m_mainPresenter->notify(
      IReflMainWindowPresenter::Flag::ConfirmReductionResumedFlag);
}

/** Notifies main presenter that data reduction is confirmed to be paused
*/
void ReflRunsTabPresenter::confirmReductionPaused() {
  m_mainPresenter->notify(
      IReflMainWindowPresenter::Flag::ConfirmReductionPausedFlag);
  m_view->enableEditMenuAction(DataProcessorAction::PROCESS);
  m_view->autoreduceWillReduce();
  allowTableModification();
}

const std::array<ReflectometryAction, 5>
    ReflRunsTabPresenter::disabledWhileProcessing = {
        {ReflectometryAction::OPEN_TABLE, ReflectometryAction::NEW_TABLE,
         ReflectometryAction::SAVE_TABLE, ReflectometryAction::SAVE_TABLE_AS,
         ReflectometryAction::IMPORT_TBL}};

void ReflRunsTabPresenter::preventTableModification() {
  m_view->disableTransfer();

  forEachTableModificationAction([this](DataProcessorAction action)
                               -> void { this->m_view->disableEditMenuAction(action); });
  for (auto reflectometryMenuAction : disabledWhileProcessing)
    m_view->disableReflectometryMenuAction(reflectometryMenuAction);
}

void ReflRunsTabPresenter::allowTableModification() {
  m_view->enableTransfer();

  forEachTableModificationAction([this](DataProcessorAction action)
                              -> void { this->m_view->enableEditMenuAction(action); });

  for (auto reflectometryMenuAction : disabledWhileProcessing)
    m_view->enableReflectometryMenuAction(reflectometryMenuAction);
}

/** Determines whether to start a new autoreduction. Starts a new one if the
* either the search number, transfer method or instrument has changed
* @return : Boolean on whether to start a new autoreduction
*/
bool ReflRunsTabPresenter::startNewAutoreduction() const {
  bool searchNumChanged = m_autoSearchString != m_view->getSearchString();
  bool transferMethodChanged =
      m_currentTransferMethod != m_view->getTransferMethod();

  return searchNumChanged || transferMethodChanged || m_instrumentChanged;
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
