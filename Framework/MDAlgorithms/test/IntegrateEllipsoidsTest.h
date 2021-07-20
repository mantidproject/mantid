// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/FindPeaksMD.h"
#include "MantidMDAlgorithms/IntegrateEllipsoids.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/tuple/tuple.hpp>
#include <cxxtest/TestSuite.h>
#include <memory>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::Geometry::IPeak_uptr;
using namespace Mantid::DataObjects;
using Mantid::DataObjects::Peak;
using Mantid::DataObjects::Peak_uptr;
using Mantid::Types::Event::TofEvent;

namespace {
// Add A Fake 'Peak' to both the event data and to the peaks workspace
void addFakeEllipsoid(const V3D &peakHKL, const int &totalNPixels, const int &nEvents, const double tofGap,
                      EventWorkspace_sptr &eventWS, PeaksWorkspace_sptr &peaksWS) {
  // Create the peak and add it to the peaks ws
  IPeak_uptr ipeak = peaksWS->createPeakHKL(peakHKL);
  Peak_uptr peak(dynamic_cast<Peak *>(ipeak.release()));
  peaksWS->addPeak(*peak);
  const auto detectorId = peak->getDetectorID();
  const auto tofExact = peak->getTOF();
  EventList &el = eventWS->getSpectrum(detectorId - totalNPixels);
  // Add more events to the event list corresponding to the peak centre
  auto nPkEvents = nEvents - 4;
  double start = tofExact - (double(nPkEvents) / 2 * tofGap);
  for (int i = 0; i < nPkEvents; ++i) {
    const double tof = start + (i * tofGap);
    el.addEventQuickly(TofEvent(tof));
  }

  /* Add single events at  +/- step in each direction perp QHKL
  to ensure covariance matrix is not singular */

  // vector to hold axes unit vectors perp Q
  std::vector<V3D> eigvects;
  auto const Q = peak->getQLabFrame();
  auto const Qhat = Q / Q.norm(); // unit-vector (principal axis of ellipse)
  V3D u;
  double dotprod = 1;
  // loop over 3 mutually-orthogonal vectors (100, 010, 001)
  // until get one with component perp to Q (within tolerance)
  size_t ii = 0;
  do {
    u = V3D(0, 0, 0); // reset u
    u[ii] = 1;
    dotprod = Qhat.scalar_prod(u);
    ii++;
  } while (abs(dotprod) > 1.0 - 1e-6);
  auto v = Qhat.cross_prod(u);
  eigvects.push_back(v / v.norm());
  v = Qhat.cross_prod(v);
  eigvects.push_back(v / v.norm());

  // get appropriate step in directions perp Qhkl
  // so events are in different detector ID
  std::vector<double> step_perp{0.0, 0.0};
  for (size_t ivect = 0; ivect < step_perp.size(); ivect++) {
    auto detId = detectorId;
    do {
      step_perp[ivect] += 0.02;
      auto q = Q + eigvects[ivect] * step_perp[ivect];
      IPeak_uptr ipk = peaksWS->createPeak(q);
      Peak_uptr pk(dynamic_cast<Peak *>(ipk.release()));
      detId = pk->getDetectorID();
    } while (detId == detectorId);
  }

  // and single events
  for (int istep = -1; istep < 2; istep += 2) {
    for (size_t ivect = 0; ivect < step_perp.size(); ivect++) {
      auto q = Q + eigvects[ivect] * step_perp[ivect] * istep;
      IPeak_uptr ipk = peaksWS->createPeak(q);
      Peak_uptr pk(dynamic_cast<Peak *>(ipk.release()));
      // add event
      auto detId = pk->getDetectorID();
      EventList &el = eventWS->getSpectrum(detId - totalNPixels);
      el.addEventQuickly(TofEvent(pk->getTOF()));
    }
  }
}

// Create diffraction data for test scenarios
boost::tuple<EventWorkspace_sptr, PeaksWorkspace_sptr>
createDiffractionData(const int nPixels = 200, const int nEventsPerPeak = 24, const double tofGapBetweenEvents = 10) {
  Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(
      1 /*num_banks*/, nPixels /*pixels in each direction yields n by n*/, 0.01, 1.0);

  // Create a peaks workspace
  auto peaksWS = std::make_shared<PeaksWorkspace>();
  // Set the instrument to be the fake rectangular bank above.
  peaksWS->setInstrument(inst);
  // Set the oriented lattice for a cubic crystal
  auto lattice = std::make_unique<OrientedLattice>(6, 6, 6, 90, 90, 90);
  lattice->setUFromVectors(V3D(6, 0, 0), V3D(0, 6, 0));
  peaksWS->mutableSample().setOrientedLattice(std::move(lattice));

  // Make an event workspace and add fake peak data
  auto eventWS = std::make_shared<EventWorkspace>();
  eventWS->setInstrument(inst);
  eventWS->initialize(nPixels * nPixels /*n spectra*/, 3 /* x-size */, 3 /* y-size */);
  eventWS->getAxis(0)->setUnit("TOF");
  // Give the spectra-detector mapping for all event lists
  const int nPixelsTotal = nPixels * nPixels;
  for (int i = 0; i < nPixelsTotal; ++i) {
    EventList &el = eventWS->getSpectrum(i);
    el.setDetectorID(i + nPixelsTotal);
  }

  // Add some peaks which should correspond to real reflections (could
  // calculate these). Same function also adds a fake ellipsoid
  addFakeEllipsoid(V3D(1, -5, -3), nPixelsTotal, nEventsPerPeak, tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -4, -4), nPixelsTotal, nEventsPerPeak, tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -3, -5), nPixelsTotal, nEventsPerPeak, tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -4, -2), nPixelsTotal, nEventsPerPeak, tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -5, -1), nPixelsTotal, nEventsPerPeak, tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(V3D(1, -3, -4), nPixelsTotal, nEventsPerPeak, tofGapBetweenEvents, eventWS, peaksWS);

  // Return test data.
  return boost::tuple<EventWorkspace_sptr, PeaksWorkspace_sptr>(eventWS, peaksWS);
}
} // namespace

