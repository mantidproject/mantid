
#include "MantidAPI/ISISInstrDataCache.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iterator>
#include <json/reader.h>
#include <json/value.h>
#include <string>

namespace {
Mantid::Kernel::Logger g_log("ISISInstrDataCache");
} // namespace

std::string Mantid::API::ISISInstrDataCache::getInstrFilePath(std::string fileName) {

  std::string suffix = FileFinder::Instance().extractAllowedSuffix(fileName);
  if (!suffix.empty()) {
    g_log.notice() << "Suffix detected, skipping data cache search ...";
    return "";
  }

  // Find the last non-digit as the instrument name can contain numbers
  std::string::reverse_iterator itRev = std::find_if(fileName.rbegin(), fileName.rend(), std::not_fn(isdigit));
  std::string::size_type nChars = std::distance(itRev, fileName.rend());

  // Check run number
  std::string runNumber = fileName.substr(nChars);
  if (!std::all_of(runNumber.begin(), runNumber.end(), ::isdigit)) { // Check for wrong format
    g_log.notice() << "Filename not in correct format, skipping data cache search ...";
    return "";
  };
  runNumber.erase(0, runNumber.find_first_not_of('0')); // Remove padding zeros

  // Get instrument full name from short name
  std::string instrName = fileName.substr(0, nChars);
  try {
    auto instrInfo = Kernel::ConfigService::Instance().getInstrument(instrName);
    instrName = instrInfo.name();
  } catch (const Kernel::Exception::NotFoundError &) {
    g_log.notice() << "Instrument name not recognized, skipping data cache search ...";
    return "";
  }

  // Build path to index file
  std::string jsonPath = m_dataCachePath + "/" + instrName + "/" + instrName + "_index.json";
  std::ifstream ifstrm{jsonPath};
  if (!ifstrm) {
    return "";
    g_log.notice() << "Error opennig file at " << jsonPath;
  }

  // Read directory path from json file
  Json::Value json;
  ifstrm >> json;
  std::string relativePath = json[runNumber].asString();
  std::string dirPath = m_dataCachePath + "/" + instrName + "/" + relativePath;
  return dirPath;
}
