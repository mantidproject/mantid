// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogSearch.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace ICatTestHelper;

class CatalogSearchTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogSearchTest *createSuite() { return new CatalogSearchTest(); }

  static void destroySuite(CatalogSearchTest *suite) { delete suite; }

  CatalogSearchTest() : m_fakeLogin(std::make_unique<FakeICatLogin>()) {}

  void testInit() {
    CatalogSearch searchobj;
    CatalogLogin loginobj;

    TS_ASSERT_THROWS_NOTHING(searchobj.initialize());
    TS_ASSERT(searchobj.isInitialized());
  }

  void testSearchByRunNumberandInstrumentExecutes() {
    // Uses an unused keyword to produce an empty workspace and be fast
    CatalogSearch searchobj;

    if (!searchobj.isInitialized())
      searchobj.initialize();

    searchobj.setPropertyValue("RunRange", "1000000-1000001");
    // search ALF instrument it is much faster
    searchobj.setPropertyValue("Instrument", "ALF");
    searchobj.setPropertyValue("Session", m_fakeLogin->getSessionId());
    searchobj.setPropertyValue("OutputWorkspace", "Investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());
  }

  void testSearchByKeywordsExecutes() {
    CatalogSearch searchobj;

    if (!searchobj.isInitialized())
      searchobj.initialize();

    // This is a keyword that is chosen to return an empty dataset - very fast
    searchobj.setPropertyValue("Keywords", ":-)");
    searchobj.setPropertyValue("Instrument", "HRPD");
    searchobj.setPropertyValue("Session", m_fakeLogin->getSessionId());
    searchobj.setPropertyValue("OutputWorkspace", "Investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());
  }

  void testSearchByStartDateEndDateExecutes() {
    // Uses a search date outside of general operation to produce an empty
    // workspace and be fast
    CatalogSearch searchobj;

    if (!searchobj.isInitialized())
      searchobj.initialize();

    searchobj.setPropertyValue("StartDate", "10/08/1980");
    searchobj.setPropertyValue("EndDate", "22/08/1980");
    searchobj.setPropertyValue("Session", m_fakeLogin->getSessionId());
    searchobj.setPropertyValue("OutputWorkspace", "Investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());
  }

  void testSearchByRunNumberInvalidInput() {
    CatalogSearch searchobj;

    if (!searchobj.isInitialized())
      searchobj.initialize();

    // start run number > end run number
    searchobj.setPropertyValue("RunRange", "150-102");
    searchobj.setPropertyValue("Instrument", "LOQ");
    searchobj.setPropertyValue("Session", m_fakeLogin->getSessionId());
    searchobj.setPropertyValue("OutputWorkspace", "Investigations");
    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    // should fail
    TS_ASSERT(!searchobj.isExecuted());
  }

  void testSearchByInvalidDates1() {
    CatalogSearch searchobj;

    if (!searchobj.isInitialized())
      searchobj.initialize();

    TS_ASSERT_THROWS(searchobj.setPropertyValue("StartDate", "sssss"), const std::invalid_argument &);
    TS_ASSERT_THROWS(searchobj.setPropertyValue("EndDate", "aaaaa"), const std::invalid_argument &);
  }

  void testSearchByInvalidDates2() {
    CatalogSearch searchobj;

    if (!searchobj.isInitialized())
      searchobj.initialize();

    TS_ASSERT_THROWS(searchobj.setPropertyValue("StartDate", "39/22/2009"), const std::invalid_argument &);
    TS_ASSERT_THROWS(searchobj.setPropertyValue("EndDate", "1/22/2009"), const std::invalid_argument &);
  }

private:
  std::unique_ptr<FakeICatLogin> m_fakeLogin;
};
