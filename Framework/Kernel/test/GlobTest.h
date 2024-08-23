// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Glob.h"

#include <Poco/Path.h>
#include <filesystem>
#include <fstream>

using namespace Mantid::Kernel;

class GlobTest : public CxxTest::TestSuite {
  Poco::Path base;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GlobTest *createSuite() { return new GlobTest(); }
  static void destroySuite(GlobTest *suite) { delete suite; }

  GlobTest() {
    base.assign(ConfigService::Instance().getInstrumentDirectory());
    base.makeParent();
    base.assign(base.toString());
  }

  void test_Glob() {
    std::string pattern = base.toString() + "Framework/*/CMakeLists.*t";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(files.size() > 0);

    size_t matches = 0;
    for (const auto &file : files) {
      Poco::Path path = file;
      std::string project = path[path.depth() - 1];
      if (project == "API")
        ++matches;
      if (project == "Algorithms")
        ++matches;
      if (project == "Kernel")
        ++matches;
      if (project == "Geometry")
        ++matches;
      if (project == "DataObjects")
        ++matches;
      TS_ASSERT_EQUALS(path.getFileName(), "CMakeLists.txt");
    }
    TS_ASSERT_EQUALS(matches, 5);
  }

  void test_no_match() {
    std::string pattern = base.toString() + "Doesnotexist/*/CMakeLists.*t";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(files.empty());
  }

  void test_no_match_1() {
    std::string pattern = "Doesnotexist/*/File.*";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(files.empty());
  }

  void test_match_relative_path() {
    std::filesystem::path testDir("GlobTestDir");
    TS_ASSERT(std::filesystem::create_directory(testDir));

    std::filesystem::path file1("GlobTestDir/File.1");
    {
      std::ofstream handle(file1);
      handle.close();
    }

    std::filesystem::path file2("GlobTestDir/File.2");
    {
      std::ofstream handle(file2);
      handle.close();
    }

    std::string pattern = "GlobTestDir/File.*";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT_EQUALS(files.size(), 2);
    std::filesystem::remove(file1);
    std::filesystem::remove(file2);
    std::filesystem::remove(testDir);
  }

  void test_double_dots_in_pattern() {
    std::string pattern = base.toString() + "Framework/*/CMakeLists.*t";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(files.size() > 0);

    size_t matches = 0;
    for (const auto &file : files) {
      Poco::Path path = file;
      std::string project = path[path.depth() - 1];
      if (project == "API")
        ++matches;
      if (project == "Algorithms")
        ++matches;
      if (project == "Kernel")
        ++matches;
      if (project == "Geometry")
        ++matches;
      if (project == "DataObjects")
        ++matches;
      TS_ASSERT_EQUALS(path.getFileName(), "CMakeLists.txt");
    }
    TS_ASSERT_EQUALS(matches, 5);
  }

  void test_filename_contains_directory() {
    Poco::Path pattern(base.toString() + "instrument", "unit_testing/DUM_Definition.xml");

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(!files.empty());
  }

  void test_caseless() {
    Poco::Path pattern(base.toString() + "instrument", "unit_TESTING/dum_Definition.xml");

    std::set<std::string> files;
    Glob::glob(pattern, files, Poco::Glob::GLOB_CASELESS);
    TS_ASSERT(!files.empty());
  }
};
