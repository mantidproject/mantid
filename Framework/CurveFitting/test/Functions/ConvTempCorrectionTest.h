// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Functions/ConvTempCorrection.h"
using Mantid::CurveFitting::Functions::ConvTempCorrection;

class ConvTempCorrectionTest : public CxxTest::TestSuite {
public:
  void test_function_gives_expected_value_for_given_input() {
    ConvTempCorrection fn;
    fn.initialize();
    fn.setParameter("Temperature", 10);

    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(-2, 2, 10);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.2526368276, 1e-4);
    TS_ASSERT_DELTA(y[2], 0.4901068378, 1e-4);
    TS_ASSERT_DELTA(y[4], 0.8765814870, 1e-4);
    TS_ASSERT_DELTA(y[6], 1.4362644888, 1e-4);
    TS_ASSERT_DELTA(y[8], 2.1606084558, 1e-4);
  }
  void test_function_handles_zero_x_val() {
    ConvTempCorrection fn;
    fn.initialize();
    fn.setParameter("Temperature", 10);
    // Define 1d domain
    Mantid::API::FunctionDomain1DVector x(-1, 1, 3);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[1], 1, 1e-4);
  }
};
