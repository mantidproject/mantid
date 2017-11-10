#ifndef MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESSTEST_H_
#define MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidWorkflowAlgorithms/LoadEventAndCompress.h"

using Mantid::WorkflowAlgorithms::LoadEventAndCompress;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class LoadEventAndCompressTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadEventAndCompressTest *createSuite() {
    return new LoadEventAndCompressTest();
  }
  static void destroySuite(LoadEventAndCompressTest *suite) { delete suite; }

  void test_Init() {
    LoadEventAndCompress alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    const std::string FILENAME("ARCS_sim_event.nxs");

    // run without chunks
    const std::string WS_NAME_NO_CHUNKS("LoadEventAndCompress_no_chunks");
    LoadEventAndCompress algWithoutChunks;
    TS_ASSERT_THROWS_NOTHING(algWithoutChunks.initialize());
    TS_ASSERT(algWithoutChunks.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        algWithoutChunks.setPropertyValue("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(algWithoutChunks.setPropertyValue(
        "OutputWorkspace", WS_NAME_NO_CHUNKS));
    TS_ASSERT_THROWS_NOTHING(algWithoutChunks.execute(););
    TS_ASSERT(algWithoutChunks.isExecuted());

    // Retrieve the workspace from data service
    EventWorkspace_sptr wsNoChunks;
    TS_ASSERT_THROWS_NOTHING(
        wsNoChunks = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            WS_NAME_NO_CHUNKS));
    TS_ASSERT(wsNoChunks);
    if (!wsNoChunks)
      return;
    TS_ASSERT_EQUALS(wsNoChunks->getEventType(), EventType::WEIGHTED_NOTIME);

    // run without chunks
    const std::string WS_NAME_CHUNKS("LoadEventAndCompress_chunks");
    LoadEventAndCompress algWithChunks;
    TS_ASSERT_THROWS_NOTHING(algWithChunks.initialize());
    TS_ASSERT(algWithChunks.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        algWithChunks.setPropertyValue("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(
        algWithChunks.setPropertyValue("OutputWorkspace", WS_NAME_CHUNKS));
    TS_ASSERT_THROWS_NOTHING(
        algWithChunks.setProperty("MaxChunkSize", .005)); // REALLY small file
    TS_ASSERT_THROWS_NOTHING(algWithChunks.execute(););
    TS_ASSERT(algWithChunks.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    EventWorkspace_sptr wsWithChunks;
    TS_ASSERT_THROWS_NOTHING(
        wsWithChunks =
            AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
                WS_NAME_CHUNKS));
    TS_ASSERT(wsWithChunks);
    if (!wsWithChunks)
      return;
    TS_ASSERT_EQUALS(wsWithChunks->getEventType(), EventType::WEIGHTED_NOTIME);

    // compare the two workspaces
    TS_ASSERT_EQUALS(wsWithChunks->getNumberEvents(),
                     wsNoChunks->getNumberEvents());
    auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
    checkAlg->setPropertyValue("Workspace1", WS_NAME_NO_CHUNKS);
    checkAlg->setPropertyValue("Workspace2", WS_NAME_CHUNKS);
    checkAlg->execute();
    TS_ASSERT(checkAlg->getProperty("Result"));

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WS_NAME_NO_CHUNKS);
    AnalysisDataService::Instance().remove(WS_NAME_CHUNKS);
  }
};

#endif /* MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESSTEST_H_ */
