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
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

#include <boost/regex.hpp>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

namespace { // unnamed
void removeResultsWithoutFilenameExtension(
    const ITableWorkspace_sptr &results) {
  std::set<size_t> toRemove;
  for (size_t i = 0; i < results->rowCount(); ++i) {
    std::string &run = results->String(i, 0);

    // Too short to be more than ".raw or .nxs"
    if (run.size() < 5) {
      toRemove.insert(i);
    }
  }

  // Sets are sorted so if we go from back to front we won't trip over ourselves
  for (auto row = toRemove.rbegin(); row != toRemove.rend(); ++row)
    results->removeRow(*row);
}

bool runHasCorrectInstrument(std::string const &run,
                             std::string const &instrument) {
  // Return false if the run appears to be from another instruement
  return (run.substr(0, instrument.size()) == instrument);
}

std::string trimRunName(std::string const &runFile,
                        std::string const &instrument) {
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
    : m_view(view), m_notifyee(nullptr), m_searchText(), m_instrument(),
      m_cycle(), m_searchType(SearchType::NONE), m_searchInProgress(false) {
  m_view->subscribeSearch(this);
}

void QtCatalogSearcher::subscribe(SearcherSubscriber *notifyee) {
  m_notifyee = notifyee;
}

SearchResults QtCatalogSearcher::search(const std::string &text,
                                        const std::string &instrument,
                                        const std::string &cycle,
                                        ISearcher::SearchType searchType) {
  m_searchText = text;
  m_instrument = instrument;
  m_cycle = cycle;
  m_searchType = searchType;
  auto algSearch = createSearchAlgorithm();
  algSearch->execute();
  ITableWorkspace_sptr resultsTable = algSearch->getProperty("OutputWorkspace");
  removeResultsWithoutFilenameExtension(resultsTable);
  auto searchResults = convertTableToSearchResults(resultsTable);
  return searchResults;
}

SearchResults QtCatalogSearcher::convertTableToSearchResults(
    ITableWorkspace_sptr tableWorkspace) {
  if (requiresICat())
    return convertICatResultsTableToSearchResults(tableWorkspace);
  else
    return convertJournalResultsTableToSearchResults(tableWorkspace);
}

SearchResults QtCatalogSearcher::convertICatResultsTableToSearchResults(
    ITableWorkspace_sptr tableWorkspace) {
  auto searchResults = SearchResults();
  searchResults.reserve(tableWorkspace->rowCount());

  for (size_t i = 0; i < tableWorkspace->rowCount(); ++i) {
    const std::string runFile = tableWorkspace->String(i, 0);

    if (!runHasCorrectInstrument(runFile, m_instrument))
      continue;

    if (requiresICat() && !knownFileType(runFile))
      continue;

    auto const run = trimRunName(runFile, m_instrument);
    const std::string description = tableWorkspace->String(i, 6);
    auto result = SearchResult(run, description);

    if (!resultExists(result, searchResults))
      searchResults.emplace_back(std::move(result));
  }
  return searchResults;
}

SearchResults QtCatalogSearcher::convertJournalResultsTableToSearchResults(
    ITableWorkspace_sptr tableWorkspace) {
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

bool QtCatalogSearcher::startSearchAsync(const std::string &text,
                                         const std::string &instrument,
                                         const std::string &cycle,
                                         SearchType searchType) {
  m_searchText = text;
  m_instrument = instrument;
  m_cycle = cycle;
  m_searchType = searchType;

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

void QtCatalogSearcher::errorHandle(const Mantid::API::IAlgorithm *alg,
                                    const std::string &what) {
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
    ITableWorkspace_sptr resultsTable =
        searchAlg->getProperty("OutputWorkspace");
    auto resultsList = convertTableToSearchResults(resultsTable);
    results().mergeNewResults(resultsList);
  }

  m_notifyee->notifySearchComplete();
}

bool QtCatalogSearcher::searchInProgress() const { return m_searchInProgress; }

SearchResult const &QtCatalogSearcher::getSearchResult(int index) const {
  return results().getRowData(index);
}

void QtCatalogSearcher::reset() {
  m_searchText.clear();
  m_instrument.clear();
  m_searchType = SearchType::NONE;
  results().clear();
}

bool QtCatalogSearcher::searchSettingsChanged(const std::string &text,
                                              const std::string &instrument,
                                              const std::string &cycle,
                                              SearchType searchType) const {
  return m_searchText != text || m_instrument != instrument ||
         m_cycle != cycle || searchType != m_searchType;
}

bool hasActiveCatalogSession() {
  auto sessions = CatalogManager::Instance().getActiveSessions();
  return !sessions.empty();
}

void QtCatalogSearcher::execLoginDialog(const IAlgorithm_sptr &alg) {
  API::InterfaceManager interfaceMgr;
  auto dlg = dynamic_cast<MantidQt::API::AlgorithmDialog *>(
      interfaceMgr.createDialog(alg));
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
    algSearch =
        AlgorithmManager::Instance().create("ISISJournalGetExperimentRuns");
    algSearch->setProperty("Instrument", m_instrument);
    algSearch->setProperty("Cycle", m_cycle);
  }

  algSearch->setProperty("InvestigationId", m_searchText);
  algSearch->setProperty("OutputWorkspace", "_ReflSearchResults");
  algSearch->initialize();
  algSearch->setChild(true);
  algSearch->setLogging(false);

  return algSearch;
}

ISearchModel &QtCatalogSearcher::results() const {
  return m_view->mutableSearchResults();
}

/** Returns true if the search requires ICat, false otherwise. If the cycle is
 * given then we use the journal file search instead so ICat is not required.
 */
bool QtCatalogSearcher::requiresICat() const { return m_cycle.empty(); }
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
