// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESSTEST_H_
#define MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidWorkflowAlgorithms/LoadEventAndCompress.h"

using Mantid::WorkflowAlgorithms::LoadEventAndCompress;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

namespace {
const std::string FILENAME{"ARCS_sim_event.nxs"};
const double CHUNKSIZE{.00001}; // REALLY small file
const size_t NUMEVENTS(117760);
} // anonymous namespace

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
    TS_ASSERT_EQUALS(wsNoChunks->getNEvents(), NUMEVENTS);

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
        algWithChunks.setProperty("MaxChunkSize", CHUNKSIZE));
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
    TS_ASSERT_EQUALS(wsWithChunks->getNEvents(), NUMEVENTS);

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

  void test_execNoFilter() {
    // run without chunks
    const std::string WS_NAME("LoadEventAndCompress_no_filter");
    LoadEventAndCompress alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterBadPulses", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxChunkSize", CHUNKSIZE));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", WS_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service
    EventWorkspace_sptr wksp;
    TS_ASSERT_THROWS_NOTHING(
        wksp = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            WS_NAME));
    TS_ASSERT(wksp);
    if (!wksp)
      return;
    TS_ASSERT_EQUALS(wksp->getEventType(), EventType::WEIGHTED_NOTIME);
    TS_ASSERT_EQUALS(wksp->getNEvents(), NUMEVENTS);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WS_NAME);
  }
};

#endif /* MANTID_WORKFLOWALGORITHMS_LOADEVENTANDCOMPRESSTEST_H_ */
