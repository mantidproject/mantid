// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/IntegratePeaksShapeMD.h"

#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/SingleCrystalDiffractionTestHelper.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"

#include <cmath>
#include <cxxtest/TestSuite.h>
#include <tuple>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using Mantid::Kernel::V3D;
using namespace Mantid::SingleCrystalDiffractionTestHelper;

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

  void test_exec_throws_with_fewer_than_three_indexed_peaks() {
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
    peaksWS->getPeak(2).setHKL(0, 0, 0);

    IntegratePeaksShapeMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");

    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &error, std::string(error.what()),
                            "At least three linearly independent indexed peaks are needed.");
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_does_not_modify_input_events() {
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    // Six well-separated peaks (matching test_exec_events_reuses_existing_shape),
    // rather than the minimum of three, so Optimize_UB has a well-conditioned fit.
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
};
