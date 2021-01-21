// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidICat/CatalogGetDataSets.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace ICatTestHelper;

class CatalogGetDataSetsTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogGetDataSetsTest *createSuite() { return new CatalogGetDataSetsTest(); }
  static void destroySuite(CatalogGetDataSetsTest *suite) { delete suite; }

  CatalogGetDataSetsTest() : m_fakeLogin(std::make_unique<FakeICatLogin>()) {}

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(datasets.initialize());
    TS_ASSERT(datasets.isInitialized());
  }

  void testgetDataFilesExecutes() {
    if (!datasets.isInitialized())
      datasets.initialize();
    datasets.setPropertyValue("InvestigationId", "12576918");
    datasets.setPropertyValue("Session", m_fakeLogin->getSessionId());
    datasets.setPropertyValue("OutputWorkspace",
                              "investigation"); // selected invesigation

    TS_ASSERT_THROWS_NOTHING(datasets.execute());
    TS_ASSERT(datasets.isExecuted());
  }

private:
  CatalogGetDataSets datasets;
  std::unique_ptr<FakeICatLogin> m_fakeLogin;
};
