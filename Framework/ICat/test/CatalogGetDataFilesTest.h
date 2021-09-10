// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidICat/CatalogGetDataFiles.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace ICatTestHelper;

class CatalogGetDataFilesTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogGetDataFilesTest *createSuite() { return new CatalogGetDataFilesTest(); }

  static void destroySuite(CatalogGetDataFilesTest *suite) { delete suite; }

  CatalogGetDataFilesTest() : m_fakeLogin(std::make_unique<FakeICatLogin>()) {}

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(invstObj.initialize());
    TS_ASSERT(invstObj.isInitialized());
  }

  void testgetDataFilesExecutes() {
    if (!invstObj.isInitialized())
      invstObj.initialize();
    invstObj.setPropertyValue("InvestigationId", "12576918");
    invstObj.setPropertyValue("Session", m_fakeLogin->getSessionId());
    invstObj.setPropertyValue("OutputWorkspace", "investigation"); // selected invesigation data files

    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());
  }

private:
  CatalogGetDataFiles invstObj;
  std::unique_ptr<FakeICatLogin> m_fakeLogin;
};
