#ifndef MANTID_DATAHANDLING_ApplyGroupingFromMuonNexusTEST_H_
#define MANTID_DATAHANDLING_ApplyGroupingFromMuonNexusTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/ApplyGroupingFromMuonNexus.h"
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/scoped_array.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class ApplyGroupingFromMuonNexusTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyGroupingFromMuonNexusTest *createSuite() { return new ApplyGroupingFromMuonNexusTest(); }
  static void destroySuite( ApplyGroupingFromMuonNexusTest *suite ) { delete suite; }


  void test_init()
  {
    ApplyGroupingFromMuonNexus alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_execSingle()
  {
    AnalysisDataService::Instance().clear();

    // Name of the output workspace.
    std::string loadedWsName("ApplyGroupingFromMuonNexusTest_LoadedWS");
    std::string outWsName("ApplyGroupingFromMuonNexusTest_OutputWS");
    std::string dataFileName("emu00006473");

    // Load the data we will group
    LoadMuonNexus1 loadAlg;
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", dataFileName);
    loadAlg.setPropertyValue("OutputWorkspace", loadedWsName);
    loadAlg.execute();

    ApplyGroupingFromMuonNexus alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", loadedWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", dataFileName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    Workspace2D_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outWsName) );
    TS_ASSERT(ws);

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->readY(0).size(), 2000);

    TS_ASSERT_EQUALS(ws->readX(0), ws->readX(1));
    TS_ASSERT_DELTA(std::accumulate(ws->readX(0).begin(), ws->readX(0).end(), 0.0), 31507.736, 0.001);

    TS_ASSERT_EQUALS(std::accumulate(ws->readY(0).begin(), ws->readY(0).end(), 0), 32571161);
    TS_ASSERT_EQUALS(std::accumulate(ws->readY(1).begin(), ws->readY(1).end(), 0), 18184711);

    TS_ASSERT_DELTA(std::accumulate(ws->readE(0).begin(), ws->readE(0).end(), 0.0), 133292.1, 0.1);
    TS_ASSERT_DELTA(std::accumulate(ws->readE(1).begin(), ws->readE(1).end(), 0.0), 101157.1, 0.1);

    AnalysisDataService::Instance().clear();
  }

  void test_execGroup()
  {
    AnalysisDataService::Instance().clear();

    // Name of the output workspace.
    std::string loadedWsName("ApplyGroupingFromMuonNexusTest_LoadedWS");
    std::string outWsName("ApplyGroupingFromMuonNexusTest_OutputWS");
    std::string dataFileName("MUSR00015189");

    // Load the data we will group
    LoadMuonNexus1 loadAlg;
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", dataFileName);
    loadAlg.setPropertyValue("OutputWorkspace", loadedWsName);
    loadAlg.execute();

    ApplyGroupingFromMuonNexus alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", loadedWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", dataFileName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    WorkspaceGroup_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outWsName) );
    TS_ASSERT(ws);
    
    TS_ASSERT_EQUALS(ws->size(), 2);
    
    // Check the first workspace in group ----------------------------------------------------------

    Workspace2D_sptr ws1 = boost::dynamic_pointer_cast<Workspace2D>(ws->getItem(0));
    TS_ASSERT(ws1);

    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws1->readY(0).size(), 2000);

    TS_ASSERT_EQUALS(ws1->readX(0), ws1->readX(1));
    TS_ASSERT_DELTA(std::accumulate(ws1->readX(0).begin(), ws1->readX(0).end(), 0.0), 30915.451, 0.001);

    TS_ASSERT_EQUALS(std::accumulate(ws1->readY(0).begin(), ws1->readY(0).end(), 0), 355655);
    TS_ASSERT_EQUALS(std::accumulate(ws1->readY(1).begin(), ws1->readY(1).end(), 0), 262852);

    TS_ASSERT_DELTA(std::accumulate(ws1->readE(0).begin(), ws1->readE(0).end(), 0.0), 14046.9, 0.1);
    TS_ASSERT_DELTA(std::accumulate(ws1->readE(1).begin(), ws1->readE(1).end(), 0.0), 12079.8, 0.1);

    // Check the second workspace in group ---------------------------------------------------------

    Workspace2D_sptr ws2 = boost::dynamic_pointer_cast<Workspace2D>(ws->getItem(1));
    TS_ASSERT(ws2);

    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws2->readY(0).size(), 2000);

    TS_ASSERT_EQUALS(ws1->readX(0), ws2->readX(0));
    TS_ASSERT_EQUALS(ws2->readX(0), ws2->readX(1));

    TS_ASSERT_EQUALS(std::accumulate(ws2->readY(0).begin(), ws2->readY(0).end(), 0), 359076);
    TS_ASSERT_EQUALS(std::accumulate(ws2->readY(1).begin(), ws2->readY(1).end(), 0), 258629);

    TS_ASSERT_DELTA(std::accumulate(ws2->readE(0).begin(), ws2->readE(0).end(), 0.0), 14054.2, 0.1);
    TS_ASSERT_DELTA(std::accumulate(ws2->readE(1).begin(), ws2->readE(1).end(), 0.0), 11976.0, 0.1);

    AnalysisDataService::Instance().clear();
  }

};


#endif /* MANTID_DATAHANDLING_ApplyGroupingFromMuonNexusTEST_H_ */