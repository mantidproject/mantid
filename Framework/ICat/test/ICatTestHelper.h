// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
