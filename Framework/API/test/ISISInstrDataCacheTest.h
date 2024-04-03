#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "MantidAPI/ISISInstrDataCache.h"
#include "MantidKernel/Strings.h"
#include <boost/algorithm/string.hpp>

using namespace Mantid::API;

std::string marJson = R"({
"25054": "2019/RB1868000-1",
}
)";

std::string sansJson = R"({
"101115": "2018/RB1800009-2",
}
)";

std::unordered_map<std::string, std::string> instrFiles = {{"MARI", marJson}, {"SANS2D", sansJson}};

class ISISInstrDataCacheTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_dataCacheDir = "./TestDataCache";
    // Create test JSON file
    std::filesystem::create_directory(m_dataCacheDir);

    for (const auto &[instrName, instrIndex] : instrFiles) {

      std::filesystem::create_directory(m_dataCacheDir + "/" + instrName);
      std::ofstream ofstrm{m_dataCacheDir + "/" + instrName + "/" + instrName + "_index.json"};

      if (!ofstrm)
        std::cout << "\nCould not open file!\n";
      ofstrm << instrIndex;
      if (ofstrm)
        std::cout << "\nWrote to file!\n" << std::endl;
      ofstrm.close();
    }
  };

  void tearDown() override { std::filesystem::remove_all(m_dataCacheDir); };

  void testInstrNameExpanded() {
    ISISInstrDataCache dc(m_dataCacheDir);
    std::string actualPath = dc.getInstrFilePath("MAR25054");
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/MARI/2019/RB1868000-1");
  }

  void testCorrectInstrRunSplit() {
    ISISInstrDataCache dc(m_dataCacheDir);
    std::string actualPath = dc.getInstrFilePath("SANS2D101115");
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/SANS2D/2018/RB1800009-2");
  }

private:
  std::string m_dataCacheDir;
};
