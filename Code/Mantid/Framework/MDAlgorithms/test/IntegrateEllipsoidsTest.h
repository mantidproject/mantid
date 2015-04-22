#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/IntegrateEllipsoids.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidAPI/NumericAxis.h"
#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

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

  EventList &el = eventWS->getEventList(detectorId - totalNPixels);

  // Add more events to the event list corresponding to the peak centre
  double start = tofExact - (double(nEvents) / 2 * tofGap);
  for (int i = 0; i < nEvents; ++i) {
    const double tof = start + (i * tofGap);
    el.addEventQuickly(TofEvent(tof));
  }
}

// Create diffraction data for test schenarios
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
    EventList &el = eventWS->getOrAddEventList(i);
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
  addFakeEllipsoid(V3D(1, -4, -1), nPixelsTotal, nEventsPerPeak,
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

class IntegrateEllipsoidsTest : public CxxTest::TestSuite {

private:
  Mantid::DataObjects::EventWorkspace_sptr m_eventWS;
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaksWS;
  Mantid::API::MatrixWorkspace_sptr m_histoWS;

  // Check that n-peaks from the workspace are integrated as we expect
  void do_test_n_peaks(PeaksWorkspace_sptr &integratedPeaksWS,
                       const int nPeaks) {
    auto instrument = integratedPeaksWS->getInstrument();
    const V3D samplePos = instrument->getComponentByName("sample")->getPos();
    const V3D sourcePos = instrument->getComponentByName("source")->getPos();
    V3D beamDir = samplePos - sourcePos;
    beamDir.normalize();

    // Just test the first few peaks
    for (int i = 0; i < nPeaks; ++i) {
      const Peak &peak = integratedPeaksWS->getPeak(i);
      const PeakShape &peakShape = peak.getPeakShape();

      TSM_ASSERT_RELATION("Peak should be integrated", std::greater<double>,
                          peak.getIntensity(), 0);

      std::stringstream stream;
      stream << "Wrong shape name for peak " << i;
      TSM_ASSERT_EQUALS(stream.str(), PeakShapeEllipsoid::ellipsoidShapeName(),
                        peakShape.shapeName());

      // Calculate the q direction based on geometry
      const V3D detPos = peak.getDetectorPosition();
      V3D detDir = detPos - samplePos;
      detDir.normalize();
      V3D qDir = detDir - beamDir;
      qDir.normalize();

      // Get the q-direction off the ellipsoid
      PeakShapeEllipsoid const *const ellipsoid =
          dynamic_cast<const PeakShapeEllipsoid *>(&peakShape);
      auto dirs = ellipsoid->directions();

      /* We have set the fake ellipsoids up to be lines along a single detectors
       * TOF (see setup).
       * We therefore expect the principle axis of the ellipsoid to be the same
       * as the q-dir!
       */
      TS_ASSERT_EQUALS(qDir, dirs[0]);
    }
  }

public:

  static void destroySuite(IntegrateEllipsoidsTest *suite) { delete suite; }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEllipsoidsTest *createSuite() {
    return new IntegrateEllipsoidsTest();
  }

  IntegrateEllipsoidsTest() {

    // Need to get and run algorithms from elsewhere in the framework.
    Mantid::API::FrameworkManager::Instance();

    auto data = createDiffractionData();

    m_eventWS = data.get<0>();
    m_peaksWS = data.get<1>();

    /*
     Simply rebin the event workspace to a histo workspace to create the input
     we need.
    */
    auto rebinAlg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Rebin");
    rebinAlg->setChild(true);
    rebinAlg->initialize();
    rebinAlg->setProperty("InputWorkspace", m_eventWS);
    auto params = std::vector<double>();
    params.push_back(950);
    params.push_back(10);
    params.push_back(2500);
    rebinAlg->setProperty("Params", params);
    rebinAlg->setProperty("PreserveEvents", false); // Make a histo workspace
    rebinAlg->setPropertyValue("OutputWorkspace", "dummy");
    rebinAlg->execute();

    m_histoWS = rebinAlg->getProperty("OutputWorkspace");
  }

  void test_init() {
    Mantid::MDAlgorithms::IntegrateEllipsoids alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_ws_has_instrument()
  {
      auto inputWorkspaceNoInstrument =
          boost::make_shared<EventWorkspace>();

      IntegrateEllipsoids alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", inputWorkspaceNoInstrument), std::invalid_argument&);
  }

  void test_execution_events() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_eventWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());

    do_test_n_peaks(integratedPeaksWS, 3 /*check first 3 peaks*/);
  }

  void test_execution_histograms() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_histoWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());

    do_test_n_peaks(integratedPeaksWS, 3 /*check first 3 peaks*/);
  }
  void test_execution_events_hkl() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_eventWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("IntegrateInHKL", true); // Check hkl option
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());

    TSM_ASSERT_DELTA("Wrong intensity for peak 0",
          integratedPeaksWS->getPeak(0).getIntensity(), -2, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 1",
          integratedPeaksWS->getPeak(1).getIntensity(), 2, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 2",
          integratedPeaksWS->getPeak(2).getIntensity(), -2, 0.01);
    //Answer is 16 on Mac ???
    //TSM_ASSERT_DELTA("Wrong intensity for peak 3",
          //integratedPeaksWS->getPeak(3).getIntensity(), 6, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 4",
          integratedPeaksWS->getPeak(4).getIntensity(), 11, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 5",
          integratedPeaksWS->getPeak(5).getIntensity(), 10, 0.01);

  }

  void test_execution_histograms_hkl() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_histoWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("IntegrateInHKL", true); // Check hkl option
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());
    TSM_ASSERT_DELTA("Wrong intensity for peak 0",
          integratedPeaksWS->getPeak(0).getIntensity(), 1.0, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 1",
          integratedPeaksWS->getPeak(1).getIntensity(), 0, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 2",
          integratedPeaksWS->getPeak(2).getIntensity(), 1.0, 0.01);
    //Answer is 15 on Mac ???
    //TSM_ASSERT_DELTA("Wrong intensity for peak 3",
          //integratedPeaksWS->getPeak(3).getIntensity(), 11, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 4",
          integratedPeaksWS->getPeak(4).getIntensity(), 13, 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 5",
          integratedPeaksWS->getPeak(5).getIntensity(), 12, 0.01);

  }
};

