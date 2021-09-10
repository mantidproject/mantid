// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ICatTestHelper.h"

#include "MantidAPI/CatalogManager.h"

using namespace Mantid::API;

namespace ICatTestHelper {

FakeICatLogin::FakeICatLogin() : m_loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST") {
  m_session = CatalogManager::Instance().login("", "", "", "TEST");
}

FakeICatLogin::~FakeICatLogin() { CatalogManager::Instance().destroyCatalog(m_session->getSessionId()); }

std::string FakeICatLogin::getSessionId() const { return m_session->getSessionId(); }

} // namespace ICatTestHelper
