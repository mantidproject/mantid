#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "MantidDataHandling/ISISInstrDataCache.h"

using namespace Mantid::DataHandling;

std::string jsonTest = R"({
"1234": "some/1234/path",
"2342": "some/2342/path",
"6789": "some/6789/path"
}
)";

class ISISInstrDataCacheTest : public CxxTest::TestSuite {
public:
  void testGetCorrectFilePath() {

    // Create test JSON file
    std::filesystem::create_directory("./MAR");
    std::ofstream ofstrm{"./MAR/index.json"};
    if (!ofstrm)
      std::cout << "\nCould not open file!\n";
    ofstrm << jsonTest;
    if (ofstrm)
      std::cout << "\nWrote to file!\n" << std::endl;
    ofstrm.close();

    ISISInstrDataCache dc(".");
    std::string actualPath = dc.getInstrFilePath("MAR1234");
    TS_ASSERT_EQUALS(actualPath, "some/1234/path");
    actualPath = dc.getInstrFilePath("MAR6789");
    TS_ASSERT_EQUALS(actualPath, "some/6789/path");
  }
};
