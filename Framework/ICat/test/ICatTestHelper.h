// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CatalogSession.h"
#include "MantidTestHelpers/FacilityHelper.h"

namespace ICatTestHelper {

class FakeICatLogin {
public:
  FakeICatLogin();
  ~FakeICatLogin();

private:
  Mantid::API::CatalogSession_sptr m_session;
  FacilityHelper::ScopedFacilities m_loadTESTFacility;
};

/// Skip all unit tests if ICat server is down
bool skipTests();

/// Helper to login with test credentials, returns true if login successful
bool login();

/// Helper to logout of ICat
void logout();
} // namespace ICatTestHelper
