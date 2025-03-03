// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidKernel/UsageService.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ClearCache.h"
#include <Poco/File.h>

#include <filesystem>

using Mantid::Algorithms::ClearCache;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class ClearCacheTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClearCacheTest *createSuite() { return new ClearCacheTest(); }
  static void destroySuite(ClearCacheTest *suite) { delete suite; }

  void setUp() override {
    const std::string TEST_SUFFIX = "TEMPORARY_ClearCacheUnitTest";
    m_originalInstDir = Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();

    // change the local download directory by adding a unittest subdirectory
    auto testDirectories = m_originalInstDir;
    std::filesystem::path localDownloadPath = m_originalInstDir[0] / TEST_SUFFIX;
    m_localInstDir = localDownloadPath;
    createDirectory(localDownloadPath);
    testDirectories[0] = m_localInstDir;

    Mantid::Kernel::ConfigService::Instance().setInstrumentDirectories(testDirectories);

    // create a geometryCache subdirectory
    std::filesystem::path GeomPath = localDownloadPath / "geometryCache";
    createDirectory(GeomPath);
  }

  void createDirectory(const std::filesystem::path &path) {
    Poco::File file(path.string());
    if (file.createDirectory()) {
      m_directoriesToRemove.emplace_back(file);
    }
  }

  void removeDirectories() {
    for (auto directory : m_directoriesToRemove) {
      try {
        if (directory.exists()) {
          directory.remove(true);
        }
      } catch (Poco::FileException &fe) {
        std::cout << fe.what() << std::endl;
      }
    }
    m_directoriesToRemove.clear();
  }

  void tearDown() override {
    Mantid::Kernel::ConfigService::Instance().setInstrumentDirectories(m_originalInstDir);
    removeDirectories();
  }

  void test_Init() {
    ClearCache alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_Algorithm_Cache() {
    ClearCache alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AlgorithmCache", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 0);
    int filesRemoved = alg.getProperty("FilesRemoved");
    TS_ASSERT_EQUALS(filesRemoved, 0);
  }

  void test_exec_Instrument_Cache() {
    ClearCache alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InstrumentCache", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(InstrumentDataService::Instance().size(), 0);
    int filesRemoved = alg.getProperty("FilesRemoved");
    TS_ASSERT_EQUALS(filesRemoved, 0);
  }

  void test_exec_DownloadInstrument_Cache() {
    ClearCache alg;

    auto instrumentDirs = ConfigService::Instance().getInstrumentDirectories();
    std::filesystem::path localPath(instrumentDirs[0]);
    if (std::filesystem::exists(localPath) && !std::filesystem::is_directory(localPath)) {
      localPath = localPath.parent_path();
    }
    // create a file in the directory
    Poco::File testFile((localPath / "test_exec_DownloadInstrument_Cache.xml").string());
    testFile.createFile();

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DownloadedInstrumentFileCache", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TSM_ASSERT("The test file has not been deleted", !testFile.exists());
    int filesRemoved = alg.getProperty("FilesRemoved");
    TS_ASSERT_LESS_THAN_EQUALS(1, filesRemoved);
  }

  void test_exec_Geometry_Cache() {
    ClearCache alg;

    auto instrumentDirs = ConfigService::Instance().getInstrumentDirectories();
    std::filesystem::path localPath(instrumentDirs[0]);
    if (std::filesystem::exists(localPath) && !std::filesystem::is_directory(localPath)) {
      localPath = localPath.parent_path();
    }

    // create a file in the directory
    Poco::File testFile((localPath / "geometryCache" / "test_exec_Geometry_Cache.vtp").string());
    testFile.createFile();

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GeometryFileCache", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TSM_ASSERT("The test file has not been deleted", !testFile.exists());
    int filesRemoved = alg.getProperty("FilesRemoved");
    TS_ASSERT_LESS_THAN_EQUALS(1, filesRemoved);
  }

  void test_exec_Usage_Cache() {
    ClearCache alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UsageServiceCache", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    int filesRemoved = alg.getProperty("FilesRemoved");
    TS_ASSERT_EQUALS(filesRemoved, 0);
  }

  std::filesystem::path m_localInstDir;
  std::vector<std::filesystem::path> m_originalInstDir;
  std::vector<Poco::File> m_directoriesToRemove;
};
