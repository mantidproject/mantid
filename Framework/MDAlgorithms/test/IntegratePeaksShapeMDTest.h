// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/IntegratePeaksShapeMD.h"

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/SingleCrystalDiffractionTestHelper.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/EllipsoidTrajectoryNorm.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"
#include "MantidMDAlgorithms/MDNorm.h"

#include <cmath>
#include <cxxtest/TestSuite.h>
#include <numeric>
#include <random>
#include <tuple>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using Mantid::Kernel::V3D;
using namespace Mantid::SingleCrystalDiffractionTestHelper;

namespace {
/// solid angle workspace for the detectors of ws, with the same solid angle for every pixel
MatrixWorkspace_sptr createSolidAngleWorkspace(const MatrixWorkspace &ws, double solidAngle) {
  MatrixWorkspace_sptr solidAngleWS = create<Workspace2D>(ws, HistogramData::Points(1));
  for (size_t i = 0; i < solidAngleWS->getNumberHistograms(); ++i)
    solidAngleWS->mutableY(i)[0] = solidAngle;
  return solidAngleWS;
}

/// integrated flux workspace for the detectors of ws, for a flat flux phi from k = 0 to kMax
MatrixWorkspace_sptr createFluxWorkspace(const MatrixWorkspace &ws, double phi, double kMax) {
  const size_t numPoints = 3001;
  MatrixWorkspace_sptr fluxWS = create<Workspace2D>(ws, HistogramData::Points(numPoints));
  auto &x = fluxWS->mutableX(0);
  auto &y = fluxWS->mutableY(0);
  for (size_t j = 0; j < numPoints; ++j) {
    x[j] = kMax * static_cast<double>(j) / static_cast<double>(numPoints - 1);
    y[j] = phi * x[j];
  }
  for (size_t i = 1; i < fluxWS->getNumberHistograms(); ++i) {
    fluxWS->setSharedX(i, fluxWS->sharedX(0));
    fluxWS->setSharedY(i, fluxWS->sharedY(0));
  }
  return fluxWS;
}

/// the logs CropWorkspaceForMDNorm would add, and a proton charge
void addMDNormLogs(MatrixWorkspace &ws, double kMin, double kMax, double protonCharge) {
  const auto numSpectra = ws.getNumberHistograms();
  ws.mutableRun().addProperty("MDNorm_low", std::vector<double>(numSpectra, kMin), true);
  ws.mutableRun().addProperty("MDNorm_high", std::vector<double>(numSpectra, kMax), true);
  ws.mutableRun().setProtonCharge(protonCharge);
}

/**
 * Normalization of a sphere for a flat flux phi and the same weight
 * (solid angle times proton charge) for every pixel, from the closest
 * approach of each pixel's line Q = k u to the sphere's center.
 */
double sphereNorm(const MatrixWorkspace &ws, const V3D &center, double radius, double weightTimesFlux, double kMin,
                  double kMax, size_t &numPixels) {
  const auto &spectrumInfo = ws.spectrumInfo();
  double norm = 0.0;
  numPixels = 0;
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;
    const double polar = spectrumInfo.twoTheta(i);
    const double azim = spectrumInfo.detector(i).getPhi();
    const V3D u(-std::sin(polar) * std::cos(azim), -std::sin(polar) * std::sin(azim), 1.0 - std::cos(polar));
    const double kClosest = center.scalar_prod(u) / u.norm2();
    const double dist2 = (center - u * kClosest).norm2();
    if (dist2 >= radius * radius)
      continue;
    const double halfChord = std::sqrt(radius * radius - dist2) / u.norm();
    const double k1 = std::max(kClosest - halfChord, kMin);
    const double k2 = std::min(kClosest + halfChord, kMax);
    if (k2 > k1) {
      norm += weightTimesFlux * (k2 - k1);
      ++numPixels;
    }
  }
  return norm;
}

void setSphericalShapes(PeaksWorkspace &peaksWS, double peakRadius, double innerRadius, double outerRadius) {
  const PeakEllipsoidFrame directions{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
  const PeakEllipsoidExtent peak{peakRadius, peakRadius, peakRadius};
  const PeakEllipsoidExtent inner{innerRadius, innerRadius, innerRadius};
  const PeakEllipsoidExtent outer{outerRadius, outerRadius, outerRadius};
  for (auto &p : peaksWS.getPeaks())
    p.setPeakShape(new PeakShapeEllipsoid(directions, peak, inner, outer, Kernel::QLab));
}

/// run IntegratePeaksShapeMD in count mode, normalized if solidAngleWS and fluxWS are given
std::pair<PeaksWorkspace_sptr, ITableWorkspace_sptr>
integrateCounts(const MatrixWorkspace_sptr &inputWS, const PeaksWorkspace_sptr &peaksWS, double regionRadius,
                const MatrixWorkspace_sptr &solidAngleWS = nullptr, const MatrixWorkspace_sptr &fluxWS = nullptr) {
  IntegratePeaksShapeMD alg;
  alg.setChild(true);
  alg.setRethrows(true);
  alg.initialize();
  alg.setProperty("InputWorkspace", inputWS);
  alg.setProperty("PeaksWorkspace", peaksWS);
  alg.setProperty("RegionRadius", regionRadius);
  alg.setProperty("UseOnePercentBackgroundCorrection", false);
  alg.setPropertyValue("OutputWorkspace", "out");
  if (solidAngleWS && fluxWS) {
    alg.setProperty("SolidAngleWorkspace", solidAngleWS);
    alg.setProperty("FluxWorkspace", fluxWS);
    alg.setPropertyValue("OutputDiagnosticsWorkspace", "diagnostics");
  }
  alg.execute();
  PeaksWorkspace_sptr out = alg.getProperty("OutputWorkspace");
  ITableWorkspace_sptr diagnostics;
  if (solidAngleWS && fluxWS)
    diagnostics = alg.getProperty("OutputDiagnosticsWorkspace");
  return {out, diagnostics};
}

/**
 * Add events to ws drawn from the rate density f(Q) = height * g(Q) +
 * background about each center, where g is a spherical Gaussian of standard
 * deviation sigma and peak height 1, within radius of each center: along
 * each pixel's line Q = k u the expected counts in dk are
 * weightTimesFlux * f(k u) dk, as for a flat flux and the same solid angle
 * for every pixel.
 */
void generateEvents(EventWorkspace &ws, const std::vector<V3D> &centers, const std::vector<double> &heights,
                    const PeakEllipsoidFrame &directions, const PeakEllipsoidExtent &sigmas, double background,
                    double radius, double weightTimesFlux) {
  std::mt19937 rng(7);
  std::uniform_real_distribution<double> uniform(0.0, 1.0);
  const auto &spectrumInfo = ws.spectrumInfo();
  const double l1 = spectrumInfo.l1();
  const double dk = 0.002;
  const PeakEllipsoidFrame axes{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!spectrumInfo.hasDetectors(i))
      continue;
    const double polar = spectrumInfo.twoTheta(i);
    const double azim = spectrumInfo.detector(i).getPhi();
    const V3D u(-std::sin(polar) * std::cos(azim), -std::sin(polar) * std::sin(azim), 1.0 - std::cos(polar));
    const double flightPath = l1 + spectrumInfo.l2(i);
    auto &events = ws.getSpectrum(i);
    for (size_t c = 0; c < centers.size(); ++c) {
      double k1, k2;
      if (!EllipsoidTrajectoryNorm::intersect(u, centers[c], axes, {radius, radius, radius}, k1, k2))
        continue;
      for (double k = k1; k < k2; k += dk) {
        const V3D q = u * (k + 0.5 * dk) - centers[c];
        double m = 0.0;
        for (size_t k = 0; k < 3; ++k) {
          const double comp = q.scalar_prod(directions[k]) / sigmas[k];
          m += comp * comp;
        }
        const double rate = heights[c] * std::exp(-0.5 * m) + background;
        std::poisson_distribution<int> poisson(weightTimesFlux * rate * dk);
        for (int n = poisson(rng); n > 0; --n) {
          // time of flight for momentum kEvent, in microseconds
          const double kEvent = k + uniform(rng) * dk;
          const double wavelength = 2.0 * M_PI / kEvent * 1e-10;
          const double tof = 1e6 * flightPath * PhysicalConstants::NeutronMass * wavelength / PhysicalConstants::h;
          events.addEventQuickly(Types::Event::TofEvent(tof));
        }
      }
    }
  }
}

