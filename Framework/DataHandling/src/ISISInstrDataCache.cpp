
#include "MantidDataHandling/ISISInstrDataCache.h"
#include <fstream>
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <string>

// fileName argument as MAR11045
std::string Mantid::DataHandling::ISISInstrDataCache::getInstrFilePath(std::string fileName) {

  // Split into instrument name and run number
  char const *digits = "0123456789";
  std::size_t const n = fileName.find_first_of(digits);
  std::string instrName = fileName.substr(0, n);
  std::string runNumber = fileName.substr(n);
  std::cout << "\n" << instrName << " " << runNumber << std::endl;

  // Build path to index file
  std::string jsonPath = m_dataCachePath + "/" + instrName + "/" + instrName + "_index.json";

  std::ifstream ifstrm{jsonPath};
  Json::Value json;
  ifstrm >> json;

  std::string path = json[runNumber].asString();
  return path;
}
