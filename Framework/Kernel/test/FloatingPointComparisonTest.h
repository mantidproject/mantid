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
    double const a = 0x1.4p1; // i.e. 2.5
    // increase by the machine precision
    double const diff = std::ldexp(std::numeric_limits<double>::epsilon(), std::ilogb(a));
    TS_ASSERT_DIFFERS(a, a + diff);
    TS_ASSERT(Mantid::Kernel::equals(a, a + diff));
  }

  void test_Difference_By_Machine_Eps_Plus_Small_Does_Not_Compare_Equal() {
    double const a = 0x1.4p1; // i.e. 2.5
    // as above, but increase by twice the machine precision
    double const diff = std::ldexp(std::numeric_limits<double>::epsilon(), std::ilogb(a) + 1);
    TS_ASSERT_DIFFERS(a, a + diff);
    TS_ASSERT(!Mantid::Kernel::equals(a, a + diff));
  }

  void test_Similar_Small_Numbers_Compare_Equal() {
    double const a = 0x1p-100;
    // increase by the machine precision
    double const diff = std::ldexp(std::numeric_limits<double>::epsilon(), std::ilogb(a));
    TS_ASSERT_DIFFERS(a, a + diff);
    TS_ASSERT(Mantid::Kernel::equals(a, a + diff));
  }

  void test_Different_Small_Numbers_Do_Not_Compare_Equal() {
    // two small but machine-distinguishable numbers
    double const a = 0x1.0p-100; // 1.0 * 2^{-100}
    double const b = 0x1.8p-100; // 1.5 * 2^{-100}
    double const diff = std::abs(a - b);
    // the difference is less than machine epsilon (when scaled to 1)
    TS_ASSERT_LESS_THAN(diff, std::numeric_limits<double>::epsilon());
    // ne'ertheless, the numbers compare different
    TS_ASSERT(!Mantid::Kernel::equals(a, b))
  }

  void test_Same_Large_Numbers_Compare_Equal() { TS_ASSERT(Mantid::Kernel::equals(DBL_MAX, DBL_MAX)); }

  void test_Similar_Large_Numbers_Compare_Equal() {
    double const a = DBL_MAX / 2;
    double const diff = std::ldexp(std::numeric_limits<double>::epsilon(), std::ilogb(a));
    // the difference is a sizeable number and not by itself insignificant
    TS_ASSERT_LESS_THAN(0x1.p50, diff);
    // the numbers are technicaly different
    TS_ASSERT_DIFFERS(a, a + diff);
    // but they compare different
    TS_ASSERT(Mantid::Kernel::equals(a, a + diff));
  }

  void test_Different_Large_Numbers_Do_Not_Compare_Equal() {
    double const a = DBL_MAX / 2;
    // as above, but increase by twice the machine precision
    double const diff = std::ldexp(std::numeric_limits<double>::epsilon(), std::ilogb(a) + 1);
    TS_ASSERT_LESS_THAN(0x1.p50, diff);
    TS_ASSERT_DIFFERS(a, a + diff);
    TS_ASSERT(Mantid::Kernel::equals(a, a + diff));
  }

  void test_Numbers_Outside_Custom_Tolerance_Are_Not_Equal() {
    const double tol(1e-08);
    TS_ASSERT_EQUALS(Mantid::Kernel::equals(0.1, 1.0001 * tol), false);
  }

  void test_with_NaN() {
    // everything compares false with an NaN
    constexpr double anan = std::numeric_limits<double>::quiet_NaN();
    constexpr double bnan = std::numeric_limits<double>::quiet_NaN();
    constexpr double real = 3.0;
    // equals
    TS_ASSERT_EQUALS(Mantid::Kernel::equals(anan, real), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::equals(real, anan), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::equals(anan, bnan), false);
    // ltEquals
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(anan, real), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(real, anan), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::ltEquals(anan, bnan), false);
    // gtEquals
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(anan, real), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(real, anan), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::gtEquals(anan, bnan), false);
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
    // absolute difference with NaN is NaN
    constexpr double anan = std::numeric_limits<double>::quiet_NaN();
    constexpr double bnan = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT(std::isnan(Mantid::Kernel::absoluteDifference(left, anan)));
    TS_ASSERT(std::isnan(Mantid::Kernel::absoluteDifference(bnan, anan)));
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
    // check no errors using machine epsilon
    constexpr double realsmall = std::numeric_limits<double>::epsilon();
    TS_ASSERT_THROWS_NOTHING(Mantid::Kernel::relativeDifference(0.0, realsmall))
    TS_ASSERT(!std::isnan(Mantid::Kernel::relativeDifference(0.0, realsmall)));
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(0.0, realsmall), 0.0);
    // check we get correct values for normal situations
    const double left = 2.6, right = 2.7;
    const double reldiff = 2.0 * std::abs(left - right) / (left + right);
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(left, right), reldiff);
    TS_ASSERT_EQUALS(Mantid::Kernel::relativeDifference(right, left), reldiff);
    // relative difference with NaN is NaN
    constexpr double anan = std::numeric_limits<double>::quiet_NaN();
    constexpr double bnan = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT(std::isnan(Mantid::Kernel::relativeDifference(left, anan)));
    TS_ASSERT(std::isnan(Mantid::Kernel::absoluteDifference(bnan, anan)));
  }

  void test_withinAbsoluteDifference() {
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.3, 0.2, 0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.3, 0.1, 0.1), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.01, 0.011, 0.01), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(0.01, -0.011, 0.01), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(100.1, 100.15, 0.1), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(12345678923456.789, 12345679023456.788, 0.0001), false);
    // case of NaNs -- nothing is close to an NaN
    constexpr double anan = std::numeric_limits<double>::quiet_NaN();
    constexpr double bnan = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(anan, 0.3, 0.1), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(anan, bnan, 0.1), false);
  }

  void test_withinRelativeDifference() {
    // things difference at machine epsilon are equal
    const double point3 = 0.3, notquitepoint3 = 0.2 + 0.1;
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(point3, notquitepoint3, 1.e-307), true);
    // some cases with zero difference
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3, 2.3, 1.e-307), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3e208, 2.3e208, 1.e-307), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3e-208, 2.3e-208, 0.0), true);
    // case of large magnitude values -- even though abs diff would always fail, rel diff can still pass
    //  - passing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(2.31e208, 2.32e208, 0.01), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.31e208, 2.32e208, 0.01), true);
    //  - failing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3e208, 2.4e208, 0.01), false);
    // case of small magnitude values -- even though abs diff would always pass, rel diff still can fail
    //  - passing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(2.31e-10, 2.32e-10, 0.01), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.31e-10, 2.32e-10, 0.01), true);
    //  - failing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinAbsoluteDifference(2.3e-10, 2.4e-10, 0.01), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(2.3e-10, 2.4e-10, 0.01), false);
    // case of normal-sized values
    const double left = 2.6, right = 2.7, far = 3.0;
    const double reldiff = 2.0 * std::abs(left - right) / (left + right);
    const double tolerance = 1.01 * reldiff;
    //  - passing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(left, right, tolerance), true);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(right, left, tolerance), true);
    //  - failing
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(left, far, tolerance), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(far, left, tolerance), false);
    // case of NaNs -- nothing is close to an NaN
    constexpr double anan = std::numeric_limits<double>::quiet_NaN();
    constexpr double bnan = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(anan, 0.3, 0.1), false);
    TS_ASSERT_EQUALS(Mantid::Kernel::withinRelativeDifference(anan, bnan, 0.1), false);
  }
};
