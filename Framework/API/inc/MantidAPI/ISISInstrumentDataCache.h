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
  std::vector<std::string> getRunNumbersInCache(const std::string &instrument,
                                                std::vector<std::string> runNumbers) const;

private:
  std::pair<Mantid::Kernel::InstrumentInfo, std::string> validateInstrumentAndNumber(const std::string &filename) const;
  std::string makeIndexFilePath(const std::string &instrumentName) const;
  std::pair<std::string, std::string> splitIntoInstrumentAndNumber(const std::string &filename) const;
  [[nodiscard]] std::pair<std::string, Json::Value> openCacheJsonFile(std::string const &instrumentName) const;
  std::string m_dataCachePath;
};
} // namespace API
} // namespace Mantid
