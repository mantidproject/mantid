#include "MantidAPI/ISISInstrDataCache.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <fstream>
#include <json/reader.h>
#include <json/value.h>

namespace {
Mantid::Kernel::Logger g_log("ISISInstrDataCache");
} // namespace

std::string Mantid::API::ISISInstrDataCache::getFileParentDirPath(std::string fileName) {
  g_log.debug() << "ISISInstrDataCache::getFileParentDirPath(" << fileName << ")" << std::endl;

  // Check if suffix eg. -add is present in filename
  std::string suffix = FileFinder::Instance().extractAllowedSuffix(fileName);
  if (!suffix.empty()) {
    throw std::invalid_argument("Unsuported format: Suffix detected: " + suffix);
  }

  // Find the last non-digit as the instrument name can contain numbers
  std::string::reverse_iterator itRev = std::find_if(fileName.rbegin(), fileName.rend(), std::not_fn(isdigit));
  std::string::size_type nChars = std::distance(itRev, fileName.rend());

  // Check run number
  std::string runNumber = fileName.substr(nChars);
  if (runNumber.empty() || !std::all_of(runNumber.begin(), runNumber.end(), ::isdigit)) { // Check for wrong format
    throw std::invalid_argument("Filename not in correct format.");
  };
  runNumber.erase(0, runNumber.find_first_not_of('0')); // Remove padding zeros

  // Get instrument full name from short name
  std::transform(fileName.begin(), fileName.end(), fileName.begin(), toupper);
  std::string instrName = fileName.substr(0, nChars);
  try {
    auto instrInfo = FileFinder::Instance().getInstrument(instrName, false);
    instrName = instrInfo.name();
  } catch (const Kernel::Exception::NotFoundError &) {
    throw std::invalid_argument("Instrument name not recognized.");
  }

  // Build path to index file
  std::string jsonPath = m_dataCachePath + "/" + instrName + "/" + instrName + "_index.json";
  g_log.debug() << "Opening instrument index file at " << jsonPath << std::endl;
  std::ifstream ifstrm{jsonPath};
  if (!ifstrm) {
    throw std::invalid_argument("Error opennig instrument index file: " + jsonPath);
  }

  // Read directory path from json file
  Json::Value json;
  ifstrm >> json;
  std::string relativePath = json[runNumber].asString();

  if (relativePath.empty()) {
    throw std::invalid_argument("Run number " + runNumber + " not found for instrument " + instrName + ".");
  }

  std::string dirPath = m_dataCachePath + "/" + instrName + "/" + relativePath;
  return dirPath;
}
