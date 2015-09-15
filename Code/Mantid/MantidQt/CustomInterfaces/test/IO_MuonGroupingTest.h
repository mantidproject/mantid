#ifndef MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_
#define MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_

#include <numeric>

#include <cxxtest/TestSuite.h>
#include <Poco/Path.h>
#include <Poco/File.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Muon/IO_MuonGrouping.h"

using namespace Mantid::API;
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

      if(path.directory(path.depth() - 1) == "UnitTest")
      {
        m_testDataDir = *it;
        break;
      }
    }

    TSM_ASSERT("Unable to find UnitTest data directory", !m_testDataDir.empty());

    m_tmpDir = ConfigService::Instance().getTempDir();

    // To make sure API is initialized properly
    FrameworkManager::Instance();
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

  void test_groupWorkspace()
  {
    // Load grouping for MUSR
    Grouping g;
    TS_ASSERT_THROWS_NOTHING(loadGroupingFromXML(m_testDataDir + "MUSRGrouping.xml", g));

    // Load MUSR data file
    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("LoadMuonNexus");
    loadAlg->setChild(true); // So outptu ws don't end up in the ADS
    loadAlg->initialize();
    loadAlg->setPropertyValue("Filename", "MUSR00015189.nxs");
    loadAlg->setPropertyValue("OutputWorkspace", "data"); // Is not used, just for validator
    loadAlg->execute();

    Workspace_sptr loadedWs = loadAlg->getProperty("OutputWorkspace");
    WorkspaceGroup_sptr loadedGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWs);
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(loadedGroup->getItem(0));

    // Group the loaded workspace using loaded grouping
    MatrixWorkspace_sptr gWs; // Grouped workspace
    TS_ASSERT_THROWS_NOTHING(gWs = groupWorkspace(ws, g));
    TS_ASSERT(gWs);

    if(!gWs) 
      return;

    // Check that was grouped properly
    TS_ASSERT_EQUALS(gWs->getNumberHistograms(), 2);

    TS_ASSERT_EQUALS(gWs->getSpectrum(0)->getDetectorIDs(), setFromRange(33, 64));
    TS_ASSERT_EQUALS(gWs->getSpectrum(1)->getDetectorIDs(), setFromRange(1, 32));

    TS_ASSERT_EQUALS(std::accumulate(gWs->readY(0).begin(), gWs->readY(0).end(), 0.0), 355655);
    TS_ASSERT_DELTA(std::accumulate(gWs->readX(0).begin(), gWs->readX(0).end(), 0.0), 30915.5, 0.1);
    TS_ASSERT_DELTA(std::accumulate(gWs->readE(0).begin(), gWs->readE(0).end(), 0.0), 14046.9, 0.1);

    TS_ASSERT_EQUALS(std::accumulate(gWs->readY(1).begin(), gWs->readY(1).end(), 0.0), 262852);
    TS_ASSERT_EQUALS(gWs->readX(1), gWs->readX(0));
    TS_ASSERT_DELTA(std::accumulate(gWs->readE(1).begin(), gWs->readE(1).end(), 0.0), 12079.8, 0.1);
  }

private:
  std::string m_testDataDir;
  std::string m_tmpDir;

  std::set<int> setFromRange(int from, int to)
  {
    std::set<int> result;

    for(int i = from; i <= to; i++)
      result.insert(i);
    
    return result;
  }

};


#endif /* MANTID_CUSTOMINTERFACES_IO_MUONGROUPINGTEST_H_ */
