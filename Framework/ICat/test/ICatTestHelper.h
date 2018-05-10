#ifndef ICATTESTHELPER_H_
#define ICATTESTHELPER_H_

#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogLogout.h"
#include "MantidKernel/ConfigService.h"

namespace ICatTestHelper {
/// Skip all unit tests if ICat server is down
bool skipTests();

/// Helper to login with test credentials, returns true if login successful
bool login();

/// Helper to logout of ICat
void logout();
} // namespace ICatTestHelper

#endif
