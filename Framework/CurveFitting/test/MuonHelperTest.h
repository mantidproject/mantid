// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/MuonHelpers.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
// #include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::CurveFitting::MuonHelper;
// using namespace Mantid::API;

class MuonHelperTest : public CxxTest::TestSuite {
public:
  void testgetAz() {
    double xValue1 = 2.0;
    const double charField1 = 5.0;
    double Az1 = getAz(xValue1, charField1);
    TS_ASSERT_DELTA(Az1, 0.3551823638, 1e-8);

    double xValue2 = 2.4;
    double charField2 = 1.5;
    double Az2 = getAz(xValue2, charField2);
    TS_ASSERT_DELTA(Az2, 0.7612450528, 1e-8);
  }

  void testgetDiffAz() {
    double xValue1 = 100.0;
    const double charField1 = 5.0;
    double DiffAz1 = getDiffAz(xValue1, charField1);
    TS_ASSERT_DELTA(DiffAz1, -0.0004159996, 1e-8);

    double xValue2 = 0.1;
    double charField2 = 100.0;
    double DiffAz2 = getDiffAz(xValue2, charField2);
    TS_ASSERT_DELTA(DiffAz2, 0.0, 1e-8);
  }

  void testgetActivationFunc() {
    double xValue1 = 100.0;
    const double attemptRate1 = 5.0;
    const double barrier1 = 14.0;
    const double unitMultiply1 = 1.0;
    double ActivFunc1 = getActivationFunc(xValue1, attemptRate1, barrier1, unitMultiply1);
    TS_ASSERT_DELTA(ActivFunc1, 4.3467911769, 1e-8);

    double xValue2 = 20.0;
    const double attemptRate2 = 5.0;
    const double barrier2 = 14.0;
    const double unitMultiply2 = Mantid::PhysicalConstants::meVtoKelvin;
    double ActivFunc2 = getActivationFunc(xValue2, attemptRate2, barrier2, unitMultiply2);
    TS_ASSERT_DELTA(ActivFunc2, 0.0014829448, 1e-8);
  }

  void testgetAttemptRateDiff() {
    double xValue1 = 100.0;
    const double barrier1 = 14.0;
    const double unitMultiply1 = 1.0;
    double AttemptRateDiff1 = getAttemptRateDiff(xValue1, barrier1, unitMultiply1);
    TS_ASSERT_DELTA(AttemptRateDiff1, 0.8693582353, 1e-8);

    double xValue2 = 20.0;
    const double barrier2 = 14.0;
    const double unitMultiply2 = Mantid::PhysicalConstants::meVtoKelvin;
    double AttemptRateDiff2 = getAttemptRateDiff(xValue2, barrier2, unitMultiply2);
    TS_ASSERT_DELTA(AttemptRateDiff2, 0.0002965889, 1e-8);
  }

  void testgetBarrierDiff() {
    double xValue1 = 100.0;
    const double attemptRate1 = 5.0;
    const double barrier1 = 14.0;
    const double unitMultiply1 = 1.0;
    double BarrierDiff1 = getBarrierDiff(xValue1, attemptRate1, barrier1, unitMultiply1);
    TS_ASSERT_DELTA(BarrierDiff1, -0.0434679117, 1e-8);

    double xValue2 = 20.0;
    const double attemptRate2 = 5.0;
    const double barrier2 = 14.0;
    const double unitMultiply2 = Mantid::PhysicalConstants::meVtoKelvin;
    double BarrierDiff2 = getBarrierDiff(xValue2, attemptRate2, barrier2, unitMultiply2);
    TS_ASSERT_DELTA(BarrierDiff2, -0.0008604430, 1e-8);
  }
};
