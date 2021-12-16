// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/MuonHelpers.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid::CurveFitting::MuonHelper;

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
};
