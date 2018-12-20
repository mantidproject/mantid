#ifndef MANTID_ICAT_COMPOSITECATALOGTEST_H_
#define MANTID_ICAT_COMPOSITECATALOGTEST_H_

#include "ICatTestHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeCatalog.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidICat/CatalogSearchParam.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::ICat;

/**
 * Used to verify that multiple catalogs function as expected.
 */
class DummyCatalog : public ICatalog {
public:
  CatalogSession_sptr login(const std::string &, const std::string &,
                            const std::string &, const std::string &) override {
    throw std::runtime_error(
        "You cannot log into multiple catalogs at the same time.");
  }

  void logout() override { m_counter++; }

  void search(const CatalogSearchParam &, ITableWorkspace_sptr &ws, const int &,
              const int &) override {
    // Each implementation of the ICatalog methods should always append data to
    // the workspace.
    ws->appendRow();
    m_counter++;
  }

  int64_t getNumberOfSearchResults(const CatalogSearchParam &) override {
    m_counter++;
    // Return 5 to verify that the NumberOfResults gets added together across
    // catalogs as expected.
    return 5;
  }

  void myData(ITableWorkspace_sptr &ws) override {
    ws->appendRow();
    m_counter++;
  }

  void getDataSets(const std::string &, ITableWorkspace_sptr &ws) override {
    ws->appendRow();
    m_counter++;
  }

  void getDataFiles(const std::string &, ITableWorkspace_sptr &ws) override {
    ws->appendRow();
    m_counter++;
  }

  void listInstruments(std::vector<std::string> &instruments) override {
    instruments.emplace_back("");
    m_counter++;
  }

  void
  listInvestigationTypes(std::vector<std::string> &investigations) override {
    investigations.emplace_back("");
    m_counter++;
  }

  void keepAlive() override { m_counter++; }

  /// The counter used to verify each method works when
  /// multiple counters perform the same operation on the catalog.
  static int m_counter;
};

// Initialise to make global.
int DummyCatalog::m_counter(0);

class CompositeCatalogTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CompositeCatalogTest *createSuite() {
    return new CompositeCatalogTest();
  }
  static void destroySuite(CompositeCatalogTest *suite) { delete suite; }

  /// Skip all unit tests if ICat server is down
  bool skipTests() override { return ICatTestHelper::skipTests(); }

  CompositeCatalogTest() { Mantid::API::FrameworkManager::Instance(); }

  /// Verifies that multiple catalogs are being logged in to.
  void testLogin() {
    std::string temp = "";
    TS_ASSERT_THROWS(createCompositeCatalog()->login(temp, temp, temp, temp),
                     std::runtime_error &);
  }

  void testLogout() {
    createCompositeCatalog()->logout();
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testSearch() {
    CatalogSearchParam params;
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    createCompositeCatalog()->search(params, ws, 0, 0);
    // Check that the search results are appended correctly to the same table.
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testGetNumberOfSearchResults() {
    CatalogSearchParam params;
    // The number of results returned by getNumberOfSearchResults is hard-coded
    // to 5
    int64_t numberOfResults =
        createCompositeCatalog()->getNumberOfSearchResults(params);
    // As two catalogs are created in createCompositeCatalog we verify that
    // getNumberOfSearchResults functions correctly.
    TS_ASSERT_EQUALS(numberOfResults, 10);
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testMyData() {
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    createCompositeCatalog()->myData(ws);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testGetDataSets() {
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    std::string id = "";
    createCompositeCatalog()->getDataSets(id, ws);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testGetDataFiles() {
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    std::string id = "";
    createCompositeCatalog()->getDataFiles(id, ws);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testListInstruments() {
    std::vector<std::string> instruments;
    createCompositeCatalog()->listInstruments(instruments);
    TS_ASSERT_EQUALS(instruments.size(), 2);
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testListInvestigationTypes() {
    std::vector<std::string> investigations;
    createCompositeCatalog()->listInvestigationTypes(investigations);
    TS_ASSERT_EQUALS(investigations.size(), 2);
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

  void testKeepAlive() {
    createCompositeCatalog()->keepAlive();
    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 2);
  }

private:
  /**
   * Create a compositeCatalog and add two DummyCatalogs to it.
   * @return A shared pointer to a CompositeCatalog.
   */
  const boost::shared_ptr<CompositeCatalog> createCompositeCatalog() {
    const boost::shared_ptr<CompositeCatalog> compositeCatalog(
        new CompositeCatalog());
    DummyCatalog::m_counter = 0;

    compositeCatalog->add(boost::make_shared<DummyCatalog>());
    compositeCatalog->add(boost::make_shared<DummyCatalog>());

    TS_ASSERT_EQUALS(DummyCatalog::m_counter, 0);

    return compositeCatalog;
  }
};

#endif /* MANTID_ICAT_COMPOSITECATALOGTEST_H_ */
