#ifndef MANTID_ALGORITHMS_REMOVELOWRESTOFTEST_H_
#define MANTID_ALGORITHMS_REMOVELOWRESTOFTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <set>
#include <string>

#include "MantidAlgorithms/RemoveLowResTOF.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class RemoveLowResTOFTest : public CxxTest::TestSuite {
private:
  double BIN_DELTA;
  int NUMPIXELS;
  int NUMBINS;

  void makeFakeEventWorkspace(std::string wsName) {
    // Make an event workspace with 2 events in each bin.
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::createEventWorkspace(
        NUMPIXELS, NUMBINS, NUMBINS, 0.0, BIN_DELTA, 2);
    // Fake a TOF unit in the data.
    test_in->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    test_in->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(NUMPIXELS /
                                                                 9));
    // Make sure the detector IDs are ok
    for (int i = 0; i < NUMPIXELS; i++)
      test_in->getSpectrum(i).setDetectorID(i + 1);

    // Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveLowResTOFTest *createSuite() {
    return new RemoveLowResTOFTest();
  }
  static void destroySuite(RemoveLowResTOFTest *suite) { delete suite; }

  RemoveLowResTOFTest() {
    BIN_DELTA = 2.;
    NUMPIXELS = 36;
    NUMBINS = 50;
  }

  void Ptest_RemoveLowResEventsInplace() {
    // setup
    std::string name("RemoveLowResTOF");
    this->makeFakeEventWorkspace(name);
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(name);
    size_t num_events = ws->getNumberEvents();
    double min_event0 = ws->getSpectrum(0).getTofMin();
    double max_event0 = ws->getSpectrum(0).getTofMax();
    double min_eventN = ws->getSpectrum(NUMPIXELS - 1).getTofMin();
    double max_eventN = ws->getSpectrum(NUMPIXELS - 1).getTofMax();

    // run the algorithm
    RemoveLowResTOF algo;
    if (!algo.isInitialized())
      algo.initialize();
    algo.setPropertyValue("InputWorkspace", name);
    algo.setPropertyValue("OutputWorkspace", name);
    algo.setProperty("ReferenceDIFC", 5.);
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(name);
    TS_ASSERT_EQUALS(NUMPIXELS,
                     ws->getNumberHistograms());   // shouldn't drop histograms
    TS_ASSERT(num_events > ws->getNumberEvents()); // should drop events

    // pixel 0 shouldn't be adjusted
    TS_ASSERT_EQUALS(min_event0, ws->getSpectrum(0).getTofMin());
    TS_ASSERT_EQUALS(max_event0, ws->getSpectrum(0).getTofMax());

    // pixel NUMPIXELS - 1 should be moved
    TS_ASSERT(min_eventN < ws->getSpectrum(NUMPIXELS - 1).getTofMin());
    TS_ASSERT_EQUALS(max_eventN, ws->getSpectrum(NUMPIXELS - 1).getTofMax());
  }

  /** Test the functionality to output the removed low resolution TOF events to
   * additional
    * workspace.
    */
  void test_OutputRemovedLowRefTOF() {
    // setup
    std::string name("RemoveLowResTOF");
    this->makeFakeEventWorkspace(name);
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(name);
    size_t num_events = ws->getNumberEvents();
    double min_event0 = ws->getSpectrum(0).getTofMin();
    double max_event0 = ws->getSpectrum(0).getTofMax();
    double min_eventN = ws->getSpectrum(NUMPIXELS - 1).getTofMin();
    double max_eventN = ws->getSpectrum(NUMPIXELS - 1).getTofMax();

    // run the algorithm
    RemoveLowResTOF algo;
    if (!algo.isInitialized())
      algo.initialize();

    std::string lowreswsname("LowResolutionTOF");

    algo.setPropertyValue("InputWorkspace", name);
    algo.setPropertyValue("OutputWorkspace", name);
    algo.setPropertyValue("LowResTOFWorkspace", lowreswsname);
    algo.setProperty("ReferenceDIFC", 5.);
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspaces
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(name);
    TS_ASSERT(ws);
    if (!ws)
      return;

    EventWorkspace_sptr lowresws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            lowreswsname);
    TS_ASSERT(lowresws);
    if (!lowresws)
      return;

    // shouldn't drop histograms
    TS_ASSERT_EQUALS(NUMPIXELS, ws->getNumberHistograms());
    TS_ASSERT_EQUALS(NUMPIXELS, lowresws->getNumberHistograms());

    // should drop events, but summed should be same as original
    std::cout << "Events (Input) = " << num_events
              << "; Result = " << ws->getNumberEvents()
              << ", Low Res = " << lowresws->getNumberEvents() << ".\n";
    TS_ASSERT(num_events > ws->getNumberEvents());
    TS_ASSERT(num_events > lowresws->getNumberEvents());
    // There are 400 events in 4 spectra that are cleared
    TS_ASSERT_EQUALS(ws->getNumberEvents() + lowresws->getNumberEvents() + 400,
                     num_events);

    // pixel 0 shouldn't be adjusted
    TS_ASSERT_EQUALS(min_event0, ws->getSpectrum(0).getTofMin());
    TS_ASSERT_EQUALS(max_event0, ws->getSpectrum(0).getTofMax());
    TS_ASSERT_EQUALS(lowresws->getSpectrum(0).getNumberEvents(), 0);

    // pixel NUMPIXELS - 1 should be moved
    TS_ASSERT(min_eventN < ws->getSpectrum(NUMPIXELS - 1).getTofMin());
    TS_ASSERT_EQUALS(max_eventN, ws->getSpectrum(NUMPIXELS - 1).getTofMax());
    TS_ASSERT_EQUALS(min_eventN,
                     lowresws->getSpectrum(NUMPIXELS - 1).getTofMin());
    TS_ASSERT(max_eventN > lowresws->getSpectrum(NUMPIXELS - 1).getTofMax());
  }
};

#endif /* MANTID_ALGORITHMS_REMOVELOWRESTOFTEST_H_ */
