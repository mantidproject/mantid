// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/FloatingPointComparison.h"
#include <cfloat>
#include <cxxtest/TestSuite.h>
#include <limits>

class FloatingPointComparisonTest : public CxxTest::TestSuite {
public:
  void test_Same_Value_Compare_Equal() { TS_ASSERT(Mantid::Kernel::equals(2.5, 2.5)); }

  void test_Difference_By_Machine_Eps_Compare_Equal() {
    TS_ASSERT(Mantid::Kernel::equals(2.5, 2.5 + std::numeric_limits<double>::epsilon()));
  }

  void test_Difference_By_Machine_Eps_Plus_Small_Does_Not_Compare_Equal() {
    TS_ASSERT_EQUALS(Mantid::Kernel::equals(2.5, 2.5 + 1.1 * std::numeric_limits<double>::epsilon()), false);
  }

  void test_Same_Large_Numbers_Compare_Equal() { TS_ASSERT(Mantid::Kernel::equals(DBL_MAX, DBL_MAX)); }

  void test_Numbers_Outside_Custom_Tolerance_Are_Not_Equal() {
    const double tol(1e-08);
    TS_ASSERT_EQUALS(Mantid::Kernel::equals(0.1, 1.0001 * tol), false);
  }

  void test_LtEquals_With_X_Equal_To_Y_Produces_True() {
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(0.1, 0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(-0.1, -0.1), true);
  }

  void test_LtEquals_With_X_Lower_Than_Y_Produces_True() {
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(0.1, 0.2), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(-0.1, 0.2), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(-5.0, -0.2), true);
  }

  void test_LtEquals_With_X_Greater_Than_Y_Produces_False() {
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(0.5, 0.2), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(-0.1, -0.9), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(50.0, -0.00002), false);
  }

  void test_GtEquals_With_X_Equal_To_Y_Produces_True() {
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(0.1, 0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(-0.1, -0.1), true);
  }

  void test_GtEquals_With_X_Greater_Than_Y_Produces_True() {
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(0.2, 0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(0.2, -0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(8.0, -5.0), true);
  }

  void test_GtEquals_With_X_Lower_Than_Y_Produces_False() {
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(1.01, 50.23), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(-5.56, 0.23), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(-0.00002, -0.00001), false);
  }

  void test_absoluteDifference() {
    constexpr double left = 1.1, right = 1.0;
    // test value
    TS_ASSERT_EQUALS(Mantid::Kernel::absoluteDifference(left, right), std::abs(left - right));
    // test positive-definiteness
    TS_ASSERT_LESS_THAN(0.0, Mantid::Kernel::absoluteDifference(left, -right));
    TS_ASSERT_LESS_THAN(0.0, Mantid::Kernel::absoluteDifference(-left, right));
    TS_ASSERT_LESS_THAN(0.0, Mantid::Kernel::absoluteDifference(-left, -right));
    // test symmetry
    TS_ASSERT_EQUALS(Mantid::Kernel::absoluteDifference(left, right), Mantid::Kernel::absoluteDifference(right, left));
  }

  void test_relativeDifference() {
    constexpr double point3 = 0.3, notquitepoint3 = 0.2 + 0.1;
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(point3, notquitepoint3), 0.0);
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(2.3, 2.3), 0.0);
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(2.3e208, 2.3e208), 0.0);
    // check no errors using zero
    TS_ASSERT_THROWS_NOTHING(Mantid::Kernel::relativeDifference(0.0, 0.0))
    TS_ASSERT(!std::isnan(Mantid::Kernel::relativeDifference(0.0, 0.0)));
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(0.0, 0.0), 0.0);
    // check no errors using machine epsiolon
    constexpr double realsmall = std::numeric_limits<double>::epsilon();
    TS_ASSERT_THROWS_NOTHING(Mantid::Kernel::relativeDifference(0.0, realsmall))
    TS_ASSERT(!std::isnan(Mantid::Kernel::relativeDifference(0.0, realsmall)));
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(0.0, realsmall), 0.0);
    // check we get correct values for normal situations
    constexpr double left = 2.6, right = 2.7;
    constexpr double reldiff = 2.0 * std::fabs(left - right) / (left + right);
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(left, right), reldiff);
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(right, left), reldiff);
  }

  void test_withinAbsoluteDifference() {
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.3, 0.2, 0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.3, 0.1, 0.1), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.01, 0.011, 0.01), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.01, -0.011, 0.01), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(100.1, 100.15, 0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(12345678923456.789, 12345679023456.788, 0.0001), false);
    constexpr double anan = std::numeric_limits<double>::quiet_NaN();
    // TS_ASSERT_EQUALS( Mantid::Kernel::withinAbsoluteDifference(anan, 0.3, 0.1), false);
  }

  void test_withinRelativeDifference() {
    // some cases with zero difference
    constexpr double point3 = 0.3, notquitepoint3 = 0.2 + 0.1;
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(point3, notquitepoint3, 1.e-307), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3, 2.3, 1.e-307), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3e208, 2.3e208, 1.e-307), true);
    // case of large magnitude values -- even though abs diff would always fail, rel diff can still pass
    // passing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(2.31e208, 2.32e208, 0.01), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.31e208, 2.32e208, 0.01), true);
    // failing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3e208, 2.4e208, 0.01), false);
    // case of small magnitude values -- even though abs diff would always pass, rel diff still can fail
    // passing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(2.31e-10, 2.32e-10, 0.01), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.31e-10, 2.32e-10, 0.01), true);
    // failing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(2.3e-10, 2.4e-10, 0.01), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3e-10, 2.4e-10, 0.01), false);
    // case of normal-sized values
    constexpr double left = 2.6, right = 2.7, far = 3.0;
    constexpr double reldiff = 2.0 * std::fabs(left - right) / (left + right);
    constexpr double tolerance = 1.01 * reldiff;
    // passing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(left, right, tolerance), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(right, left, tolerance), true);
    // failing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(left, far, tolerance), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(far, left, tolerance), false);
  }
};
