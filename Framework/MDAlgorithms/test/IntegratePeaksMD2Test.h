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
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidMDAlgorithms/IntegratePeaksMD2.h"

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
using Mantid::Kernel::Logger;
using Mantid::Kernel::V3D;

class IntegratePeaksMD2Test : public CxxTest::TestSuite {
public:
  IntegratePeaksMD2Test() { Mantid::API::FrameworkManager::Instance(); }
  ~IntegratePeaksMD2Test() override = default;

  void test_Init() {
    IntegratePeaksMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------
  /** Run the IntegratePeaksMD2 with the given peak radius integration param */
  static void doRun(std::vector<double> PeakRadius, std::vector<double> BackgroundRadius,
                    std::string OutputWorkspace = "IntegratePeaksMD2Test_peaks",
                    std::vector<double> BackgroundStartRadius = {}, bool edge = true, bool cyl = false,
                    std::string fnct = "NoFit", double adaptive = 0.0, bool ellip = false, bool fixQAxis = false,
                    bool useCentroid = false, bool fixMajorAxisLength = true, int maxIterations = 1,
                    bool maskEdgeTubes = true) {
    IntegratePeaksMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakRadius", PeakRadius));
    if (!BackgroundRadius.empty()) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterRadius", BackgroundRadius));
    }

    if (!BackgroundStartRadius.empty()) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerRadius", BackgroundStartRadius));
    }
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", edge));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "IntegratePeaksMD2Test_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OutputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Ellipsoid", ellip));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FixQAxis", fixQAxis));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseCentroid", useCentroid));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FixMajorAxisLength", fixMajorAxisLength));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxIterations", maxIterations));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Cylinder", cyl));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CylinderLength", 4.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PercentBackground", 20.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ProfileFunction", fnct));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrationOption", "Sum"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQMultiplier", adaptive));
    if (adaptive > 0.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQBackground", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskEdgeTubes", maskEdgeTubes));
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
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Extents", "-10,10,-10,10,-10,10"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Names", "h,k,l"));
    std::string units = Mantid::Kernel::Units::Symbol::RLU.ascii() + "," + Mantid::Kernel::Units::Symbol::RLU.ascii() +
                        "," + Mantid::Kernel::Units::Symbol::RLU.ascii();
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Units", units));
    std::string frames =
        Mantid::Geometry::HKL::HKLName + "," + Mantid::Geometry::HKL::HKLName + "," + Mantid::Geometry::HKL::HKLName;
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Frames", frames));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("SplitInto", "5"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("MaxRecursionDepth", "2"));
    TS_ASSERT_THROWS_NOTHING(algC.setPropertyValue("OutputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
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
    TS_ASSERT_THROWS_NOTHING(algF.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("PeakParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake ellipsoid peak */
  static void addEllipsoid(size_t num, double x, double y, double z, std::vector<std::vector<double>> eigvects,
                           std::vector<double> eigvals, double doCounts) {

    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", ";
    // add in eigenvects
    for (size_t ivect = 0; ivect < eigvects.size(); ivect++) {
      std::copy(eigvects[ivect].begin(), eigvects[ivect].end(), std::ostream_iterator<double>(mess, ", "));
    }
    // add in eigenvalues
    std::copy(eigvals.begin(), eigvals.end(), std::ostream_iterator<double>(mess, ", "));
    // doCounts
    mess << doCounts;

    std::ostringstream seed;
    seed << 77;

    FakeMDEventData algF;
    TS_ASSERT_THROWS_NOTHING(algF.initialize())
    TS_ASSERT(algF.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algF.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("EllipsoidParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("RandomSeed", seed.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake uniform  background*/
  static void addUniform(size_t num, std::vector<std::pair<double, double>> range) {
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
    TS_ASSERT_THROWS_NOTHING(algF.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("UniformParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Setup simple algorithm*/
  static void simpleRun(bool shouldPass, size_t numberValuesPeakRadius, size_t numberValuesBkgInnerRadius,
                        size_t numberValuesBkgOuterRadius, bool cyl = false, bool ellip = false) {
    createMDEW();
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    peakWS->addPeak(Peak(inst, 1, 1.0, V3D(0., 0., 0.)));
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    std::vector<double> peakRadius(numberValuesPeakRadius, 1.0);
    std::vector<double> bkgInnerRadius(numberValuesBkgInnerRadius, 1.1);
    std::vector<double> bkgOuterRadius(numberValuesBkgOuterRadius, 2.0);

    IntegratePeaksMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakRadius", peakRadius));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "IntegratePeaksMD2Test_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "IntegratePeaksMD2Test_Output"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerRadius", bkgInnerRadius));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterRadius", bkgOuterRadius));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Ellipsoid", ellip));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Cylinder", cyl));
    Logger logger("Logger_simpleRun");
    logger.notice() << "Running simpleRun with inputs: shouldPass:" << shouldPass
                    << " numberValuesPeakRadius:" << numberValuesPeakRadius
                    << " numberValuesBkgInnerRadius:" << numberValuesBkgInnerRadius
                    << " numberValuesBkgOuterRadius:" << numberValuesBkgOuterRadius << " cyl:" << cyl
                    << " ellip:" << ellip << "\n";
    if (shouldPass) {
      alg.setRethrows(true);
      TS_ASSERT_THROWS_NOTHING(alg.execute());
    } else {
      alg.setRethrows(true);
      TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    }
    AnalysisDataService::Instance().remove("IntegratePeaksMD2Test_peaks");
  }

  /** In the following four validation tests, three validation principles must be met:
      (where radius can be peak/BackgroundInner/BackgroundOuter)
   A) radius vector has 1 or 3 values
   B) radius vector has 1 value for sphere/cylinder/ellipsoid, 3 values only for ellipsoid
      (sphere implied when ellipsoid=false and cylinder=false)
   C) ellipsoid and cylinder cannot both be true
   */
  void test_validationSphere() {
    // To diagnose a test error, find in the test output the last
    // "Running simpleRun with inputs:" statement just before the "Error:"
    bool shouldPass = true;
    bool cyl = false;
    bool ellip = false;
    for (size_t peak = 1; peak < 5; peak++) {
      for (size_t inner = 1; inner < 5; inner++) {
        for (size_t outer = 1; outer < 5; outer++) {
          if (peak != 1 || inner != 1 || outer != 1) {
            shouldPass = false; // fails principle A or B
          }
          simpleRun(shouldPass, peak, inner, outer, cyl, ellip);
          shouldPass = true;
        }
      }
    }
  }

  void test_validationCylinder() {
    // To diagnose a test error, find in the test output the last
    // "Running simpleRun with inputs:" statement just before the Error
    bool shouldPass = true;
    bool cyl = true;
    bool ellip = false;
    for (size_t peak = 1; peak < 5; peak++) {
      for (size_t inner = 1; inner < 5; inner++) {
        for (size_t outer = 1; outer < 5; outer++) {
          if (peak != 1 || inner != 1 || outer != 1) {
            shouldPass = false; // fails principle A or B
          }
          simpleRun(shouldPass, peak, inner, outer, cyl, ellip);
          shouldPass = true;
        }
      }
    }
  }

  void test_validationEllipsoid() {
    // To diagnose a test error, find in the test output the last
    // "Running simpleRun with inputs:" statement just before the Error
    bool shouldPass = true;
    bool cyl = false;
    bool ellip = true;
    for (size_t peak = 1; peak < 5; peak++) {
      for (size_t inner = 1; inner < 5; inner++) {
        for (size_t outer = 1; outer < 5; outer++) {
          if ((peak != 1 && peak != 3) || (inner != 1 && inner != 3) || (outer != 1 && outer != 3)) {
            shouldPass = false; // fails principle A
          }
          simpleRun(shouldPass, peak, inner, outer, cyl, ellip);
          shouldPass = true;
        }
      }
    }
  }

  void test_validationEllipsoidandCylinder() {
    // To diagnose a test error, find in the test output the last
    // "Running simpleRun with inputs:" statement just before the Error
    bool shouldPass = false; // fails principle C
    bool cyl = true;
    bool ellip = true;
    simpleRun(shouldPass, 1, 1, 1, cyl, ellip);
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
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("IntegratePeaksMD2Test_MDEWS");
    mdews->setCoordinateSystem(Mantid::Kernel::HKL);
    TS_ASSERT_EQUALS(mdews->getNPoints(), 3000);
    TS_ASSERT_DELTA(mdews->getBox()->getSignal(), 3000.0, 1e-2);

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS0 = std::make_shared<PeaksWorkspace>();
    peakWS0->setInstrument(inst);
    peakWS0->addPeak(Peak(inst, 15050, 1.0));

    TS_ASSERT_EQUALS(peakWS0->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().add("IntegratePeaksMD2Test_peaks", peakWS0);

    // ------------- Integrating with cylinder ------------------------
    doRun({0.1}, {0.0}, "IntegratePeaksMD2Test_peaks", {0.0}, true, true);

    TS_ASSERT_DELTA(peakWS0->getPeak(0).getIntensity(), 2.0, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2, 1e-2);

    // Test profile Gaussian
    std::string fnct = "Gaussian";
    doRun({0.1}, {0.0}, "IntegratePeaksMD2Test_peaks", {0.0}, true, true, fnct);
    // More accurate integration changed values
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getIntensity(), 2.0, 1e-2);
    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2, 1e-2);
    Poco::File(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") +
               "IntegratePeaksMD2Test_MDEWSGaussian.dat")
        .remove();

    // Test profile back to back exponential
    fnct = "BackToBackExponential";
    doRun({0.1}, {0.0}, "IntegratePeaksMD2Test_peaks", {0.0}, true, true, fnct);

    // TS_ASSERT_DELTA( peakWS0->getPeak(0).getIntensity(), 2.0, 0.2);
    // Error is also calculated
    // TS_ASSERT_DELTA( peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2,
    // 0.2);
    Poco::File(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") +
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
    doRun({0.1}, {0.0}, "IntegratePeaksMD2Test_peaks", {0.0}, true, false, "NoFit", 0.01);
    TS_ASSERT_DELTA(peakWS0->getPeak(1).getIntensity(), 27.0, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(1).getSigmaIntensity(), sqrt(27.0), 1e-2);

    // ------------- Integrate with 0.1 radius but IntegrateIfOnEdge
    // false------------------------
    doRun({0.1}, {0.0}, "IntegratePeaksMD2Test_peaks", {0.0}, false);

    TS_ASSERT_DELTA(peakWS0->getPeak(0).getIntensity(), 2.0, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getSigmaIntensity(), M_SQRT2, 1e-2);

    AnalysisDataService::Instance().remove("IntegratePeaksMD2Test_peaks");

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->addPeak(Peak(inst, 15050, 1.0, V3D(0., 0., 0.)));
    peakWS->addPeak(Peak(inst, 15050, 1.0, V3D(2., 3., 4.)));
    peakWS->addPeak(Peak(inst, 15050, 1.0, V3D(6., 6., 6.)));

    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().add("IntegratePeaksMD2Test_peaks", peakWS);

    // ------------- Integrate with 1.0 radius ------------------------
    doRun({1.0}, {0.0});

    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    // Peak is of radius 2.0, but we get half that radius = ~1/8th the volume
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 125.0, 10.0);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS->getPeak(0).getSigmaIntensity(), sqrt(1000.0), 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getSigmaIntensity(), sqrt(1000.0), 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(2).getSigmaIntensity(), sqrt(peakWS->getPeak(2).getIntensity()), 1e-2);

    // ------------- Let's do it again with 2.0 radius ------------------------
    doRun({2.0}, {0.0});

    // All peaks are fully contained
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 1000.0, 1e-2);

    // ------------- Let's do it again with 0.5 radius ------------------------
    doRun({0.5}, {0.0});

    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 125.0, 10.0);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 15.0, 10.0);

    // ===============================================================================
    // ---- Now add a background signal over one peak--------------
    addPeak(1000, 0., 0., 0., 2.0);

    // ------------- Integrate with 1.0 radius and 2.0
    // background------------------------
    doRun({1.0}, {2.0});
    // Same 1000 since the background (~125) was subtracted, with some random
    // variation of the BG around
    //    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1000.0, 10.0);
    // Error on peak is the SUM of the error of peak and the subtracted
    // background
    TS_ASSERT_DELTA(peakWS->getPeak(0).getSigmaIntensity(), sqrt(1125.0 + 125.0), 2.0);

    // Had no bg, so they are the same
    TS_ASSERT_DELTA(peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA(peakWS->getPeak(1).getSigmaIntensity(), sqrt(1000.0), 1e-1);

    // This one is a 2.0 radius fake peak, so the background and peak have ~ the
    // same density! So ~0 total intensity.
    TS_ASSERT_DELTA(peakWS->getPeak(2).getIntensity(), 0.0, 12.0);
    // But the error is large since it is 125 - 125 (with errors)
    TS_ASSERT_DELTA(peakWS->getPeak(2).getSigmaIntensity(), sqrt(125.0 + 25.0), 2.);

    // ------------- Integrating without the background gives higher counts
    // ------------------------
    doRun({1.0}, {0.0});

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
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->addPeak(Peak(inst, 1, 1.0, V3D(0., 0., 0.)));
    AnalysisDataService::Instance().add("IntegratePeaksMD2Test_peaks", peakWS);

    // Integrate and copy to a new peaks workspace
    doRun({1.0}, {0.0}, "IntegratePeaksMD2Test_peaks_out");

    // Old workspace is unchanged
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);

    PeaksWorkspace_sptr newPW = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
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
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    peakWS->addPeak(Peak(inst, 1, 1.0, V3D(0., 0., 0.)));
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    // First, a check with no background
    doRun({1.0}, {0.0}, "IntegratePeaksMD2Test_peaks", {0.0});
    // approx. + 500 + 333 counts due to 2 backgrounds
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000 + 500 + 333, 100.0);
    TSM_ASSERT_DELTA("Simple sqrt() error", peakWS->getPeak(0).getSigmaIntensity(), sqrt(1833.0), 2);

    // Set background from 2.0 to 3.0.
    // So the 1/2 density background remains, we subtract the 1/3 density =
    // about 1500 counts
    doRun({1.0}, {3.0}, "IntegratePeaksMD2Test_peaks", {2.0});
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 1000 + 500, 100.0);
    // Error is larger, since it is error of peak + error of background
    TSM_ASSERT_DELTA("Error has increased", peakWS->getPeak(0).getSigmaIntensity(), sqrt(1830.0), 2);

    // Now do the same without the background start radius
    // So we subtract both densities = a lower count
    doRun({1.0}, {3.0});
    TSM_ASSERT_LESS_THAN("Peak intensity is lower if you do not include the "
                         "spacer shell (higher background)",
                         peakWS->getPeak(0).getIntensity(), 1500);
  }

  //-------------------------------------------------------------------------------
  //// Tests of ellipsoidal integration

  void test_exec_EllipsoidNoBackground_SingleCount() { EllipsoidTestHelper(-1.0, false, false); }
  void test_exec_EllipsoidNoBackground_SingleCount_FixQAxis() { EllipsoidTestHelper(-1.0, true, false); }
  void test_exec_EllipsoidNoBackground_NonSingleCount() { EllipsoidTestHelper(1.0, false, false); }
  void test_exec_EllipsoidNoBackground_NonSingleCount_FixQAxis() { EllipsoidTestHelper(1.0, true, false); }
  void test_exec_EllipsoidWithBackground_SingleCount() { EllipsoidTestHelper(-1.0, false, true); }

  void test_exec_EllipsoidRadii_NoBackground_SingleCount() {
    std::vector<double> radii = {0.05, 0.03, 0.02};
    std::for_each(radii.begin(), radii.end(), [](double &r) { r = 4.0 * sqrt(r); });
    EllipsoidTestHelper(-1.0, false, false, {0.05, 0.03, 0.02}, radii);
  }

  void test_exec_EllipsoidRadii_NoBackground_SingleCount_FixQAxis() {
    std::vector<double> radii = {0.05, 0.03, 0.02};
    std::for_each(radii.begin(), radii.end(), [](double &r) { r = 4.0 * sqrt(r); });
    EllipsoidTestHelper(-1.0, true, false, {0.05, 0.03, 0.02}, radii);
  }

  void test_exec_EllipsoidRadii_NoBackground_SingleCount_Qy() {
    std::vector<double> radii = {0.03, 0.05, 0.02};
    std::for_each(radii.begin(), radii.end(), [](double &r) { r = 4.0 * sqrt(r); });
    EllipsoidTestHelper(-1.0, false, false, {0.03, 0.05, 0.02}, radii);
  }

  void test_exec_EllipsoidRadii_NoBackground_SingleCount_Qz() {
    std::vector<double> radii = {0.03, 0.02, 0.05};
    std::for_each(radii.begin(), radii.end(), [](double &r) { r = 4.0 * sqrt(r); });
    EllipsoidTestHelper(-1.0, false, false, {0.03, 0.02, 0.05}, radii);
  }

  void test_exec_EllipsoidRadii_NoBackground_NonSingleCount() {
    std::vector<double> radii = {0.05, 0.03, 0.02};
    std::for_each(radii.begin(), radii.end(), [](double &r) { r = 4.0 * sqrt(r); });
    EllipsoidTestHelper(1.0, false, false, {0.05, 0.03, 0.02}, radii);
  }

  void test_exec_EllipsoidRadii_NoBackground_NonSingleCount_FixQAxis() {
    std::vector<double> radii = {0.05, 0.03, 0.02};
    std::for_each(radii.begin(), radii.end(), [](double &r) { r = 4.0 * sqrt(r); });
    EllipsoidTestHelper(1.0, true, false, {0.05, 0.03, 0.02}, radii);
  }

  void test_exec_EllipsoidRadii_WithBackground_SingleCount() {
    std::vector<double> radii = {0.05, 0.03, 0.02};
    std::for_each(radii.begin(), radii.end(), [](double &r) { r = 4.0 * sqrt(r); });
    std::vector<double> outer;
    std::transform(radii.begin(), radii.end(), std::back_inserter(outer),
                   [](double &r) { return r * pow(2.0, (1.0 / 3.0)); });
    EllipsoidTestHelper(-1.0, false, true, {0.05, 0.03, 0.02}, radii, radii, outer);
  }

  void test_exec_EllipsoidRadii_NoBackground_SingleCount_ScaledSphere() {
    // Test if elipsoid of volume 0.5*V gives half the intensity as a sphere of
    // volume V
    size_t numEvents = 20000;
    V3D pos(1.0, 0.0, 0.0); // peak position
    double peakRad = 1.0;

    createMDEW();

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    addPeak(numEvents, pos[0], pos[1], pos[2], peakRad);
    peakWS->addPeak(Peak(inst, 1, 1.0, pos));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    doRun({1.0}, {0.0}, "IntegratePeaksMD2Test_peaks_out", {}, false);

    PeaksWorkspace_sptr peakResult = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(peakResult);

    double sphereInten = peakResult->getPeak(0).getIntensity();

    // Semi axis lengths corresponding to a sphere of 0.5*V
    std::vector<double> radii(3, peakRad * pow(0.5, 1.0 / 3.0));

    // Perform ellipsoidal integration of sphere
    doRun(radii, {0.0}, "IntegratePeaksMD2Test_peaks_out", {0.0}, false, false, "NoFit", 0.0, true, false);

    peakResult = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(peakResult);

    double ellipInten = peakResult->getPeak(0).getIntensity();

    TS_ASSERT_DELTA(ellipInten, 0.5 * sphereInten, ceil(0.003 * static_cast<double>(sphereInten)));
  }

  void test_exec_EllipsoidRadii_NoBackground_SingleCount_Vol_LongEllipse() {

    CreateMDWorkspace algC;
    TS_ASSERT_THROWS_NOTHING(algC.initialize())
    TS_ASSERT(algC.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Dimensions", "3"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Extents", "-0.5,0.5,-0.5,0.5,-0.5,0.5"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Names", "h,k,l"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Units", "U,U,U"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Frames", "HKL,HKL,HKL"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("SplitInto", "2"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("MaxRecursionDepth", "5"));
    TS_ASSERT_THROWS_NOTHING(algC.setPropertyValue("OutputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algC.execute());
    TS_ASSERT(algC.isExecuted());

    // Test an ellipsoid against theoretical vol
    size_t numEvents = 200000;
    V3D pos(0.0, 0.0, 0.0); // peak position

    // Major axis along x
    double fail_val = 0.013;
    std::vector<double> radii = {0.05, fail_val, fail_val};

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    addUniform(numEvents, {std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5)});
    peakWS->addPeak(Peak(inst, 1, 1.0, pos));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    double ellipVol = (4.0 / 3.0) * M_PI * static_cast<double>(numEvents) *
                      std::accumulate(radii.begin(), radii.end(), 1.0, std::multiplies<double>());

    doRun(radii, {0.0}, "IntegratePeaksMD2Test_peaks_out", {0.0}, false, false, "NoFit", 0.0, true, false);

    PeaksWorkspace_sptr peakResult = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(peakResult);

    double ellipInten = peakResult->getPeak(0).getIntensity();
    TS_ASSERT_DELTA(ellipInten, ellipVol, ceil(0.05 * static_cast<double>(ellipVol)));
  }

  void test_exec_EllipsoidRadii_NoBackground_SingleCount_Vol() {
    // Test an ellipsoid against theoretical vol
    size_t numEvents = 2000000;
    V3D pos(0.0, 0.0, 0.0); // peak position

    createMDEW();

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    addUniform(numEvents, {std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5)});
    peakWS->addPeak(Peak(inst, 1, 1.0, pos));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    // Major axis along z
    std::vector<double> radii = {0.03, 0.04, 0.05};
    doRun(radii, {0.0}, "IntegratePeaksMD2Test_peaks_out", {0.0}, false, false, "NoFit", 0.0, true, false);

    PeaksWorkspace_sptr peakResult = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(peakResult);

    double ellipInten = peakResult->getPeak(0).getIntensity();
    double ellipVol = (4.0 / 3.0) * M_PI * static_cast<double>(numEvents) *
                      std::accumulate(radii.begin(), radii.end(), 1.0, std::multiplies<double>());
    TS_ASSERT_DELTA(ellipInten, ellipVol, ceil(0.05 * static_cast<double>(ellipVol)));

    // Major axis along y
    radii = {0.04, 0.05, 0.03};
    doRun(radii, {0.0}, "IntegratePeaksMD2Test_peaks_out", {0.0}, false, false, "NoFit", 0.0, true, false);

    peakResult = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(peakResult);

    ellipInten = peakResult->getPeak(0).getIntensity();
    TS_ASSERT_DELTA(ellipInten, ellipVol, ceil(0.05 * static_cast<double>(ellipVol)));

    // Major axis along x
    radii = {0.05, 0.04, 0.03};
    doRun(radii, {0.0}, "IntegratePeaksMD2Test_peaks_out", {0.0}, false, false, "NoFit", 0.0, true, false);

    peakResult = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(peakResult);

    ellipInten = peakResult->getPeak(0).getIntensity();
    TS_ASSERT_DELTA(ellipInten, ellipVol, ceil(0.05 * static_cast<double>(ellipVol)));
  }

  void test_exec_EllipsoidRadii_WithBackground() {
    // Test an ellipsoid against theoretical vol
    size_t numEvents = 1000000;
    V3D pos(0.0, 0.0, 0.0); // peak position

    createMDEW();

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    addUniform(numEvents, {std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5)});
    std::vector<std::vector<double>> eigenvects;
    eigenvects.push_back(std::vector<double>{1.0, 0.0, 0.0});
    eigenvects.push_back(std::vector<double>{0.0, 1.0, 0.0});
    eigenvects.push_back(std::vector<double>{0.0, 0.0, 1.0});
    std::vector<double> radii = {0.4, 0.3, 0.2};
    // addEllipsoid(numEvents, pos[0], pos[1], pos[2], eigenvects, radii, 0.0);
    peakWS->addPeak(Peak(inst, 1, 1.0, pos));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    std::vector<double> background = {0.4, 0.45, 0.5};
    doRun(radii, background, "IntegratePeaksMD2Test_peaks_out", {0.0}, true, false, "NoFit", 0.0, true, false);

    PeaksWorkspace_sptr peakResult = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(peakResult);

    // the integrated intensity should end up around 0 as this is just
    // approximately uniform data
    double ellipInten = peakResult->getPeak(0).getIntensity();
    TS_ASSERT_DELTA(ellipInten, 0, 200);

    // Get the peak's shape
    const PeakShape &shape = peakResult->getPeak(0).getPeakShape();
    PeakShapeEllipsoid const *const ellipsoidShape =
        dynamic_cast<PeakShapeEllipsoid *>(const_cast<PeakShape *>(&shape));

    // Check the shape is what we expect
    TSM_ASSERT("Wrong sort of peak", ellipsoidShape);

    TS_ASSERT_DELTA(ellipsoidShape->abcRadii(), radii, 1e-9);
    TS_ASSERT_DELTA(ellipsoidShape->abcRadiiBackgroundInner(), radii, 1e-9);
    TS_ASSERT_DELTA(ellipsoidShape->abcRadiiBackgroundOuter(), background, 1e-9);
    TS_ASSERT_EQUALS(ellipsoidShape->directions()[0], V3D(1, 0, 0));
    TS_ASSERT_EQUALS(ellipsoidShape->directions()[1], V3D(0, 1, 0));
    TS_ASSERT_EQUALS(ellipsoidShape->directions()[2], V3D(0, 0, 1));
  }

  void test_exec_ellipsoid_integrate_on_centroid() {
    createMDEW();

    // peak Q
    V3D Q(1.0, 0.0, 0.0);
    // set eigenvectors and eigenvalues
    std::vector<double> eigenvals = {0.05, 0.03, 0.02};
    std::vector<std::vector<double>> eigenvects;
    eigenvects.push_back(std::vector<double>{Q[0], Q[1], Q[2]}); // para Q
    eigenvects.push_back(std::vector<double>{0.0, 1.0, 0.0});
    eigenvects.push_back(std::vector<double>{0.0, 0.0, 1.0});

    size_t numEvents = 20000;
    V3D offset(0.05, 0.0, 0.0); // generate peak away from nominal position
    addEllipsoid(numEvents, Q[0] + offset[0], Q[1] + offset[1], Q[2] + offset[2], eigenvects, eigenvals, true);

    // radius for integration (4 stdevs of principal axis)
    std::vector<double> peakRadius{4 * sqrt(eigenvals[0])};
    std::vector<double> innerBgRadius{0.0};
    std::vector<double> outerBgRadius{0.0}; // set to less than peak
    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->addPeak(Peak(inst, 1, 1.0, Q));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    doRun(peakRadius, outerBgRadius, "IntegratePeaksMD2Test_peaks_out", innerBgRadius, false, /* edge correction */
          false,                                                                              /* cylinder*/
          "NoFit", 0.0,                                                                       /* adaptive*/
          true,                                                                               /* ellipsoid integration*/
          false, /* fix Q axis of ellipsoid*/
          true); /* integrate around centroid*/

    PeaksWorkspace_sptr newPW = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(newPW);

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

    // check translation is same as offset
    auto translation = ellipsoidShape->translation();
    for (size_t idim = 0; idim < eigenvals.size(); idim++) {
      TS_ASSERT_DELTA(translation[idim], offset[idim], 0.005);
    }
  }

  void test_exec_ellipsoid_integrate_use_estimated_stdevs_nobg() {
    createMDEW();

    // peak Q
    V3D Q(1.0, 0.0, 0.0);
    // set eigenvectors and eigenvalues
    std::vector<double> eigenvals = {0.05, 0.03, 0.02};
    std::vector<std::vector<double>> eigenvects;
    eigenvects.push_back(std::vector<double>{Q[0], Q[1], Q[2]}); // para Q
    eigenvects.push_back(std::vector<double>{0.0, 1.0, 0.0});
    eigenvects.push_back(std::vector<double>{0.0, 0.0, 1.0});

    size_t numEvents = 20000;
    addEllipsoid(numEvents, Q[0], Q[1], Q[2], eigenvects, eigenvals, true);

    // radius for integration (4 stdevs of principal axis)
    std::vector<double> peakRadius{4 * sqrt(eigenvals[0])};
    std::vector<double> innerBgRadius{0.0};
    std::vector<double> outerBgRadius{0.0}; // set to less than peak
    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->addPeak(Peak(inst, 1, 1.0, Q));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    doRun(peakRadius, outerBgRadius, "IntegratePeaksMD2Test_peaks_out", innerBgRadius, false, /* edge correction */
          false,                                                                              /* cylinder*/
          "NoFit", 0.0,                                                                       /* adaptive*/
          true,                                                                               /* ellipsoid integration*/
          false,  /* fix Q axis of ellipsoid*/
          false,  /* integrate around centroid*/
          false); /* fix major axis length*/

    PeaksWorkspace_sptr newPW = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(newPW);

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

    // check translation is zero (not integrating round centroid)
    auto translation = ellipsoidShape->translation();
    for (size_t idim = 0; idim < eigenvals.size(); idim++) {
      TS_ASSERT_DELTA(translation[idim], 0.0, 0.005);
    }

    // check axes corrsepond to 3 stdevs
    auto radii = ellipsoidShape->abcRadii();
    for (size_t idim = 0; idim < eigenvals.size(); idim++) {
      TS_ASSERT_DELTA(radii[idim] * radii[idim], 9 * eigenvals[idim], 0.05);
    }
  }

  void test_exec_ellipsoid_integrate_multi_iterations() {
    createMDEW();

    // peak Q
    V3D Q(1.0, 0.0, 0.0);
    // set eigenvectors and eigenvalues
    std::vector<double> eigenvals = {0.05, 0.03, 0.02};
    std::vector<std::vector<double>> eigenvects;
    eigenvects.push_back(std::vector<double>{Q[0], Q[1], Q[2]}); // para Q
    eigenvects.push_back(std::vector<double>{0.0, 1.0, 0.0});
    eigenvects.push_back(std::vector<double>{0.0, 0.0, 1.0});

    size_t numEvents = 2000;
    addEllipsoid(numEvents, Q[0], Q[1], Q[2], eigenvects, eigenvals, true);
    // add uniform bg
    std::vector<std::pair<double, double>> range(eigenvals.size(), std::make_pair(0, 2));
    addUniform(static_cast<size_t>(numEvents), range);

    // radius for integration (4 stdevs of principal axis)
    std::vector<double> peakRadius{4 * sqrt(eigenvals[0])};
    std::vector<double> innerBgRadius{0.0};
    std::vector<double> outerBgRadius{0.0}; // set to less than peak
    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->addPeak(Peak(inst, 1, 1.0, Q));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    std::vector<double> prev_radii(eigenvals.size(), DBL_MAX);
    for (int niter = 1; niter < 3; niter++) {
      doRun(peakRadius, outerBgRadius, "IntegratePeaksMD2Test_peaks_out", innerBgRadius, false, /* edge correction */
            false,                                                                              /* cylinder*/
            "NoFit", 0.0,                                                                       /* adaptive*/
            true,   /* ellipsoid integration*/
            true,   /* fix Q axis of ellipsoid*/
            false,  /* integrate around centroid*/
            false,  /* fix major axis length*/
            niter); /* maxIterations */
      PeaksWorkspace_sptr newPW = std::dynamic_pointer_cast<PeaksWorkspace>(
          AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
      IPeak &iPeak = newPW->getPeak(0);
      Peak *const peak = dynamic_cast<Peak *>(&iPeak);
      const PeakShape &shape = peak->getPeakShape();
      PeakShapeEllipsoid const *const ellipsoidShape =
          dynamic_cast<PeakShapeEllipsoid *>(const_cast<PeakShape *>(&shape));
      auto radii = ellipsoidShape->abcRadii();
      // test radii are smaller than previous iteration
      for (size_t idim = 0; idim < eigenvals.size(); idim++) {
        TS_ASSERT_LESS_THAN(radii[idim], prev_radii[idim]);
        prev_radii[idim] = radii[idim];
      }
    }
  }

  void EllipsoidTestHelper(double doCounts, bool fixQAxis, bool doBkgrd,
                           std::vector<double> peakEigenvals = std::vector<double>(),
                           std::vector<double> ellipsoidRadii = std::vector<double>(),
                           std::vector<double> ellipsoidBgInnerRad = std::vector<double>(),
                           std::vector<double> ellipsoidBgOuterRad = std::vector<double>()) {
    // doCounts < 0 -> all events have a count of 1
    // doCounts > 0 -> counts follow multivariate normal dist

    if (!ellipsoidRadii.empty()) {
      TS_ASSERT(ellipsoidRadii.size() == 3);
    }

    if (!ellipsoidBgInnerRad.empty()) {
      TS_ASSERT(ellipsoidBgInnerRad.size() == 3);
    }

    if (!ellipsoidBgOuterRad.empty()) {
      TS_ASSERT(ellipsoidBgOuterRad.size() == 3);
    }

    createMDEW();

    // peak Q
    V3D Q(1.0, 0.0, 0.0);
    // set eigenvectors and eigenvalues
    std::vector<std::vector<double>> eigenvects;
    std::vector<double> eigenvals;
    eigenvects.push_back(std::vector<double>{Q[0], Q[1], Q[2]}); // para Q
    // other orthogonal axes
    eigenvects.push_back(std::vector<double>{0.0, 1.0, 0.0});
    eigenvects.push_back(std::vector<double>{0.0, 0.0, 1.0});

    if (ellipsoidRadii.empty()) {
      // Use default eigenvals for ellipsoid peaks
      eigenvals.push_back(0.05);
      eigenvals.push_back(0.03);
      eigenvals.push_back(0.02);
    } else {
      eigenvals = peakEigenvals;
    }

    size_t numEvents = 20000;
    addEllipsoid(numEvents, Q[0], Q[1], Q[2], eigenvects, eigenvals, doCounts);

    // radius for integration (4 stdevs of principal axis)
    auto peakRadius = 4 * sqrt(eigenvals[0]);
    std::vector<double> bgInnerRadius = {};
    std::vector<double> bgOuterRadius = {};

    // background w
    if (doBkgrd == true) {
      // add random uniform
      std::vector<std::pair<double, double>> range;
      if (ellipsoidBgInnerRad.empty() && ellipsoidBgOuterRad.empty()) {
        bgInnerRadius.push_back(peakRadius);
        bgOuterRadius.push_back(peakRadius * pow(2.0, 1.0 / 3.0)); // twice vol of peak sphere
      } else {
        bgInnerRadius = ellipsoidBgInnerRad;
        bgOuterRadius = ellipsoidBgOuterRad;
      }
      for (size_t d = 0; d < eigenvals.size(); d++) {
        range.push_back(std::pair(Q[d] - bgOuterRadius[0], Q[d] + bgOuterRadius[0]));
      }
      addUniform(static_cast<size_t>(numEvents), range);
    }

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->addPeak(Peak(inst, 1, 1.0, Q));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    // Integrate and copy to a new peaks workspace
    std::vector<double> radiiVec;
    if (ellipsoidRadii.empty()) {
      radiiVec.push_back(peakRadius);
    } else {
      radiiVec = ellipsoidRadii;
    }

    doRun(radiiVec, bgOuterRadius, "IntegratePeaksMD2Test_peaks_out", bgInnerRadius, false, /* edge correction */
          false,                                                                            /* cylinder*/
          "NoFit", 0.0,                                                                     /* adaptive*/
          true,                                                                             /* ellipsoid integration*/
          fixQAxis);                                                                        /* fix Q axis of ellipsoid*/

    // Old workspace is unchanged
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);

    PeaksWorkspace_sptr newPW = std::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_peaks_out"));
    TS_ASSERT(newPW);

    // test integrated counts
    if (doCounts < 0) {
      if (doBkgrd == true) {
        // use slightly more lenient tolerance
        TS_ASSERT_DELTA(newPW->getPeak(0).getIntensity(), static_cast<double>(numEvents),
                        ceil(0.005 * static_cast<double>(numEvents)));
      } else {
        TS_ASSERT_DELTA(newPW->getPeak(0).getIntensity(), static_cast<double>(numEvents),
                        ceil(0.002 * static_cast<double>(numEvents)));
      }
    } else {
      // sum = 0.2175*Npts (for 3D from simulation regardless of covar etc.)
      TS_ASSERT_DELTA(newPW->getPeak(0).getIntensity(), static_cast<double>(numEvents) * 0.2175,
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
    std::sort(isort.begin(), isort.end(), [&](size_t ii, size_t jj) { return radii[ii] > radii[jj]; });

    // loop over eigen vectors
    for (size_t ivect = 0; ivect < eigenvals.size(); ivect++) {
      auto rad = sqrt(eigenvals[ivect] / eigenvals[0]);
      if (ellipsoidRadii.size() < 3) {
        rad *= peakRadius;
      } else {
        rad *= ellipsoidRadii[0];
      }

      size_t ind = isort[ivect];
      if (ellipsoidRadii.size() == 3) {
        ind = ivect;
      }
      double angle = axes[ind].angle(V3D(eigenvects[ivect][0], eigenvects[ivect][1], eigenvects[ivect][2]));
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
          TS_ASSERT_DELTA(radii[ind], rad, 0.15 * rad);
        } else {
          TS_ASSERT_DELTA(radii[ind], rad, 0.05 * rad);
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

    doRun({peakRadius}, {backgroundOuterRadius}, "OutWS", {backgroundInnerRadius});

    auto outWS = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("OutWS");

    double actualPeakRadius = std::stod(outWS->mutableRun().getProperty("PeakRadius")->value());
    double actualBackgroundOutterRadius = std::stod(outWS->mutableRun().getProperty("BackgroundOuterRadius")->value());
    double actualBackgroundInnerRadius = std::stod(outWS->mutableRun().getProperty("BackgroundInnerRadius")->value());

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
    TS_ASSERT_EQUALS(backgroundOuterRadius, sphericalShape->backgroundOuterRadius().value());
    TS_ASSERT_EQUALS(backgroundInnerRadius, sphericalShape->backgroundInnerRadius().value());
  }

  void test_writes_out_peak_shape() {
    createMDEW();
    const double peakRadius = 2;
    const double backgroundOuterRadius = 3;
    const double backgroundInnerRadius = 2.5;

    doRun({peakRadius}, {backgroundOuterRadius}, "OutWS", {backgroundInnerRadius});

    PeaksWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("OutWS");

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
    TS_ASSERT_EQUALS(backgroundOuterRadius, sphericalShape->backgroundOuterRadius().value());
    TS_ASSERT_EQUALS(backgroundInnerRadius, sphericalShape->backgroundInnerRadius().value());
  }

  void test_exec_EllipsoidRadii_with_LeanElasticPeaks() {
    // Test an ellipsoid against theoretical vol
    const std::vector<double> radii = {0.4, 0.3, 0.2};
    const V3D pos(0.0, 0.0, 0.0); // peak position
    const int numEvents = 1000000;

    createMDEW();
    addUniform(numEvents, {std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5), std::make_pair(-0.5, 0.5)});

    auto peakWS = std::make_shared<LeanElasticPeaksWorkspace>();
    peakWS->addPeak(LeanElasticPeak(pos));
    AnalysisDataService::Instance().addOrReplace("IntegratePeaksMD2Test_peaks", peakWS);

    doRun(radii, {0.0}, "IntegratePeaksMD2Test_Leanpeaks_out", {0.0}, true, false, "NoFit", 0.0, true, false);

    LeanElasticPeaksWorkspace_sptr peakResult = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("IntegratePeaksMD2Test_Leanpeaks_out"));
    TS_ASSERT(peakResult);

    // the integrated intensity should end up around
    // 4/3*PI*0.4*0.3*0.2*numEvents
    double ellipInten = peakResult->getPeak(0).getIntensity();
    TS_ASSERT_DELTA(ellipInten, 4. / 3. * M_PI * 0.4 * 0.3 * 0.2 * numEvents, 25);

    // Get the peak's shape
    const PeakShape &shape = peakResult->getPeak(0).getPeakShape();
    PeakShapeEllipsoid const *const ellipsoidShape =
        dynamic_cast<PeakShapeEllipsoid *>(const_cast<PeakShape *>(&shape));

    // Check the shape is what we expect
    TSM_ASSERT("Wrong sort of peak", ellipsoidShape);

    TS_ASSERT_DELTA(ellipsoidShape->abcRadii(), radii, 1e-9);
    TS_ASSERT_EQUALS(ellipsoidShape->directions()[0], V3D(1, 0, 0));
    TS_ASSERT_EQUALS(ellipsoidShape->directions()[1], V3D(0, 1, 0));
    TS_ASSERT_EQUALS(ellipsoidShape->directions()[2], V3D(0, 0, 1));
  }
};

