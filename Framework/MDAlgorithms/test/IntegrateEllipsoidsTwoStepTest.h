#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidKernel/NearestNeighbours.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/SingleCrystalDiffractionTestHelper.h"

#include <cxxtest/TestSuite.h>
#include <tuple>
#include <random>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using Mantid::Kernel::V3D;
using Mantid::Geometry::OrientedLattice;

class IntegrateEllipsoidsTwoStepTest : public CxxTest::TestSuite {

public:
  void test_init() {
    Mantid::MDAlgorithms::IntegrateEllipsoidsTwoStep alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_exec_events_with_no_background() {
    const int numEventsPerPeak = 10000;
    // Very tight distribution with events happening at a single point
    const auto sigmas = std::make_tuple(.002, .002, 0.1);

    // Build some diffraction data
    SingleCrystalDiffractionTestHelper::WorkspaceBuilder builder;
    builder.setNumPixels(100);
    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerPeak, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);

    // Run algorithm
    Mantid::MDAlgorithms::IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.4));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WeakPeakThreshold", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", true));
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

    for (int i = 0; i < 5; ++i) {
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i),
                       integratedPeaksWS->getPeak(i).getIntensity(), numEventsPerPeak, 0.01);

    }
  }

  void test_exec_histogram_with_no_background() {
    const int numEventsPerPeak = 10000;
    const auto sigmas = std::make_tuple(.002, .002, 0.01);
    const std::vector<double> rebinParams = { 800, 5, 10000 };

    SingleCrystalDiffractionTestHelper::WorkspaceBuilder builder;
    builder.setNumPixels(100);
    builder.setOutputAsHistogram(true);
    builder.setRebinParameters(rebinParams);

    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, 0), numEventsPerPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerPeak, sigmas);

    auto data = builder.build();
    auto histoWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);

    IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", histoWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", .5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", .5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", .6));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WeakPeakThreshold", 0.1));
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      peaksWS->getNumberPeaks());

    for (int i = 0; i < 5; ++i) {
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i),
                       integratedPeaksWS->getPeak(i).getIntensity(), numEventsPerPeak, 0.01);
    }
  }
};

