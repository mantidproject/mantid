#include "MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "MantidQtCustomInterfaces/ProgressableView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflTablePresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ProgressPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCatalogSearcher.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflLegacyTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflNexusMeasurementItemSource.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <sstream>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
namespace CustomInterfaces {
ReflMainViewPresenter::ReflMainViewPresenter(
    ReflMainView *mainView, IReflTablePresenter *tablePresenter,
    ProgressableView *progressView, boost::shared_ptr<IReflSearcher> searcher)
    : m_view(mainView), m_tablePresenter(tablePresenter),
      m_progressView(progressView), m_searcher(searcher) {

  // Register this presenter as the workspace receiver
  // When doing so, the inner presenter will notify this
  // presenter with the list of commands
  m_tablePresenter->accept(this);

  // TODO. Select strategy.
  /*
  std::unique_ptr<CatalogConfigService> catConfigService(
  makeCatalogConfigServiceAdapter(ConfigService::Instance()));
  UserCatalogInfo catalogInfo(
  ConfigService::Instance().getFacility().catalogInfo(), *catConfigService);
  */

  // If we don't have a searcher yet, use ReflCatalogSearcher
  if (!m_searcher)
    m_searcher.reset(new ReflCatalogSearcher());

  // Set the possible tranfer methods
  std::set<std::string> methods;
  methods.insert(LegacyTransferMethod);
  methods.insert(MeasureTransferMethod);
  m_view->setTransferMethods(methods);

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
    m_tablePresenter->setInstrumentList(instruments, defaultInst);
  } else {
    m_view->setInstrumentList(instruments, "INTER");
    m_tablePresenter->setInstrumentList(instruments, "INTER");
  }
}

ReflMainViewPresenter::~ReflMainViewPresenter() {}

