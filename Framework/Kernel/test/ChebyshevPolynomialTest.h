// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Math/Distributions/ChebyshevPolynomial.h"

using Mantid::Kernel::ChebyshevPolynomial;

class ChebyshevPolynomialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChebyshevPolynomialTest *createSuite() { return new ChebyshevPolynomialTest(); }
  static void destroySuite(ChebyshevPolynomialTest *suite) { delete suite; }

  void test_values_are_correct_for_x_in_range() {
    const double delta(1e-12);

    // The exapansions for the polynomials were found at
    // http://mathworld.wolfram.com/ChebyshevPolynomialoftheFirstKind.html
    ChebyshevPolynomial chebp;
    const double x(0.75), xsq(x * x);
    TS_ASSERT_DELTA(1.0, chebp(0, x), delta);
    TS_ASSERT_DELTA(x, chebp(1, x), delta);
    TS_ASSERT_DELTA(2. * xsq - 1., chebp(2, x), delta);
    TS_ASSERT_DELTA(4. * x * xsq - 3. * x, chebp(3, x), delta);
    TS_ASSERT_DELTA(8. * xsq * xsq - 8. * xsq + 1.0, chebp(4, x), delta);
  }

  // No test for out of range as there is an assert in there..
};
