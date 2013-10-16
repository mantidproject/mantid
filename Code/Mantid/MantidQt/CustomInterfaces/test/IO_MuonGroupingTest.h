#ifndef MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_
#define MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <Poco/Path.h>
#include <Poco/File.h>

#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"

using namespace MantidQt::CustomInterfaces::Muon;

class IO_MuonGroupingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IO_MuonGroupingTest *createSuite() { return new IO_MuonGroupingTest(); }
  static void destroySuite( IO_MuonGroupingTest *suite ) { delete suite; }

  /// Constructor
  IO_MuonGroupingTest()
  {
    using Mantid::Kernel::ConfigService;

    auto dataPaths = ConfigService::Instance().getDataSearchDirs();

    // Find the path of AutoTestData
    for(auto it = dataPaths.begin(); it != dataPaths.end(); ++it)
    {
      Poco::Path path(*it);

      if(path.directory(path.depth() - 1) == "AutoTestData")
      {
        m_testDataDir = *it;
        break;
      }
    }

    TSM_ASSERT("Unable to find AutoTestData directory", !m_testDataDir.empty());

    m_tmpDir = ConfigService::Instance().getTempDir();
  }

  void test_loadGroupingFromXML()
  {
    Grouping g;

    TS_ASSERT_THROWS_NOTHING(loadGroupingFromXML(m_testDataDir + "MUSRGrouping.xml", g));

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

  void test_saveGroupingToXML()
  {
    Grouping g, lg;

    std::string tmpFile = m_tmpDir + "tmp_MUSRGrouping.xml";

    // Load grouping first
    TS_ASSERT_THROWS_NOTHING(loadGroupingFromXML(m_testDataDir + "MUSRGrouping.xml", g));

    // Then save it
    TS_ASSERT_THROWS_NOTHING(saveGroupingToXML(g, tmpFile));

    // And load it again
    TS_ASSERT_THROWS_NOTHING(loadGroupingFromXML(tmpFile, lg));

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

};


#endif /* MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_ */