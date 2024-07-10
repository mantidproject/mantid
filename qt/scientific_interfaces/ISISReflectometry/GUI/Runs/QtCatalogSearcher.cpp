// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtCatalogSearcher.h"
#include "GUI/Runs/IRunsView.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ISISInstrumentDataCache.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/QtAlgorithmRunner.h"

#include <algorithm>
#include <boost/regex.hpp>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Catalog Searcher");
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace { // unnamed
bool runHasCorrectInstrument(std::string const &run, std::string const &instrument) {
  // Return false if the run appears to be from another instruement
  return (run.substr(0, instrument.size()) == instrument);
}

std::string trimRunName(std::string const &runFile, std::string const &instrument) {
  // Trim the instrument prefix and ".raw" suffix
  auto run = runFile;
  run = run.substr(instrument.size(), run.size() - (instrument.size() + 4));

  // Also get rid of any leading zeros
  size_t numZeros = 0;
  while (run[numZeros] == '0')
    numZeros++;
  run = run.substr(numZeros, run.size() - numZeros);

  return run;
}

bool resultExists(SearchResult const &result, SearchResults const &runDetails) {
  auto resultIter = std::find(runDetails.cbegin(), runDetails.cend(), result);
  return resultIter != runDetails.cend();
}

bool knownFileType(std::string const &filename) {
  boost::regex pattern("raw$", boost::regex::icase);
  boost::smatch match; // Unused.
  return boost::regex_search(filename, match, pattern);
}
} // unnamed namespace

QtCatalogSearcher::QtCatalogSearcher(IRunsView *view)
    : m_view(view), m_notifyee(nullptr), m_searchCriteria(), m_searchInProgress(false) {
  m_view->subscribeSearch(this);
}

void QtCatalogSearcher::subscribe(SearcherSubscriber *notifyee) { m_notifyee = notifyee; }

SearchResults QtCatalogSearcher::search(SearchCriteria searchCriteria) {
  m_searchCriteria = std::move(searchCriteria);
  auto algSearch = createSearchAlgorithm();
  algSearch->execute();
  auto resultsTable = getSearchAlgorithmResultsTable(algSearch);
  auto searchResults = convertResultsTableToSearchResults(resultsTable);
  return searchResults;
}

ITableWorkspace_sptr QtCatalogSearcher::getSearchAlgorithmResultsTable(IAlgorithm_sptr algSearch) {
  ITableWorkspace_sptr resultsTable = algSearch->getProperty("OutputWorkspace");
  return resultsTable;
}

SearchResults QtCatalogSearcher::convertResultsTableToSearchResults(const ITableWorkspace_sptr &resultsTable) {
  auto searchResults = requiresICat() ? convertICatResultsTableToSearchResults(resultsTable)
                                      : convertJournalResultsTableToSearchResults(resultsTable);
  // If the archive is switched on, just return the whole set of results.
  auto const &archiveSetting = ConfigService::Instance().getString("datasearch.searcharchive");
  if (archiveSetting != "off") {
    return searchResults;
  }
  // Check if we're on IDAaaS with the Data Cache available.
  auto const &dataCache = ISISInstrumentDataCache(ConfigService::Instance().getString("datacachesearch.directory"));
  if (!dataCache.isIndexFileAvailable(searchCriteria().instrument)) {
    return searchResults;
  }
  // If so, only show the runs available in the instrument data cache.
  auto const &runNumbers = dataCache.getRunNumbersInCache(searchCriteria().instrument);
  searchResults.erase(std::remove_if(searchResults.begin(), searchResults.end(),
                                     [&](auto const &result) {
                                       return std::find(runNumbers.cbegin(), runNumbers.cend(), result.runNumber()) ==
                                              std::end(runNumbers);
                                     }),
                      searchResults.end());
  return searchResults;
}

SearchResults QtCatalogSearcher::convertICatResultsTableToSearchResults(const ITableWorkspace_sptr &tableWorkspace) {
  auto searchResults = SearchResults();
  searchResults.reserve(tableWorkspace->rowCount());

  for (size_t i = 0; i < tableWorkspace->rowCount(); ++i) {
    const std::string runFile = tableWorkspace->String(i, 0);

    if (!runHasCorrectInstrument(runFile, searchCriteria().instrument))
      continue;

    if (requiresICat() && !knownFileType(runFile))
      continue;

    auto const run = trimRunName(runFile, searchCriteria().instrument);
    const std::string description = tableWorkspace->String(i, 6);
    auto result = SearchResult(run, description);

    if (!resultExists(result, searchResults))
      searchResults.emplace_back(std::move(result));
  }
  return searchResults;
}

SearchResults QtCatalogSearcher::convertJournalResultsTableToSearchResults(const ITableWorkspace_sptr &tableWorkspace) {
  auto searchResults = SearchResults();
  searchResults.reserve(tableWorkspace->rowCount());

  for (size_t i = 0; i < tableWorkspace->rowCount(); ++i) {
    const std::string run = tableWorkspace->String(i, 1);
    const std::string description = tableWorkspace->String(i, 2);
    auto result = SearchResult(run, description);

    if (!resultExists(result, searchResults))
      searchResults.emplace_back(std::move(result));
  }
  return searchResults;
}