class IntegrateEllipsoidsTest : public CxxTest::TestSuite {

private:
  Mantid::DataObjects::EventWorkspace_sptr m_eventWS;
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaksWS;
  Mantid::API::MatrixWorkspace_sptr m_histoWS;

  // Check that n-peaks from the workspace are integrated as we expect
  void do_test_n_peaks(PeaksWorkspace_sptr &integratedPeaksWS, const int nPeaks) {
    auto instrument = integratedPeaksWS->getInstrument();
    const V3D samplePos = instrument->getComponentByName("sample")->getPos();
    const V3D sourcePos = instrument->getComponentByName("source")->getPos();
    const V3D beamDir = normalize(samplePos - sourcePos);

    // Just test the first few peaks
    for (int i = 0; i < nPeaks; ++i) {
      const Peak &peak = integratedPeaksWS->getPeak(i);
      const PeakShape &peakShape = peak.getPeakShape();

      TSM_ASSERT_RELATION("Peak should be integrated", std::greater<double>, peak.getIntensity(), 0);

      std::stringstream stream;
      stream << "Wrong shape name for peak " << i;
      TSM_ASSERT_EQUALS(stream.str(), PeakShapeEllipsoid::ellipsoidShapeName(), peakShape.shapeName());

      // Calculate the q direction based on geometry
      const V3D detPos = peak.getDetectorPosition();
      const V3D detDir = normalize(detPos - samplePos);
      const V3D qDir = normalize(detDir - beamDir);

      // Get the q-direction off the ellipsoid
      PeakShapeEllipsoid const *const ellipsoid = dynamic_cast<const PeakShapeEllipsoid *>(&peakShape);
      auto dirs = ellipsoid->directions();

      /* We expect an axis of the ellipsoid to be mostly
       * along Qhkl (probably the principal axis (i.e.
       * direction of max varaince) but not necassarily)
       */
      double minangle = M_PI / 2;
      for (size_t ii = 0; ii < dirs.size(); ii++) {
        double angle = qDir.angle(dirs[ii]);
        if (angle > M_PI / 2) {
          // possible axis is flipepd
          angle = M_PI - angle;
        }
        minangle = std::min(minangle, angle);
      }
      TS_ASSERT_LESS_THAN(minangle, 0.05); // aprox 3 deg
    }
  }

public:
  static void destroySuite(IntegrateEllipsoidsTest *suite) { delete suite; }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEllipsoidsTest *createSuite() { return new IntegrateEllipsoidsTest(); }

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
    auto rebinAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("Rebin");
    rebinAlg->setChild(true);
    rebinAlg->initialize();
    rebinAlg->setProperty("InputWorkspace", m_eventWS);
    auto params = std::vector<double>();
    params.emplace_back(950);
    params.emplace_back(10);
    params.emplace_back(2500);
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

  void test_ws_has_instrument() {
    auto inputWorkspaceNoInstrument = std::make_shared<EventWorkspace>();

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", inputWorkspaceNoInstrument), std::invalid_argument &);
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
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());

    do_test_n_peaks(integratedPeaksWS, 3 /*check first 3 peaks*/);

