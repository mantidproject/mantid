
#include "MantidAPI/ISISInstrDataCache.h"
#include "MantidKernel/Logger.h"
#include <fstream>
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <string>

Mantid::Kernel::Logger g_log("ISISInstrDataCache");

std::string Mantid::API::ISISInstrDataCache::getInstrFilePath(std::string fileName) {

  // Put all letters to upper case
  std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::toupper);
  // Split into instrument name and run number
  std::size_t const runNumStart = fileName.find_first_of("0123456789");
  std::string instrName = fileName.substr(0, runNumStart);
  std::string runNumber = fileName.substr(runNumStart);

  // Build path to index file
  std::string jsonPath = m_dataCachePath + "/" + instrName + "/" + instrName + "_index.json";

  std::ifstream ifstrm{jsonPath};
  if (!ifstrm) {
    return "";
    g_log.notice() << "Error opennig file, returning empty string.";
  }
  Json::Value json;
  ifstrm >> json;
  std::string relativePath = json[runNumber].asString();
  std::string dirPath = m_dataCachePath + "/" + instrName + "/" + relativePath;
  return dirPath;
}