/**
Used by the view to tell the presenter something has changed
*/
void ReflMainViewPresenter::notify(IReflPresenter::Flag flag) {
  switch (flag) {
  case IReflPresenter::SearchFlag:
    search();
    break;
  case IReflPresenter::ICATSearchCompleteFlag: {
    auto algRunner = m_view->getAlgorithmRunner();
    IAlgorithm_sptr searchAlg = algRunner->getAlgorithm();
    populateSearch(searchAlg);
  } break;
  case IReflPresenter::TransferFlag:
    transfer();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/** Pushes the list of commands (actions) */
void ReflMainViewPresenter::pushCommands() {

  m_view->clearCommands();

  // The expected number of commands
  const size_t nCommands = 26;
  auto commands = m_tablePresenter->publishCommands();
  if (commands.size() != nCommands) {
    throw std::runtime_error("Invalid list of commands");
  }
  // The index at which "row" commands start
  const size_t rowCommStart = 10;
  // We want to have two menus
  // Populate the "Reflectometry" menu
  std::vector<ReflCommand_uptr> tableCommands;
  for (size_t i = 0; i < rowCommStart; i++)
    tableCommands.push_back(std::move(commands[i]));
  m_view->setTableCommands(std::move(tableCommands));
  // Populate the "Edit" menu
  std::vector<ReflCommand_uptr> rowCommands;
  for (size_t i = rowCommStart; i < nCommands; i++)
    rowCommands.push_back(std::move(commands[i]));
  m_view->setRowCommands(std::move(rowCommands));
}

/** Searches for runs that can be used */
void ReflMainViewPresenter::search() {
  const std::string searchString = m_view->getSearchString();
  // Don't bother searching if they're not searching for anything
  if (searchString.empty())
    return;

  // This is breaking the abstraction provided by IReflSearcher, but provides a
  // nice usability win
  // If we're not logged into a catalog, prompt the user to do so
  if (CatalogManager::Instance().getActiveSessions().empty()) {
    try {
      m_view->showAlgorithmDialog("CatalogLogin");
    } catch (std::runtime_error &e) {
      m_view->giveUserCritical("Error Logging in:\n" + std::string(e.what()),
                               "login failed");
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
    m_view->giveUserInfo(
        "Error Logging in: Please press 'Search' to try again.",
        "Login Failed");
    return;
  }
  auto algSearch = AlgorithmManager::Instance().create("CatalogGetDataFiles");
  algSearch->initialize();
  algSearch->setChild(true);
  algSearch->setLogging(false);
  algSearch->setProperty("Session", sessionId);
  algSearch->setProperty("InvestigationId", searchString);
  algSearch->setProperty("OutputWorkspace", "_ReflSearchResults");
  auto algRunner = m_view->getAlgorithmRunner();
  algRunner->startAlgorithm(algSearch);
}

/** Populates the search results table
* @param searchAlg : [input] The search algorithm
*/
void ReflMainViewPresenter::populateSearch(IAlgorithm_sptr searchAlg) {
  if (searchAlg->isExecuted()) {
    ITableWorkspace_sptr results = searchAlg->getProperty("OutputWorkspace");
    m_searchModel = ReflSearchModel_sptr(new ReflSearchModel(
        *getTransferStrategy(), results, m_view->getSearchInstrument()));
    m_view->showSearch(m_searchModel);
  }
}

/** Transfers the selected runs in the search results to the processing table
* @return : The runs to transfer as a vector of maps
*/
void ReflMainViewPresenter::transfer() {
  // Build the input for the transfer strategy
  SearchResultMap runs;
  auto selectedRows = m_view->getSelectedSearchRows();

  for (auto rowIt = selectedRows.begin(); rowIt != selectedRows.end();
       ++rowIt) {
    const int row = *rowIt;
    const std::string run = m_searchModel->data(m_searchModel->index(row, 0))
                                .toString()
                                .toStdString();
    SearchResult searchResult;

    searchResult.description = m_searchModel->data(m_searchModel->index(row, 1))
                                   .toString()
                                   .toStdString();

    searchResult.location = m_searchModel->data(m_searchModel->index(row, 2))
                                .toString()
                                .toStdString();
    runs[run] = searchResult;
  }

  ProgressPresenter progress(0, static_cast<double>(selectedRows.size()),
                             static_cast<int64_t>(selectedRows.size()),
                             this->m_progressView);

  TransferResults results = getTransferStrategy()->transferRuns(runs, progress);

  auto invalidRuns =
      results.getErrorRuns(); // grab our invalid runs from the transfer

  // iterate through invalidRuns to set the 'invalid transfers' in the search
  // model
  if (!invalidRuns.empty()) { // check if we have any invalid runs
    for (auto invalidRowIt = invalidRuns.begin();
         invalidRowIt != invalidRuns.end(); ++invalidRowIt) {
      auto &error = *invalidRowIt; // grab row from vector
      // iterate over row containing run number and reason why it's invalid
      for (auto errorRowIt = error.begin(); errorRowIt != error.end();
           ++errorRowIt) {
        const std::string runNumber = errorRowIt->first; // grab run number

        // iterate over rows that are selected in the search table
        for (auto rowIt = selectedRows.begin(); rowIt != selectedRows.end();
             ++rowIt) {
          const int row = *rowIt;
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
  }

  m_tablePresenter->transfer(results.getTransferRuns());
}

/**
* Select and make a transfer strategy on demand based. Pick up the
* user-provided transfer strategy to do this.
* @return new TransferStrategy
*/
std::unique_ptr<ReflTransferStrategy>
ReflMainViewPresenter::getTransferStrategy() {
  const std::string currentMethod = m_view->getTransferMethod();
  std::unique_ptr<ReflTransferStrategy> rtnStrategy;
  if (currentMethod == MeasureTransferMethod) {

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
  } else if (currentMethod == LegacyTransferMethod) {
    rtnStrategy = make_unique<ReflLegacyTransferStrategy>();
    return rtnStrategy;
  } else {
    throw std::runtime_error("Unknown tranfer method selected: " +
                             currentMethod);
  }
}

/**
Used to tell the presenter something has changed in the ADS
*/
void ReflMainViewPresenter::notify(WorkspaceReceiver::Flag flag) {

  switch (flag) {
  case WorkspaceReceiver::ADSChangedFlag:
    pushCommands();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

const std::string ReflMainViewPresenter::MeasureTransferMethod = "Measurement";
const std::string ReflMainViewPresenter::LegacyTransferMethod = "Description";
}
}