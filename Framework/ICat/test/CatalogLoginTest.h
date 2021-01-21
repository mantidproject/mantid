// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidICat/CatalogLogin.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::ICat;
using namespace ICatTestHelper;

class CatalogLoginTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogLoginTest *createSuite() { return new CatalogLoginTest(); }

  static void destroySuite(CatalogLoginTest *suite) { delete suite; }

  CatalogLoginTest() : m_fakeLogin(std::make_unique<FakeICatLogin>()) {}

  void testInit() {
    CatalogLogin loginobj;
    TS_ASSERT_THROWS_NOTHING(loginobj.initialize());
    TS_ASSERT(loginobj.isInitialized());
  }

  void testLoginMandatoryParams() {
    CatalogLogin loginobj;

    if (!loginobj.isInitialized())
      loginobj.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loginobj.execute(), const std::runtime_error &);
  }

  void testLogin() {
    CatalogLogin loginobj;

    if (!loginobj.isInitialized())
      loginobj.initialize();

    loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
    loginobj.setPropertyValue("Password", "MantidTestUser4");
    loginobj.setProperty("KeepSessionAlive", false);

    TS_ASSERT_THROWS_NOTHING(loginobj.execute());
    TS_ASSERT(loginobj.isExecuted());
  }

private:
  std::unique_ptr<FakeICatLogin> m_fakeLogin;
};