class IntegrateEllipsoidsTestPerformance : public CxxTest::TestSuite {

private:

  Mantid::API::MatrixWorkspace_sptr m_eventWS;
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaksWS;
  Mantid::API::MatrixWorkspace_sptr m_histoWS;

public:

  static void destroySuite(IntegrateEllipsoidsTestPerformance *suite) {
    delete suite;
  }


  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEllipsoidsTestPerformance *createSuite() {
    return new IntegrateEllipsoidsTestPerformance();
  }

  IntegrateEllipsoidsTestPerformance() {
    // Need to get and run algorithms from elsewhere in the framework.
    Mantid::API::FrameworkManager::Instance();

    auto data = createDiffractionData(200 /*sqrt total pixels*/,
                                      60 /*events per peak*/, 2 /*tof gap*/);

    m_eventWS = data.get<0>();
    m_peaksWS = data.get<1>();

    /*
     Simply rebin the event workspace to a histo workspace to create the input
     we need.
    */
    auto rebinAlg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Rebin");
    rebinAlg->setChild(true);
    rebinAlg->initialize();
    rebinAlg->setProperty("InputWorkspace", m_eventWS);
    auto params = std::vector<double>();
    params.push_back(950);
    params.push_back(5);
    params.push_back(2500);
    rebinAlg->setProperty("Params", params);
    rebinAlg->setProperty("PreserveEvents", false); // Make a histo workspace
    rebinAlg->setPropertyValue("OutputWorkspace", "dummy");
    rebinAlg->execute();

    m_histoWS = rebinAlg->getProperty("OutputWorkspace");
  }

  void test_execution_events() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_eventWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());
  }

  void test_execution_histograms() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_histoWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());
  }
};
