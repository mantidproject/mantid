#ifndef GLOBTEST_H_
#define GLOBTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Glob.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/Path.h>
#include <Poco/File.h>

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
    TS_ASSERT_EQUALS(base[base.depth() - 1], "Mantid");
    std::string pattern = base.toString() + "Framework/*/CMakeLists.*t";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(files.size() > 0);

    size_t matches = 0;
    for (std::set<std::string>::const_iterator f = files.begin();
         f != files.end(); ++f) {
      Poco::Path path = *f;
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

    Poco::File testDir("GlobTestDir");
    testDir.createDirectory();
    Poco::File file1("GlobTestDir/File.1");
    file1.createFile();
    Poco::File file2("GlobTestDir/File.2");
    file2.createFile();

    std::string pattern = "GlobTestDir/File.*";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT_EQUALS(files.size(), 2);
    file1.remove();
    file2.remove();
    testDir.remove();
  }

  void test_double_dots_in_pattern() {
    TS_ASSERT_EQUALS(base[base.depth() - 1], "Mantid");
    std::string pattern =
        base.toString() + "../Mantid/Framework/*/CMakeLists.*t";

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(files.size() > 0);

    size_t matches = 0;
    for (std::set<std::string>::const_iterator f = files.begin();
         f != files.end(); ++f) {
      Poco::Path path = *f;
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
    Poco::Path pattern(base.toString() + "instrument",
                       "IDFs_for_UNIT_TESTING/DUM_Definition.xml");

    std::set<std::string> files;
    Glob::glob(pattern, files);
    TS_ASSERT(!files.empty());
  }

  void test_caseless() {
    Poco::Path pattern(base.toString() + "instrument",
                       "IDFs_for_unit_TESTING/dum_Definition.xml");

    std::set<std::string> files;
    Glob::glob(pattern, files, Poco::Glob::GLOB_CASELESS);
    TS_ASSERT(!files.empty());
  }
};

#endif
