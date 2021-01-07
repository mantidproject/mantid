// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include "MantidKernel/UnitLabelTypes.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidMDAlgorithms/IntegratePeaksMD2.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/pow.hpp>

#include <cxxtest/TestSuite.h>
#include <random>

#include <MantidDataObjects/PeakShapeEllipsoid.h>
#include <Poco/File.h>

using Mantid::API::AnalysisDataService;
using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;
using Mantid::Kernel::V3D;

class IntegratePeaksMD2Test : public CxxTest::TestSuite {
public:
  IntegratePeaksMD2Test() { Mantid::API::FrameworkManager::Instance(); }
  ~IntegratePeaksMD2Test() override {}

  void test_Init() {
    IntegratePeaksMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------
  /** Run the IntegratePeaksMD2 with the given peak radius integration param */
  static void doRun(std::vector<double> PeakRadius, double BackgroundRadius,
                    std::string OutputWorkspace = "IntegratePeaksMD2Test_peaks",
                    double BackgroundStartRadius = 0.0, bool edge = true,
                    bool cyl = false, std::string fnct = "NoFit",
                    double adaptive = 0.0, bool ellip = false,
                    bool fixQAxis = false) {
    IntegratePeaksMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakRadius", PeakRadius));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BackgroundOuterRadius", BackgroundRadius));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BackgroundInnerRadius", BackgroundStartRadius));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", edge));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PeaksWorkspace", "IntegratePeaksMD2Test_peaks"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", OutputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Ellipsoid", ellip));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FixQAxis", fixQAxis));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Cylinder", cyl));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CylinderLength", 4.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PercentBackground", 20.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ProfileFunction", fnct));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrationOption", "Sum"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQMultiplier", adaptive));
    if (adaptive > 0.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQBackground", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  static void createMDEW() {
    // ---- Start with empty MDEW ----

    CreateMDWorkspace algC;
    TS_ASSERT_THROWS_NOTHING(algC.initialize())
    TS_ASSERT(algC.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Dimensions", "3"));
    TS_ASSERT_THROWS_NOTHING(
        algC.setProperty("Extents", "-10,10,-10,10,-10,10"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Names", "h,k,l"));
    std::string units = Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii();
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Units", units));
    std::string frames = Mantid::Geometry::HKL::HKLName + "," +
                         Mantid::Geometry::HKL::HKLName + "," +
                         Mantid::Geometry::HKL::HKLName;
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Frames", frames));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("SplitInto", "5"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("MaxRecursionDepth", "2"));
    TS_ASSERT_THROWS_NOTHING(algC.setPropertyValue(
        "OutputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algC.execute());
    TS_ASSERT(algC.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake peak */
  static void addPeak(size_t num, double x, double y, double z, double radius) {
    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    FakeMDEventData algF;
    TS_ASSERT_THROWS_NOTHING(algF.initialize())
    TS_ASSERT(algF.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algF.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(
        algF.setProperty("PeakParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake ellipsoid peak */
  static void addEllipsoid(size_t num, double x, double y, double z,
                           std::vector<std::vector<double>> eigvects,
                           std::vector<double> eigvals, double doCounts) {

    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", ";
    // add in eigenvects
    for (size_t ivect = 0; ivect < eigvects.size(); ivect++) {
      std::copy(eigvects[ivect].begin(), eigvects[ivect].end(),
                std::ostream_iterator<double>(mess, ", "));
    }
    // add in eigenvalues
    std::copy(eigvals.begin(), eigvals.end(),
              std::ostream_iterator<double>(mess, ", "));
    // doCounts
    mess << doCounts;

    std::ostringstream seed;
    seed << 77;

    FakeMDEventData algF;
    TS_ASSERT_THROWS_NOTHING(algF.initialize())
    TS_ASSERT(algF.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algF.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(
        algF.setProperty("EllipsoidParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(
        algF.setProperty("RandomSeed", seed.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake uniform  background*/
  static void addUniform(size_t num,
                         std::vector<std::pair<double, double>> range) {
    // each element of range is a pair min max

    std::ostringstream mess;
    mess << num;
    // add in eigenvects
    for (size_t d = 0; d < range.size(); d++) {
      mess << ", " << range[d].first << ", " << range[d].second;
    }

    FakeMDEventData algF;
    TS_ASSERT_THROWS_NOTHING(algF.initialize())
    TS_ASSERT(algF.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algF.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(
        algF.setProperty("UniformParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Full test using faked-out peak data */
  void test_exec() {
    // --- Fake workspace with 3 peaks ------
    createMDEW();
    addPeak(1000, 0., 0., 0., 1.0);
    addPeak(1000, 2., 3., 4., 0.5);
    addPeak(1000, 6., 6., 6., 2.0);

    MDEventWorkspace3Lean::sptr mdews =
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "IntegratePeaksMD2Test_MDEWS");
    mdews->setCoordinateSystem(Mantid::Kernel::HKL);
    TS_ASSERT_EQUALS(mdews->getNPoints(), 3000);
    TS_ASSERT_DELTA(mdews->getBox()->getSignal(), 3000.0, 1e-2);

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS0(new PeaksWorkspace());
    peakWS0->setInstrument(inst);
    peakWS0->addPeak(Peak(inst, 15050, 1.0));

    TS_ASSERT_EQUALS(peakWS0->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().add("IntegratePeaksMD2Test_peaks", peakWS0);

    // ------------- Integrating with cylinder ------------------------
    doRun({0.1}, 0.0, "IntegratePeaksMD2Test_peaks", 0.0, true, true);

    TS_ASSERT_DELTA(peakWS0->getPeak(0).getIntensity(), 2.0, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2, 1e-2);

    // Test profile Gaussian
    std::string fnct = "Gaussian";
    doRun({0.1}, 0.0, "IntegratePeaksMD2Test_peaks", 0.0, true, true, fnct);
    // More accurate integration changed values
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getIntensity(), 2.0, 1e-2);
    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2, 1e-2);
    Poco::File(Mantid::Kernel::ConfigService::Instance().getString(
                   "defaultsave.directory") +
               "IntegratePeaksMD2Test_MDEWSGaussian.dat")
        .remove();

    // Test profile back to back exponential
    fnct = "BackToBackExponential";
    doRun({0.1}, 0.0, "IntegratePeaksMD2Test_peaks", 0.0, true, true, fnct);

    // TS_ASSERT_DELTA( peakWS0->getPeak(0).getIntensity(), 2.0, 0.2);
    // Error is also calculated
    // TS_ASSERT_DELTA( peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2,
    // 0.2);
    Poco::File(Mantid::Kernel::ConfigService::Instance().getString(
                   "defaultsave.directory") +
               "IntegratePeaksMD2Test_MDEWSBackToBackExponential.dat")
        .remove();
    /*fnct = "ConvolutionExpGaussian";
    doRun(0.1,0.0,"IntegratePeaksMD2Test_peaks",0.0,true,true,fnct);

    TS_ASSERT_DELTA( peakWS0->getPeak(0).getIntensity(), 2.0, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA( peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2,
    1e-2);*/

    // ------------- Adaptive Integration r=MQ+b where b is PeakRadius and m is
    // 0.01 ------------------------
    peakWS0->addPeak(Peak(inst, 15050, 1.0, V3D(2., 3., 4.)));
    doRun({0.1}, 0.0, "IntegratePeaksMD2Test_peaks", 0.0, true, false, "NoFit",
          0.01);
    TS_ASSERT_DELTA(peakWS0->getPeak(1).getIntensity(), 29.0, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(1).getSigmaIntensity(), sqrt(29.0), 1e-2);

    // ------------- Integrate with 0.1 radius but IntegrateIfOnEdge
    // false------------------------
    doRun({0.1}, 0.0, "IntegratePeaksMD2Test_peaks", 0.0, false);

    TS_ASSERT_DELTA(peakWS0->getPeak(0).getIntensity(), 2.0, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2, 1e-2);

    AnalysisDataService::Instance().remove("IntegratePeaksMD2Test_peaks");

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->addPeak(Peak(inst, 15050, 1.0, V3D(0., 0., 0.)));
    peakWS->addPeak(Peak(inst, 15050, 1.0, V3D(2., 3., 4.)));
    peakWS->addPeak(Peak(inst, 15050, 1.0, V3D(6., 6., 6.)));

    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().add("IntegratePeaksMD2Test_peaks", peakWS);

    // ------------- Integrate with 1.0 radius ------------------------
    doRun({1.0}, 0.0);

    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    // Peak is of radius 2.0, but we get half that radius = ~1/8th the volume
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 125.0, 10.0);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS->getPeak(0).getSigmaIntensity(), sqrt(1000.0), 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getSigmaIntensity(), sqrt(1000.0), 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(2).getSigmaIntensity(),
                    sqrt(peakWS->getPeak(2).getIntensity()), 1e-2);

    // ------------- Let's do it again with 2.0 radius ------------------------
    doRun({2.0}, 0.0);

    // All peaks are fully contained
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 1000.0, 1e-2);

    // ------------- Let's do it again with 0.5 radius ------------------------
    doRun({0.5}, 0.0);

    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 125.0, 10.0);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 15.0, 10.0);

    // ===============================================================================
    // ---- Now add a background signal over one peak--------------
    addPeak(1000, 0., 0., 0., 2.0);

    // ------------- Integrate with 1.0 radius and 2.0
    // background------------------------
    doRun({1.0}, 2.0);
    // Same 1000 since the background (~125) was subtracted, with some random
    // variation of the BG around
    //    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1000.0, 10.0);
    // Error on peak is the SUM of the error of peak and the subtracted
    // background
    TS_ASSERT_DELTA(peakWS->getPeak(0).getSigmaIntensity(),
                    sqrt(1125.0 + 125.0), 2.0);

    // Had no bg, so they are the same
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getSigmaIntensity(), sqrt(1000.0), 1e-1);

    // This one is a 2.0 radius fake peak, so the background and peak have ~ the
    // same density! So ~0 total intensity.
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 0.0, 12.0);
    // But the error is large since it is 125 - 125 (with errors)
    TS_ASSERT_DELTA(peakWS->getPeak(2).getSigmaIntensity(), sqrt(125.0 + 25.0),
                    2.);

    // ------------- Integrating without the background gives higher counts
    // ------------------------
    doRun({1.0}, 0.0);

    // +125 counts due to background
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1125.0, 10.0);

    // These had no bg, so they are the same
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 125.0, 10.0);

    AnalysisDataService::Instance().remove("IntegratePeaksMD2Test_MDEWS");
    AnalysisDataService::Instance().remove("IntegratePeaksMD2Test_peaks");
  }

  //-------------------------------------------------------------------------------
  void test_exec_NotInPlace() {
    // --- Fake workspace with 3 peaks ------
    createMDEW();
    addPeak(1000, 0., 0., 0., 1.0);

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->addPeak(Peak(inst, 1, 1.0, V3D(0., 0., 0.)));
    AnalysisDataService::Instance().add("IntegratePeaksMD2Test_peaks", peakWS);

    // Integrate and copy to a new peaks workspace
    doRun({1.0}, 0.0, "IntegratePeaksMD2Test_peaks_out");

    // Old workspace is unchanged
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);

    PeaksWorkspace_sptr newPW = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(
            "IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(newPW);

    TS_ASSERT_DELTA(newPW->getPeak(0).getIntensity(), 1000.0, 1e-2);
  }

  //-------------------------------------------------------------------------------
  /// Integrate background between start/end background radius
  void test_exec_shellBackground() {
    createMDEW();
    /* Create 3 overlapping shells so that density goes like this:
     * r < 1 : density 1.0
     * 1 < r < 2 : density 1/2
     * 2 < r < 3 : density 1/3
     */
    addPeak(1000, 0., 0., 0., 1.0);
    addPeak(1000 * 4, 0., 0., 0.,
            2.0); // 8 times the volume / 4 times the counts = 1/2 density
    addPeak(1000 * 9, 0., 0., 0.,
            3.0); // 27 times the volume / 9 times the counts = 1/3 density

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    peakWS->addPeak(Peak(inst, 1, 1.0, V3D(0., 0., 0.)));
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks",
                                                 peakWS);

    // First, a check with no background
    doRun({1.0}, 0.0, "IntegratePeaksMD2Test_peaks", 0.0);
    // approx. + 500 + 333 counts due to 2 backgrounds
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000 + 500 + 333, 30.0);
    TSM_ASSERT_DELTA("Simple sqrt() error",
                     peakWS->getPeak(0).getSigmaIntensity(), sqrt(1833.0), 2);

    // Set background from 2.0 to 3.0.
    // So the 1/2 density background remains, we subtract the 1/3 density =
    // about 1500 counts
    doRun({1.0}, 3.0, "IntegratePeaksMD2Test_peaks", 2.0);
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000 + 500, 80.0);
    // Error is larger, since it is error of peak + error of background
    TSM_ASSERT_DELTA("Error has increased",
                     peakWS->getPeak(0).getSigmaIntensity(), sqrt(1830.0), 2);

    // Now do the same without the background start radius
    // So we subtract both densities = a lower count
    doRun({1.0}, 3.0);
    TSM_ASSERT_LESS_THAN("Peak intensity is lower if you do not include the "
                         "spacer shell (higher background)",
                         peakWS->getPeak(0).getIntensity(), 1500);
  }

  //-------------------------------------------------------------------------------
  //// Tests of ellipsoidal integration

  void test_exec_EllipsoidNoBackground_SingleCount() {
    EllipsoidTestHelper(-1.0, false, false);
  }
  void test_exec_EllipsoidNoBackground_SingleCount_FixQAxis() {
    EllipsoidTestHelper(-1.0, true, false);
  }
  void test_exec_EllipsoidNoBackground_NonSingleCount() {
    EllipsoidTestHelper(1.0, false, false);
  }
  void test_exec_EllipsoidNoBackground_NonSingleCount_FixQAxis() {
    EllipsoidTestHelper(1.0, true, false);
  }
  void test_exec_EllipsoidWithBackground_SingleCount() {
    EllipsoidTestHelper(-1.0, false, true);
  }

  void EllipsoidTestHelper(double doCounts, bool fixQAxis, bool doBkgrd) {
    // doCounts < 0 -> all events have a count of 1
    // doCounts > 0 -> counts follow multivariate normal dist

    createMDEW();

    // peak Q
    V3D Q(1.0, 0.0, 0.0);
    // set eigenvectors and eigenvalues
    std::vector<std::vector<double>> eigenvects;
    std::vector<double> eigenvals;
    eigenvects.push_back(std::vector<double>{Q[0], Q[1], Q[2]}); // para Q
    eigenvals.push_back(0.05);
    // other orthogonal axes
    eigenvects.push_back(std::vector<double>{0.0, 1.0, 0.0});
    eigenvals.push_back(0.03);
    eigenvects.push_back(std::vector<double>{0.0, 0.0, 1.0});
    eigenvals.push_back(0.02);
    size_t numEvents = 20000;
    addEllipsoid(numEvents, Q[0], Q[1], Q[2], eigenvects, eigenvals, doCounts);

    // radius for integration (4 stdevs of principal axis)
    auto peakRadius = 4 * sqrt(eigenvals[0]);
    double bgInnerRadius = 0.0;
    double bgOuterRadius = 0.0;

    // background w
    if (doBkgrd == true) {
      // add random uniform
      bgInnerRadius = peakRadius;
      bgOuterRadius =
          peakRadius * pow(2.0, 1.0 / 3.0); // twice vol of peak sphere
      std::vector<std::pair<double, double>> range;
      for (size_t d = 0; d < eigenvals.size(); d++) {
        range.push_back(std::pair(Q[d] - bgOuterRadius, Q[d] + bgOuterRadius));
      }
      addUniform(static_cast<size_t>(numEvents), range);
    }

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->addPeak(Peak(inst, 1, 1.0, Q));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks",
                                                 peakWS);

    // Integrate and copy to a new peaks workspace
    doRun({peakRadius}, bgOuterRadius, "IntegratePeaksMD2Test_peaks_out",
          bgInnerRadius, false, /* edge correction */
          false,                /* cylinder*/
          "NoFit", 0.0,         /* adaptive*/
          true,                 /* ellipsoid integration*/
          fixQAxis);            /* fix Q axis of ellipsoid*/

    // Old workspace is unchanged
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);

    PeaksWorkspace_sptr newPW = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(
            "IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(newPW);

    // test integrated counts
    if (doCounts < 0) {
      if (doBkgrd == true) {
        // use slightly more lenient tolerance
        TS_ASSERT_DELTA(newPW->getPeak(0).getIntensity(),
                        static_cast<double>(numEvents),
                        ceil(0.005 * static_cast<double>(numEvents)));
      } else {
        TS_ASSERT_DELTA(newPW->getPeak(0).getIntensity(),
                        static_cast<double>(numEvents),
                        ceil(0.002 * static_cast<double>(numEvents)));
      }
    } else {
      // sum = 0.2175*Npts (for 3D from simulation regardless of covar etc.)
      TS_ASSERT_DELTA(newPW->getPeak(0).getIntensity(),
                      static_cast<double>(numEvents) * 0.2175,
                      static_cast<double>(numEvents) * 0.0015);
    }

    // retrieve the peak
    IPeak &iPeak = newPW->getPeak(0);
    Peak *const peak = dynamic_cast<Peak *>(&iPeak);
    TS_ASSERT(peak);

    // Get the peak's shape
    const PeakShape &shape = peak->getPeakShape();
    PeakShapeEllipsoid const *const ellipsoidShape =
        dynamic_cast<PeakShapeEllipsoid *>(const_cast<PeakShape *>(&shape));

    // Check the shape is what we expect
    TSM_ASSERT("Wrong sort of peak", ellipsoidShape);

    // get radii
    auto radii = ellipsoidShape->abcRadii();
    auto axes = ellipsoidShape->directions();

    // sort by radius (in descending order)
    std::vector<size_t> isort(radii.size());
    std::iota(isort.begin(), isort.end(), 0);
    std::sort(isort.begin(), isort.end(),
              [&](size_t ii, size_t jj) { return radii[ii] > radii[jj]; });

    // loop over eigen vectors
    for (size_t ivect = 0; ivect < eigenvals.size(); ivect++) {
      auto rad = peakRadius * sqrt(eigenvals[ivect] / eigenvals[0]);
      double angle = axes[isort[ivect]].angle(V3D(
          eigenvects[ivect][0], eigenvects[ivect][1], eigenvects[ivect][2]));
      if (angle > M_PI / 2) {
        // axis is flipped
        angle = M_PI - angle;
      }
      if ((fixQAxis == true) & (ivect == 0)) {
        // first axis should be parallel to Q
        TS_ASSERT_EQUALS(isort[ivect], 0)
        TS_ASSERT_DELTA(radii[isort[ivect]], rad, 1E-10);
        TS_ASSERT_DELTA(angle, 0, 1E-10);
      } else {
        // compare eigenvalue (radii propto sqrt eigenval)
        if (doBkgrd == true) {
          // use much more lenient tolerance
          TS_ASSERT_DELTA(radii[isort[ivect]], rad, 0.15 * rad);
        } else {
          TS_ASSERT_DELTA(radii[isort[ivect]], rad, 0.05 * rad);
        }
        TS_ASSERT_DELTA(angle, 0, M_PI * (5.0 / 180.0));
      }
    }
  }

  void test_writes_out_selected_algorithm_parameters() {
    createMDEW();
    const double peakRadius = 2;
    const double backgroundOuterRadius = 3;
    const double backgroundInnerRadius = 2.5;

    doRun({peakRadius}, backgroundOuterRadius, "OutWS", backgroundInnerRadius);

    auto outWS =
        AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("OutWS");

    double actualPeakRadius =
        std::stod(outWS->mutableRun().getProperty("PeakRadius")->value());
    double actualBackgroundOutterRadius = std::stod(
        outWS->mutableRun().getProperty("BackgroundOuterRadius")->value());
    double actualBackgroundInnerRadius = std::stod(
        outWS->mutableRun().getProperty("BackgroundInnerRadius")->value());

    TS_ASSERT_EQUALS(peakRadius, actualPeakRadius);
    TS_ASSERT_EQUALS(backgroundOuterRadius, actualBackgroundOutterRadius);
    TS_ASSERT_EQUALS(backgroundInnerRadius, actualBackgroundInnerRadius);
    TS_ASSERT(outWS->hasIntegratedPeaks());

    // TODO. the methods above will be obsolete soon.
    IPeak &iPeak = outWS->getPeak(0);
    Peak *const peak = dynamic_cast<Peak *>(&iPeak);
    TS_ASSERT(peak);
    const PeakShape &shape = peak->getPeakShape();
    PeakShapeSpherical const *const sphericalShape =
        dynamic_cast<PeakShapeSpherical *>(const_cast<PeakShape *>(&shape));
    TS_ASSERT(sphericalShape);
    TS_ASSERT_EQUALS(peakRadius, sphericalShape->radius());
    TS_ASSERT_EQUALS(backgroundOuterRadius,
                     sphericalShape->backgroundOuterRadius().get());
    TS_ASSERT_EQUALS(backgroundInnerRadius,
                     sphericalShape->backgroundInnerRadius().get());
  }

  void test_writes_out_peak_shape() {
    createMDEW();
    const double peakRadius = 2;
    const double backgroundOuterRadius = 3;
    const double backgroundInnerRadius = 2.5;

    doRun({peakRadius}, backgroundOuterRadius, "OutWS", backgroundInnerRadius);

    PeaksWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("OutWS");

    // Get a peak.
    IPeak &iPeak = outWS->getPeak(0);
    Peak *const peak = dynamic_cast<Peak *>(&iPeak);
    TS_ASSERT(peak);
    // Get the peak's shape
    const PeakShape &shape = peak->getPeakShape();
    PeakShapeSpherical const *const sphericalShape =
        dynamic_cast<PeakShapeSpherical *>(const_cast<PeakShape *>(&shape));
    TSM_ASSERT("Wrong sort of peak", sphericalShape);

    // Check the shape is what we expect
    TS_ASSERT_EQUALS(peakRadius, sphericalShape->radius());
    TS_ASSERT_EQUALS(backgroundOuterRadius,
                     sphericalShape->backgroundOuterRadius().get());
    TS_ASSERT_EQUALS(backgroundInnerRadius,
                     sphericalShape->backgroundInnerRadius().get());
  }
};

//=========================================================================================
class IntegratePeaksMD2TestPerformance : public CxxTest::TestSuite {
public:
  size_t numPeaks;
  PeaksWorkspace_sptr peakWS;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksMD2TestPerformance *createSuite() {
    return new IntegratePeaksMD2TestPerformance();
  }
  static void destroySuite(IntegratePeaksMD2TestPerformance *suite) {
    delete suite;
  }

  IntegratePeaksMD2TestPerformance() {
    numPeaks = 1000;
    // Original MDEW.
    IntegratePeaksMD2Test::createMDEW();

    // Add a uniform, random background.

    FakeMDEventData algF2;
    TS_ASSERT_THROWS_NOTHING(algF2.initialize())
    TS_ASSERT(algF2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algF2.setPropertyValue(
        "InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algF2.setProperty("UniformParams", "100000"));
    TS_ASSERT_THROWS_NOTHING(algF2.execute());
    TS_ASSERT(algF2.isExecuted());

    MDEventWorkspace3Lean::sptr mdews =
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "IntegratePeaksMD2Test_MDEWS");
    mdews->setCoordinateSystem(Mantid::Kernel::HKL);

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);

    std::mt19937 rng;
    std::uniform_real_distribution<double> flat(-9.0, 9.0);

    peakWS = PeaksWorkspace_sptr(new PeaksWorkspace());
    for (size_t i = 0; i < numPeaks; ++i) {
      // Random peak center
      double x = flat(rng);
      double y = flat(rng);
      double z = flat(rng);

      // Make the peak
      IntegratePeaksMD2Test::addPeak(1000, x, y, z, 0.02);
      // With a center with higher density. 2000 events total.
      IntegratePeaksMD2Test::addPeak(1000, x, y, z, 0.005);

      // Make a few very strong peaks
      if (i % 21 == 0)
        IntegratePeaksMD2Test::addPeak(10000, x, y, z, 0.015);

      // Add to peaks workspace
      peakWS->addPeak(Peak(inst, 1, 1.0, V3D(x, y, z)));
    }
    AnalysisDataService::Instance().add("IntegratePeaksMD2Test_peaks", peakWS);
  }

  ~IntegratePeaksMD2TestPerformance() override {
    AnalysisDataService::Instance().remove("IntegratePeaksMD2Test_MDEWS");
    AnalysisDataService::Instance().remove("IntegratePeaksMD2Test_peaks");
  }

  void setUp() override {}

  void tearDown() override {}

  void test_performance_NoBackground() {
    for (size_t i = 0; i < 10; i++) {
      IntegratePeaksMD2Test::doRun({0.02}, 0.0);
    }
    // All peaks should be at least 1000 counts (some might be more if they
    // overla)
    for (size_t i = 0; i < numPeaks; i += 7) {
      double expected = 2000.0;
      if ((i % 21) == 0)
        expected += 10000.0;
      TS_ASSERT_LESS_THAN(expected - 1, peakWS->getPeak(int(i)).getIntensity());
    }
  }

  void test_performance_WithBackground() {
    for (size_t i = 0; i < 10; i++) {
      IntegratePeaksMD2Test::doRun({0.02}, 0.03);
    }
  }
};
