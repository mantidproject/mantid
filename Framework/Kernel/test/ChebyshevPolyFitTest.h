// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Math/ChebyshevPolyFit.h"

using Mantid::Kernel::ChebyshevPolyFit;

class ChebyshevPolyFitTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChebyshevPolyFitTest *createSuite() { return new ChebyshevPolyFitTest(); }
  static void destroySuite(ChebyshevPolyFitTest *suite) { delete suite; }

  void test_case_with_quadratic_polynomial() {
    const std::vector<double> xs = {1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> ys = {1.0, 4.0, 9.0, 16.0, 25.0};
    const std::vector<double> wgts = {1.0, 0.9, 0.5, 0.2, 0.05};

    ChebyshevPolyFit polyfit(2);
    auto coeffs = polyfit(xs, ys, wgts);

    TS_ASSERT_EQUALS(3, coeffs.size());
    const double delta(1e-8);
    TS_ASSERT_DELTA(11, coeffs[0], delta);
    TS_ASSERT_DELTA(12, coeffs[1], delta);
    TS_ASSERT_DELTA(2, coeffs[2], delta);
  }
};
