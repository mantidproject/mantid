// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Math/Distributions/ChebyshevSeries.h"

using Mantid::Kernel::ChebyshevSeries;

class ChebyshevSeriesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChebyshevSeriesTest *createSuite() { return new ChebyshevSeriesTest(); }
  static void destroySuite(ChebyshevSeriesTest *suite) { delete suite; }

  void test_expected_values_for_x_in_range() {
    const double delta(1e-12);
    std::vector<double> coeffs = {0.5, 2.4, -3.2, -1.7, 2.1};

    const double x(0.75);
    // Expected values computed on paper using reccurrence relation
    // http://docs.scipy.org/doc/numpy-dev/reference/generated/numpy.polynomial.chebyshev.chebval.html#numpy.polynomial.chebyshev.chebval
    TS_ASSERT_DELTA(0.5, ChebyshevSeries(0)(coeffs, x), delta);
    TS_ASSERT_DELTA(2.3, ChebyshevSeries(1)(coeffs, x), delta);
    TS_ASSERT_DELTA(1.9, ChebyshevSeries(2)(coeffs, x), delta);
    TS_ASSERT_DELTA(2.85625, ChebyshevSeries(3)(coeffs, x), delta);
    TS_ASSERT_DELTA(0.821875, ChebyshevSeries(4)(coeffs, x), delta);
  }
};
