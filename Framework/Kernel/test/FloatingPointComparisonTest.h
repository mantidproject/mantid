#ifndef MANTID_KERNEL_FLOATINGPOINTCOMPARISONTEST_H_
#define MANTID_KERNEL_FLOATINGPOINTCOMPARISONTEST_H_

#include "MantidKernel/FloatingPointComparison.h"
#include <cfloat>
#include <cxxtest/TestSuite.h>
#include <limits>

class FloatingPointComparisonTest : public CxxTest::TestSuite {
public:
  void test_Same_Value_Compare_Equal() {
    TS_ASSERT(Mantid::Kernel::equals(2.5, 2.5));
  }

  void test_Difference_By_Machine_Eps_Compare_Equal() {
    TS_ASSERT(Mantid::Kernel::equals(
        2.5, 2.5 + std::numeric_limits<double>::epsilon()));
  }

  void test_Difference_By_Machine_Eps_Plus_Small_Does_Not_Compare_Equal() {
    TS_ASSERT_EQUALS(
        Mantid::Kernel::equals(
            2.5, 2.5 + 1.1 * std::numeric_limits<double>::epsilon()),
        false);
  }

  void test_Same_Large_Numbers_Compare_Equal() {
    TS_ASSERT(Mantid::Kernel::equals(DBL_MAX, DBL_MAX));
  }

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
};

#endif // MANTID_KERNEL_FLOATINGPOINTCOMPARISONTEST_H_