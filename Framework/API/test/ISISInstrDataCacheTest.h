#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "MantidAPI/ISISInstrDataCache.h"

using namespace Mantid::API;

std::string marJson = R"({
"1234": "some/1234/path",
"2342": "some/2342/path",
"6789": "some/6789/path"
}
)";

std::string wishJson = R"({
"1111": "some/111/path",
"22222": "some/22222/path",
"66666": "some/66666/path"
}
)";

std::unordered_map<std::string, std::string> instrFiles = {{"MAR", marJson}, {"WISH", wishJson}};

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

  void testGetCorrectFilePath() {
    ISISInstrDataCache dc(m_dataCacheDir);
    std::string actualPath = dc.getInstrFilePath("MAR1234");
    TS_ASSERT_EQUALS(actualPath, "some/1234/path");
    actualPath = dc.getInstrFilePath("WISH22222");
    TS_ASSERT_EQUALS(actualPath, "some/22222/path");
  }

private:
  std::string m_dataCacheDir;
};
