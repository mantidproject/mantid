// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ConfigService.h"
#include <filesystem>
#include <json/value.h>
#include <string>
#include <utility>

namespace Mantid {
namespace API {

class MANTID_API_DLL ISISInstrumentDataCache {
public:
  ISISInstrumentDataCache(const std::string &path) : m_dataCachePath(path) {}
  std::string getFileParentDirectoryPath(const std::string &filename) const;
  bool isIndexFileAvailable(std::string const &instrument) const;
  std::vector<std::string> getRunNumbersInCache(std::string const &instrumentName) const;

private:
  std::pair<Mantid::Kernel::InstrumentInfo, std::string> validateInstrumentAndNumber(const std::string &filename) const;
  std::filesystem::path makeIndexFilePath(const std::string &instrumentName) const;
  std::pair<std::string, std::string> splitIntoInstrumentAndNumber(const std::string &filename) const;
  [[nodiscard]] std::pair<std::string, Json::Value>
  openCacheJsonFile(const Mantid::Kernel::InstrumentInfo &instrument) const;
  [[nodiscard]] Mantid::Kernel::InstrumentInfo getInstrumentFromName(const std::string &instName) const;
  std::string m_dataCachePath;
};
} // namespace API
} // namespace Mantid
