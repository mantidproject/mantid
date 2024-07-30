// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ISISInstrumentDataCache.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <algorithm>
#include <fstream>
#include <json/reader.h>

using Mantid::API::ISISInstrumentDataCache;
using Mantid::Kernel::InstrumentInfo;

namespace {
Mantid::Kernel::Logger g_log("ISISInstrumentDataCache");
} // namespace

std::string ISISInstrumentDataCache::getFileParentDirectoryPath(const std::string &fileName) const {
  g_log.debug() << "ISISInstrumentDataCache::getFileParentDirectoryPath(" << fileName << ")" << std::endl;

  auto [instrumentInfo, runNumber] = validateInstrumentAndNumber(fileName);
  std::string instrName = instrumentInfo.name();

  auto const [jsonPath, json] = openCacheJsonFile(instrumentInfo);

  std::string relativePath = json[runNumber].asString();

  if (relativePath.empty()) {
    throw std::invalid_argument("Run number " + runNumber + " not found for instrument " + instrName + ".");
  }

  std::string dirPath = m_dataCachePath + "/" + instrName + "/" + relativePath;

  g_log.debug() << "Opened instrument index file: " << jsonPath << ". Found path to search: " << dirPath << "."
                << std::endl;
  return dirPath;
}

/**
 * Get a vector of the run numbers' files present in the given instrument's data cache.
 * @param instrumentName The instrument to get run numbers for.
 * @return A vector containing the run numbers that can be accessed from the data cache.
 */
std::vector<std::string> ISISInstrumentDataCache::getRunNumbersInCache(const std::string &instrumentName) const {
  const auto &json = openCacheJsonFile(getInstrumentFromName(instrumentName)).second;
  return json.getMemberNames();
}

/**
 * Open the Json file and return the path and the opened file object.
 * @param instrument The instrument to open the index file for.
 * @return A pair containing the path and the contents of the json file.
 * @throws std::invalid_argument if the file could not be found.
 */
std::pair<std::string, Json::Value> ISISInstrumentDataCache::openCacheJsonFile(const InstrumentInfo &instrument) const {
  // Open index json file
  std::filesystem::path jsonPath = makeIndexFilePath(instrument.name());
  std::ifstream ifstrm{jsonPath};
  if (!ifstrm.is_open()) { // Try again with shortname
    jsonPath = makeIndexFilePath(instrument.shortName());
    ifstrm.open(jsonPath);
    if (!ifstrm.is_open()) {
      throw std::invalid_argument("Could not open index file: " + jsonPath.string());
    }
  }
  // Read directory path from json file
  Json::Value json;
  ifstrm >> json;
  return {jsonPath.string(), json};
}

std::pair<InstrumentInfo, std::string>
ISISInstrumentDataCache::validateInstrumentAndNumber(const std::string &fileName) const {

  // Check if suffix eg. -add is present in filename
  std::string fileNameCopy = fileName;
  std::string suffix = FileFinder::Instance().extractAllowedSuffix(fileNameCopy);
  if (!suffix.empty()) {
    throw std::invalid_argument("Unsuported format: Suffix detected: " + suffix);
  }

  auto [instrName, runNumber] = splitIntoInstrumentAndNumber(fileName);

  if (runNumber.empty() || !std::all_of(runNumber.begin(), runNumber.end(), ::isdigit)) { // Check run number
    throw std::invalid_argument("Filename not in correct format.");
  }
  runNumber.erase(0, runNumber.find_first_not_of('0')); // Remove padding zeros

  return std::pair(getInstrumentFromName(instrName), runNumber);
}

InstrumentInfo ISISInstrumentDataCache::getInstrumentFromName(const std::string &instName) const {
  try { // Expand instrument name
    auto instrumentInfo = FileFinder::Instance().getInstrument(instName, false);
    return instrumentInfo;
  } catch (const Kernel::Exception::NotFoundError &) {
    throw std::invalid_argument("Instrument name not recognized.");
  }
}

std::filesystem::path ISISInstrumentDataCache::makeIndexFilePath(const std::string &instrumentName) const {
  g_log.debug() << "ISISInstrumentDataCache::makeIndexFilePath(" << instrumentName << ")" << std::endl;
  auto const &indexFilePath =
      std::filesystem::path(m_dataCachePath) / instrumentName / (instrumentName + "_index.json");
  return indexFilePath;
}

std::pair<std::string, std::string>
ISISInstrumentDataCache::splitIntoInstrumentAndNumber(const std::string &fileName) const {

  // Find the last non-digit as the instrument name can contain numbers
  const auto itRev = std::find_if(fileName.rbegin(), fileName.rend(), std::not_fn(isdigit));
  const auto nChars = std::distance(itRev, fileName.rend());
  std::string runNumber = fileName.substr(nChars);

  std::string fileNameUpperCase = fileName;
  std::transform(fileNameUpperCase.begin(), fileNameUpperCase.end(), fileNameUpperCase.begin(), toupper);
  std::string instrName = fileNameUpperCase.substr(0, nChars);

  return std::pair(instrName, runNumber);
}

/**
 * Check if the data cache index file is available on the current system for a given instrument.
 * @param instrumentName The instrument to find the index file for.
 * @return if there is an index file (and therefore instrument data cache) on the system.
 */
bool ISISInstrumentDataCache::isIndexFileAvailable(std::string const &instrumentName) const {
  return std::filesystem::exists(makeIndexFilePath(instrumentName));
}
