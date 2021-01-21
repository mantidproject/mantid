// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidICat/CatalogMyDataSearch.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace ICatTestHelper;

class CatalogMyDataSearchTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogMyDataSearchTest *createSuite() { return new CatalogMyDataSearchTest(); }
  static void destroySuite(CatalogMyDataSearchTest *suite) { delete suite; }

  CatalogMyDataSearchTest() : m_fakeLogin(std::make_unique<FakeICatLogin>()) {}

  void testInit() {
    CatalogMyDataSearch mydata;
    TS_ASSERT_THROWS_NOTHING(mydata.initialize());
    TS_ASSERT(mydata.isInitialized());
  }
  void testMyDataSearch() {
    CatalogMyDataSearch mydata;

    if (!mydata.isInitialized())
      mydata.initialize();

    mydata.setPropertyValue("Session", m_fakeLogin->getSessionId());
    mydata.setPropertyValue("OutputWorkspace", "MyInvestigations");

    TS_ASSERT_THROWS_NOTHING(mydata.execute());
    TS_ASSERT(mydata.isExecuted());
  }

private:
  std::unique_ptr<FakeICatLogin> m_fakeLogin;
};
