// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IArchiveSearch.h"
#include "MantidDataHandling/DllConfig.h"

#include <sstream>
#include <string>

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL ISISInstrDataCache {
public:
  ISISInstrDataCache(std::string path) : m_dataCachePath(path){};
  std::string getInstrFilePath(std::string filename);

private:
  std::string m_dataCachePath;
};
} // namespace DataHandling
} // namespace Mantid