/// generateEvents for spherical Gaussians of standard deviation sigma
void generateEvents(EventWorkspace &ws, const std::vector<V3D> &centers, const std::vector<double> &heights,
                    double sigma, double background, double radius, double weightTimesFlux) {
  generateEvents(ws, centers, heights, {V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)}, {sigma, sigma, sigma}, background,
                 radius, weightTimesFlux);
}

/// covariance of an ellipsoid as xx, yy, zz, xy, xz, yz
std::array<double, 6> covariance(const PeakEllipsoidFrame &directions, const PeakEllipsoidExtent &sigmas) {
  constexpr std::array<std::array<size_t, 2>, 6> pairs{{{0, 0}, {1, 1}, {2, 2}, {0, 1}, {0, 2}, {1, 2}}};
  std::array<double, 6> cov{};
  for (size_t k = 0; k < 3; ++k)
    for (size_t e = 0; e < 6; ++e)
      cov[e] += directions[k][pairs[e][0]] * directions[k][pairs[e][1]] * sigmas[k] * sigmas[k];
  return cov;
}

/// relative (Frobenius) difference of two covariances
double relativeDifference(const std::array<double, 6> &a, const std::array<double, 6> &b) {
  double diff = 0.0, size = 0.0;
  for (size_t e = 0; e < 6; ++e) {
    const double weight = e < 3 ? 1.0 : 2.0;
    diff += weight * (a[e] - b[e]) * (a[e] - b[e]);
    size += weight * b[e] * b[e];
  }
  return std::sqrt(diff / size);
}

/// a frame rotated away from the lab axes
PeakEllipsoidFrame tiltedAxes() {
  V3D e0(1, 1, 0), e1(-1, 1, 1);
  e0.normalize();
  e1.normalize();
  return {e0, e1, e0.cross_prod(e1)};
}

/// three peaks without events, for generateEvents, with spherical shapes
std::pair<EventWorkspace_sptr, PeaksWorkspace_sptr> buildEmptyPeaks() {
  WorkspaceBuilder builder;
  builder.setRandomSeed(1);
  builder.setNumPixels(100);
  builder.addBackground(false);
  const auto sigmas = std::make_tuple(.002, .002, .1);
  builder.addPeakByHKL(V3D(1, -4, -2), 0, sigmas);
  builder.addPeakByHKL(V3D(1, -4, 0), 0, sigmas);
  builder.addPeakByHKL(V3D(2, -3, -4), 0, sigmas);
  auto data = builder.build();
  return {std::dynamic_pointer_cast<EventWorkspace>(std::get<0>(data)), std::get<1>(data)};
}

/// run IntegratePeaksShapeMD in profile mode, normalized by flux and solid angle
std::pair<PeaksWorkspace_sptr, ITableWorkspace_sptr>
integrateProfile(const MatrixWorkspace_sptr &inputWS, const PeaksWorkspace_sptr &peaksWS, double regionRadius,
                 const MatrixWorkspace_sptr &solidAngleWS, const MatrixWorkspace_sptr &fluxWS,
                 bool adjustCenter = false, bool adjustShape = false, double maxShapeChange = 2.0,
                 double minSignalToNoise = 10.0, double maxCenterShift = 3.0) {
  IntegratePeaksShapeMD alg;
  alg.setChild(true);
  alg.setRethrows(true);
  alg.initialize();
  alg.setProperty("InputWorkspace", inputWS);
  alg.setProperty("PeaksWorkspace", peaksWS);
  alg.setProperty("RegionRadius", regionRadius);
  alg.setProperty("ProfileFit", true);
  alg.setProperty("AdjustCenter", adjustCenter);
  alg.setProperty("AdjustShape", adjustShape);
  alg.setProperty("MaxShapeChange", maxShapeChange);
  alg.setProperty("AdjustShapeMinSignalToNoise", minSignalToNoise);
  alg.setProperty("MaxCenterShift", maxCenterShift);
  alg.setProperty("SolidAngleWorkspace", solidAngleWS);
  alg.setProperty("FluxWorkspace", fluxWS);
  alg.setPropertyValue("OutputWorkspace", "out");
  alg.setPropertyValue("OutputDiagnosticsWorkspace", "diagnostics");
  alg.execute();
  PeaksWorkspace_sptr out = alg.getProperty("OutputWorkspace");
  ITableWorkspace_sptr diagnostics = alg.getProperty("OutputDiagnosticsWorkspace");
  return {out, diagnostics};
}
} // namespace

