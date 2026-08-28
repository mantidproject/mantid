// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/IntegratePeaksShapeMD.h"

#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/SingleCrystalDiffractionTestHelper.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"

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

    for (int i = 0; i < 5; ++i) {
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i), integratedPeaksWS->getPeak(i).getIntensity(),
                       numEventsPerPeak, 5);
    }
  }
};