    const auto &peak1 = integratedPeaksWS->getPeak(0);
    const auto &peak2 = integratedPeaksWS->getPeak(1);
    const auto &peak3 = integratedPeaksWS->getPeak(2);
    const auto &peak4 = integratedPeaksWS->getPeak(3);
    const auto &peak5 = integratedPeaksWS->getPeak(4);
    const auto &peak6 = integratedPeaksWS->getPeak(5);

    TS_ASSERT_DELTA(peak1.getIntensity(), 8., 1e-6);
    TS_ASSERT_DELTA(peak2.getIntensity(), 7., 1e-6);
    TS_ASSERT_DELTA(peak3.getIntensity(), 8., 1e-6);
    TS_ASSERT_DELTA(peak4.getIntensity(), 17., 1e-6);
    TS_ASSERT_DELTA(peak5.getIntensity(), 11., 1e-6);
    TS_ASSERT_DELTA(peak6.getIntensity(), 12., 1e-6);
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
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());

    do_test_n_peaks(integratedPeaksWS, 3 /*check first 3 peaks*/);

    const auto &peak1 = integratedPeaksWS->getPeak(0);
    const auto &peak2 = integratedPeaksWS->getPeak(1);
    const auto &peak3 = integratedPeaksWS->getPeak(2);

    TS_ASSERT_DELTA(peak1.getIntensity(), 4., 1e-6);
    TS_ASSERT_DELTA(peak2.getIntensity(), 7., 1e-6);
    TS_ASSERT_DELTA(peak3.getIntensity(), 4., 1e-6);
  }

  void test_execution_histograms_distribution_data() {
    using namespace Mantid::API;
    const auto &algManager = AlgorithmManager::Instance();

    auto cloneWorkspace = algManager.createUnmanaged("CloneWorkspace");
    cloneWorkspace->setChild(true);
    cloneWorkspace->initialize();
    cloneWorkspace->setProperty("InputWorkspace", m_histoWS);
    cloneWorkspace->setPropertyValue("OutputWorkspace", "dist_workspace");
    cloneWorkspace->execute();
    Workspace_sptr temp = cloneWorkspace->getProperty("OutputWorkspace");
    auto distWS = std::dynamic_pointer_cast<MatrixWorkspace>(temp);

    auto convertToDist = algManager.createUnmanaged("ConvertToDistribution");
    convertToDist->setChild(true);
    convertToDist->initialize();
    convertToDist->setProperty("Workspace", distWS);
    convertToDist->execute();
    distWS = convertToDist->getProperty("Workspace");

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", distWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());

    do_test_n_peaks(integratedPeaksWS, 3 /*check first 3 peaks*/);

    const auto &peak1 = integratedPeaksWS->getPeak(0);
    const auto &peak2 = integratedPeaksWS->getPeak(1);
    const auto &peak3 = integratedPeaksWS->getPeak(2);

    const double binWidth{10.};
    TS_ASSERT_DELTA(peak1.getIntensity(), 4. / binWidth, 1e-6);
    TS_ASSERT_DELTA(peak2.getIntensity(), 7. / binWidth, 1e-6);
    TS_ASSERT_DELTA(peak3.getIntensity(), 4. / binWidth, 1e-6);
  }

  void test_execution_events_adaptive() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_eventWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.20));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.23));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.26));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQMultiplier", 0.01));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQBackground", true));
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());

    TSM_ASSERT_DELTA("Wrong intensity for peak 0", integratedPeaksWS->getPeak(0).getIntensity(), 8., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 1", integratedPeaksWS->getPeak(1).getIntensity(), 9., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 2", integratedPeaksWS->getPeak(2).getIntensity(), 8., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 3", integratedPeaksWS->getPeak(3).getIntensity(), 16., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 4", integratedPeaksWS->getPeak(4).getIntensity(), 12., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 5", integratedPeaksWS->getPeak(5).getIntensity(), 9.97, 0.01);
  }

  void test_execution_histograms_adaptive() {

    IntegrateEllipsoids alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_histoWS);
    alg.setProperty("PeaksWorkspace", m_peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.20));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.23));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.26));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQMultiplier", 0.01));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQBackground", true));
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());
    TSM_ASSERT_DELTA("Wrong intensity for peak 0", integratedPeaksWS->getPeak(0).getIntensity(), 5., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 1", integratedPeaksWS->getPeak(1).getIntensity(), 8., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 2", integratedPeaksWS->getPeak(2).getIntensity(), 4., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 3", integratedPeaksWS->getPeak(3).getIntensity(), 16., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 4", integratedPeaksWS->getPeak(4).getIntensity(), 12., 0.01);
    TSM_ASSERT_DELTA("Wrong intensity for peak 5", integratedPeaksWS->getPeak(5).getIntensity(), 13., 0.01);
  }

  void test_execution_background_shell() {

    const std::vector<double> ub = {0.15468228,  0.10908475,  -0.14428671, -0.08922105, -0.08617147,
                                    -0.22976459, -0.05616441, 0.12536522,  -0.03238277};

    auto loadalg = AlgorithmManager::Instance().createUnmanaged("LoadNexusProcessed");
    TS_ASSERT_THROWS_NOTHING(loadalg->initialize());
    TS_ASSERT_THROWS_NOTHING(loadalg->setPropertyValue("Filename", "TOPAZ_36079_crop.nxs"));
    TS_ASSERT_THROWS_NOTHING(loadalg->setProperty("OutputWorkspace", "TOPAZ_36079_event"));
    TS_ASSERT_THROWS_NOTHING(loadalg->execute());
    TS_ASSERT_THROWS_NOTHING(loadalg->isExecuted());

    auto loadinstralg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
    TS_ASSERT_THROWS_NOTHING(loadinstralg->initialize());
    TS_ASSERT_THROWS_NOTHING(loadinstralg->setProperty("Workspace", "TOPAZ_36079_event"));
    TS_ASSERT_THROWS_NOTHING(loadinstralg->setProperty("InstrumentName", "TOPAZ"));
    TS_ASSERT_THROWS_NOTHING(loadinstralg->setProperty("RewriteSpectraMap", "False"));
    TS_ASSERT_THROWS_NOTHING(loadinstralg->execute());

    auto loadcalalg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadIsawDetCal");
    TS_ASSERT_THROWS_NOTHING(loadcalalg->initialize());
    TS_ASSERT_THROWS_NOTHING(loadcalalg->setProperty("Filename", "TOPAZ_2020A.DetCal"));
    TS_ASSERT_THROWS_NOTHING(loadcalalg->setProperty("InputWorkspace", "TOPAZ_36079_event"));
    TS_ASSERT_THROWS_NOTHING(loadcalalg->execute());

    auto convertalg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertToMD");
    TS_ASSERT_THROWS_NOTHING(convertalg->initialize());
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("InputWorkspace", "TOPAZ_36079_event"));
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("OutputWorkspace", "TOPAZ_36079_md"));
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("dEAnalysisMode", "Elastic"));
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("Q3DFrames", "Q_sample"));
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("LorentzCorrection", true));
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("MinValues", std::vector<double>{1.0, 1.0, 1.675}));
    TS_ASSERT_THROWS_NOTHING(convertalg->setProperty("MaxValues", std::vector<double>{10.0, 5.0, 8.425}));
    TS_ASSERT_THROWS_NOTHING(convertalg->execute());

    auto peaksalg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreatePeaksWorkspace");
    TS_ASSERT_THROWS_NOTHING(peaksalg->initialize());
    TS_ASSERT_THROWS_NOTHING(peaksalg->setProperty("InstrumentWorkspace", "TOPAZ_36079_event"));
    TS_ASSERT_THROWS_NOTHING(peaksalg->setProperty("NumberOfPeaks", 0));
    TS_ASSERT_THROWS_NOTHING(peaksalg->setProperty("OutputWorkspace", "TOPAZ_36079_peaks"));
    TS_ASSERT_THROWS_NOTHING(peaksalg->execute());

    auto setubalg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SetUB");
    TS_ASSERT_THROWS_NOTHING(setubalg->initialize());
    TS_ASSERT_THROWS_NOTHING(setubalg->setProperty("Workspace", "TOPAZ_36079_peaks"));
    TS_ASSERT_THROWS_NOTHING(setubalg->setProperty("UB", ub));
    TS_ASSERT_THROWS_NOTHING(setubalg->execute());

    // add some peaks for testing
    PeaksWorkspace_sptr peakWS = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("TOPAZ_36079_peaks");
    peakWS->addPeak(*peakWS->createPeakHKL(V3D(0.15, 1.85, -1.0)));
    peakWS->addPeak(*peakWS->createPeakHKL(V3D(1.0, 4.0, -3.0)));
    peakWS->addPeak(*peakWS->createPeakHKL(V3D(1.0, 5.0, -3.0)));
    TS_ASSERT_EQUALS(peakWS->getNumberPeaks(), 3);

    auto indexalg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("IndexPeaks");
    TS_ASSERT_THROWS_NOTHING(indexalg->initialize());
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("PeaksWorkspace", "TOPAZ_36079_peaks"));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("Tolerance", 0.06));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("ToleranceForSatellite", 0.05));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("RoundHKLs", false));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("ModVector1", std::vector<double>{0.125, 0.0, 0.0}));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("ModVector2", std::vector<double>{0.0, 0.125, 0.0}));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("ModVector3", std::vector<double>{-0.125, 0.125, 0.0}));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("MaxOrder", 1));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("CrossTerms", false));
    TS_ASSERT_THROWS_NOTHING(indexalg->setProperty("SaveModulationInfo", true));
    TS_ASSERT_THROWS_NOTHING(indexalg->execute());

    // integrate without satellite background for comparison
    IntegrateEllipsoids alg;
    TS_ASSERT_THROWS_NOTHING(alg.setChild(true));
    TS_ASSERT_THROWS_NOTHING(alg.setRethrows(true));
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TOPAZ_36079_event"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeaksWorkspace", "TOPAZ_36079_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "peaks_integrated_nosatellite"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RegionRadius", 0.055));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.0425));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.043));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.055));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CutoffIsigI", 5.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseOnePercentBackgroundCorrection", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT_THROWS_NOTHING(alg.isExecuted());

    PeaksWorkspace_sptr peaksNoSatellite = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(peaksNoSatellite->getNumberPeaks(), 3);

    // integrate the data now with satellite background options
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TOPAZ_36079_event"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeaksWorkspace", "TOPAZ_36079_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "peaks_integrated_satellite"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RegionRadius", 0.055));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.0425));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.043));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.055));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CutoffIsigI", 5.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseOnePercentBackgroundCorrection", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SatelliteRegionRadius", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SatellitePeakSize", 0.08));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SatelliteBackgroundInnerSize", 0.081));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SatelliteBackgroundOuterSize", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT_THROWS_NOTHING(alg.isExecuted());

    PeaksWorkspace_sptr peaksSatellite = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(peaksSatellite->getNumberPeaks(), 3);

    for (int peakind = 0; peakind < peaksSatellite->getNumberPeaks(); peakind++) {
      const Peak &peakNoSatellite = peaksNoSatellite->getPeak(peakind);
      const Peak &peakSatellite = peaksSatellite->getPeak(peakind);

      // first peak is satellite peak and should have different intensity than first run of algorithm
      // last two peaks should be identical to algorithm without satellite background options
      if (peakind == 0) {
        TS_ASSERT_DELTA(peakNoSatellite.getIntensity(), 17.0, 1e-6);
        TS_ASSERT_DELTA(peakSatellite.getIntensity(), 13.0, 1e-6);
      } else {
        TS_ASSERT_DELTA(peakNoSatellite.getIntensity(), peakSatellite.getIntensity(), 1e-6);
      }

      TS_ASSERT_EQUALS(peakNoSatellite.getDetectorPosition(), peakSatellite.getDetectorPosition());
    }

    AnalysisDataService::Instance().remove("TOPAZ_36079_event");
    AnalysisDataService::Instance().remove("TOPAZ_36079_md");
    AnalysisDataService::Instance().remove("TOPAZ_36079_peaks");
    AnalysisDataService::Instance().remove("peaks_integrated_satellite");
    AnalysisDataService::Instance().remove("peaks_integrated_nosatellite");
  }
};