//=========================================================================================
class IntegratePeaksMD2TestPerformance : public CxxTest::TestSuite {
public:
  size_t numPeaks;
  PeaksWorkspace_sptr peakWS;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksMD2TestPerformance *createSuite() { return new IntegratePeaksMD2TestPerformance(); }
  static void destroySuite(IntegratePeaksMD2TestPerformance *suite) { delete suite; }

  IntegratePeaksMD2TestPerformance() {
    numPeaks = 1000;
    // Original MDEW.
    IntegratePeaksMD2Test::createMDEW();

    // Add a uniform, random background.

    FakeMDEventData algF2;
    TS_ASSERT_THROWS_NOTHING(algF2.initialize())
    TS_ASSERT(algF2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algF2.setPropertyValue("InputWorkspace", "IntegratePeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algF2.setProperty("UniformParams", "100000"));
    TS_ASSERT_THROWS_NOTHING(algF2.execute());
    TS_ASSERT(algF2.isExecuted());

    MDEventWorkspace3Lean::sptr mdews =
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("IntegratePeaksMD2Test_MDEWS");
    mdews->setCoordinateSystem(Mantid::Kernel::HKL);

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    std::mt19937 rng;
    std::uniform_real_distribution<double> flat(-9.0, 9.0);

    PeaksWorkspace_sptr peakWS = std::make_shared<PeaksWorkspace>();
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
      IntegratePeaksMD2Test::doRun({0.02}, {0.0});
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
      IntegratePeaksMD2Test::doRun({0.02}, {0.03});
    }
  }
};
