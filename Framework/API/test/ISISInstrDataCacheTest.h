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
"1234": "2023/MR1234",
"2342": "2023/MR2342",
"6789": "2023/MR6789"
}
)";

std::string alfJson = R"({
"89123": "2024/RB2220540-3",
"89124": "2024/RB2220540-3",
"89125": "2024/RB2220540-3"
}
)";

std::unordered_map<std::string, std::string> instrFiles = {{"MAR", marJson}, {"ALF", alfJson}};

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
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/MAR/2023/MR1234");
    actualPath = dc.getInstrFilePath("ALF89123");
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/ALF/2024/RB2220540-3");
  }

private:
  std::string m_dataCacheDir;
};