void QtCatalogSearcher::searchAsync() {
  auto algSearch = createSearchAlgorithm();
  auto algRunner = m_view->getAlgorithmRunner();
  algRunner->startAlgorithm(algSearch);
  m_searchInProgress = true;
}

bool QtCatalogSearcher::startSearchAsync(SearchCriteria searchCriteria) {
  m_searchCriteria = std::move(searchCriteria);

  // Check if ICat login is required
  if (!requiresICat() || hasActiveCatalogSession()) {
    searchAsync();
  } else {
    // Else attempt to login, once login is complete finishHandle will be
    // called.
    logInToCatalog();
  }

  return true;
}

void QtCatalogSearcher::finishHandle(const Mantid::API::IAlgorithm *alg) {
  stopObserving(alg);
  if (!hasActiveCatalogSession()) {
    m_notifyee->notifySearchFailed();
  } else {
    searchAsync();
  }
}

void QtCatalogSearcher::errorHandle(const Mantid::API::IAlgorithm *alg, const std::string &what) {
  UNUSED_ARG(what)
  stopObserving(alg);
  if (!hasActiveCatalogSession()) {
    m_notifyee->notifySearchFailed();
  }
}

void QtCatalogSearcher::notifySearchComplete() {
  m_searchInProgress = false;
  auto algRunner = m_view->getAlgorithmRunner();
  IAlgorithm_sptr searchAlg = algRunner->getAlgorithm();

  if (searchAlg->isExecuted()) {
    auto resultsTable = getSearchAlgorithmResultsTable(searchAlg);
    auto searchResults = convertResultsTableToSearchResults(resultsTable);
    results().mergeNewResults(searchResults);
  }

  m_notifyee->notifySearchComplete();
}

void QtCatalogSearcher::notifySearchResultsChanged() { results().setUnsaved(); }

bool QtCatalogSearcher::searchInProgress() const { return m_searchInProgress; }

SearchResult const &QtCatalogSearcher::getSearchResult(int index) const { return results().getRowData(index); }

void QtCatalogSearcher::reset() {
  results().clear();
  setSaved();
}

bool QtCatalogSearcher::hasUnsavedChanges() const { return results().hasUnsavedChanges(); }

void QtCatalogSearcher::setSaved() { results().setSaved(); }

SearchCriteria QtCatalogSearcher::searchCriteria() const { return m_searchCriteria; }

std::string QtCatalogSearcher::getSearchResultsCSV() const { return results().getSearchResultsCSV(); }

bool QtCatalogSearcher::hasActiveCatalogSession() const {
  auto sessions = CatalogManager::Instance().getActiveSessions();
  return !sessions.empty();
}

void QtCatalogSearcher::execLoginDialog(const IAlgorithm_sptr &alg) {
  API::InterfaceManager interfaceMgr;
  auto dlg = dynamic_cast<MantidQt::API::AlgorithmDialog *>(interfaceMgr.createDialog(alg));
  QObject::connect(dlg, SIGNAL(closeEventCalled()), this, SLOT(dialogClosed()));
  dlg->setModal(true);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

void QtCatalogSearcher::dialogClosed() {
  if (!hasActiveCatalogSession()) {
    m_notifyee->notifySearchFailed();
  }
}

/** Log in to the catalog
 * @returns : true if login succeeded
 */
void QtCatalogSearcher::logInToCatalog() {
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CatalogLogin");
  alg->initialize();
  alg->setProperty("KeepSessionAlive", true);
  this->observeFinish(alg);
  this->observeError(alg);
  execLoginDialog(alg);
}

std::string QtCatalogSearcher::activeSessionId() const {
  auto sessions = CatalogManager::Instance().getActiveSessions();
  if (sessions.empty())
    throw std::runtime_error("You are not logged into any catalogs.");

  return sessions.front()->getSessionId();
}

IAlgorithm_sptr QtCatalogSearcher::createSearchAlgorithm() {
  IAlgorithm_sptr algSearch;

  if (requiresICat()) {
    // Use ICat search
    auto sessionId = activeSessionId();
    algSearch = AlgorithmManager::Instance().create("CatalogGetDataFiles");
    algSearch->setProperty("Session", sessionId);
  } else {
    // Use journal search
    algSearch = AlgorithmManager::Instance().create("ISISJournalGetExperimentRuns");
    algSearch->setProperty("Instrument", searchCriteria().instrument);
    algSearch->setProperty("Cycle", searchCriteria().cycle);
  }

  algSearch->setProperty("InvestigationId", searchCriteria().investigation);
  algSearch->setProperty("OutputWorkspace", "_ReflSearchResults");
  algSearch->initialize();
  algSearch->setChild(true);
  algSearch->setLogging(false);

  return algSearch;
}

ISearchModel &QtCatalogSearcher::results() const { return m_view->mutableSearchResults(); }

/** Returns true if the search requires ICat, false otherwise. If the cycle is
 * given then we use the journal file search instead so ICat is not required.
 */
bool QtCatalogSearcher::requiresICat() const { return searchCriteria().cycle.empty(); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
