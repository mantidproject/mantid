// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/FilterBadPulses.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class FilterBadPulsesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterBadPulsesTest *createSuite() { return new FilterBadPulsesTest(); }
  static void destroySuite(FilterBadPulsesTest *suite) { delete suite; }

  FilterBadPulsesTest() : inputWS("testInput"), outputWS("testOutput") {}

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void setUp_Event() {
    auto loader = AlgorithmManager::Instance().create("LoadEventNexus");
    loader->initialize();
    loader->setPropertyValue("Filename", "CNCS_7860_event.nxs");
    loader->setPropertyValue("OutputWorkspace", inputWS);
    loader->execute();
    TS_ASSERT(loader->isExecuted());
  }

  void testExec() {
    // Retrieve Workspace
    this->setUp_Event();
    WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWS);
    TS_ASSERT(WS); // workspace is loaded

    // DEBUG: remove these lines
    auto l0 = WS->run().getProperty("proton_charge");
    std::string s0 = l0->units();
    auto tempWS = Mantid::DataObjects::create<EventWorkspace>(*WS);
    auto l1 = tempWS->run().getProperty("proton_charge");
    std::string s1 = l1->units();
    auto l2 = tempWS->mutableRun().getProperty("proton_charge");
    std::string s2 = l2->units();

    size_t start_num_events = WS->getNumberEvents();
    double start_proton_charge = WS->run().getProtonCharge();
    size_t num_sample_logs = WS->run().getProperties().size();
    TS_ASSERT_EQUALS(start_num_events, 112266);
    TS_ASSERT_DELTA(start_proton_charge, 26.4589, 0.0001);
    alg.setPropertyValue("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", outputWS);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputWS);
    TS_ASSERT(outWS); // workspace is loaded

    // Things that haven't changed
    TS_ASSERT_EQUALS(outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), WS->getNumberHistograms());

    // There should be some events
    TS_ASSERT_LESS_THAN(0, outWS->getNumberEvents());

    TS_ASSERT_LESS_THAN(outWS->getNumberEvents(), start_num_events);
    TS_ASSERT_DELTA(outWS->getNumberEvents(), 83434, 100);

    // Proton charge is lower
    TS_ASSERT_EQUALS(outWS->run().getProperties().size(), num_sample_logs);
    TS_ASSERT_DELTA(outWS->run().getProtonCharge(), 20.576, 0.001);

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
  }

private:
  std::string inputWS;
  std::string outputWS;
  FilterBadPulses alg;
  EventWorkspace_sptr WS;
};
