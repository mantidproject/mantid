// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FakeCatalog.h"

#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
// Required so compiler knows MantidDataObjects library is needed for ICatTests
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid::API;

namespace Mantid {
DECLARE_CATALOG(FakeCatalog)

FakeCatalog::FakeCatalog() = default;

FakeCatalog::~FakeCatalog() = default;

API::CatalogSession_sptr FakeCatalog::login([[maybe_unused]] std::string const &username,
                                            [[maybe_unused]] std::string const &password, std::string const &endPoint,
                                            std::string const &facility) {
  return std::make_shared<API::CatalogSession>("FakeID", facility, endPoint);
}

void FakeCatalog::logout() { m_counter++; }

void FakeCatalog::search([[maybe_unused]] ICat::CatalogSearchParam const &inputs, ITableWorkspace_sptr &outputWorkspace,
                         [[maybe_unused]] int const &offset, [[maybe_unused]] int const &limit) {
  outputWorkspace->appendRow();
  m_counter++;
}

int64_t FakeCatalog::getNumberOfSearchResults([[maybe_unused]] ICat::CatalogSearchParam const &inputs) {
  m_counter++;
  return 5;
}

void FakeCatalog::myData(ITableWorkspace_sptr &outputWorkspace) {
  outputWorkspace->appendRow();
  m_counter++;
}

void FakeCatalog::getDataSets([[maybe_unused]] std::string const &investigationID,
                              ITableWorkspace_sptr &outputWorkspace) {
  outputWorkspace->appendRow();
  m_counter++;
}

void FakeCatalog::getDataFiles([[maybe_unused]] std::string const &investigationID,
                               ITableWorkspace_sptr &outputWorkspace) {
  outputWorkspace->appendRow();
  m_counter++;
}

void FakeCatalog::listInstruments(std::vector<std::string> &instruments) {
  instruments.emplace_back("");
  m_counter++;
}

void FakeCatalog::listInvestigationTypes(std::vector<std::string> &investigationTypes) {
  investigationTypes.emplace_back("");
  m_counter++;
}

void FakeCatalog::keepAlive() { m_counter++; }

std::string const FakeCatalog::getFileLocation([[maybe_unused]] long long const &fileID) { return ""; }

std::string const FakeCatalog::getDownloadURL([[maybe_unused]] long long const &fileID) { return ""; }

std::string const FakeCatalog::getUploadURL([[maybe_unused]] std::string const &investigationID,
                                            [[maybe_unused]] std::string const &createFileName,
                                            [[maybe_unused]] std::string const &dataFileDescription) {
  return "";
}

ITableWorkspace_sptr FakeCatalog::getPublishInvestigations() { return std::make_shared<DataObjects::TableWorkspace>(); }

void FakeCatalog::setCount(std::size_t const &count) { m_counter = count; }

std::size_t FakeCatalog::count() { return m_counter; }

std::size_t FakeCatalog::m_counter(0);

} // namespace Mantid