class IntegrateEllipsoidsTestPerformance : public CxxTest::TestSuite {

private:
  Mantid::API::MatrixWorkspace_sptr m_eventWS;
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaksWS;
  Mantid::API::MatrixWorkspace_sptr m_histoWS;

public:
  static void destroySuite(IntegrateEllipsoidsTestPerformance *suite) { delete suite; }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEllipsoidsTestPerformance *createSuite() { return new IntegrateEllipsoidsTestPerformance(); }

  IntegrateEllipsoidsTestPerformance() {
    // Need to get and run algorithms from elsewhere in the framework.
    Mantid::API::FrameworkManager::Instance();

    auto data = createDiffractionData(200 /*sqrt total pixels*/, 60 /*events per peak*/, 2 /*tof gap*/);

    m_eventWS = data.get<0>();
    m_peaksWS = data.get<1>();

    /*
     Simply rebin the event workspace to a histo workspace to create the input
     we need.
    */
    auto rebinAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("Rebin");
    rebinAlg->setChild(true);
    rebinAlg->initialize();
    rebinAlg->setProperty("InputWorkspace", m_eventWS);
    auto params = std::vector<double>();
    params.emplace_back(950);
    params.emplace_back(5);
    params.emplace_back(2500);
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

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
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

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
                      m_peaksWS->getNumberPeaks());
  }
};
