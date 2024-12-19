// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCrystal/PeakIntensityVsRadius.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class PeakIntensityVsRadiusTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    PeakIntensityVsRadius alg;
    assertSuccessfulInitialization(alg);
  }

  void assertSuccessfulInitialization(PeakIntensityVsRadius &alg) {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  static void createMDEW() {
    // ---- Start with empty MDEW ----
    FrameworkManager::Instance().exec("CreateMDWorkspace", 14, "Dimensions", "3", "Extents", "-10,10,-10,10,-10,10",
                                      "Names", "h,k,l", "Units", "-,-,-", "SplitInto", "5", "MaxRecursionDepth", "2",
                                      "OutputWorkspace", "PeakIntensityVsRadiusTest_MDEWS");
  }

  //-------------------------------------------------------------------------------
  /** Add a fake peak */
  static void addPeak(size_t num, double x, double y, double z, double radius) {
    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    FrameworkManager::Instance().exec("FakeMDEventData", 4, "InputWorkspace", "PeakIntensityVsRadiusTest_MDEWS",
                                      "PeakParams", mess.str().c_str());
  }

  void setUp() override {
    // Fake MDWorkspace with 2 peaks
    createMDEW();
    addPeak(1000, 0, 0, 0, 1.0);
    addPeak(1000, 5, 5, 5, 1.0);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5); // Unused fake instruement
    peakWS->addPeak(Peak(inst, 1, 1.0, V3D(0., 0., 0.)));
    peakWS->addPeak(Peak(inst, 1, 1.0, V3D(5., 5., 5.)));
    AnalysisDataService::Instance().addOrReplace("PeakIntensityVsRadiusTest_peaks", peakWS);
  }

  void test_worksWithValidInputs() {
    ensureExecutionNoThrow(1.0, 2.0, 0, 0);
    ensureExecutionNoThrow(0, 0, 0.12, 0.15);
    ensureExecutionNoThrow(1.0, 0, 0, 0.15);
    ensureExecutionNoThrow(0, 1.5, 0.15, 0);
    // Can't specify fixed and variable
  }

  void test_throwsWhenExecutingForInvalidInputs() {
    ensureExecutionThrows(1.0, 0, 0.15, 0);
    ensureExecutionThrows(0, 1.0, 0, 0.15);
    ensureExecutionThrows(1.0, 0, 0.15, 0);
    ensureExecutionThrows(1.0, 1.0, 0.12, 0.15);
  }

  void ensureExecutionThrows(double BackgroundInnerFactor, double BackgroundOuterFactor, double BackgroundInnerRadius,
                             double BackgroundOuterRadius) {
    doTestValid(false, BackgroundInnerFactor, BackgroundOuterFactor, BackgroundInnerRadius, BackgroundOuterRadius);
  }

  void ensureExecutionNoThrow(double BackgroundInnerFactor, double BackgroundOuterFactor, double BackgroundInnerRadius,
                              double BackgroundOuterRadius) {
    doTestValid(true, BackgroundInnerFactor, BackgroundOuterFactor, BackgroundInnerRadius, BackgroundOuterRadius);
  }

  /** Check the validateInputs() calls */
  void doTestValid(bool assertExecuteSuccess, double BackgroundInnerFactor, double BackgroundOuterFactor,
                   double BackgroundInnerRadius, double BackgroundOuterRadius) {
    PeakIntensityVsRadius alg;
    // Name of the output workspace.
    std::string outWSName("PeakIntensityVsRadiusTest_OutputWS");
    assertSuccessfulInitialization(alg);
    assertNoThrowWhenSettingProperties(alg, outWSName, BackgroundInnerFactor, BackgroundOuterFactor,
                                       BackgroundInnerRadius, BackgroundOuterRadius);
    if (assertExecuteSuccess) {
      TS_ASSERT_THROWS_NOTHING(alg.execute(););
    } else {
      TS_ASSERT_THROWS_ANYTHING(alg.execute(););
    }
  }

  void assertNoThrowWhenSettingProperties(PeakIntensityVsRadius &alg, std::string &outWSName,
                                          double BackgroundInnerFactor, double BackgroundOuterFactor,
                                          double BackgroundInnerRadius, double BackgroundOuterRadius) {
    auto constexpr DEFAULT_NUM_STEPS = 16;
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "PeakIntensityVsRadiusTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", "PeakIntensityVsRadiusTest_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RadiusStart", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RadiusEnd", 1.5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumSteps", DEFAULT_NUM_STEPS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerFactor", BackgroundInnerFactor));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterFactor", BackgroundOuterFactor));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerRadius", BackgroundInnerRadius));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterRadius", BackgroundOuterRadius));
  }

  void test_throwsWhenSettingInvalidPropertyValues() {
    PeakIntensityVsRadius alg;
    assertSuccessfulInitialization(alg);
    assertInvalidPropertyValue(alg, "NumSteps", -8);
  }

  template <typename PropertyType>
  void assertInvalidPropertyValue(PeakIntensityVsRadius &alg, std::string const &name, PropertyType value) {
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty(name, value))
  }

  void test_VariableBackground() {
    MatrixWorkspace_sptr ws = doTest(1.0, 2.0, 0, 0);
    // Check the results
    TSM_ASSERT_EQUALS("Two peaks", ws->getNumberHistograms(), 2);
    assertFirstFourYValuesCloseToZero(ws);

    // Points before 0.5 are approximately zero because the background shell is
    // in the peak.
    assertFlatAfter1(ws);
  }

  MatrixWorkspace_sptr doTest(double BackgroundInnerFactor, double BackgroundOuterFactor, double BackgroundInnerRadius,
                              double BackgroundOuterRadius) {
    // Name of the output workspace.
    std::string outWSName("PeakIntensityVsRadiusTest_OutputWS");

    PeakIntensityVsRadius alg;
    assertSuccessfulInitialization(alg);
    assertNoThrowWhenSettingProperties(alg, outWSName, BackgroundInnerFactor, BackgroundOuterFactor,
                                       BackgroundInnerRadius, BackgroundOuterRadius);
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    return ws;
  }

  void test_FixedBackground() {
    // Background
    MatrixWorkspace_sptr ws = doTest(0, 0, 0.4, 0.5);
    // Check the results
    TSM_ASSERT_EQUALS("Two peaks", ws->getNumberHistograms(), 2);

    // Points before 0.5 are approximately zero because the background shell is
    // in the peak.
    assertFirstFourYValuesCloseToZero(ws);
    assertFlatAfter1(ws);
  }

  void assertFlatAfter1(const MatrixWorkspace_sptr &ws) {
    TSM_ASSERT_DELTA("After 1.0, the signal is flat", ws->y(0)[12], 1000, 1e-6);
    TSM_ASSERT_DELTA("After 1.0, the signal is flat", ws->y(0)[15], 1000, 1e-6)
  }

  void assertFirstFourYValuesCloseToZero(const MatrixWorkspace_sptr &ws) {
    TS_ASSERT_DELTA(ws->y(0)[0], 0, 10);
    TS_ASSERT_DELTA(ws->y(0)[1], 0, 10);
    TS_ASSERT_DELTA(ws->y(0)[2], 0, 10);
    TS_ASSERT_DELTA(ws->y(0)[3], 0, 10);
  }

  void test_NoBackground() {
    MatrixWorkspace_sptr ws = doTest(0, 0, 0, 0);
    // Check the results
    TSM_ASSERT_EQUALS("Two peaks", ws->getNumberHistograms(), 2);
    TSM_ASSERT_EQUALS("16 radii specified", ws->blocksize(), 16);
    TS_ASSERT_DELTA(ws->x(0)[1], 0.1, 1e-6);
    TS_ASSERT_DELTA(ws->x(0)[2], 0.2, 1e-6);

    TS_ASSERT_LESS_THAN(ws->y(0)[5], 1000);
    assertFlatAfter1(ws);
  }
};
