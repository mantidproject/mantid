// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/RemovePromptPulse.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class RemovePromptPulseTest : public CxxTest::TestSuite {
private:
  double BIN_DELTA;
  int NUMPIXELS;
  int NUMBINS;
  int NUMEVENTS;
  std::string inWSName;
  std::string outWSName;

  void makeFakeEventWorkspace(const std::string &wsName, int shift = 0) {
    // Make an event workspace with 2 events in each bin.
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createEventWorkspace(NUMPIXELS, NUMBINS, NUMEVENTS, 1000., BIN_DELTA, 2);
    // Fake a TOF unit in the data.
    test_in->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    for (size_t i = 0; i < test_in->dataX(0).size(); ++i) {
      test_in->dataX(0)[i] += shift;
    }
    test_in->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(NUMPIXELS / 9));
    // Make sure the detector IDs are ok
    for (int i = 0; i < NUMPIXELS; i++)
      test_in->getSpectrum(i).setDetectorID(i + 1);

    // Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemovePromptPulseTest *createSuite() { return new RemovePromptPulseTest(); }
  static void destroySuite(RemovePromptPulseTest *suite) { delete suite; }

  RemovePromptPulseTest() {
    BIN_DELTA = 100.;
    NUMPIXELS = 36;
    NUMBINS = 50;
    NUMEVENTS = 1000;
  }

  void test_Init() {
    RemovePromptPulse alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_miss() {
    inWSName = "RemovePromptPulseTest_InputWS_miss";
    outWSName = "RemovePromptPulseTest_OutputWS_miss";
    this->makeFakeEventWorkspace(inWSName);

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWSName));
    std::size_t num_events = ws->getNumberEvents();
    RemovePromptPulse alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Width", "1000."));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Frequency", "100"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Verify the results
    // drop events 36 spectra, 2 events/bin, 10 bins/pulse, 10 pulses
    TS_ASSERT_EQUALS(num_events - 36 * 2 * 10 * 10, ws->getNumberEvents());

    std::cout << "num_events = " << ws->getNumberEvents() << std::endl;

    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_hit_tmin() {
    inWSName = "RemovePromptPulseTest_InputWS_hit";
    outWSName = inWSName; //"RemovePromptPulseTest_OutputWS_hit";
    this->makeFakeEventWorkspace(inWSName);

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWSName));
    std::size_t num_events = ws->getNumberEvents();

    RemovePromptPulse alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Width", "1000."));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Frequency", "200"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Verify the results
    TS_ASSERT(num_events > ws->getNumberEvents()); // should drop events
    // Clean up
    AnalysisDataService::Instance().remove(inWSName);
  }

  void test_exec_miss_tmin() {
    inWSName = "RemovePromptPulseTest_InputWS_miss_tmin";
    outWSName = "RemovePromptPulseTest_OutputWS_miss_tmin";
    this->makeFakeEventWorkspace(inWSName, -1100);

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWSName));
    std::size_t num_events = ws->getNumberEvents();
    RemovePromptPulse alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Width", "1000."));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Frequency", "100"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("TMin", "-100"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Verify the results
    // drop events 36 spectra, 2 events/bin, 10 bins/pulse, 10 pulses
    TS_ASSERT_EQUALS(num_events - 36 * 2 * 10 * 10, ws->getNumberEvents());

    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_hit_negative_tmin() {
    inWSName = "RemovePromptPulseTest_InputWS_tmin";
    outWSName = inWSName;
    this->makeFakeEventWorkspace(inWSName, -1100);

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWSName));
    std::size_t num_events = ws->getNumberEvents();

    RemovePromptPulse alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Width", "1000."));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Frequency", "200"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("TMin", "-100"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Verify the results
    TS_ASSERT(num_events > ws->getNumberEvents()); // should drop events
    // Clean up
    AnalysisDataService::Instance().remove(inWSName);
  }
};
