#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "MantidAPI/ISISInstrumentDataCache.h"
#include "MantidKernel/Strings.h"
#include <boost/algorithm/string.hpp>

using namespace Mantid::API;

class ISISInstrumentDataCacheTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_dataCacheDir = "./TestDataCache";

    // Example index files
    std::string marJson = R"({"25054": "2019/RB1868000-1"})";
    std::string sansJson = R"({"101115": "2018/RB1800009-2"})";
    std::string pg3Json = R"({"11111": "mock/path"})";
    std::string wishJson = R"({"12345": "subdir1/subdir2"})";

    // Create test JSON file
    std::filesystem::create_directory(m_dataCacheDir);

    std::unordered_map<std::string, std::string> instrFiles = {
        {"MARI", marJson}, {"SANS2D", sansJson}, {"POWGEN", pg3Json}, {"WISH", wishJson}};
    for (const auto &[instrName, instrIndex] : instrFiles) {

      std::filesystem::create_directory(m_dataCacheDir + "/" + instrName);

      if (instrName == "WISH") {
        // Do not create index file to test for missing file
      } else {
        std::ofstream ofstrm{m_dataCacheDir + "/" + instrName + "/" + instrName + "_index.json"};
        ofstrm << instrIndex;
        ofstrm.close();
      }
    }
  }

  void tearDown() override { std::filesystem::remove_all(m_dataCacheDir); }

  void testInstrNameExpanded() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    std::string actualPath = dc.getFileParentDirectoryPath("MAR25054");
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/MARI/2019/RB1868000-1");
  }

  void testLowerCaseInstrName() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    std::string actualPath = dc.getFileParentDirectoryPath("mar25054");
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/MARI/2019/RB1868000-1");
  }

  void testCorrectInstrRunSplit() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    std::string actualPath = dc.getFileParentDirectoryPath("SANS2D101115");
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/SANS2D/2018/RB1800009-2");
  }

  void testInstrWithDelimiter() {
    // Checks short name + delimiter gets correctly identified
    ISISInstrumentDataCache dc(m_dataCacheDir);
    std::string actualPath = dc.getFileParentDirectoryPath("PG3_11111");
    TS_ASSERT_EQUALS(actualPath, m_dataCacheDir + "/POWGEN/mock/path");
  }

  void testInstrWithSuffix() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    TS_ASSERT_THROWS_EQUALS(dc.getFileParentDirectoryPath("LOQ11111-add"), const std::invalid_argument &e,
                            std::string(e.what()), "Unsuported format: Suffix detected: -add");
  }

  void testBadInput() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    TS_ASSERT_THROWS_EQUALS(dc.getFileParentDirectoryPath("s0me_us$r_dEfined_n4me"), const std::invalid_argument &e,
                            std::string(e.what()), "Filename not in correct format.");
  }

  void testBadInstrument() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    TS_ASSERT_THROWS_EQUALS(dc.getFileParentDirectoryPath("BADINSTR111111"), const std::invalid_argument &e,
                            std::string(e.what()), "Instrument name not recognized.");
  }

  void testMissingIndexFile() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    TS_ASSERT_THROWS_EQUALS(dc.getFileParentDirectoryPath("WISH12345"), const std::invalid_argument &e,
                            std::string(e.what()),
                            "Could not open index file: " + m_dataCacheDir + "/WISH/WISH_index.json");
  }

  void testRunNumberNotFound() {
    ISISInstrumentDataCache dc(m_dataCacheDir);
    TS_ASSERT_THROWS_EQUALS(dc.getFileParentDirectoryPath("SANS2D1234"), const std::invalid_argument &e,
                            std::string(e.what()), "Run number 1234 not found for instrument SANS2D.");
  }

private:
  std::string m_dataCacheDir;
};
