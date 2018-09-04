#ifndef MANTID_API_GROUPINGLOADERTEST_H_
#define MANTID_API_GROUPINGLOADERTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

using Mantid::API::Grouping;
using Mantid::API::GroupingLoader;

class GroupingLoaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupingLoaderTest *createSuite() { return new GroupingLoaderTest(); }
  static void destroySuite(GroupingLoaderTest *suite) { delete suite; }

  /// Constructor
  GroupingLoaderTest() {
    using Mantid::Kernel::ConfigService;

    auto dataPaths = ConfigService::Instance().getDataSearchDirs();

    // Find the path of AutoTestData
    for (auto &dataPath : dataPaths) {
      Poco::Path path(dataPath);

      if (path.directory(path.depth() - 1) == "UnitTest") {
        m_testDataDir = dataPath;
        break;
      }
    }

    TSM_ASSERT("Unable to find UnitTest data directory",
               !m_testDataDir.empty());

    m_tmpDir = ConfigService::Instance().getTempDir();

    // To make sure API is initialized properly
    Mantid::API::FrameworkManager::Instance();
  }

  void test_loadGroupingFromXML() {
    Grouping g;

    TS_ASSERT_THROWS_NOTHING(GroupingLoader::loadGroupingFromXML(
        m_testDataDir + "MUSRGrouping.xml", g));

    TS_ASSERT_EQUALS(g.groupNames.size(), 2);
    TS_ASSERT_EQUALS(g.groupNames[0], "fwd");
    TS_ASSERT_EQUALS(g.groupNames[1], "bwd");

    TS_ASSERT_EQUALS(g.groups.size(), 2);
    TS_ASSERT_EQUALS(g.groups[0], "33-64");
    TS_ASSERT_EQUALS(g.groups[1], "1-32");

    TS_ASSERT_EQUALS(g.pairNames.size(), 1);
    TS_ASSERT_EQUALS(g.pairNames[0], "long");

    TS_ASSERT_EQUALS(g.pairs.size(), 1);
    TS_ASSERT_EQUALS(g.pairs[0].first, 0);
    TS_ASSERT_EQUALS(g.pairs[0].second, 1);

    TS_ASSERT_EQUALS(g.pairAlphas.size(), 1);
    TS_ASSERT_EQUALS(g.pairAlphas[0], 1);

    TS_ASSERT_EQUALS(g.description, "musr longitudinal (64 detectors)");
    TS_ASSERT_EQUALS(g.defaultName, "long");
  }

private:
  std::string m_testDataDir;
  std::string m_tmpDir;
};

#endif /* MANTID_API_GROUPINGLOADERTEST_H_ */
