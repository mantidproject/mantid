// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ICatTestHelper.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger logger("ICatTest");
}

namespace ICatTestHelper {
/// Skip all unit tests if ICat server is down
bool skipTests() {
  Mantid::Kernel::ConfigService::Instance().setString("default.facility",
                                                      "ISIS");
  if (!login()) {
    logger.error() << "ICat server seems to be down. Skipping tests\n";
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
} // namespace ICatTestHelper
