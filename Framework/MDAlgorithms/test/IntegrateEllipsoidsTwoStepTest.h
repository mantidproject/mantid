#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <tuple>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using Mantid::Kernel::V3D;
using Mantid::Geometry::OrientedLattice;

namespace {
// Add A Fake 'Peak' to both the event data and to the peaks workspace
void addFakeEllipsoid(const V3D &peakHKL, const int &totalNPixels,
                      const int &nEvents, const double tofGap,
                      EventWorkspace_sptr &eventWS,
                      PeaksWorkspace_sptr &peaksWS) {
  // Create the peak and add it to the peaks ws
  Peak *peak = peaksWS->createPeakHKL(peakHKL);

  peaksWS->addPeak(*peak);
  const int detectorId = peak->getDetectorID();
  const double tofExact = peak->getTOF();
  delete peak;

  EventList &el = eventWS->getSpectrum(detectorId - totalNPixels);

  // Add more events to the event list corresponding to the peak centre
  double start = tofExact - (double(nEvents) / 2 * tofGap);
  for (int i = 0; i < nEvents; ++i) {
    const double tof = start + (i * tofGap);
    el.addEventQuickly(TofEvent(tof));
  }
}

// Create diffraction data for test scenarios
boost::tuple<EventWorkspace_sptr, PeaksWorkspace_sptr>
createDiffractionData(const int nPixels = 100, const int nEventsPerPeak = 20,
                      const double tofGapBetweenEvents = 10) {
  Mantid::Geometry::Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular(
          1 /*num_banks*/, nPixels /*pixels in each direction yields n by n*/,
          0.01, 1.0);

  // Create a peaks workspace
  auto peaksWS = boost::make_shared<PeaksWorkspace>();
  // Set the instrument to be the fake rectangular bank above.
  peaksWS->setInstrument(inst);
  // Set the oriented lattice for a cubic crystal
  OrientedLattice ol(6, 6, 6, 90, 90, 90);
  ol.setUFromVectors(V3D(6, 0, 0), V3D(0, 6, 0));
  peaksWS->mutableSample().setOrientedLattice(&ol);

  // Make an event workspace and add fake peak data
  auto eventWS = boost::make_shared<EventWorkspace>();
  eventWS->setInstrument(inst);
  eventWS->initialize(nPixels * nPixels /*n spectra*/, 3 /* x-size */,
                      3 /* y-size */);
  eventWS->getAxis(0)->setUnit("TOF");
  // Give the spectra-detector mapping for all event lists
  const int nPixelsTotal = nPixels * nPixels;
  for (int i = 0; i < nPixelsTotal; ++i) {
    EventList &el = eventWS->getSpectrum(i);
    el.setDetectorID(i + nPixelsTotal);
  }

  // Add some peaks which should correspond to real reflections (could
  // calculate these). Same function also adds a fake ellipsoid
  addFakeEllipsoid(V3D(1, -5, -3), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -4, -4), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -3, -5), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -4, -2), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -4, 0), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(2, -3, -4), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);

  // Return test data.
  return boost::tuple<EventWorkspace_sptr, PeaksWorkspace_sptr>(eventWS,
                                                                peaksWS);
}
}

class IntegrateEllipsoidsTwoStepTest : public CxxTest::TestSuite {

public:
  void test_init() {
    Mantid::MDAlgorithms::IntegrateEllipsoidsTwoStep alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_exec() {

    auto data = createDiffractionData();
    auto eventWS = data.get<0>();
    auto peaksWS = data.get<1>();

    // Run algorithm
    Mantid::MDAlgorithms::IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.20));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.23));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.26));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WeakPeakThreshold", 1.0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "dummy"));
    TS_ASSERT_THROWS_NOTHING(alg.execute() );

    // Check output
    TS_ASSERT( alg.isExecuted() );
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT( integratedPeaksWS );

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      peaksWS->getNumberPeaks());
    const auto& run = integratedPeaksWS->mutableRun();
    TSM_ASSERT("Output workspace must be integrated",
                run.hasProperty("PeaksIntegrated"));
    TSM_ASSERT_EQUALS("Output workspace must be integrated",
                run.getProperty("PeaksIntegrated")->value(), "1");

    TSM_ASSERT_DELTA("Wrong intensity for peak 0",
                     integratedPeaksWS->getPeak(0).getIntensity(), 2, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 1",
                     integratedPeaksWS->getPeak(1).getIntensity(), 0.8, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 2",
                     integratedPeaksWS->getPeak(2).getIntensity(), 2, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 3",
                     integratedPeaksWS->getPeak(3).getIntensity(), 7, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 4",
                     integratedPeaksWS->getPeak(4).getIntensity(), 0, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 5",
                     integratedPeaksWS->getPeak(5).getIntensity(), 5.83, 0.01);
  }
};

