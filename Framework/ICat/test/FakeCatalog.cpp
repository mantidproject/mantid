// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FakeCatalog.h"

#include "MantidAPI/CatalogFactory.h"

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

void FakeCatalog::logout(){};

void FakeCatalog::search(ICat::CatalogSearchParam const &inputs,
                         ITableWorkspace_sptr &outputWorkspace,
                         int const &offset, int const &limit) {}

int64_t
FakeCatalog::getNumberOfSearchResults(ICat::CatalogSearchParam const &inputs) {
  int64_t numberOfSearchResults = 5;
  return numberOfSearchResults;
}

void FakeCatalog::myData(ITableWorkspace_sptr &outputWorkspace) {}

void FakeCatalog::getDataSets(std::string const &investigationID,
                              ITableWorkspace_sptr &outputWorkspace) {}

void FakeCatalog::getDataFiles(std::string const &investigationID,
                               ITableWorkspace_sptr &outputWorkspace) {}

void FakeCatalog::listInstruments(std::vector<std::string> &instruments) {}

void FakeCatalog::listInvestigationTypes(
    std::vector<std::string> &investigationTypes) {}

void FakeCatalog::keepAlive() {}

} // namespace Mantid
