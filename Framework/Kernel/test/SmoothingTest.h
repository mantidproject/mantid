// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Smoothing.h"
#include <cxxtest/TestSuite.h>

#include <cmath>
#include <stdexcept>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Smoothing;

class SmoothingTest : public CxxTest::TestSuite {
public:
  static SmoothingTest *createSuite() { return new SmoothingTest(); }
  static void destroySuite(SmoothingTest *suite) { delete suite; }

  void test_boxcarSmooth_npoints_validation() {
    std::vector<double> input{1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (unsigned npts = 0; npts < 3; npts++) {
      TS_ASSERT_THROWS(boxcarSmooth(input, npts), std::invalid_argument const &);
      TS_ASSERT_THROWS_NOTHING(boxcarSmooth(input, npts + 3);)
    }
  }

  void test_boxcarSmooth_flat() {
    double const flatValue = 1.0;
    std::vector<double> input(20, flatValue);
    std::vector<double> output = boxcarSmooth(input, 3);
    TS_ASSERT_EQUALS(input, output);
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, flatValue);
    }
  }

  void test_boxcarSmooth_two() {
    // a series of values which should smooth out to 2
    std::vector<double> input{1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1};
    std::vector<double> output = boxcarSmooth(input, 3);
    output.pop_back(); // NOTE the last value can't ever equal 2
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, 2.0);
    }
  }

  void test_boxcarSumSquareSmooth_flat() {
    double const flatValue = 2.0; // NOTE using 2 to make sure square operation makes a different value
    std::vector<double> input(20, flatValue);
    std::vector<double> output = boxcarSmooth(input, 3);
    TS_ASSERT_EQUALS(input, output);
    for (double const &x : output) {
      TS_ASSERT_EQUALS(x, flatValue);
    }
  }

  void test_boxcarSumSquareSmooth_two() {
    // a series of values which should sum-square-smooth out to 2
    double const root2 = 1.41421356237309505;
    std::vector<double> input{1, 3, root2, 1, 3, root2, 1, 3, root2, 1, 3, root2, 1};
    std::vector<double> output = boxcarSumSquareSmooth(input, 3);
    output.erase(output.begin()); // NOTE the first value can't ever equal 2
    output.erase(output.end());   // NOTE the last value can't ever equal 2
    int i = 0;
    for (double const &x : output) {
      printf("AT %d\t", i++);
      TS_ASSERT_EQUALS(x, 2.0);
      printf("\n");
    }
  }

  void test_boxcarSmooth() {
    std::vector<double> yVals(10);
    for (int i = 0; i < 10; ++i) {
      yVals[i] = i + 1.0;
    }
    std::vector<double> Y = boxcarSmooth(yVals, 5);
    std::vector<double> expected{2, 2.5, 3, 4, 5, 6, 7, 8, 8.5, 9};

    TS_ASSERT_EQUALS(Y, expected);
  }
};
