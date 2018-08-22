#ifndef MANTID_ALGORITHMS_UNWRAPSNSTEST_H_
#define MANTID_ALGORITHMS_UNWRAPSNSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <string>

#include "MantidAlgorithms/UnwrapSNS.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class UnwrapSNSTest : public CxxTest::TestSuite {
private:
  double BIN_DELTA;
  int NUMPIXELS;
  int NUMBINS;

  void makeFakeEventWorkspace(std::string wsName) {
    // Make an event workspace with 2 events in each bin.
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::createEventWorkspace(
        NUMPIXELS, NUMBINS, NUMBINS, 0.0, BIN_DELTA, 2);
    // Fake a d-spacing unit in the data.
    test_in->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    test_in->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(NUMPIXELS /
                                                                 9));
    // Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnwrapSNSTest *createSuite() { return new UnwrapSNSTest(); }
  static void destroySuite(UnwrapSNSTest *suite) { delete suite; }

  UnwrapSNSTest() {
    BIN_DELTA = 2.;
    NUMPIXELS = 36;
    NUMBINS = 50;
  }

  void test_UnwrapSNSEventsInplace() {
    // setup
    std::string name("UnwrapSNS");
    this->makeFakeEventWorkspace(name);
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(name);
    size_t num_events = ws->getNumberEvents();
    double min_event0 = ws->getSpectrum(0).getTofMin();
    double max_event0 = ws->getSpectrum(0).getTofMax();
    double min_eventN = ws->getSpectrum(NUMPIXELS - 1).getTofMin();
    double max_eventN = ws->getSpectrum(NUMPIXELS - 1).getTofMax();

    // run the algorithm
    UnwrapSNS algo;
    if (!algo.isInitialized())
      algo.initialize();
    algo.setPropertyValue("InputWorkspace", name);
    algo.setPropertyValue("OutputWorkspace", name);
    algo.setProperty("LRef", 10.);
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(name);
    TS_ASSERT_EQUALS(NUMPIXELS,
                     ws->getNumberHistograms()); // shouldn't drop histograms
    TS_ASSERT_EQUALS(num_events,
                     ws->getNumberEvents()); // shouldn't drop events

    // pixel 0 shouldn't be adjusted
    TS_ASSERT_EQUALS(min_event0, ws->getSpectrum(0).getTofMin());
    TS_ASSERT_EQUALS(max_event0, ws->getSpectrum(0).getTofMax());

    // pixel NUMPIXELS - 1 should be moved
    TS_ASSERT(min_eventN < ws->getSpectrum(NUMPIXELS - 1).getTofMin());
    TS_ASSERT(max_eventN < ws->getSpectrum(NUMPIXELS - 1).getTofMax());

    TS_ASSERT_EQUALS(ws->getSpectrum(0).x()[0], 0.0);
    TS_ASSERT_EQUALS(ws->getSpectrum(0).x()[1], 2.0);
    TS_ASSERT_EQUALS(ws->getSpectrum(0).x()[2], 4.0);
  }
};

#endif /* MANTID_ALGORITHMS_UNWRAPSNSTEST_H_ */
