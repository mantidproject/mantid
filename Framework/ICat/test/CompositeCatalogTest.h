// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "FakeCatalog.h"
#include "MantidAPI/CompositeCatalog.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidICat/CatalogSearchParam.h"

#include <memory>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::ICat;

class CompositeCatalogTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CompositeCatalogTest *createSuite() { return new CompositeCatalogTest(); }
  static void destroySuite(CompositeCatalogTest *suite) { delete suite; }

  CompositeCatalogTest() : m_fakeCatalog(std::make_unique<FakeCatalog>()) {}

  /// Verifies that multiple catalogs are being logged in to.
  void testLogin() {
    std::string temp = "";
    TS_ASSERT_THROWS(createCompositeCatalog()->login(temp, temp, temp, temp), std::runtime_error &);
  }

  void testLogout() {
    createCompositeCatalog()->logout();
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testSearch() {
    CatalogSearchParam params;
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    createCompositeCatalog()->search(params, ws, 0, 0);
    // Check that the search results are appended correctly to the same table.
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testGetNumberOfSearchResults() {
    CatalogSearchParam params;
    // The number of results returned by getNumberOfSearchResults is hard-coded
    // to 5
    int64_t numberOfResults = createCompositeCatalog()->getNumberOfSearchResults(params);
    // As two catalogs are created in createCompositeCatalog we verify that
    // getNumberOfSearchResults functions correctly.
    TS_ASSERT_EQUALS(numberOfResults, 10);
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testMyData() {
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    createCompositeCatalog()->myData(ws);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testGetDataSets() {
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    std::string id = "";
    createCompositeCatalog()->getDataSets(id, ws);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testGetDataFiles() {
    auto ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    std::string id = "";
    createCompositeCatalog()->getDataFiles(id, ws);
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testListInstruments() {
    std::vector<std::string> instruments;
    createCompositeCatalog()->listInstruments(instruments);
    TS_ASSERT_EQUALS(instruments.size(), 2);
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testListInvestigationTypes() {
    std::vector<std::string> investigations;
    createCompositeCatalog()->listInvestigationTypes(investigations);
    TS_ASSERT_EQUALS(investigations.size(), 2);
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

  void testKeepAlive() {
    createCompositeCatalog()->keepAlive();
    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 2);
  }

private:
  /**
   * Create a compositeCatalog and add two DummyCatalogs to it.
   * @return A shared pointer to a CompositeCatalog.
   */
  const std::shared_ptr<CompositeCatalog> createCompositeCatalog() {
    const std::shared_ptr<CompositeCatalog> compositeCatalog(new CompositeCatalog());
    m_fakeCatalog->setCount(0);

    compositeCatalog->add(std::make_shared<FakeCatalog>());
    compositeCatalog->add(std::make_shared<FakeCatalog>());

    TS_ASSERT_EQUALS(m_fakeCatalog->count(), 0);

    return compositeCatalog;
  }

  std::unique_ptr<FakeCatalog> m_fakeCatalog;
};
