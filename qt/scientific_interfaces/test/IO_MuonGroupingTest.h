#ifndef MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_
#define MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_

#include <numeric>

#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

#include "../Muon/IO_MuonGrouping.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::Muon;

class IO_MuonGroupingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IO_MuonGroupingTest *createSuite() {
    return new IO_MuonGroupingTest();
  }
  static void destroySuite(IO_MuonGroupingTest *suite) { delete suite; }

  /// Constructor
  IO_MuonGroupingTest() {
    using Mantid::Kernel::ConfigService;

    auto dataPaths = ConfigService::Instance().getDataSearchDirs();

    // Find the path of AutoTestData
    for (auto it = dataPaths.begin(); it != dataPaths.end(); ++it) {
      Poco::Path path(*it);

      if (path.directory(path.depth() - 1) == "UnitTest") {
        m_testDataDir = *it;
        break;
      }
    }

    TSM_ASSERT("Unable to find UnitTest data directory",
               !m_testDataDir.empty());

    m_tmpDir = ConfigService::Instance().getTempDir();

    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_saveGroupingToXML() {
    Grouping g, lg;

    std::string tmpFile = m_tmpDir + "tmp_MUSRGrouping.xml";

    // Load grouping first
    TS_ASSERT_THROWS_NOTHING(API::GroupingLoader::loadGroupingFromXML(
        m_testDataDir + "MUSRGrouping.xml", g));

    // Then save it
    TS_ASSERT_THROWS_NOTHING(MuonGroupingHelper::saveGroupingToXML(g, tmpFile));

    // And load it again
    TS_ASSERT_THROWS_NOTHING(
        API::GroupingLoader::loadGroupingFromXML(tmpFile, lg));

    // Check that all the information was saved
    TS_ASSERT_EQUALS(lg.groupNames.size(), 2);
    TS_ASSERT_EQUALS(lg.groupNames[0], "fwd");
    TS_ASSERT_EQUALS(lg.groupNames[1], "bwd");

    TS_ASSERT_EQUALS(lg.groups.size(), 2);
    TS_ASSERT_EQUALS(lg.groups[0], "33-64");
    TS_ASSERT_EQUALS(lg.groups[1], "1-32");

    TS_ASSERT_EQUALS(lg.pairNames.size(), 1);
    TS_ASSERT_EQUALS(lg.pairNames[0], "long");

    TS_ASSERT_EQUALS(lg.pairs.size(), 1);
    TS_ASSERT_EQUALS(lg.pairs[0].first, 0);
    TS_ASSERT_EQUALS(lg.pairs[0].second, 1);

    TS_ASSERT_EQUALS(lg.pairAlphas.size(), 1);
    TS_ASSERT_EQUALS(lg.pairAlphas[0], 1);

    TS_ASSERT_EQUALS(lg.description, "musr longitudinal (64 detectors)");
    TS_ASSERT_EQUALS(lg.defaultName, "long");

    // Remove temporary file
    Poco::File(tmpFile).remove();
  }

private:
  std::string m_testDataDir;
  std::string m_tmpDir;

  std::set<int> setFromRange(int from, int to) {
    std::set<int> result;

    for (int i = from; i <= to; i++)
      result.insert(i);

    return result;
  }
};

#endif /* MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_ */