class IntegratePeaksShapeMDTest : public CxxTest::TestSuite {

public:
  void test_init() {
    IntegratePeaksShapeMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_EQUALS(alg.name(), "IntegratePeaksShapeMD");
    TS_ASSERT_EQUALS(alg.version(), 1);
    TS_ASSERT_EQUALS(alg.category(), "Crystal\\Integration");
    const double regionRadius = alg.getProperty("RegionRadius");
    TS_ASSERT_DELTA(regionRadius, 0.35, 1e-10);
    TS_ASSERT_EQUALS(alg.getPropertyValue("ProfileFit"), "0");
    TS_ASSERT_EQUALS(alg.getPropertyValue("AdjustCenter"), "0");
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("RegionRadius", -0.01));
  }

  void test_exec_throws_without_peak_shapes() {
    const int numEventsPerPeak = 10000;
    const auto sigmas = std::make_tuple(.002, .002, 0.1);

    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerPeak, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);

    // peaksWS has not been integrated yet, so none of its peaks have a shape
    IntegratePeaksShapeMD alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_throws_for_shape_not_in_qlab() {
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), 100, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);
    const PeakEllipsoidFrame directions{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
    const PeakEllipsoidExtent radii{0.35, 0.35, 0.35};
    peaksWS->getPeak(0).setPeakShape(new PeakShapeEllipsoid(directions, radii, radii, radii, Kernel::QSample));

    IntegratePeaksShapeMD alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  // Peaks need not be indexed: each event is assigned to the nearest peak, so
  // unindexed peaks are integrated as they are when indexed.
  void test_exec_integrates_unindexed_peaks() {
    const auto sigmas = std::make_tuple(.002, .002, .1);
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), 100, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);
    const PeakEllipsoidFrame directions{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
    const PeakEllipsoidExtent radii{.35, .35, .35};
    for (auto &peak : peaksWS->getPeaks())
      peak.setPeakShape(new PeakShapeEllipsoid(directions, radii, radii, radii, Kernel::QLab));

    auto integrate = [&](const PeaksWorkspace_sptr &peaks) {
      IntegratePeaksShapeMD alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", eventWS);
      alg.setProperty("PeaksWorkspace", peaks);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      TS_ASSERT(alg.isExecuted());
      PeaksWorkspace_sptr out = alg.getProperty("OutputWorkspace");
      return out;
    };

    const auto indexed = integrate(peaksWS);
    PeaksWorkspace_sptr unindexedWS = peaksWS->clone();
    for (auto &peak : unindexedWS->getPeaks())
      peak.setHKL(0, 0, 0);
    const auto unindexed = integrate(unindexedWS);
    for (int i = 0; i < indexed->getNumberPeaks(); ++i) {
      TS_ASSERT_LESS_THAN(0.0, indexed->getPeak(i).getIntensity());
      TS_ASSERT_DELTA(unindexed->getPeak(i).getIntensity(), indexed->getPeak(i).getIntensity(), 1e-9);
      TS_ASSERT_DELTA(unindexed->getPeak(i).getSigmaIntensity(), indexed->getPeak(i).getSigmaIntensity(), 1e-9);
    }
  }

  // A satellite near a nuclear peak, close enough to round to its h,k,l, is
  // integrated as a peak of its own, and the nuclear peak as without it.
  void test_satellite_peaks_are_integrated_as_peaks_of_their_own() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.07, height = 6000.0, background = 10.0, region = 0.35;
    const auto satellite = peaksWS->createPeakHKL(peaksWS->getPeak(0).getHKL() + V3D(0.45, 0.45, 0.45));
    peaksWS->addPeak(*satellite);
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    // clear of the other peaks' regions
    for (int i = 0; i < 3; ++i)
      TS_ASSERT_LESS_THAN(2 * (region + 0.05), (centers[3] - centers[i]).norm());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), sigma, background, region + 0.05,
                   1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, sigma, sigma, sigma);
    const auto [profile, diagnostics] = integrateProfile(
        eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0), createFluxWorkspace(*eventWS, 1.0, 50.0));

    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigma * sigma * sigma;
    for (int i = 0; i < profile->getNumberPeaks(); ++i) {
      TS_ASSERT_LESS_THAN(0, diagnostics->cell<int>(i, 5));
      const auto &peak = profile->getPeak(i);
      TSM_ASSERT_DELTA("Intensity of peak " + std::to_string(i), peak.getIntensity(), trueIntensity,
                       4 * peak.getSigmaIntensity() + 0.005 * trueIntensity);
    }
  }

  void test_exec_does_not_modify_input_events() {
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    // Six well-separated peaks (matching test_exec_events_reuses_existing_shape).
    builder.addPeakByHKL(V3D(1, -5, -3), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), 100, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), 100, sigmas);

    auto data = builder.build();
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(std::get<0>(data));
    auto peaksWS = std::get<1>(data);
    const PeakEllipsoidFrame directions{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
    const PeakEllipsoidExtent radii{0.35, 0.35, 0.35};
    for (auto &peak : peaksWS->getPeaks())
      peak.setPeakShape(new PeakShapeEllipsoid(directions, radii, radii, radii, Kernel::QLab));

    const auto originalEventType = eventWS->getEventType();
    const auto originalEventCount = eventWS->getNumberEvents();

    IntegratePeaksShapeMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT_EQUALS(eventWS->getEventType(), originalEventType);
    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), originalEventCount);
  }

  void test_exec_events_reuses_existing_shape() {
    const int numEventsPerPeak = 10000;
    // Very tight distribution with events happening at a single point
    const auto sigmas = std::make_tuple(.002, .002, 0.1);

    // Build some diffraction data
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerPeak, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);

    // First integrate normally so every peak gets an ellipsoid shape
    IntegrateEllipsoidsTwoStep shapeAlg;
    shapeAlg.setChild(true);
    shapeAlg.setRethrows(true);
    shapeAlg.initialize();
    shapeAlg.setProperty("InputWorkspace", eventWS);
    shapeAlg.setProperty("PeaksWorkspace", peaksWS);
    shapeAlg.setProperty("SpecifySize", true);
    shapeAlg.setProperty("PeakSize", 0.35);
    shapeAlg.setProperty("BackgroundInnerSize", 0.35);
    shapeAlg.setProperty("BackgroundOuterSize", 0.4);
    shapeAlg.setPropertyValue("OutputWorkspace", "shaped");
    shapeAlg.execute();
    PeaksWorkspace_sptr shapedPeaksWS = shapeAlg.getProperty("OutputWorkspace");

    // Now re-integrate the same events using only the shapes already on the
    // peaks workspace
    IntegratePeaksShapeMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", shapedPeaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RegionRadius", 0.5));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "dummy"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(integratedPeaksWS);

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(),
                      peaksWS->getNumberPeaks());
    const auto &run = integratedPeaksWS->mutableRun();
    TSM_ASSERT("Output workspace must be integrated", run.hasProperty("PeaksIntegrated"));

    for (int i = 0; i < integratedPeaksWS->getNumberPeaks(); ++i) {
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i), integratedPeaksWS->getPeak(i).getIntensity(),
                       numEventsPerPeak, 5);
    }
  }

  // AdjustCenter should persist any found correction as the output shape's
  // translation (leaving directions/radii/frame/algorithm metadata
  // untouched), and should not touch translation at all when disabled.
  void test_exec_profileFit_adjustCenter_writes_translation() {
    const int numEventsPerPeak = 10000;
    const auto sigmas = std::make_tuple(.002, .002, 0.1);

    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerPeak, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);

    // Simple, sigma-scaled shapes (not a hard SpecifySize cutoff) so
    // ProfileFit's Gaussian kernel is well-conditioned against the region.
    const PeakEllipsoidFrame directions{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
    const PeakEllipsoidExtent radii{0.03, 0.03, 0.03};
    for (auto &peak : peaksWS->getPeaks())
      peak.setPeakShape(new PeakShapeEllipsoid(directions, radii, radii, radii, Mantid::Kernel::QLab));

    auto runIntegration = [&](bool adjustCenter, const std::string &outputName) {
      IntegratePeaksShapeMD alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", eventWS);
      alg.setProperty("PeaksWorkspace", peaksWS);
      alg.setProperty("RegionRadius", 0.5);
      alg.setProperty("ProfileFit", true);
      alg.setProperty("AdjustCenter", adjustCenter);
      alg.setPropertyValue("OutputWorkspace", outputName);
      alg.execute();
      PeaksWorkspace_sptr result = alg.getProperty("OutputWorkspace");
      return result;
    };

    PeaksWorkspace_sptr withoutAdjust = runIntegration(false, "withoutAdjust");
    PeaksWorkspace_sptr withAdjust = runIntegration(true, "withAdjust");

    for (int i = 0; i < withoutAdjust->getNumberPeaks(); ++i) {
      const auto *inputShape = dynamic_cast<const PeakShapeEllipsoid *>(&peaksWS->getPeak(i).getPeakShape());
      const auto *noAdjustShape = dynamic_cast<const PeakShapeEllipsoid *>(&withoutAdjust->getPeak(i).getPeakShape());
      const auto *adjustShape = dynamic_cast<const PeakShapeEllipsoid *>(&withAdjust->getPeak(i).getPeakShape());
      TS_ASSERT(noAdjustShape);
      TS_ASSERT(adjustShape);

      TSM_ASSERT_EQUALS("AdjustCenter=false must not write a translation for peak " + std::to_string(i),
                        noAdjustShape->translation(), V3D(0, 0, 0));

      // Everything except translation must survive the reconstruction
      // unchanged: same directions, radii, frame and algorithm metadata.
      for (size_t k = 0; k < 3; ++k) {
        TS_ASSERT_EQUALS(adjustShape->directions()[k], inputShape->directions()[k]);
        TS_ASSERT_EQUALS(adjustShape->abcRadii()[k], inputShape->abcRadii()[k]);
        TS_ASSERT_EQUALS(adjustShape->abcRadiiBackgroundInner()[k], inputShape->abcRadiiBackgroundInner()[k]);
        TS_ASSERT_EQUALS(adjustShape->abcRadiiBackgroundOuter()[k], inputShape->abcRadiiBackgroundOuter()[k]);
      }
      TS_ASSERT_EQUALS(adjustShape->frame(), inputShape->frame());
    }
  }

  void test_exec_histogram_reuses_existing_shape() {
    const int numEventsPerPeak = 1000;
    const auto sigmas = std::make_tuple(.002, .002, .01);

    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.outputAsHistogram(true);
    builder.setRebinParameters({800, 5, 10000});
    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerPeak, sigmas);

    auto data = builder.build();
    auto histoWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);
    const PeakEllipsoidFrame directions{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
    const PeakEllipsoidExtent radii{.5, .5, .5};
    for (auto &peak : peaksWS->getPeaks())
      peak.setPeakShape(new PeakShapeEllipsoid(directions, radii, radii, radii, Kernel::QLab));

    IntegratePeaksShapeMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", histoWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setProperty("RegionRadius", .6);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT(integratedPeaksWS);
    for (int i = 0; i < 5; ++i) {
      TSM_ASSERT_DELTA("Wrong histogram intensity for peak " + std::to_string(i),
                       integratedPeaksWS->getPeak(i).getIntensity(), numEventsPerPeak, 5);
    }
  }

  void test_exec_profile_fit_with_center_adjustment() {
    const auto sigmas = std::make_tuple(.002, .002, .1);

    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), 1000, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), 1000, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), 1000, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), 1000, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), 1000, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), 1000, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);
    const PeakEllipsoidFrame directions{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
    const PeakEllipsoidExtent peakSigmas{.15, .15, .15};
    for (auto &peak : peaksWS->getPeaks())
      peak.setPeakShape(new PeakShapeEllipsoid(directions, peakSigmas, peakSigmas, peakSigmas, Kernel::QLab));

    IntegratePeaksShapeMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setProperty("RegionRadius", .5);
    alg.setProperty("ProfileFit", true);
    alg.setProperty("AdjustCenter", true);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT(integratedPeaksWS);
    for (int i = 0; i < integratedPeaksWS->getNumberPeaks(); ++i) {
      const auto &peak = integratedPeaksWS->getPeak(i);
      TSM_ASSERT("Profile-fit intensity should be positive for peak " + std::to_string(i), peak.getIntensity() > 0.0);
      TSM_ASSERT("Profile-fit uncertainty should be finite for peak " + std::to_string(i),
                 std::isfinite(peak.getSigmaIntensity()));
      TSM_ASSERT("Profile-fit uncertainty should be positive for peak " + std::to_string(i),
                 peak.getSigmaIntensity() > 0.0);
    }
  }

  void test_normalization_requires_both_workspaces() {
    auto [eventWS, peaksWS] = buildThreePeaks();
    addMDNormLogs(*eventWS, 1.0, 25.0, 1.0);
    IntegratePeaksShapeMD alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("SolidAngleWorkspace", createSolidAngleWorkspace(*eventWS, 1.0));
    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.count("FluxWorkspace"), 1);

    alg.setProperty("FluxWorkspace", createFluxWorkspace(*eventWS, 1.0, 30.0));
    issues = alg.validateInputs();
    TS_ASSERT(issues.empty());

    alg.setProperty("ProfileFit", true);
    issues = alg.validateInputs();
    TS_ASSERT(issues.empty());
  }

  void test_normalization_requires_mdnorm_logs() {
    auto [eventWS, peaksWS] = buildThreePeaks();
    IntegratePeaksShapeMD alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("SolidAngleWorkspace", createSolidAngleWorkspace(*eventWS, 1.0));
    alg.setProperty("FluxWorkspace", createFluxWorkspace(*eventWS, 1.0, 30.0));
    const auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.count("InputWorkspace"), 1);
  }

  void test_diagnostics_require_normalization() {
    auto [eventWS, peaksWS] = buildThreePeaks();
    IntegratePeaksShapeMD alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setPropertyValue("OutputDiagnosticsWorkspace", "diagnostics");
    const auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.count("OutputDiagnosticsWorkspace"), 1);
  }

  // With a flat flux and the same solid angle for every pixel, each peak's
  // normalization is solid angle * flux * proton charge * the summed k-length
  // of the pixel lines through the sphere, and its intensity is
  // V * (N_peak / Norm_peak - N_shell / Norm_shell).
  void test_flat_flux_normalization_matches_sphere_chords() {
    const int numEventsPerPeak = 10000;
    auto [eventWS, peaksWS] = buildSixPeaks(numEventsPerPeak);
    // wide enough that no pixel's chord through any of the spheres leaves it
    const double solidAngle = 0.5, phi = 2.0, protonCharge = 3.0, kMin = 1.0, kMax = 29.0;
    const double radius = 0.2, inner = 0.25, outer = 0.3;
    addMDNormLogs(*eventWS, kMin, kMax, protonCharge);
    setSphericalShapes(*peaksWS, radius, inner, outer);

    const auto raw = integrateCounts(eventWS, peaksWS, 0.35).first;
    const auto [normalized, diagnostics] =
        integrateCounts(eventWS, peaksWS, 0.35, createSolidAngleWorkspace(*eventWS, solidAngle),
                        createFluxWorkspace(*eventWS, phi, 40.0));
    TS_ASSERT(diagnostics);
    TS_ASSERT_EQUALS(diagnostics->rowCount(), static_cast<size_t>(peaksWS->getNumberPeaks()));

    const double volume = 4.0 / 3.0 * M_PI * radius * radius * radius;
    const double weightTimesFlux = solidAngle * protonCharge * phi;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i) {
      const auto center = peaksWS->getPeak(i).getQLabFrame();
      size_t numPixels = 0, numInner = 0, numOuter = 0;
      const double peakNorm = sphereNorm(*eventWS, center, radius, weightTimesFlux, kMin, kMax, numPixels);
      const double shellNorm = sphereNorm(*eventWS, center, outer, weightTimesFlux, kMin, kMax, numOuter) -
                               sphereNorm(*eventWS, center, inner, weightTimesFlux, kMin, kMax, numInner);
      TS_ASSERT_LESS_THAN(size_t(0), numPixels);

      const double countsPeak = diagnostics->cell<double>(i, 1);
      const double countsShell = diagnostics->cell<double>(i, 2);
      TS_ASSERT_EQUALS(diagnostics->cell<int>(i, 0), i);
      TS_ASSERT_DELTA(diagnostics->cell<double>(i, 3), peakNorm, 1e-9 * peakNorm);
      TS_ASSERT_DELTA(diagnostics->cell<double>(i, 4), shellNorm, 1e-9 * shellNorm);
      TS_ASSERT_EQUALS(diagnostics->cell<int>(i, 5), static_cast<int>(numPixels));
      TS_ASSERT(!diagnostics->cell<Boolean>(i, 6));
      TS_ASSERT(!diagnostics->cell<Boolean>(i, 7));
      // no background, so the raw intensity is the peak count
      TS_ASSERT_DELTA(countsPeak, raw->getPeak(i).getIntensity(), 1e-9);
      TS_ASSERT_DELTA(countsPeak, numEventsPerPeak, 5);

      const double expected = volume * (countsPeak / peakNorm - countsShell / shellNorm);
      const double expectedSigma =
          volume * std::sqrt(countsPeak / (peakNorm * peakNorm) + countsShell / (shellNorm * shellNorm));
      TS_ASSERT_DELTA(normalized->getPeak(i).getIntensity(), expected, 1e-9 * expected);
      TS_ASSERT_DELTA(normalized->getPeak(i).getSigmaIntensity(), expectedSigma, 1e-9 * expectedSigma);
    }
  }

  void test_momentum_range_clipping_is_flagged() {
    auto [eventWS, peaksWS] = buildSixPeaks(1000);
    setSphericalShapes(*peaksWS, 0.2, 0.25, 0.3);
    // a k range ending at the center of peak 0's trajectories
    const double kPeak = 2.0 * M_PI / peaksWS->getPeak(0).getWavelength();
    addMDNormLogs(*eventWS, 1.0, kPeak, 1.0);
    const auto diagnostics = integrateCounts(eventWS, peaksWS, 0.35, createSolidAngleWorkspace(*eventWS, 1.0),
                                             createFluxWorkspace(*eventWS, 1.0, 30.0))
                                 .second;
    TS_ASSERT(diagnostics->cell<Boolean>(0, 6));
  }

  // Events from masked detectors must not be integrated, and peaks whose
  // ellipsoids they cross are flagged.
  void test_masked_detector_events_are_skipped() {
    const int numEventsPerPeak = 1000;
    auto [eventWS, peaksWS] = buildSixPeaks(numEventsPerPeak);
    const auto events = std::dynamic_pointer_cast<EventWorkspace>(eventWS);
    setSphericalShapes(*peaksWS, 0.2, 0.25, 0.3);
    addMDNormLogs(*eventWS, 1.0, 25.0, 1.0);

    auto totalIntensity = [](const PeaksWorkspace &ws) {
      double total = 0.0;
      for (int i = 0; i < ws.getNumberPeaks(); ++i)
        total += ws.getPeak(i).getIntensity();
      return total;
    };
    const double unmasked = totalIntensity(*integrateCounts(eventWS, peaksWS, 0.35).first);

    // mask the pixel with the most events
    size_t busiest = 0;
    for (size_t i = 0; i < eventWS->getNumberHistograms(); ++i)
      if (events->getSpectrum(i).getNumberEvents() > events->getSpectrum(busiest).getNumberEvents())
        busiest = i;
    const auto maskedEvents = static_cast<double>(events->getSpectrum(busiest).getNumberEvents());
    TS_ASSERT_LESS_THAN(0.0, maskedEvents);
    eventWS->mutableSpectrumInfo().setMasked(busiest, true);

    TS_ASSERT_DELTA(totalIntensity(*integrateCounts(eventWS, peaksWS, 0.35).first), unmasked - maskedEvents, 1e-6);

    const auto diagnostics = integrateCounts(eventWS, peaksWS, 0.35, createSolidAngleWorkspace(*eventWS, 1.0),
                                             createFluxWorkspace(*eventWS, 1.0, 30.0))
                                 .second;
    bool anyFlagged = false;
    for (size_t i = 0; i < diagnostics->rowCount(); ++i)
      anyFlagged = anyFlagged || diagnostics->cell<Boolean>(i, 7);
    TS_ASSERT(anyFlagged);
  }

  // Spectra without detectors are left out of the preprocessed detector
  // table, so every later spectrum must still be matched to its own row.
  void test_spectra_without_detectors_do_not_shift_detector_directions() {
    const int numEventsPerPeak = 1000;
    auto [eventWS, peaksWS] = buildSixPeaks(numEventsPerPeak);
    setSphericalShapes(*peaksWS, 0.35, 0.35, 0.4);
    const auto events = std::dynamic_pointer_cast<EventWorkspace>(eventWS);
    for (size_t i = 0; i < 50; ++i) {
      TS_ASSERT_EQUALS(events->getSpectrum(i).getNumberEvents(), size_t(0));
      events->getSpectrum(i).clearDetectorIDs();
    }

    const auto integrated = integrateCounts(eventWS, peaksWS, 0.5).first;
    for (int i = 0; i < integrated->getNumberPeaks(); ++i)
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i), integrated->getPeak(i).getIntensity(),
                       numEventsPerPeak, 5);
  }

  // Count mode normalized by flux and solid angle must agree with binning
  // the same events with MDNorm and summing normalized signal * voxel volume
  // over the peak, when exposure is close to uniform across the peak.
  void test_normalized_counts_match_mdnorm_binning() {
    const int numEvents = 20000;
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(200);
    builder.addBackground(false);
    // a peak spread over many pixels and momenta, and two more to define UB
    builder.addPeakByHKL(V3D(2, -3, -4), numEvents, std::make_tuple(.01, .01, 40.));
    builder.addPeakByHKL(V3D(1, -5, -3), 100, std::make_tuple(.002, .002, .1));
    builder.addPeakByHKL(V3D(1, -4, -2), 100, std::make_tuple(.002, .002, .1));
    auto data = builder.build();
    MatrixWorkspace_sptr eventWS = std::get<0>(data);
    PeaksWorkspace_sptr peaksWS = std::get<1>(data);

    // a sphere holding the whole peak, and a k range wide enough that every
    // pixel covers its whole chord through the sphere
    const double radius = 0.5, kMin = 1.0, kMax = 20.0;
    setSphericalShapes(*peaksWS, radius, 0.55, 0.6);
    addMDNormLogs(*eventWS, kMin, kMax, 2.0);
    // identity goniometer, so Q_sample is Q_lab
    Mantid::Geometry::Goniometer goniometer;
    goniometer.pushAxis("omega", 0., 1., 0., 0.);
    eventWS->mutableRun().setGoniometer(goniometer, false);
    const auto solidAngleWS = createSolidAngleWorkspace(*eventWS, 0.5);
    const auto fluxWS = createFluxWorkspace(*eventWS, 3.0, 30.0);

    const auto [normalized, diagnostics] = integrateCounts(eventWS, peaksWS, 0.65, solidAngleWS, fluxWS);
    const double countMode = normalized->getPeak(0).getIntensity();
    TS_ASSERT_LESS_THAN(0.0, countMode);
    TS_ASSERT(!diagnostics->cell<Boolean>(0, 6));

    // bin with MDNorm in Q_sample (the goniometer is the identity, so Q_lab)
    ConvertToMD convert;
    convert.setChild(true);
    convert.setRethrows(true);
    convert.initialize();
    convert.setProperty("InputWorkspace", eventWS);
    convert.setPropertyValue("QDimensions", "Q3D");
    convert.setPropertyValue("dEAnalysisMode", "Elastic");
    convert.setPropertyValue("Q3DFrames", "Q_sample");
    convert.setPropertyValue("MinValues", "-30,-30,-30");
    convert.setPropertyValue("MaxValues", "30,30,30");
    convert.setPropertyValue("OutputWorkspace", "md");
    convert.execute();
    IMDEventWorkspace_sptr mdWS = convert.getProperty("OutputWorkspace");

    const auto center = peaksWS->getPeak(0).getQLabFrame();
    const double step = 0.1;
    MDNorm norm;
    norm.setChild(true);
    norm.setRethrows(true);
    norm.initialize();
    norm.setProperty("InputWorkspace", mdWS);
    norm.setProperty("SolidAngleWorkspace", solidAngleWS);
    norm.setProperty("FluxWorkspace", fluxWS);
    norm.setProperty("RLU", false);
    for (size_t d = 0; d < 3; ++d) {
      const std::string binning =
          std::to_string(center[d] - radius) + "," + std::to_string(step) + "," + std::to_string(center[d] + radius);
      norm.setPropertyValue("Dimension" + std::to_string(d) + "Binning", binning);
    }
    norm.setPropertyValue("OutputWorkspace", "normalized");
    norm.setPropertyValue("OutputDataWorkspace", "data");
    norm.setPropertyValue("OutputNormalizationWorkspace", "norm");
    norm.execute();
    Workspace_sptr out = norm.getProperty("OutputWorkspace");
    auto normalizedMD = std::dynamic_pointer_cast<IMDHistoWorkspace>(out);
    TS_ASSERT(normalizedMD);

    Workspace_sptr dataOut = norm.getProperty("OutputDataWorkspace");
    Workspace_sptr normOut = norm.getProperty("OutputNormalizationWorkspace");
    auto dataMD = std::dynamic_pointer_cast<IMDHistoWorkspace>(dataOut);
    auto normMD = std::dynamic_pointer_cast<IMDHistoWorkspace>(normOut);

    // over the voxels whose centers are in the sphere, sum the normalized
    // signal * voxel volume, and separately the raw counts, normalization
    // and volume
    const double voxelVolume = 1.0 / normalizedMD->getInverseVolume();
    double binned = 0.0, binnedCounts = 0.0, binnedNorm = 0.0, binnedVolume = 0.0;
    for (size_t i = 0; i < normalizedMD->getNPoints(); ++i) {
      const auto voxelCenter = normalizedMD->getCenter(i);
      if ((V3D(voxelCenter[0], voxelCenter[1], voxelCenter[2]) - center).norm() > radius)
        continue;
      const double signal = normalizedMD->getSignalAt(i);
      if (std::isfinite(signal))
        binned += signal * voxelVolume;
      binnedCounts += dataMD->getSignalAt(i);
      binnedNorm += normMD->getSignalAt(i);
      binnedVolume += voxelVolume;
    }

    // the normalization per unit volume over the peak matches MDNorm's
    const double peakVolume = 4.0 / 3.0 * M_PI * radius * radius * radius;
    TS_ASSERT_DELTA((diagnostics->cell<double>(0, 3) / peakVolume) / (binnedNorm / binnedVolume), 1.0, 0.01);
    // and so count mode matches the ratio of the binned sums
    TS_ASSERT_DELTA(countMode / (binnedVolume * binnedCounts / binnedNorm), 1.0, 0.01);
    // Count mode divides the counts by the mean exposure over the whole
    // sphere rather than the exposure where the peak is, so it differs from
    // the sum of normalized voxels by the curvature of the exposure across
    // the sphere, here about 4% (exposure falls off roughly as 1/k^2 and k
    // ranges from about 6 to 10 across the sphere).
    TS_ASSERT_DELTA(countMode / binned, 1.0, 0.06);
  }

  // Events drawn along every pixel's trajectory from a known Gaussian peak
  // plus flat background: the normalized profile fit recovers the peak's
  // integrated intensity, as does normalized count mode up to the curvature
  // of the exposure over its ellipsoid.
  void test_normalized_profile_fit_recovers_true_intensity() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.1, height = 6000.0, background = 10.0, region = 0.4;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), sigma, background, region + 0.05,
                   1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    const auto solidAngleWS = createSolidAngleWorkspace(*eventWS, 1.0);
    const auto fluxWS = createFluxWorkspace(*eventWS, 1.0, 50.0);
    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigma * sigma * sigma;

    setSphericalShapes(*peaksWS, sigma, sigma, sigma);
    const auto [profile, diagnostics] = integrateProfile(eventWS, peaksWS, region, solidAngleWS, fluxWS);
    setSphericalShapes(*peaksWS, 3 * sigma, 3.2 * sigma, region);
    const auto counts = integrateCounts(eventWS, peaksWS, region, solidAngleWS, fluxWS).first;

    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i) {
      const auto &peak = profile->getPeak(i);
      TSM_ASSERT_DELTA("Profile intensity for peak " + std::to_string(i), peak.getIntensity() / trueIntensity, 1.0,
                       0.03);
      TS_ASSERT_LESS_THAN(0.0, peak.getSigmaIntensity());
      TS_ASSERT_LESS_THAN(peak.getSigmaIntensity(), 0.02 * trueIntensity);
      // a 3 sigma sphere holds 97% of a Gaussian, and count mode is also
      // biased by the curvature of the exposure across it, either way
      TSM_ASSERT_DELTA("Count intensity for peak " + std::to_string(i),
                       counts->getPeak(i).getIntensity() / trueIntensity, 1.0, 0.1);

      // expected counts in the fit region match the observed counts
      const double observed = diagnostics->cell<double>(i, 1);
      const double expected = peak.getIntensity() * diagnostics->cell<double>(i, 3) + diagnostics->cell<double>(i, 2);
      TS_ASSERT_DELTA(expected / observed, 1.0, 0.01);
      TS_ASSERT(!diagnostics->cell<Boolean>(i, 6));
      TS_ASSERT(!diagnostics->cell<Boolean>(i, 7));
    }
  }

  // Masking the pixels through the middle of a peak removes much more of its
  // counts than of its ellipsoid's exposure, so count mode is biased low,
  // while the profile fit, which knows the exposure along every remaining
  // trajectory, still recovers the true intensity.
  void test_normalized_profile_fit_with_masked_block_through_peak() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.1, height = 6000.0, background = 10.0, region = 0.4;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), sigma, background, region + 0.05,
                   1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    const auto solidAngleWS = createSolidAngleWorkspace(*eventWS, 1.0);
    const auto fluxWS = createFluxWorkspace(*eventWS, 1.0, 50.0);
    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigma * sigma * sigma;

    // mask every pixel whose line passes within sigma of peak 0's center
    auto &spectrumInfo = eventWS->mutableSpectrumInfo();
    size_t numMasked = 0;
    for (size_t i = 0; i < spectrumInfo.size(); ++i) {
      const double polar = spectrumInfo.twoTheta(i);
      const double azim = spectrumInfo.detector(i).getPhi();
      const V3D u(-std::sin(polar) * std::cos(azim), -std::sin(polar) * std::sin(azim), 1.0 - std::cos(polar));
      if (centers[0].cross_prod(u).norm() / u.norm() < sigma) {
        spectrumInfo.setMasked(i, true);
        ++numMasked;
      }
    }
    TS_ASSERT_LESS_THAN(size_t(0), numMasked);

    setSphericalShapes(*peaksWS, sigma, sigma, sigma);
    const auto [profile, diagnostics] = integrateProfile(eventWS, peaksWS, region, solidAngleWS, fluxWS);
    setSphericalShapes(*peaksWS, 3 * sigma, 3.2 * sigma, region);
    const auto counts = integrateCounts(eventWS, peaksWS, region, solidAngleWS, fluxWS).first;

    TS_ASSERT(diagnostics->cell<Boolean>(0, 7));
    TS_ASSERT_DELTA(profile->getPeak(0).getIntensity() / trueIntensity, 1.0, 0.04);
    TS_ASSERT_LESS_THAN(counts->getPeak(0).getIntensity() / trueIntensity, 0.9);
  }

  void test_normalized_profile_fit_of_empty_peak() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.1, height = 6000.0, background = 10.0, region = 0.4;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    // no peak at the second center, only background
    generateEvents(*eventWS, centers, {height, 0.0, height}, sigma, background, region + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, sigma, sigma, sigma);
    const auto profile = integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                                          createFluxWorkspace(*eventWS, 1.0, 50.0))
                             .first;
    const auto &empty = profile->getPeak(1);
    TS_ASSERT(std::isfinite(empty.getSigmaIntensity()));
    TS_ASSERT_LESS_THAN(0.0, empty.getSigmaIntensity());
    TS_ASSERT_LESS_THAN(empty.getIntensity(), 3 * empty.getSigmaIntensity());
  }

  // With normalization the expected peak counts depend on the center, so
  // AdjustCenter must include their gradient to find the true center.
  void test_normalized_profile_fit_adjusts_center() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.1, height = 6000.0, background = 10.0, region = 0.4;
    const V3D offset(0.04, -0.03, 0.02);
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame() + offset);
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), sigma, background, region + 0.1,
                   1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, sigma, sigma, sigma);
    const auto profile = integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                                          createFluxWorkspace(*eventWS, 1.0, 50.0), true)
                             .first;
    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigma * sigma * sigma;
    for (int i = 0; i < profile->getNumberPeaks(); ++i) {
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(i).getPeakShape());
      TS_ASSERT(shape);
      TSM_ASSERT_DELTA("Center correction for peak " + std::to_string(i), (shape->translation() - offset).norm(), 0.0,
                       0.1 * sigma);
      TS_ASSERT_DELTA(profile->getPeak(i).getIntensity() / trueIntensity, 1.0, 0.03);
    }
  }

  void test_adjust_shape_requires_profile_fit() {
    auto [eventWS, peaksWS] = buildThreePeaks();
    IntegratePeaksShapeMD alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("AdjustShape", true);
    TS_ASSERT_EQUALS(alg.validateInputs().count("AdjustShape"), 1);
    alg.setProperty("ProfileFit", true);
    TS_ASSERT_EQUALS(alg.validateInputs().count("AdjustShape"), 0);
  }

  // Starting from a spherical shape, AdjustShape recovers the true,
  // anisotropic and rotated, covariance of each peak, and its intensity.
  void test_normalized_profile_fit_adjusts_shape() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const auto directions = tiltedAxes();
    const PeakEllipsoidExtent sigmas{0.07, 0.1, 0.14};
    const double height = 6000.0, background = 10.0, region = 0.45;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), directions, sigmas, background,
                   region + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, 0.1, 0.1, 0.1);
    const auto [profile, diagnostics] =
        integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                         createFluxWorkspace(*eventWS, 1.0, 50.0), true, true);

    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigmas[0] * sigmas[1] * sigmas[2];
    const auto trueCovariance = covariance(directions, sigmas);
    for (int i = 0; i < profile->getNumberPeaks(); ++i) {
      TS_ASSERT(diagnostics->cell<Boolean>(i, 8));
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(i).getPeakShape());
      TS_ASSERT(shape);
      TSM_ASSERT_LESS_THAN("Covariance of peak " + std::to_string(i),
                           relativeDifference(covariance(shape->directions(), shape->abcRadii()), trueCovariance),
                           0.06);
      // within its uncertainty, which includes that of the refined shape
      const auto &peak = profile->getPeak(i);
      TSM_ASSERT_DELTA("Intensity of peak " + std::to_string(i), peak.getIntensity(), trueIntensity,
                       4 * peak.getSigmaIntensity() + 0.005 * trueIntensity);
      TS_ASSERT_LESS_THAN(shape->translation().norm(), 0.1 * sigmas[0]);
    }
  }

  // With the pixels through the middle of a peak masked, the refined shape
  // and intensity still follow the true ones.
  void test_normalized_profile_fit_adjusts_shape_with_masked_block() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const auto directions = tiltedAxes();
    const PeakEllipsoidExtent sigmas{0.07, 0.1, 0.14};
    const double height = 6000.0, background = 10.0, region = 0.45;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), directions, sigmas, background,
                   region + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    auto &spectrumInfo = eventWS->mutableSpectrumInfo();
    for (size_t i = 0; i < spectrumInfo.size(); ++i) {
      const double polar = spectrumInfo.twoTheta(i);
      const double azim = spectrumInfo.detector(i).getPhi();
      const V3D u(-std::sin(polar) * std::cos(azim), -std::sin(polar) * std::sin(azim), 1.0 - std::cos(polar));
      if (centers[0].cross_prod(u).norm() / u.norm() < 0.08)
        spectrumInfo.setMasked(i, true);
    }
    setSphericalShapes(*peaksWS, 0.1, 0.1, 0.1);
    const auto [profile, diagnostics] =
        integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                         createFluxWorkspace(*eventWS, 1.0, 50.0), true, true);

    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigmas[0] * sigmas[1] * sigmas[2];
    TS_ASSERT(diagnostics->cell<Boolean>(0, 7));
    TS_ASSERT(diagnostics->cell<Boolean>(0, 8));
    const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(0).getPeakShape());
    TS_ASSERT_LESS_THAN(
        relativeDifference(covariance(shape->directions(), shape->abcRadii()), covariance(directions, sigmas)), 0.12);
    TS_ASSERT_DELTA(profile->getPeak(0).getIntensity(), trueIntensity,
                    4 * profile->getPeak(0).getSigmaIntensity() + 0.005 * trueIntensity);
  }

  // A peak too weak to refine keeps its stored shape.
  void test_adjust_shape_leaves_weak_peaks() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.1, background = 10.0, region = 0.4;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, {6000.0, 0.0, 6000.0}, sigma, background, region + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, 0.08, 0.08, 0.08);
    const auto [profile, diagnostics] =
        integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                         createFluxWorkspace(*eventWS, 1.0, 50.0), false, true);
    TS_ASSERT(diagnostics->cell<Boolean>(0, 8));
    TS_ASSERT(!diagnostics->cell<Boolean>(1, 8));
    const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(1).getPeakShape());
    for (size_t k = 0; k < 3; ++k)
      TS_ASSERT_EQUALS(shape->abcRadii()[k], 0.08);
  }

  // A refined shape stays within MaxShapeChange of the stored one.
  void test_adjust_shape_respects_max_change() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.1, background = 10.0, region = 0.4;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), 6000.0), sigma, background, region + 0.05,
                   1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    // far too small a stored shape, so that the bound is reached
    setSphericalShapes(*peaksWS, 0.04, 0.04, 0.04);
    const auto profile = integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                                          createFluxWorkspace(*eventWS, 1.0, 50.0), false, true, 1.5)
                             .first;
    for (int i = 0; i < profile->getNumberPeaks(); ++i) {
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(i).getPeakShape());
      for (size_t k = 0; k < 3; ++k) {
        TS_ASSERT_LESS_THAN_EQUALS(shape->abcRadii()[k], 0.06 * (1 + 1e-9));
        TS_ASSERT_LESS_THAN(0.05, shape->abcRadii()[k]);
      }
    }
  }

  // With input intensities, e.g. squared structure factors, a peak expected
  // to be too weak keeps its stored shape and center however strong it looks,
  // with the intensity of its fit with them, while the others are refined as
  // without input intensities.
  void test_adjust_shape_uses_input_intensities_as_prior() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double sigma = 0.1, background = 10.0, region = 0.4;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), 6000.0), sigma, background, region + 0.05,
                   1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, 0.04, 0.04, 0.04);
    const auto sa = createSolidAngleWorkspace(*eventWS, 1.0);
    const auto fl = createFluxWorkspace(*eventWS, 1.0, 50.0);

    const auto stored = integrateProfile(eventWS, peaksWS, region, sa, fl).first;
    const auto unscaled = integrateProfile(eventWS, peaksWS, region, sa, fl, true, true).first;
    // peak 0 is expected to be ten thousand times weaker than it is
    const std::vector<double> expected{1e-4, 1.0, 1.0};
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      peaksWS->getPeak(i).setIntensity(expected[i]);
    const auto [profile, diagnostics] = integrateProfile(eventWS, peaksWS, region, sa, fl, true, true);

    TS_ASSERT(!diagnostics->cell<Boolean>(0, 8));
    const auto *kept = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(0).getPeakShape());
    for (size_t k = 0; k < 3; ++k)
      TS_ASSERT_EQUALS(kept->abcRadii()[k], 0.04);
    TS_ASSERT_EQUALS(kept->translation(), V3D());
    TS_ASSERT_DELTA(profile->getPeak(0).getIntensity(), stored->getPeak(0).getIntensity(),
                    1e-6 * stored->getPeak(0).getIntensity());
    TS_ASSERT_DELTA(profile->getPeak(0).getSigmaIntensity(), stored->getPeak(0).getSigmaIntensity(),
                    1e-6 * stored->getPeak(0).getSigmaIntensity());
    for (int i = 1; i < profile->getNumberPeaks(); ++i) {
      TS_ASSERT(diagnostics->cell<Boolean>(i, 8));
      TS_ASSERT_DELTA(profile->getPeak(i).getIntensity(), unscaled->getPeak(i).getIntensity(),
                      1e-6 * unscaled->getPeak(i).getIntensity());
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(i).getPeakShape());
      TS_ASSERT_LESS_THAN(0.07, shape->abcRadii()[0]);
    }
  }

  // A refinement that leaves the peak less significant than the threshold,
  // once the uncertainty of its shape is included, is not kept: here peak 1,
  // whose shape is poorly constrained, while peaks 0 and 2 are refined.
  void test_adjust_shape_keeps_stored_shape_if_refinement_loses_significance() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const auto directions = tiltedAxes();
    const PeakEllipsoidExtent sigmas{0.07, 0.1, 0.14};
    const double height = 6000.0, background = 10.0, region = 0.45;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), directions, sigmas, background,
                   region + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, 0.1, 0.1, 0.1);
    const auto [profile, diagnostics] =
        integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                         createFluxWorkspace(*eventWS, 1.0, 50.0), true, true, 2.0, 200.0);
    TS_ASSERT(diagnostics->cell<Boolean>(0, 8));
    TS_ASSERT(!diagnostics->cell<Boolean>(1, 8));
    TS_ASSERT(diagnostics->cell<Boolean>(2, 8));
    const auto *kept = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(1).getPeakShape());
    for (size_t k = 0; k < 3; ++k)
      TS_ASSERT_EQUALS(kept->abcRadii()[k], 0.1);
    TS_ASSERT_LESS_THAN(200.0, profile->getPeak(1).getIntensity() / profile->getPeak(1).getSigmaIntensity());
  }

  // A peak predicted several of its stored shape's standard deviations away
  // along the shape's narrow axis, e.g. from an imperfect calibration, is
  // found from the mode of its counts and refined to its true center and
  // shape, within MaxCenterShift of the stored center.
  void test_adjust_shape_and_center_reaches_peak_far_along_narrow_axis() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const auto directions = tiltedAxes();
    const PeakEllipsoidExtent sigmas{0.04, 0.1, 0.14};
    const double height = 6000.0, background = 10.0, region = 0.45;
    // 2.5 standard deviations along the narrow axis
    const V3D offset = directions[0] * (2.5 * sigmas[0]);
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame() + offset);
    // events throughout the fit region, which is about the stored center
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), directions, sigmas, background,
                   region + offset.norm() + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    // the stored shape is the true one
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      peaksWS->getPeak(i).setPeakShape(new PeakShapeEllipsoid(directions, sigmas, sigmas, sigmas, Kernel::QLab));
    const auto sa = createSolidAngleWorkspace(*eventWS, 1.0);
    const auto fl = createFluxWorkspace(*eventWS, 1.0, 50.0);
    const auto [profile, diagnostics] = integrateProfile(eventWS, peaksWS, region, sa, fl, true, true);

    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigmas[0] * sigmas[1] * sigmas[2];
    const auto trueCovariance = covariance(directions, sigmas);
    for (int i = 0; i < profile->getNumberPeaks(); ++i) {
      TS_ASSERT(diagnostics->cell<Boolean>(i, 8));
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(i).getPeakShape());
      TS_ASSERT(shape);
      // in standard deviations along each principal axis
      double error = 0.0;
      for (size_t k = 0; k < 3; ++k)
        error += std::pow((shape->translation() - offset).scalar_prod(directions[k]) / sigmas[k], 2);
      TSM_ASSERT_LESS_THAN("Center of peak " + std::to_string(i), std::sqrt(error), 0.1);
      TSM_ASSERT_LESS_THAN("Covariance of peak " + std::to_string(i),
                           relativeDifference(covariance(shape->directions(), shape->abcRadii()), trueCovariance),
                           0.06);
      const auto &peak = profile->getPeak(i);
      TSM_ASSERT_DELTA("Intensity of peak " + std::to_string(i), peak.getIntensity(), trueIntensity,
                       4 * peak.getSigmaIntensity() + 0.005 * trueIntensity);
    }

    // with a tighter bound, the center stops at it
    const auto bounded = integrateProfile(eventWS, peaksWS, region, sa, fl, true, true, 2.0, 10.0, 1.0).first;
    for (int i = 0; i < bounded->getNumberPeaks(); ++i) {
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&bounded->getPeak(i).getPeakShape());
      double distance = 0.0;
      for (size_t k = 0; k < 3; ++k)
        distance += std::pow(shape->translation().scalar_prod(directions[k]) / sigmas[k], 2);
      TS_ASSERT_LESS_THAN_EQUALS(std::sqrt(distance), 1.0 + 1e-6);
    }
  }

  // A peak predicted far enough away to look too weak to refine at its
  // stored center is found from the mode of its counts, and refined.
  void test_adjust_shape_and_center_finds_peak_that_looks_weak_where_predicted() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const auto directions = tiltedAxes();
    const PeakEllipsoidExtent sigmas{0.04, 0.1, 0.14};
    const double height = 200.0, background = 10.0, region = 0.45;
    // 3 standard deviations along the narrow axis
    const V3D offset = directions[0] * (3.0 * sigmas[0]);
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame() + offset);
    // events throughout the fit region, which is about the stored center
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), height), directions, sigmas, background,
                   region + offset.norm() + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      peaksWS->getPeak(i).setPeakShape(new PeakShapeEllipsoid(directions, sigmas, sigmas, sigmas, Kernel::QLab));
    const auto sa = createSolidAngleWorkspace(*eventWS, 1.0);
    const auto fl = createFluxWorkspace(*eventWS, 1.0, 50.0);

    // with the stored shape and center, the peaks are below the threshold for refinement
    const auto stored = integrateProfile(eventWS, peaksWS, region, sa, fl).first;
    const auto [profile, diagnostics] = integrateProfile(eventWS, peaksWS, region, sa, fl, true, true, 2.0, 10.0, 4.0);

    const double trueIntensity = height * std::pow(2.0 * M_PI, 1.5) * sigmas[0] * sigmas[1] * sigmas[2];
    // peak 1 is only partly covered by the detectors, and too weak even at its true center
    for (int i : {0, 2}) {
      TS_ASSERT_LESS_THAN(stored->getPeak(i).getIntensity() / stored->getPeak(i).getSigmaIntensity(), 10.0);
      TS_ASSERT(diagnostics->cell<Boolean>(i, 8));
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(i).getPeakShape());
      double error = 0.0;
      for (size_t k = 0; k < 3; ++k)
        error += std::pow((shape->translation() - offset).scalar_prod(directions[k]) / sigmas[k], 2);
      TSM_ASSERT_LESS_THAN("Center of peak " + std::to_string(i), std::sqrt(error), 0.2);
      const auto &peak = profile->getPeak(i);
      TSM_ASSERT_DELTA("Intensity of peak " + std::to_string(i), peak.getIntensity(), trueIntensity,
                       4 * peak.getSigmaIntensity());
    }
  }

  // With AdjustShape, peaks too weak to refine keep their stored center as
  // well as their stored shape, rather than having it refined on its own.
  void test_adjust_shape_and_center_leaves_weak_peaks_centered() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const double region = 0.45;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, {0.0, 6000.0, 0.0}, 0.1, 200.0, region + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, 0.1, 0.1, 0.1);
    const auto [profile, diagnostics] =
        integrateProfile(eventWS, peaksWS, region, createSolidAngleWorkspace(*eventWS, 1.0),
                         createFluxWorkspace(*eventWS, 1.0, 50.0), true, true);
    for (int i : {0, 2}) {
      TS_ASSERT(!diagnostics->cell<Boolean>(i, 8));
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&profile->getPeak(i).getPeakShape());
      TS_ASSERT_EQUALS(shape->translation(), V3D(0, 0, 0));
    }
    TS_ASSERT(diagnostics->cell<Boolean>(1, 8));
  }

  // Compressing the events into weighted events, with weights that are
  // counts, gives the same refined shape and intensity.
  void test_adjust_shape_with_compressed_weighted_events() {
    auto [eventWS, peaksWS] = buildEmptyPeaks();
    const auto directions = tiltedAxes();
    const PeakEllipsoidExtent sigmas{0.07, 0.1, 0.14};
    const double region = 0.45;
    std::vector<V3D> centers;
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i)
      centers.emplace_back(peaksWS->getPeak(i).getQLabFrame());
    generateEvents(*eventWS, centers, std::vector<double>(centers.size(), 6000.0), directions, sigmas, 10.0,
                   region + 0.05, 1.0);
    addMDNormLogs(*eventWS, 1.0, 40.0, 1.0);
    setSphericalShapes(*peaksWS, 0.1, 0.1, 0.1);
    const auto sa = createSolidAngleWorkspace(*eventWS, 1.0);
    const auto fl = createFluxWorkspace(*eventWS, 1.0, 50.0);
    const auto raw = integrateProfile(eventWS, peaksWS, region, sa, fl, true, true).first;

    // merge events within 2 microseconds, about 0.1% in momentum
    MatrixWorkspace_sptr compressedWS = eventWS->clone();
    auto compressed = std::dynamic_pointer_cast<EventWorkspace>(compressedWS);
    size_t numRaw = 0, numCompressed = 0;
    for (size_t i = 0; i < compressed->getNumberHistograms(); ++i) {
      auto &events = compressed->getSpectrum(i);
      numRaw += events.getNumberEvents();
      events.compressEvents(2.0, &events);
      numCompressed += events.getNumberEvents();
    }
    TS_ASSERT_LESS_THAN(numCompressed, numRaw);
    const auto weighted = integrateProfile(compressedWS, peaksWS, region, sa, fl, true, true).first;

    for (int i = 0; i < raw->getNumberPeaks(); ++i) {
      const auto &a = raw->getPeak(i);
      const auto &b = weighted->getPeak(i);
      TS_ASSERT_DELTA(b.getIntensity(), a.getIntensity(), 0.5 * a.getSigmaIntensity());
      TS_ASSERT_DELTA(b.getSigmaIntensity(), a.getSigmaIntensity(), 0.05 * a.getSigmaIntensity());
      const auto *sa_ = dynamic_cast<const PeakShapeEllipsoid *>(&a.getPeakShape());
      const auto *sb = dynamic_cast<const PeakShapeEllipsoid *>(&b.getPeakShape());
      TS_ASSERT_LESS_THAN(relativeDifference(covariance(sb->directions(), sb->abcRadii()),
                                             covariance(sa_->directions(), sa_->abcRadii())),
                          0.02);
    }
  }

private:
  std::pair<MatrixWorkspace_sptr, PeaksWorkspace_sptr> buildThreePeaks() {
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), 100, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), 100, sigmas);
    auto data = builder.build();
    setSphericalShapes(*std::get<1>(data), 0.2, 0.25, 0.3);
    return {std::get<0>(data), std::get<1>(data)};
  }

  std::pair<MatrixWorkspace_sptr, PeaksWorkspace_sptr> buildSixPeaks(int numEventsPerPeak) {
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerPeak, sigmas);
    auto data = builder.build();
    return {std::get<0>(data), std::get<1>(data)};
  }
};
