#include "MantidAPI/ISISInstrumentDataCache.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <Poco/Path.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/reader.h>
#include <json/value.h>

namespace {
Mantid::Kernel::Logger g_log("ISISInstrumentDataCache");
} // namespace

std::string Mantid::API::ISISInstrumentDataCache::getFileParentDirectoryPath(std::string fileName) {
  g_log.debug() << "ISISInstrumentDataCache::getFileParentDirectoryPath(" << fileName << ")" << std::endl;

  auto [instrName, runNumber] = validateInstrumentAndNumber(fileName);

  // Open index json file
  std::string jsonPath = m_dataCachePath + "/" + instrName + "/" + instrName + "_index.json";
  std::ifstream ifstrm(jsonPath);
  if (!ifstrm.is_open()) {
    throw std::invalid_argument("Error opennig instrument index file: " + jsonPath);
  }
  std::string jsonString = std::string(std::istreambuf_iterator<char>(ifstrm), std::istreambuf_iterator<char>());
  ifstrm.close();

  std::stringstream sstream(jsonString);

  // Read directory path from json file
  Json::Value json;
  std::string relativePath;

  try {
    sstream >> json;
    relativePath = json[runNumber].asString();
  } catch (...) {
    try {
      std::filesystem::copy_file(jsonPath, Poco::Path::home() + instrName + "_index_copy.txt",
                                 std::filesystem::copy_options::overwrite_existing);
    } catch (...) {
      g_log.debug() << "Error while copying the JSON file.";
    }

    std::ofstream ofstrm{Poco::Path::home() + "/" + instrName + "_index_log.txt"};
    std::cout << jsonString << std::endl;
    ofstrm << jsonString;
    ofstrm.close();
    g_log.debug() << "\n\nWrote string to json log to file!\n\n" << std::endl;
    throw;
  }

  if (relativePath.empty()) {
    throw std::invalid_argument("Run number " + runNumber + " not found for instrument " + instrName + ".");
  }

  std::string dirPath = m_dataCachePath + "/" + instrName + "/" + relativePath;

  g_log.debug() << "Opened instrument index file: " << jsonPath << ". Found path to search: " << dirPath << "."
                << std::endl;
  return dirPath;
}

std::pair<std::string, std::string>
Mantid::API::ISISInstrumentDataCache::validateInstrumentAndNumber(std::string fileName) {

  // Check if suffix eg. -add is present in filename
  std::string suffix = FileFinder::Instance().extractAllowedSuffix(fileName);
  if (!suffix.empty()) {
    throw std::invalid_argument("Unsuported format: Suffix detected: " + suffix);
  }

  auto [instrName, runNumber] = splitIntoInstrumentAndNumber(fileName);

  if (runNumber.empty() || !std::all_of(runNumber.begin(), runNumber.end(), ::isdigit)) { // Check run number
    throw std::invalid_argument("Filename not in correct format.");
  }
  runNumber.erase(0, runNumber.find_first_not_of('0')); // Remove padding zeros

  try { // Expand instrument name
    instrName = FileFinder::Instance().getInstrument(instrName, false).name();
  } catch (const Kernel::Exception::NotFoundError &) {
    throw std::invalid_argument("Instrument name not recognized.");
  }

  return std::pair(instrName, runNumber);
}

std::pair<std::string, std::string>
Mantid::API::ISISInstrumentDataCache::splitIntoInstrumentAndNumber(std::string fileName) {

  // Find the last non-digit as the instrument name can contain numbers
  const auto itRev = std::find_if(fileName.rbegin(), fileName.rend(), std::not_fn(isdigit));
  const auto nChars = std::distance(itRev, fileName.rend());
  std::string runNumber = fileName.substr(nChars);

  std::transform(fileName.begin(), fileName.end(), fileName.begin(), toupper);
  std::string instrName = fileName.substr(0, nChars);

  return std::pair(instrName, runNumber);
}
