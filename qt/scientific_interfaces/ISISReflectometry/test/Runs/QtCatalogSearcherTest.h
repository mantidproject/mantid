// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Runs/QtCatalogSearcher.h"
#include "../ReflMockObjects.h"
#include "../Runs/MockRunsView.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/MockQtAlgorithmRunner.h"

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using Mantid::Kernel::ConfigService;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::ReturnRef;

namespace {
static auto constexpr INSTRUMENT = "INTER";
static auto constexpr RUN1_NAME = "run1";
static auto constexpr RUN1_FILE = "INTER00012345.raw";
static auto constexpr RUN1_NUMBER = "12345";
static auto constexpr RUN1_TITLE = "run 1 title";
static auto constexpr RUN2_NAME = "run2";
static auto constexpr RUN2_FILE = "INTER00022345.raw";
static auto constexpr RUN2_NUMBER = "22345";
static auto constexpr RUN2_TITLE = "run 2 title";
} // namespace

class MockSearchAlgorithm : public Algorithm {
public:
  const std::string name() const override { return "MockSearchAlgorithm"; };
  int version() const override { return 1; };
  const std::string summary() const override { return "A mock search algorithm"; };

private:
  void init() override {};
  void exec() override {};
  bool isInitialized() const override { return true; }
};

/** Create our own version of the QtCatalogSearcher so we can override some
 * functions. This is not ideal - the class could do with some restructuring to
 * avoid this.
 */
class MockQtCatalogSearcher : public QtCatalogSearcher {
public:
  MockQtCatalogSearcher(IRunsView *view, IAlgorithm_sptr searchAlg = std::make_shared<MockSearchAlgorithm>(),
                        bool const hasActiveSession = true)
      : QtCatalogSearcher(view), m_searchAlg(searchAlg), m_hasActiveCatalogSession(hasActiveSession),
        m_logInWasCalled{false} {}

  bool logInWasCalled() const { return m_logInWasCalled; }

  // Override finishHandle to make it public
  void finishHandle(const Mantid::API::IAlgorithm *alg) override { QtCatalogSearcher::finishHandle(alg); };
  // Override errorHandle to make it public
  void errorHandle(const Mantid::API::IAlgorithm *alg, std::string const &what) override {
    QtCatalogSearcher::errorHandle(alg, what);
  };

private:
  IAlgorithm_sptr m_searchAlg;
  bool m_hasActiveCatalogSession;
  bool m_logInWasCalled;

  bool hasActiveCatalogSession() const override { return m_hasActiveCatalogSession; }

  // Override the log in function because it calls an algorithm that requires
  // ICat. Instead just allow the tests to check that it was called.
  void logInToCatalog() override { m_logInWasCalled = true; }

  virtual Mantid::API::IAlgorithm_sptr createSearchAlgorithm() override { return m_searchAlg; };

  // Override the method that gets the algorithm results so
  // we don't need a real algorithm
  ITableWorkspace_sptr getSearchAlgorithmResultsTable(Mantid::API::IAlgorithm_sptr searchAlg) override {
    UNUSED_ARG(searchAlg);
    // Return a results table of a suitable format for the algorithm used
    if (requiresICat())
      return getSampleCatalogResults();
    else
      return getSampleJournalResults();
  }

  // Get a sample results table in the format used by the catalog search
  // algoorithm, CatalogGetDataFiles
  ITableWorkspace_sptr getSampleCatalogResults() {
    auto table = WorkspaceFactory::Instance().createTable();
    table->addColumn("str", "Name");
    table->addColumn("str", "Location");
    table->addColumn("str", "Create Time");
    table->addColumn("str", "Id");
    table->addColumn("str", "File size(byts)");
    table->addColumn("str", "File size");
    table->addColumn("str", "Description");
    TableRow row1 = table->appendRow();
    TableRow row2 = table->appendRow();
    row1 << RUN1_FILE << ""
         << ""
         << ""
         << "0"
         << "0" << RUN1_TITLE;
    row2 << RUN2_FILE << ""
         << ""
         << ""
         << "0"
         << "0" << RUN2_TITLE;
    return table;
  }

  // Get a sample results table in the format used by the catalog search
  // algoorithm, ISISJournalGetExperimentRuns
  ITableWorkspace_sptr getSampleJournalResults() {
    auto table = WorkspaceFactory::Instance().createTable();
    table->addColumn("str", "Name");
    table->addColumn("str", "Run Number");
    table->addColumn("str", "Title");
    TableRow row1 = table->appendRow();
    TableRow row2 = table->appendRow();
    row1 << RUN1_NAME << RUN1_NUMBER << RUN1_TITLE;
    row2 << RUN2_NAME << RUN2_NUMBER << RUN2_TITLE;
    return table;
  }
};

class QtCatalogSearcherTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QtCatalogSearcherTest *createSuite() { return new QtCatalogSearcherTest(); }
  static void destroySuite(QtCatalogSearcherTest *suite) { delete suite; }

  QtCatalogSearcherTest() : m_view(), m_notifyee(), m_searchAlg(std::make_shared<MockSearchAlgorithm>()) {}

  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void test_constructor_subscribes_to_view() {
    EXPECT_CALL(m_view, subscribeSearch(_)).Times(1);
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
  }

  void test_journal_search() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    auto results = doJournalSearch(searcher);
    checkSearchResults(results);
  }

  void test_catalog_search() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    auto results = doCatalogSearch(searcher);
    checkSearchResults(results);
  }

  void test_catalog_search_returns_empty_results_if_incorrect_instrument() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    auto results = doCatalogSearch(searcher, "BAD_INSTR");
    TS_ASSERT_EQUALS(results.size(), 0);
  }

  void test_async_journal_search_returns_success() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    expectGetAlgorithmRunner();
    auto success = startAsyncJournalSearch(searcher);
    TS_ASSERT_EQUALS(success, true);
  }

  void test_async_catalog_search_returns_success_if_has_active_session() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    expectGetAlgorithmRunner();
    auto success = startAsyncCatalogSearch(searcher);
    TS_ASSERT_EQUALS(success, true);
  }

  void test_async_journal_search_sets_in_progress_flag() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    expectGetAlgorithmRunner();
    startAsyncJournalSearch(searcher);
    TS_ASSERT_EQUALS(searcher.searchInProgress(), true);
  }

  void test_async_catalog_search_sets_in_progress_flag() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    expectGetAlgorithmRunner();
    startAsyncCatalogSearch(searcher);
    TS_ASSERT_EQUALS(searcher.searchInProgress(), true);
  }

  void test_async_journal_search_starts_algorithm_runner() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    auto algRunner = expectGetAlgorithmRunner();
    expectAlgorithmStarted(m_searchAlg, algRunner);
    startAsyncJournalSearch(searcher);
  }

  void test_async_catalog_search_starts_algorithm_runner() {
    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    auto algRunner = expectGetAlgorithmRunner();
    expectAlgorithmStarted(m_searchAlg, algRunner);
    startAsyncCatalogSearch(searcher);
  }

  void test_async_catalog_search_returns_success_when_not_logged_in() {
    auto searcher = makeCatalogSearcher(false);
    searcher.subscribe(&m_notifyee);
    auto success = startAsyncCatalogSearch(searcher);
    TS_ASSERT_EQUALS(success, true);
  }

  void test_async_catalog_search_starts_login_when_not_logged_in() {
    auto searcher = makeCatalogSearcher(false);
    searcher.subscribe(&m_notifyee);
    startAsyncCatalogSearch(searcher);
    TS_ASSERT_EQUALS(searcher.logInWasCalled(), true);
  }

  void test_async_catalog_search_does_not_start_search_when_not_logged_in() {
    auto searcher = makeCatalogSearcher(false);
    searcher.subscribe(&m_notifyee);
    EXPECT_CALL(m_view, getAlgorithmRunner()).Times(0);
    auto success = startAsyncCatalogSearch(searcher);
    TS_ASSERT_EQUALS(success, true);
  }

  void test_finish_handle_starts_async_search_if_active_session() {
    auto searcher = makeCatalogSearcher(true);
    searcher.subscribe(&m_notifyee);
    auto algRunner = expectGetAlgorithmRunner();
    expectAlgorithmStarted(m_searchAlg, algRunner);
    searcher.finishHandle(m_searchAlg.get());
  }

  void test_finish_does_not_notify_failure_if_active_session() {
    auto searcher = makeCatalogSearcher(true);
    searcher.subscribe(&m_notifyee);
    expectGetAlgorithmRunner();
    expectNotNotifiedSearchFailed();
    searcher.finishHandle(m_searchAlg.get());
  }

  void test_finish_handle_notifies_failure_if_no_active_session() {
    auto searcher = makeCatalogSearcher(false);
    searcher.subscribe(&m_notifyee);
    expectNotifySearchFailed();
    searcher.finishHandle(m_searchAlg.get());
  }

  void test_error_handle_notifies_failure_if_no_active_session() {
    auto searcher = makeCatalogSearcher(false);
    searcher.subscribe(&m_notifyee);
    expectNotifySearchFailed();
    searcher.errorHandle(m_searchAlg.get(), "test error message");
  }

  void test_error_handle_does_not_notify_failure_if_active_session() {
    auto searcher = makeCatalogSearcher(true);
    searcher.subscribe(&m_notifyee);
    expectNotNotifiedSearchFailed();
    searcher.errorHandle(m_searchAlg.get(), "test error message");
  }

  void test_set_saved_flag() {
    auto searcher = makeCatalogSearcher(true);
    EXPECT_CALL(m_searchResults, setSaved()).Times(1);
    searcher.setSaved();
  }

  void test_notify_search_results_changed_sets_unsaved_flag() {
    auto searcher = makeCatalogSearcher(true);
    EXPECT_CALL(m_searchResults, setUnsaved()).Times(1);
    searcher.notifySearchResultsChanged();
  }

  void test_search_results_collection_passed_to_results() {
    auto searcher = makeCatalogSearcher(true);
    EXPECT_CALL(m_searchResults, getSearchResultsCSV()).Times(1);
    searcher.getSearchResultsCSV();
  }

  void test_search_results_in_data_cache() {
    auto const &defaultArchiveSetting = ConfigService::Instance().getString("datasearch.searcharchive");
    auto const &defaultSaveDirectory = ConfigService::Instance().getString("datacachesearch.directory");
    setupFakeDataCache();
    ConfigService::Instance().setString("datasearch.searcharchive", "off");

    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    auto results = doJournalSearch(searcher);
    checkFilteredSearchResults(results);
    ConfigService::Instance().setString("datacachesearch.directory", defaultSaveDirectory);
    ConfigService::Instance().setString("datasearch.searcharchive", defaultArchiveSetting);
  }

  void test_search_with_archive_on_and_cache_set() {
    auto const &defaultArchiveSetting = ConfigService::Instance().getString("datasearch.searcharchive");
    auto const &defaultSaveDirectory = ConfigService::Instance().getString("datacachesearch.directory");
    setupFakeDataCache();
    ConfigService::Instance().setString("datasearch.searcharchive", "On");

    auto searcher = makeCatalogSearcher();
    searcher.subscribe(&m_notifyee);
    auto results = doJournalSearch(searcher);
    checkSearchResults(results);
    ConfigService::Instance().setString("datacachesearch.directory", defaultSaveDirectory);
    ConfigService::Instance().setString("datasearch.searcharchive", defaultArchiveSetting);
  }

