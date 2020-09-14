// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ICatTestHelper.h"

#include "MantidAPI/CatalogManager.h"

namespace ICatTestHelper {

bool skipTests() { return false; }

FakeICatLogin::FakeICatLogin()
    : m_loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST") {
  m_session = Mantid::API::CatalogManager::Instance().login("", "", "", "TEST");
}

FakeICatLogin::~FakeICatLogin() {
  Mantid::API::CatalogManager::Instance().destroyCatalog(
      m_session->getSessionId());
}

bool login() { return true; }

void logout() {}

} // namespace ICatTestHelper
