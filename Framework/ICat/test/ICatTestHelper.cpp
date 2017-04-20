#include "ICatTestHelper.h"

#include <iostream>

namespace ICatTestHelper {
/// Skip all unit tests if ICat server is down
bool skipTests() {
  Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                      "ISIS");
  if (!login()) {
    std::cerr << "ICat server seems to be down. Skipping tests\n";
    return true;
  } else {
    logout();
    return false;
  }
}

bool login() {
  Mantid::ICat::CatalogLogin loginobj;
  loginobj.initialize();
  loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
  loginobj.setPropertyValue("Password", "MantidTestUser4");
  loginobj.setProperty("KeepSessionAlive", false);
  loginobj.execute();
  return loginobj.isExecuted();
}

void logout() {
  Mantid::ICat::CatalogLogout logoutobj;
  logoutobj.initialize();
  logoutobj.execute();
}
}
