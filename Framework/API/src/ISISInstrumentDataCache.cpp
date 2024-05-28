#include "MantidAPI/ISISInstrumentDataCache.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <filesystem>
#include <fstream>
#include <json/reader.h>

using Mantid::API::ISISInstrumentDataCache;

namespace {
Mantid::Kernel::Logger g_log("ISISInstrumentDataCache");
} // namespace

std::string Mantid::API::ISISInstrumentDataCache::getFileParentDirectoryPath(const std::string &fileName) const {
  g_log.debug() << "ISISInstrumentDataCache::getFileParentDirectoryPath(" << fileName << ")" << std::endl;

  auto [instrumentInfo, runNumber] = validateInstrumentAndNumber(fileName);
  std::string instrName = instrumentInfo.name();

  // Open index json file
  std::string jsonPath = makeIndexFilePath(instrName);
  std::ifstream ifstrm{jsonPath};
  if (!ifstrm.is_open()) { // Try again with shortname
    instrName = instrumentInfo.shortName();
    jsonPath = makeIndexFilePath(instrName);
    ifstrm.open(jsonPath);
    if (!ifstrm.is_open()) {
      throw std::invalid_argument("Could not open index file: " + jsonPath);
    }
  }

  // Read directory path from json file
  Json::Value json;
  ifstrm >> json;
  std::string relativePath = json[runNumber].asString();

  if (relativePath.empty()) {
    throw std::invalid_argument("Run number " + runNumber + " not found for instrument " + instrName + ".");
  }

  std::string dirPath = m_dataCachePath + "/" + instrName + "/" + relativePath;

  g_log.debug() << "Opened instrument index file: " << jsonPath << ". Found path to search: " << dirPath << "."
                << std::endl;
  return dirPath;
}

std::pair<Mantid::Kernel::InstrumentInfo, std::string>
Mantid::API::ISISInstrumentDataCache::validateInstrumentAndNumber(const std::string &fileName) const {

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

  try { // Expand instrument name
    auto instrumentInfo = FileFinder::Instance().getInstrument(instrName, false);
    return std::pair(instrumentInfo, runNumber);

  } catch (const Kernel::Exception::NotFoundError &) {
    throw std::invalid_argument("Instrument name not recognized.");
  }
}

std::string Mantid::API::ISISInstrumentDataCache::makeIndexFilePath(const std::string &instrumentName) const {
  g_log.debug() << "ISISInstrumentDataCache::makeIndexFilePath(" << instrumentName << ")" << std::endl;
  std::string indexFilePath = m_dataCachePath + "/" + instrumentName + "/" + instrumentName + "_index.json";
  return indexFilePath;
}

std::pair<std::string, std::string>
Mantid::API::ISISInstrumentDataCache::splitIntoInstrumentAndNumber(const std::string &fileName) const {

  // Find the last non-digit as the instrument name can contain numbers
  const auto itRev = std::find_if(fileName.rbegin(), fileName.rend(), std::not_fn(isdigit));
  const auto nChars = std::distance(itRev, fileName.rend());
  std::string runNumber = fileName.substr(nChars);

  std::string fileNameUpperCase = fileName;
  std::transform(fileNameUpperCase.begin(), fileNameUpperCase.end(), fileNameUpperCase.begin(), toupper);
  std::string instrName = fileNameUpperCase.substr(0, nChars);

  return std::pair(instrName, runNumber);
}

bool ISISInstrumentDataCache::isIndexFileAvailable(std::string const &instrument) const {
  std::filesystem::path const indexPath =
      std::filesystem::path(m_dataCachePath) / instrument / (instrument + "_index.json");
  return std::filesystem::exists(indexPath);
}
