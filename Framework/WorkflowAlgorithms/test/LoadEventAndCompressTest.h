// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  static LoadEventAndCompressTest *createSuite() { return new LoadEventAndCompressTest(); }
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
    TS_ASSERT_THROWS_NOTHING(algWithoutChunks.setPropertyValue("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(algWithoutChunks.setPropertyValue("OutputWorkspace", WS_NAME_NO_CHUNKS));
    TS_ASSERT_THROWS_NOTHING(algWithoutChunks.execute(););
    TS_ASSERT(algWithoutChunks.isExecuted());

    // Retrieve the workspace from data service
    EventWorkspace_sptr wsNoChunks;
    TS_ASSERT_THROWS_NOTHING(wsNoChunks =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(WS_NAME_NO_CHUNKS));
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
    TS_ASSERT_THROWS_NOTHING(algWithChunks.setPropertyValue("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(algWithChunks.setPropertyValue("OutputWorkspace", WS_NAME_CHUNKS));
    TS_ASSERT_THROWS_NOTHING(algWithChunks.setProperty("MaxChunkSize", CHUNKSIZE));
    TS_ASSERT_THROWS_NOTHING(algWithChunks.execute(););
    TS_ASSERT(algWithChunks.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    EventWorkspace_sptr wsWithChunks;
    TS_ASSERT_THROWS_NOTHING(wsWithChunks = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(WS_NAME_CHUNKS));
    TS_ASSERT(wsWithChunks);
    if (!wsWithChunks)
      return;
    TS_ASSERT_EQUALS(wsWithChunks->getEventType(), EventType::WEIGHTED_NOTIME);
    TS_ASSERT_EQUALS(wsWithChunks->getNEvents(), NUMEVENTS);

    // compare the two workspaces
    TS_ASSERT_EQUALS(wsWithChunks->getNumberEvents(), wsNoChunks->getNumberEvents());
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
    TS_ASSERT_THROWS_NOTHING(wksp = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(WS_NAME));
    TS_ASSERT(wksp);
    if (!wksp)
      return;
    TS_ASSERT_EQUALS(wksp->getEventType(), EventType::WEIGHTED_NOTIME);
    TS_ASSERT_EQUALS(wksp->getNEvents(), NUMEVENTS);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WS_NAME);
  }

  void test_CNCS() {
    const std::string filename{"CNCS_7860_event.nxs"};

    // create expected output workspace by doing FilterBadPulses and CompressEvents after loading
    const std::string cncs_expected = "cncs_expected";
    auto ld = AlgorithmManager::Instance().create("LoadEventNexus", 1);
    ld->setPropertyValue("Filename", filename);
    ld->setPropertyValue("OutputWorkspace", cncs_expected);
    ld->setProperty("NumberOfBins", 1);
    ld->execute();
    TS_ASSERT(ld->isExecuted());

    auto filter_bad = AlgorithmManager::Instance().create("FilterBadPulses", 1);
    filter_bad->setPropertyValue("InputWorkspace", cncs_expected);
    filter_bad->setPropertyValue("OutputWorkspace", cncs_expected);
    filter_bad->execute();
    TS_ASSERT(filter_bad->isExecuted());

    auto compress = AlgorithmManager::Instance().create("CompressEvents", 1);
    compress->setPropertyValue("InputWorkspace", cncs_expected);
    compress->setPropertyValue("OutputWorkspace", cncs_expected);
    compress->setProperty("Tolerance", 0.01);
    compress->execute();
    TS_ASSERT(compress->isExecuted());

    // must sort events and rebin the same so it can be compared with CompareWorkspaces
    {
      auto sort = AlgorithmManager::Instance().create("SortEvents", 1);
      sort->setPropertyValue("InputWorkspace", cncs_expected);
      sort->execute();
      TS_ASSERT(sort->isExecuted());

      auto rebin = AlgorithmManager::Instance().create("Rebin", 1);
      rebin->setPropertyValue("InputWorkspace", cncs_expected);
      rebin->setPropertyValue("OutputWorkspace", cncs_expected);
      rebin->setPropertyValue("Params", "50000,1000,54000");
      rebin->execute();
      TS_ASSERT(rebin->isExecuted());
    }

    // run LoadEventAndCompress with chunking
    const std::string cncs_result = "cncs_LoadEventAndCompress";
    LoadEventAndCompress alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxChunkSize", 0.001)); // results in loading by 6 chunks
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", cncs_result));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CompressTOFTolerance", -0.01));        // log but should be overruled by
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CompressBinningMode", "Linear")); // the binning mode
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // must rebin the same so it can be compared with CompareWorkspaces
    {
      auto sort = AlgorithmManager::Instance().create("SortEvents", 1);
      sort->setPropertyValue("InputWorkspace", cncs_result);
      sort->execute();
      TS_ASSERT(sort->isExecuted());

      auto rebin = AlgorithmManager::Instance().create("Rebin", 1);
      rebin->setPropertyValue("InputWorkspace", cncs_result);
      rebin->setPropertyValue("OutputWorkspace", cncs_result);
      rebin->setPropertyValue("Params", "50000,1000,54000");
      rebin->execute();
      TS_ASSERT(rebin->isExecuted());
    }

    // validate the resulting workspace is the same for post filtered/compressed LoadEventAndCompress with chunking
    auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
    checkAlg->setProperty("Workspace1", cncs_expected);
    checkAlg->setProperty("Workspace2", cncs_result);
    checkAlg->execute();
    TS_ASSERT(checkAlg->getProperty("Result"));
  }
};
