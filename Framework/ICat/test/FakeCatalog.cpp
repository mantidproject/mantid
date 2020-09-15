// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FakeCatalog.h"

#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/ITableWorkspace.h"

using namespace Mantid::API;

namespace Mantid {
DECLARE_CATALOG(FakeCatalog)

FakeCatalog::FakeCatalog() {}

FakeCatalog ::~FakeCatalog() {}

API::CatalogSession_sptr FakeCatalog::login(std::string const &username,
                                            std::string const &password,
                                            std::string const &endPoint,
                                            std::string const &facility) {
  return std::make_shared<API::CatalogSession>("", facility, endPoint);
}

void FakeCatalog::logout() { m_counter++; };

void FakeCatalog::search(ICat::CatalogSearchParam const &inputs,
                         ITableWorkspace_sptr &outputWorkspace,
                         int const &offset, int const &limit) {
  outputWorkspace->appendRow();
  m_counter++;
}

int64_t
FakeCatalog::getNumberOfSearchResults(ICat::CatalogSearchParam const &inputs) {
  m_counter++;
  return 5;
}

void FakeCatalog::myData(ITableWorkspace_sptr &outputWorkspace) {
  outputWorkspace->appendRow();
  m_counter++;
}

void FakeCatalog::getDataSets(std::string const &investigationID,
                              ITableWorkspace_sptr &outputWorkspace) {
  outputWorkspace->appendRow();
  m_counter++;
}

void FakeCatalog::getDataFiles(std::string const &investigationID,
                               ITableWorkspace_sptr &outputWorkspace) {
  outputWorkspace->appendRow();
  m_counter++;
}

void FakeCatalog::listInstruments(std::vector<std::string> &instruments) {
  instruments.emplace_back("");
  m_counter++;
}

void FakeCatalog::listInvestigationTypes(
    std::vector<std::string> &investigationTypes) {
  investigationTypes.emplace_back("");
  m_counter++;
}

void FakeCatalog::keepAlive() { m_counter++; }

void FakeCatalog::setCount(std::size_t const &count) { m_counter = count; }

std::size_t FakeCatalog::count() { return m_counter; }

std::size_t FakeCatalog::m_counter(0);

} // namespace Mantid