private:
  NiceMock<MockRunsView> m_view;
  NiceMock<MockSearcherSubscriber> m_notifyee;
  IAlgorithm_sptr m_searchAlg;
  NiceMock<MockSearchModel> m_searchResults;

  MockQtCatalogSearcher makeCatalogSearcher(bool const hasActiveSession = true) {
    ON_CALL(m_view, mutableSearchResults()).WillByDefault(ReturnRef(m_searchResults));
    return MockQtCatalogSearcher(&m_view, m_searchAlg, hasActiveSession);
  }

  void checkSearchResults(SearchResults const &actual) {
    auto const expected = SearchResults{SearchResult(RUN1_NUMBER, RUN1_TITLE), SearchResult(RUN2_NUMBER, RUN2_TITLE)};
    TS_ASSERT_EQUALS(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      TS_ASSERT_EQUALS(actual[i], expected[i]);
    }
  }

  void setupFakeDataCache() {
    auto const tempCache = std::filesystem::temp_directory_path() /= "fakeCache";
    std::filesystem::create_directories(tempCache / INSTRUMENT);
    std::ofstream indexFile(tempCache / INSTRUMENT / (std::string(INSTRUMENT) + "_index.json"));
    indexFile << "{\n"
              << "\t\"22345\": \"fake/path/to/file\"\n"
              << "}\n";
    indexFile.close();

    ConfigService::Instance().setString("datacachesearch.directory", tempCache.string());
  }

  void checkFilteredSearchResults(SearchResults const &actual) {
    auto const expected = SearchResults{SearchResult(RUN2_NUMBER, RUN2_TITLE)};
    TS_ASSERT_EQUALS(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      TS_ASSERT_EQUALS(actual[i], expected[i]);
    }
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_notifyee));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_searchResults));
  }

  std::shared_ptr<NiceMock<MockQtAlgorithmRunner>> expectGetAlgorithmRunner() {
    // Get the algorithm runner
    auto algRunner = std::make_shared<NiceMock<MockQtAlgorithmRunner>>();
    EXPECT_CALL(m_view, getAlgorithmRunner()).Times(AtLeast(1)).WillRepeatedly(Return(algRunner));
    return algRunner;
  }

  void expectAlgorithmStarted(IAlgorithm_sptr searchAlg, std::shared_ptr<NiceMock<MockQtAlgorithmRunner>> algRunner) {
    EXPECT_CALL(*algRunner, startAlgorithmImpl(searchAlg)).Times(1);
  }

  void expectNotifySearchFailed() { EXPECT_CALL(m_notifyee, notifySearchFailed()).Times(AtLeast(1)); }

  void expectNotNotifiedSearchFailed() { EXPECT_CALL(m_notifyee, notifySearchFailed()).Times(0); }

  SearchResults doJournalSearch(MockQtCatalogSearcher &searcher) {
    // Passing non-empty cycle performs journal search
    return searcher.search(SearchCriteria{"INTER", "19_4", "6543210"});
  }

  SearchResults doCatalogSearch(MockQtCatalogSearcher &searcher, std::string const &instrument = "INTER") {
    // Passing empty cycle performs catalog search
    return searcher.search(SearchCriteria{instrument, "", "6543210"});
  }

  bool startAsyncJournalSearch(MockQtCatalogSearcher &searcher) {
    // Passing non-empty cycle performs journal search
    return searcher.startSearchAsync(SearchCriteria{"INTER", "19_4", "6543210"});
  }

  bool startAsyncCatalogSearch(MockQtCatalogSearcher &searcher) {
    // Passing empty cycle performs catalog search
    return searcher.startSearchAsync(SearchCriteria{"INTER", "", "6543210"});
  }
};
