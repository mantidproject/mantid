// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCrystal/MaskPeaksWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class MaskPeaksWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    MaskPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test_MINITOPAZ(EventType type) {

    int numEventsPer = 100;
    EventWorkspace_sptr inputW =
        Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer, 10000, 1600);
    AnalysisDataService::Instance().addOrReplace("testInEW", inputW);
    if (type == WEIGHTED)
      inputW *= 2.0;
    if (type == WEIGHTED_NOTIME) {
      for (size_t i = 0; i < inputW->getNumberHistograms(); i++) {
        EventList &el = inputW->getSpectrum(i);
        el.compressEvents(0.0, &el);
      }
    }
    size_t nevents0 = inputW->getNumberEvents();
    // Register the workspace in the data service

    // Create the peaks workspace
    PeaksWorkspace_sptr pkws(new PeaksWorkspace());
    // pkws->setName("TOPAZ");

    // This loads (appends) the peaks
    Mantid::DataObjects::Peak PeakObj(inputW->getInstrument(), 1000, 100.);
    pkws->addPeak(PeakObj);
    AnalysisDataService::Instance().add("TOPAZ", pkws);

    MaskPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("InPeaksWorkspace", "TOPAZ");
    alg.setProperty("XMin", -2);
    alg.setProperty("XMax", 2);
    alg.setProperty("YMin", -2);
    alg.setProperty("YMax", 2);
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted())

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("testInEW"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    size_t nevents = ws->getNumberEvents();
    TS_ASSERT_LESS_THAN(nevents, nevents0);

    AnalysisDataService::Instance().remove("testInEW");
    AnalysisDataService::Instance().remove("TOPAZ");
  }

  void test_MINITOPAZ() { do_test_MINITOPAZ(TOF); }

  /*
   * Test make peaks with ralative TOF range
   */
  void do_test_TOFRange(EventType type) {

    int numEventsPer = 100;
    EventWorkspace_sptr inputW =
        Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer, 10000, 1600);
    AnalysisDataService::Instance().addOrReplace("testInEW", inputW);
    if (type == WEIGHTED) {
      inputW *= 2.0;
    }
    if (type == WEIGHTED_NOTIME) {
      for (size_t i = 0; i < inputW->getNumberHistograms(); i++) {
        EventList &el = inputW->getSpectrum(i);
        el.compressEvents(0.0, &el);
      }
    }
    size_t nevents0 = inputW->getNumberEvents();
    // Register the workspace in the data service

    // Create the peaks workspace
    PeaksWorkspace_sptr pkws(new PeaksWorkspace());
    // pkws->setName("TOPAZ");

    // This loads (appends) the peaks
    Mantid::DataObjects::Peak PeakObj(inputW->getInstrument(), 1000, 1.);
    pkws->addPeak(PeakObj);
    AnalysisDataService::Instance().add("TOPAZ2", pkws);

    MaskPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("InPeaksWorkspace", "TOPAZ2");
    alg.setProperty("XMin", -2);
    alg.setProperty("XMax", 2);
    alg.setProperty("YMin", -2);
    alg.setProperty("YMax", 2);
    alg.setProperty("TOFMin", -2500.0);
    alg.setProperty("TOFMax", 5000.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted())

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("testInEW"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    size_t nevents = ws->getNumberEvents();
    TS_ASSERT_LESS_THAN(nevents, nevents0);
    TS_ASSERT_LESS_THAN(999400, nevents);

    AnalysisDataService::Instance().remove("testInEW");
    AnalysisDataService::Instance().remove("TOPAZ2");

    return;
  }

  void test_TOFRange() { do_test_TOFRange(TOF); }
};

class MaskPeaksWorkspaceTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    EventWorkspace_sptr inputWS =
        Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(1000, 10000, 16000);
    AnalysisDataService::Instance().addOrReplace(inputWSName, inputWS);

    PeaksWorkspace_sptr peaksWS(new PeaksWorkspace());
    Mantid::DataObjects::Peak PeakObj(inputWS->getInstrument(), 1000, 100.);
    peaksWS->addPeak(PeakObj);

    mpwAlg.initialize();
    mpwAlg.setProperty("InputWorkspace", inputWS);
    mpwAlg.setProperty("InPeaksWorkspace", peaksWS);
  }

  void testPerformance() { mpwAlg.execute(); }

  void tearDown() override { AnalysisDataService::Instance().remove(inputWSName); }

private:
  MaskPeaksWorkspace mpwAlg;
  std::string inputWSName = "inputWS";
};
