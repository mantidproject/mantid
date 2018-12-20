#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/NearestNeighbours.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/SingleCrystalDiffractionTestHelper.h"

#include <cxxtest/TestSuite.h>
#include <random>
#include <tuple>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::V3D;
using namespace Mantid::SingleCrystalDiffractionTestHelper;

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

    // Run algorithm
    IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.4));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WeakPeakThreshold", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "dummy"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check output
    TS_ASSERT(alg.isExecuted());
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(integratedPeaksWS);

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      peaksWS->getNumberPeaks());
    const auto &run = integratedPeaksWS->mutableRun();
    TSM_ASSERT("Output workspace must be integrated",
               run.hasProperty("PeaksIntegrated"));
    TSM_ASSERT_EQUALS("Output workspace must be integrated",
                      run.getProperty("PeaksIntegrated")->value(), "1");

    for (int i = 0; i < 5; ++i) {
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i),
                       integratedPeaksWS->getPeak(i).getIntensity(),
                       numEventsPerPeak, 5);
    }
  }

  void test_exec_histogram_with_no_background() {
    const int numEventsPerPeak = 10000;
    const auto sigmas = std::make_tuple(.002, .002, 0.01);
    const std::vector<double> rebinParams = {800, 5, 10000};

    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.outputAsHistogram(true);
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
                       integratedPeaksWS->getPeak(i).getIntensity(),
                       numEventsPerPeak, 5);
    }
  }

  void test_exec_histogram_distribution_with_no_background() {
    using namespace Mantid::API;
    const int numEventsPerPeak = 10000;
    const auto sigmas = std::make_tuple(.002, .002, 0.01);
    const std::vector<double> rebinParams = {800, 5, 10000};

    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(false);
    builder.outputAsHistogram(true);
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

    const auto &algManager = AlgorithmManager::Instance();
    auto cloneWorkspace = algManager.createUnmanaged("CloneWorkspace");
    cloneWorkspace->setChild(true);
    cloneWorkspace->initialize();
    cloneWorkspace->setProperty("InputWorkspace", histoWS);
    cloneWorkspace->setPropertyValue("OutputWorkspace", "dist_workspace");
    cloneWorkspace->execute();
    Workspace_sptr temp = cloneWorkspace->getProperty("OutputWorkspace");
    auto distWS = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

    auto convertToDist = algManager.createUnmanaged("ConvertToDistribution");
    convertToDist->setChild(true);
    convertToDist->initialize();
    convertToDist->setProperty("Workspace", distWS);
    convertToDist->execute();
    distWS = convertToDist->getProperty("Workspace");

    IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", distWS);
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

    const double binWidth{0.2};
    for (int i = 0; i < 5; ++i) {
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i),
                       integratedPeaksWS->getPeak(i).getIntensity(),
                       numEventsPerPeak * binWidth, 5);
    }
  }

  void test_exec_events_with_background() {
    const int numEventsPerPeak = 10000;

    // Very tight distribution with events happening at a single point
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    const auto backgroundDetSize = 0.05;
    const auto backgroundTOFSize = 100.0;
    const auto nBackgroundEvents = 1000;

    // Build some diffraction data
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(true);
    builder.setBackgroundParameters(nBackgroundEvents, backgroundDetSize,
                                    backgroundTOFSize);

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
    IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.4));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WeakPeakThreshold", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "dummy"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check output
    TS_ASSERT(alg.isExecuted());
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(integratedPeaksWS);

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      peaksWS->getNumberPeaks());
    const auto &run = integratedPeaksWS->mutableRun();
    TSM_ASSERT("Output workspace must be integrated",
               run.hasProperty("PeaksIntegrated"));
    TSM_ASSERT_EQUALS("Output workspace must be integrated",
                      run.getProperty("PeaksIntegrated")->value(), "1");

    for (int i = 0; i < 5; ++i) {
      TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(i),
                       integratedPeaksWS->getPeak(i).getIntensity(),
                       numEventsPerPeak, 450);
    }
  }

  void test_exec_histogram_with_background() {
    const int numEventsPerPeak = 10000;
    const auto sigmas = std::make_tuple(.002, .002, 0.01);
    const std::vector<double> rebinParams = {800, 5, 10000};
    const auto backgroundDetSize = 0.05;
    const auto backgroundTOFSize = 100.0;
    const auto nBackgroundEvents = 1000;

    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(true);
    builder.setBackgroundParameters(nBackgroundEvents, backgroundDetSize,
                                    backgroundTOFSize);
    builder.outputAsHistogram(true);
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
                       integratedPeaksWS->getPeak(i).getIntensity(),
                       numEventsPerPeak, 700);
    }
  }

  void test_exec_events_with_weak_peaks() {
    const int numEventsPerStrongPeak = 10000;
    const int numEventsPerWeakPeak = 100;

    // Very tight distribution with events happening at a single point
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    const auto backgroundDetSize = 0.05;
    const auto backgroundTOFSize = 100.0;
    const auto nBackgroundEvents = 1000;

    // Build some diffraction data
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(true);
    builder.setBackgroundParameters(nBackgroundEvents, backgroundDetSize,
                                    backgroundTOFSize);

    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerStrongPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerStrongPeak, sigmas);

    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerWeakPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerWeakPeak, sigmas);

    builder.addPeakByHKL(V3D(1, -4, 0), numEventsPerStrongPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerStrongPeak, sigmas);

    // weak peak with zero intensity
    builder.addPeakByHKL(V3D(2, -5, -5), 0, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);

    // Run algorithm
    IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.4));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WeakPeakThreshold", 5.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "dummy"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check output
    TS_ASSERT(alg.isExecuted());
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(integratedPeaksWS);

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      peaksWS->getNumberPeaks());
    const auto &run = integratedPeaksWS->mutableRun();
    TSM_ASSERT("Output workspace must be integrated",
               run.hasProperty("PeaksIntegrated"));
    TSM_ASSERT_EQUALS("Output workspace must be integrated",
                      run.getProperty("PeaksIntegrated")->value(), "1");

    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(0),
                     integratedPeaksWS->getPeak(0).getIntensity(),
                     numEventsPerStrongPeak, 300);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(1),
                     integratedPeaksWS->getPeak(1).getIntensity(),
                     numEventsPerStrongPeak, 300);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(2),
                     integratedPeaksWS->getPeak(2).getIntensity(),
                     numEventsPerWeakPeak, 100);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(3),
                     integratedPeaksWS->getPeak(3).getIntensity(),
                     numEventsPerWeakPeak, 100);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(4),
                     integratedPeaksWS->getPeak(4).getIntensity(),
                     numEventsPerStrongPeak, 450);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(5),
                     integratedPeaksWS->getPeak(5).getIntensity(),
                     numEventsPerStrongPeak, 800);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(6),
                     integratedPeaksWS->getPeak(6).getIntensity(), 100, 10);
  }

  void test_exec_events_with_adaptive_q() {
    const int numEventsPerStrongPeak = 10000;
    const int numEventsPerWeakPeak = 100;

    // Very tight distribution with events happening at a single point
    const auto sigmas = std::make_tuple(.002, .002, 0.1);
    const auto backgroundDetSize = 0.05;
    const auto backgroundTOFSize = 100.0;
    const auto nBackgroundEvents = 1000;

    // Build some diffraction data
    WorkspaceBuilder builder;
    builder.setRandomSeed(1);
    builder.setNumPixels(100);
    builder.addBackground(true);
    builder.setBackgroundParameters(nBackgroundEvents, backgroundDetSize,
                                    backgroundTOFSize);

    builder.addPeakByHKL(V3D(1, -5, -3), numEventsPerStrongPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -4), numEventsPerStrongPeak, sigmas);
    builder.addPeakByHKL(V3D(2, -3, -4), numEventsPerStrongPeak, sigmas);

    builder.addPeakByHKL(V3D(1, -3, -5), numEventsPerWeakPeak, sigmas);
    builder.addPeakByHKL(V3D(1, -4, -2), numEventsPerWeakPeak, sigmas);

    auto data = builder.build();
    auto eventWS = std::get<0>(data);
    auto peaksWS = std::get<1>(data);

    // Run algorithm
    IntegrateEllipsoidsTwoStep alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", eventWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SpecifySize", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerSize", 0.35));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterSize", 0.4));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WeakPeakThreshold", 100.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQBackground", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQMultiplier", 0.01));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "dummy"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check output
    TS_ASSERT(alg.isExecuted());
    PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(integratedPeaksWS);

    TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace",
                      integratedPeaksWS->getNumberPeaks(),
                      peaksWS->getNumberPeaks());
    const auto &run = integratedPeaksWS->mutableRun();
    TSM_ASSERT("Output workspace must be integrated",
               run.hasProperty("PeaksIntegrated"));
    TSM_ASSERT_EQUALS("Output workspace must be integrated",
                      run.getProperty("PeaksIntegrated")->value(), "1");

    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(0),
                     integratedPeaksWS->getPeak(0).getIntensity(),
                     numEventsPerStrongPeak, 5100);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(1),
                     integratedPeaksWS->getPeak(1).getIntensity(),
                     numEventsPerStrongPeak, 5100);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(2),
                     integratedPeaksWS->getPeak(2).getIntensity(),
                     numEventsPerStrongPeak, 900);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(3),
                     integratedPeaksWS->getPeak(3).getIntensity(),
                     numEventsPerWeakPeak, 300);
    TSM_ASSERT_DELTA("Wrong intensity for peak " + std::to_string(4),
                     integratedPeaksWS->getPeak(4).getIntensity(),
                     numEventsPerWeakPeak, 300);
  }
};
