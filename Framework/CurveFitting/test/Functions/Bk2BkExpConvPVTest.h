// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidCurveFitting/Functions/BackToBackExponential.h"
#include "MantidCurveFitting/Functions/Bk2BkExpConvPV.h"

using namespace Mantid::CurveFitting::Functions;

class Bk2BkExpConvPVTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Bk2BkExpConvPVTest *createSuite() { return new Bk2BkExpConvPVTest(); }
  static void destroySuite(Bk2BkExpConvPVTest *suite) { delete suite; }

  void test_cateogry() {
    Bk2BkExpConvPV fn;
    TS_ASSERT_EQUALS(fn.category(), "Peak");
  }

  void test_function_evaluation_gamma_equal_zero() {

    Bk2BkExpConvPV peak;
    peak.initialize();
    peak.setParameter("Intensity", 100.0);
    peak.setParameter("X0", 400.0);
    peak.setParameter("Alpha", 1.0);
    peak.setParameter("Beta", 1.5);
    peak.setParameter("Sigma2", 200.0);
    peak.setParameter("Gamma", 0.0);

    Mantid::API::FunctionDomain1DVector x(300, 500, 100);
    Mantid::API::FunctionValues y(x);

    // We should get a peak at ~400
    TS_ASSERT_THROWS_NOTHING(peak.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.0000, 1e-4);
    TS_ASSERT_DELTA(y[50], 2.7983, 1e-4);
    TS_ASSERT_DELTA(y[99], 0.0000, 1e-4);
  }

  void test_function_evaluation_gamma_greater_than_zero() {

    Bk2BkExpConvPV peak;
    peak.initialize();
    peak.setParameter("Intensity", 100.0);
    peak.setParameter("X0", 400.0);
    peak.setParameter("Alpha", 1.0);
    peak.setParameter("Beta", 1.5);
    peak.setParameter("Sigma2", 200.0);
    peak.setParameter("Gamma", 1.0);

    Mantid::API::FunctionDomain1DVector x(300, 500, 100);
    Mantid::API::FunctionValues y(x);

    // We should get a peak at ~400
    TS_ASSERT_THROWS_NOTHING(peak.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.004167, 1e-6);
    TS_ASSERT_DELTA(y[50], 2.7231, 1e-4);
    TS_ASSERT_DELTA(y[99], 0.004113, 1e-6);
  }

  void test_function_evaluation_compare_to_b2bExp() {

    // When gamma is 0, `Bk2BkExpConvPV` should give same result
    // as `BackToBackExponential`.
    Bk2BkExpConvPV peak;
    BackToBackExponential peak_b2bExp;

    peak.initialize();
    peak.setParameter("Intensity", 100.0);
    peak.setParameter("X0", 400.0);
    peak.setParameter("Alpha", 1.0);
    peak.setParameter("Beta", 1.5);
    peak.setParameter("Sigma2", 200.0);
    peak.setParameter("Gamma", 0.0);

    peak_b2bExp.initialize();
    peak_b2bExp.setParameter("I", 100.0);
    peak_b2bExp.setParameter("X0", 400.0);
    peak_b2bExp.setParameter("A", 1.0);
    peak_b2bExp.setParameter("B", 1.5);
    // Attention here to the sigma parameter for `BackToBackExponential`
    // and sigma^2 parameter for `Bk2BkExpConvPV`. We may need to change
    // the input parameter for `Bk2BkExpConvPV` to be sigma as well.
    peak_b2bExp.setParameter("S", sqrt(200.0));

    Mantid::API::FunctionDomain1DVector x(300, 500, 100);
    Mantid::API::FunctionValues y(x);
    Mantid::API::FunctionValues y_b2bExp(x);

    TS_ASSERT_THROWS_NOTHING(peak_b2bExp.function(x, y));
    TS_ASSERT_THROWS_NOTHING(peak_b2bExp.function(x, y_b2bExp));
    TS_ASSERT_DELTA(y[0], y_b2bExp[0], 1e-4);
    TS_ASSERT_DELTA(y[50], y_b2bExp[50], 1e-4);
    TS_ASSERT_DELTA(y[99], y_b2bExp[99], 1e-4);
  }
};
