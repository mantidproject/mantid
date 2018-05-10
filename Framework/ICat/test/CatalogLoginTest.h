#ifndef LOGINTEST_H_
#define LOGINTEST_H_

#include "ICatTestHelper.h"
#include "MantidICat/CatalogLogin.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::ICat;
class CatalogLoginTest : public CxxTest::TestSuite {
public:
  /// Skip all unit tests if ICat server is down
  bool skipTests() override { return ICatTestHelper::skipTests(); }

  void testInit() {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                        "ISIS");

    CatalogLogin loginobj;
    TS_ASSERT_THROWS_NOTHING(loginobj.initialize());
    TS_ASSERT(loginobj.isInitialized());
  }

  void testLoginMandatoryParams() {
    CatalogLogin loginobj;

    if (!loginobj.isInitialized())
      loginobj.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loginobj.execute(), std::runtime_error);
  }

  void testLogin() {
    CatalogLogin loginobj;

    if (!loginobj.isInitialized())
      loginobj.initialize();

    loginobj.setPropertyValue("Username",
                              "mantidtest@fitsp10.isis.cclrc.ac.uk");
    loginobj.setPropertyValue("Password", "MantidTestUser4");
    loginobj.setProperty("KeepSessionAlive", false);

    TS_ASSERT_THROWS_NOTHING(loginobj.execute());
    TS_ASSERT(loginobj.isExecuted());

    ICatTestHelper::logout();
  }
  void testLoginFail() {

    CatalogLogin loginobj;

    if (!loginobj.isInitialized())
      loginobj.initialize();

    // invalid username
    loginobj.setPropertyValue("Username", "mantid_test");
    loginobj.setPropertyValue("Password", "mantidtestuser1");
    // loginobj.setPropertyValue("DBServer", "");

    TS_ASSERT_THROWS_NOTHING(loginobj.execute());
    // should fail
    TS_ASSERT(!loginobj.isExecuted());
  }
};
#endif
