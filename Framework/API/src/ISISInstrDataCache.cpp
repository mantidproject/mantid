
#include "MantidAPI/ISISInstrDataCache.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iterator>
#include <json/reader.h>
#include <json/value.h>
#include <string>

Mantid::Kernel::Logger g_log("ISISInstrDataCache");

std::string Mantid::API::ISISInstrDataCache::getInstrFilePath(std::string fileName) {

  // Find the last non-digit as the instrument name can contain numbers
  // TODO: Need to account for edge case 'PG3'
  std::string::reverse_iterator itRev = std::find_if(fileName.rbegin(), fileName.rend(), std::not_fn(isdigit));
  std::string::size_type nChars = std::distance(itRev, fileName.rend());

  // Get instrument full name from short name
  Mantid::Kernel::InstrumentInfo instr =
      Mantid::Kernel::ConfigService::Instance().getInstrument(fileName.substr(0, nChars));
  std::string instrName = instr.name();
  std::string runNumber = fileName.substr(nChars);
  runNumber.erase(0, runNumber.find_first_not_of('0')); // Remove padding zeros

  // Build path to index file
  std::string jsonPath = m_dataCachePath + "/" + instrName + "/" + instrName + "_index.json";
  g_log.notice() << "Trying to open path " << jsonPath;

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
