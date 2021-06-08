// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CatalogSession.h"
#include "MantidAPI/ICatalog.h"
#include "MantidAPI/ICatalogInfoService.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidICat/CatalogSearchParam.h"

namespace Mantid {

class FakeCatalog : public API::ICatalog, public API::ICatalogInfoService {
public:
  FakeCatalog();
  ~FakeCatalog() override;

  API::CatalogSession_sptr login(std::string const &username, std::string const &password, std::string const &endPoint,
                                 std::string const &facility) override;
  void logout() override;

  void search(ICat::CatalogSearchParam const &inputs, API::ITableWorkspace_sptr &outputWorkspace, int const &offset,
              int const &limit) override;
  int64_t getNumberOfSearchResults(ICat::CatalogSearchParam const &inputs) override;

  void myData(API::ITableWorkspace_sptr &outputWorkspace) override;
  void getDataSets(std::string const &investigationID, API::ITableWorkspace_sptr &outputWorkspace) override;
  void getDataFiles(std::string const &investigationID, API::ITableWorkspace_sptr &outputWorkspace) override;

  void listInstruments(std::vector<std::string> &instruments) override;
  void listInvestigationTypes(std::vector<std::string> &investigationTypes) override;

  void keepAlive() override;

  std::string const getFileLocation(long long const &fileID) override;
  std::string const getDownloadURL(long long const &fileID) override;
  std::string const getUploadURL(std::string const &investigationID, std::string const &createFileName,
                                 std::string const &dataFileDescription) override;
  API::ITableWorkspace_sptr getPublishInvestigations() override;

  static void setCount(std::size_t const &count);
  static std::size_t count();

private:
  static std::size_t m_counter;
};

} // namespace Mantid
