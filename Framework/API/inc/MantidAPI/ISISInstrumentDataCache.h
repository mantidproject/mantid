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
#include <string>

namespace Mantid {
namespace API {

class MANTID_API_DLL ISISInstrumentDataCache {
public:
  ISISInstrumentDataCache(const std::string &path) : m_dataCachePath(path) {}
  std::string getFileParentDirectoryPath(const std::string &filename) const;

private:
  std::pair<std::string, std::string> validateInstrumentAndNumber(const std::string &filename) const;
  std::pair<std::string, std::string> splitIntoInstrumentAndNumber(const std::string &filename) const;
  std::string m_dataCachePath;
};
} // namespace API
} // namespace Mantid
