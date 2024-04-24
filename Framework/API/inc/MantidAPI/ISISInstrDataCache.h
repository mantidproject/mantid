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
#include "MantidAPI/IArchiveSearch.h"
#include <string>

namespace Mantid {
namespace API {

class MANTID_API_DLL ISISInstrDataCache {
public:
  ISISInstrDataCache(const std::string &path) : m_dataCachePath(path){};
  std::string getFileParentDirPath(std::string filename);

private:
  std::string m_dataCachePath;
};
} // namespace API
} // namespace Mantid
