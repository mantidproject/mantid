// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <ostream>
#include <sstream>
#include <vector>

#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Logger.h"

using namespace Mantid;
using namespace mathSupport;
class MathSupportTest : public CxxTest::TestSuite {
public:
  void testSolveQuadratic() { // Test quadratic solution
    std::pair<std::complex<double>, std::complex<double>> output;

    // y = (x+1)^2 one solution
    double test1[3] = {1, 2, 1};
    TS_ASSERT_EQUALS(solveQuadratic(test1, output), 1);
    std::pair<std::complex<double>, std::complex<double>> result1(std::complex<double>(-1.0, 0.0),
                                                                  std::complex<double>(-1.0, 0.0));
    TS_ASSERT_EQUALS(output, result1);

    // y = x^2 -1
    // two rational roots
    double test2[3] = {1, 0, -1};
    TS_ASSERT_EQUALS(solveQuadratic(test2, output), 2);
    std::pair<std::complex<double>, std::complex<double>> result2(std::complex<double>(-1.0, 0.0),
                                                                  std::complex<double>(1.0, 0.0));
    TS_ASSERT_EQUALS(output, result2);

    // y = x^2 + 1
    // two complex roots
    double test3[3] = {1, 0, 1};
    TS_ASSERT_EQUALS(solveQuadratic(test3, output), 2);
    std::pair<std::complex<double>, std::complex<double>> result3(std::complex<double>(0.0, -1.0),
                                                                  std::complex<double>(0.0, 1.0));
    TS_ASSERT_EQUALS(output, result3);

    // y = x
    // solution is 0 (only one root)
    double test4[3] = {0, 1, 0};
    TS_ASSERT_EQUALS(solveQuadratic(test4, output), 1);
    std::pair<std::complex<double>, std::complex<double>> result4(std::complex<double>(0.0, 0.0),
                                                                  std::complex<double>(0.0, 0.0));
    TS_ASSERT_EQUALS(output, result4);

    // y = x - 1
    // solution is 1
    double test5[3] = {0, 1, -1};
    TS_ASSERT_EQUALS(solveQuadratic(test5, output), 1);
    std::pair<std::complex<double>, std::complex<double>> result5(std::complex<double>(1.0, 0.0),
                                                                  std::complex<double>(1.0, 0.0));
    TS_ASSERT_EQUALS(output, result5);

    // y = x^2
    // solution is 0
    double test6[3] = {1, 0, 0};
    TS_ASSERT_EQUALS(solveQuadratic(test6, output), 1);
    std::pair<std::complex<double>, std::complex<double>> result6(std::complex<double>(0.0, 0.0),
                                                                  std::complex<double>(0.0, 0.0));
    TS_ASSERT_EQUALS(output, result6);

    // y = x*(x-1)
    // solution is 0 and 1
    double test7[3] = {1, -1, 0};
    TS_ASSERT_EQUALS(solveQuadratic(test7, output), 2);
    std::pair<std::complex<double>, std::complex<double>> result7(std::complex<double>(1.0, 0.0),
                                                                  std::complex<double>(0.0, 0.0));
    TS_ASSERT_EQUALS(output, result7);
  }

  void testSolveCubic() {
    double test[4] = {1.0, 6.0, -4.0, -24.0};
    std::complex<double> root1, root2, root3;
    TS_ASSERT_EQUALS(solveCubic(test, root1, root2, root3), 3);
    TS_ASSERT_DELTA(root1.real(), -6, 0.0000001);
    TS_ASSERT_DELTA(root2.real(), 2, 0.000001);
    TS_ASSERT_DELTA(root3.real(), -2.0, 0.000001);
    TS_ASSERT_DELTA(root1.imag(), 0, 0.0000001);
    TS_ASSERT_DELTA(root2.imag(), 0, 0.000001);
    TS_ASSERT_DELTA(root3.imag(), 0.0, 0.000001);

    double test2[4] = {1.0, -11.0, 49.0, -75.0};
    TS_ASSERT_EQUALS(solveCubic(test2, root1, root2, root3), 3);
    TS_ASSERT_DELTA(root1.real(), 3, 0.0000001);
    TS_ASSERT_DELTA(root2.real(), 4, 0.000001);
    TS_ASSERT_DELTA(root3.real(), 4, 0.000001);
    TS_ASSERT_DELTA(root1.imag(), 0, 0.0000001);
    TS_ASSERT_DELTA(root2.imag(), 3, 0.000001);
    TS_ASSERT_DELTA(root3.imag(), -3, 0.000001);
  }
};
