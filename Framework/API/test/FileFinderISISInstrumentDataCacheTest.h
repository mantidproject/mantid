// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileFinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include <boost/lexical_cast.hpp>
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <stdio.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FileFinderISISInstrumentDataCacheTest : public CxxTest::TestSuite {
private:
  std::set<std::string> m_filesToCreate;
  std::string m_dataCacheDir;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileFinderISISInstrumentDataCacheTest *createSuite() { return new FileFinderISISInstrumentDataCacheTest(); }
  static void destroySuite(FileFinderISISInstrumentDataCacheTest *suite) { delete suite; }

  FileFinderISISInstrumentDataCacheTest()
      : // The constructor will create a temporary directory that mimicks the data cache structure and
        // populate it with the files defined here
        m_filesToCreate({"MER40871.nxs", "MAR26045.raw", "WISH39495.s01", "LOQ106084.nxs", "LARMOR26462.nxs",
                         "ZOOM4656.RAW", "GEM90421.nxs"}),
        m_dataCacheDir("_DataCacheTestDummyData") {

    ConfigService::Instance().setString("datacachesearch.directory", m_dataCacheDir);
    ConfigService::Instance().setString("datasearch.searcharchive", "Off");

    for (auto filename : m_filesToCreate) {
      // Extract extension
      auto it = filename.find('.');
      std::string ext = filename.substr(it);
      filename = filename.substr(0, it);

      // Extract instr and run number
      auto instrRunPair = FileFinder::Instance().toInstrumentAndNumber(filename);

      // Set up instrument directories and subdirectories
      auto instr = FileFinder::Instance().getInstrument(instrRunPair.first);
      std::string instrName = instr.name();
      std::filesystem::path instrDir(m_dataCacheDir + '/' + instrName);
      std::string subDir = "SUBDIR1/SUBDIR2";
      std::filesystem::path instrSubDir(instrDir.string() + '/' + subDir);
      std::filesystem::create_directories(instrSubDir);

      // Create empty file with correct name
      std::string fileToCreateName = FileFinder::Instance().makeFileName(filename, instr);
      std::string fileToCreateStr = instrSubDir.string() + '/' + fileToCreateName + ext;
      std::ofstream file{fileToCreateStr};
      TS_ASSERT(file);
      file.close();

      std::string runNumber = instrRunPair.second;
      runNumber.erase(0, runNumber.find_first_not_of('0')); // Remove padding zeros

      // Create index json file
      std::string jsonStr = "{\"" + runNumber + "\": " + "\"" + subDir + "\"}";
      std::string jsonFilePath = instrDir.string() + '/' + instrName + "_index.json";
      std::ofstream jsonFile{jsonFilePath};
      TS_ASSERT(jsonFile);
      jsonFile << jsonStr;
      jsonFile.close();
    }

    // Remove permissions to test for unauthorized access to instrument folder
    std::filesystem::permissions(m_dataCacheDir + '/' + "GEM/SUBDIR1/SUBDIR2", std::filesystem::perms::none,
                                 std::filesystem::perm_options::replace);
  }

  ~FileFinderISISInstrumentDataCacheTest() override {
    // Change permissions again to allow delete
    std::filesystem::permissions(m_dataCacheDir + '/' + "GEM/SUBDIR1/SUBDIR2", std::filesystem::perms::owner_all,
                                 std::filesystem::perm_options::add);
    // Destroy dummy folder and files.
    std::filesystem::remove_all(m_dataCacheDir);
  }

public:
  void testNormalInput() {
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"MAR26045"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/MARI/SUBDIR1/SUBDIR2/MAR26045.raw");
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"MER40871"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/MERLIN/SUBDIR1/SUBDIR2/MER40871.nxs");
  }

  void testInstrWithLowercase() {
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"mar26045"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/MARI/SUBDIR1/SUBDIR2/MAR26045.raw");
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"mAr26045"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/MARI/SUBDIR1/SUBDIR2/MAR26045.raw");
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"Mer40871"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/MERLIN/SUBDIR1/SUBDIR2/MER40871.nxs");
  }

  void testMissingInstr() {
    ConfigService::Instance().setString("default.instrument", "MAR");
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"26045"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/MARI/SUBDIR1/SUBDIR2/MAR26045.raw");

    ConfigService::Instance().setString("default.instrument", "MER");
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"40871"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/MERLIN/SUBDIR1/SUBDIR2/MER40871.nxs");
  }

  void testZeroPadding() {
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"WISH39495"}, {".raw", ".nxs", ".s01"}).result().string(),
                     m_dataCacheDir + "/WISH/SUBDIR1/SUBDIR2/WISH00039495.s01");
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"LOQ106084"}, {".raw", ".nxs"}).result().string(),
                     m_dataCacheDir + "/LOQ/SUBDIR1/SUBDIR2/LOQ00106084.nxs");
    TS_ASSERT_EQUALS(FileFinder::Instance().findRun({"ZOOM4656"}, {".RAW"}).result().string(),
                     m_dataCacheDir + "/ZOOM/SUBDIR1/SUBDIR2/ZOOM00004656.RAW");
  }

  // void testDataCacheSkipped() {
  //   TS_ASSERT_EQUALS(FileFinder::Instance().getPath({}, {"LOQ106084-add"}, {".raw"}).result().string(), "");
  //   TS_ASSERT_EQUALS(FileFinder::Instance().getPath({}, {"BADINSTR1234"}, {".raw"}).result().string(), "");
  //   TS_ASSERT_EQUALS(FileFinder::Instance().getPath({}, {"path-no-digits"}, {".raw"}).result().string(), "");
  //   TS_ASSERT_EQUALS(FileFinder::Instance().getPath({}, {"1234BADPATH"}, {".raw"}).result().string(), "");
  //   TS_ASSERT_EQUALS(FileFinder::Instance().getPath({}, {"BAD1234PATH"}, {".raw"}).result().string(), "");
  // }

  // void testDirectoryWithoutPermissin() {
  //   std::string error = FileFinder::Instance().getPath({}, {"GEM90421"}, {".nxs"}).errors();
  //   TS_ASSERT(error.find("Permission denied") != std::string::npos);
  // }
};
