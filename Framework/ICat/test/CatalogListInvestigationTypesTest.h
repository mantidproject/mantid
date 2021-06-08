// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidICat/CatalogListInvestigationTypes.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace ICatTestHelper;

class CatalogListInvestigationTypesTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogListInvestigationTypesTest *createSuite() { return new CatalogListInvestigationTypesTest(); }
  static void destroySuite(CatalogListInvestigationTypesTest *suite) { delete suite; }

  CatalogListInvestigationTypesTest() : m_fakeLogin(std::make_unique<FakeICatLogin>()) {}

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(invstTypesList.initialize());
    TS_ASSERT(invstTypesList.isInitialized());
  }

  void testListInvestigationTypes() {
    if (!invstTypesList.isInitialized())
      invstTypesList.initialize();
    // invstTypesList.setPropertyValue("OutputWorkspace","investigationtypes_list");

    TS_ASSERT_THROWS_NOTHING(invstTypesList.execute());
    TS_ASSERT(invstTypesList.isExecuted());
  }

private:
  CatalogListInvestigationTypes invstTypesList;
  std::unique_ptr<FakeICatLogin> m_fakeLogin;
};
