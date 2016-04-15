#ifndef MANTID_GEOMETRY_ISHKLTEST_H_
#define MANTID_GEOMETRY_ISHKLTEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <vector>

#include "MantidGeometry/Crystal/IsHKL.h"
#include "MantidKernel/Matrix.h"

using Mantid::Geometry::IsHKL;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::IntMatrix;

template <typename NumericType, typename Derived>
Derived transform(const IsHKL<NumericType, Derived> &hkl) {
  if (!hkl.isZero()) {
    return static_cast<const Derived &>(hkl);
  }

  return Derived(static_cast<NumericType>(4), static_cast<NumericType>(5),
                 static_cast<NumericType>(6));
}

class IntegerHKL : public IsHKL<int, IntegerHKL> {
public:
  using IsHKL<int, IntegerHKL>::IsHKL;
};

class DoubleHKL : public IsHKL<double, DoubleHKL> {
public:
  using IsHKL<double, DoubleHKL>::IsHKL;

  static constexpr double comparison_tolerance = 1e-6;
};

class IsHKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IsHKLTest *createSuite() { return new IsHKLTest(); }
  static void destroySuite(IsHKLTest *suite) { delete suite; }

  void test_is_hkl_constructor() {
    IntegerHKL hkl(0, 1, 2);

    TS_ASSERT_EQUALS(hkl.h(), 0);
    TS_ASSERT_EQUALS(hkl.k(), 1);
    TS_ASSERT_EQUALS(hkl.l(), 2);
  }

  void test_is_hkl_operator_plus_equals() {
    IntegerHKL hkl1(0, 1, 2);
    IntegerHKL hkl2(1, 2, 3);

    hkl1 += hkl2;

    TS_ASSERT_EQUALS(hkl1.h(), 1);
    TS_ASSERT_EQUALS(hkl1.k(), 3);
    TS_ASSERT_EQUALS(hkl1.l(), 5);
  }

  void test_is_hkl_operator_plus() {
    IntegerHKL hkl1(0, 1, 2);
    IntegerHKL hkl2(1, 2, 3);

    IntegerHKL hkl3 = hkl1 + hkl2;

    TS_ASSERT_EQUALS(hkl3.h(), 1);
    TS_ASSERT_EQUALS(hkl3.k(), 3);
    TS_ASSERT_EQUALS(hkl3.l(), 5);
  }

  void test_is_equal() {
    IntegerHKL hkl1(0, 1, 2);
    IntegerHKL hkl2(0, 1, 2);

    TS_ASSERT_EQUALS(hkl1, hkl2);
  }

  void test_is_not_equal() {
    IntegerHKL hkl1(0, 1, 2);
    IntegerHKL hkl2(2, 1, 2);

    TS_ASSERT_DIFFERS(hkl1, hkl2);
  }

  void test_less_than() {
    DoubleHKL hkl1(0, 2, 3);

    TS_ASSERT_LESS_THAN(hkl1, DoubleHKL(0.4, 2, 3));
    TS_ASSERT_LESS_THAN(hkl1, DoubleHKL(0, 2.1, 3));
    TS_ASSERT_LESS_THAN(hkl1, DoubleHKL(1.0, -2.1, 3));
    TS_ASSERT_LESS_THAN(hkl1, DoubleHKL(0, 2, 3.0 + 1.e-5));
  }

  void test_generic_implementation() {
    TS_ASSERT_EQUALS(transform(DoubleHKL(3, 4, 5)), DoubleHKL(3, 4, 5));
    TS_ASSERT_EQUALS(transform(IntegerHKL(3, 4, 5)), IntegerHKL(3, 4, 5));
  }
};

class IsHKLTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IsHKLTestPerformance *createSuite() {
    return new IsHKLTestPerformance();
  }
  static void destroySuite(IsHKLTestPerformance *suite) { delete suite; }

  IsHKLTestPerformance() {
    std::fill_n(std::back_inserter(integerHKLs), numHKLs, IntegerHKL(2, 1, 2));
    std::fill_n(std::back_inserter(doubleHKLs), numHKLs, DoubleHKL(2, 1, 2));
  }

  void test_equals_integer() {
    IntegerHKL hkl(2, 1, 3);
    bool test =
        std::all_of(integerHKLs.cbegin(), integerHKLs.cend(),
                    [&hkl](const IntegerHKL &lhs) { return lhs != hkl; });

    TS_ASSERT(test);
  }

  void test_less_than_integer() {
    IntegerHKL hkl(2, 1, -3);
    bool test =
        std::all_of(integerHKLs.cbegin(), integerHKLs.cend(),
                    [&hkl](const IntegerHKL &lhs) { return hkl < lhs; });

    TS_ASSERT(test);
  }

  void test_equals_double() {
    DoubleHKL hkl(2, 1, 3);
    bool test =
        std::all_of(doubleHKLs.cbegin(), doubleHKLs.cend(),
                    [&hkl](const DoubleHKL &lhs) { return lhs != hkl; });

    TS_ASSERT(test);
  }

  void test_matrix_multiply_double() {
    DblMatrix m(3, 3, false);
    m[0][1] = 1.0;
    m[1][0] = -1.0;
    m[2][2] = 1.0;

    std::vector<DoubleHKL> transformed(doubleHKLs.size());

    for (size_t i = 0; i < 10; ++i) {
      std::transform(doubleHKLs.cbegin(), doubleHKLs.cend(),
                     transformed.begin(),
                     [&m](const DoubleHKL &hkl) { return m * hkl; });
      TS_ASSERT_EQUALS(transformed.size(), doubleHKLs.size());
    }
  }

  void test_matrix_multiply_int() {
    IntMatrix m(3, 3, false);
    m[0][1] = 1;
    m[1][0] = -1;
    m[2][2] = 1;

    std::vector<IntegerHKL> transformed(integerHKLs.size());

    for (size_t i = 0; i < 10; ++i) {
      std::transform(integerHKLs.cbegin(), integerHKLs.cend(),
                     transformed.begin(),
                     [&m](const IntegerHKL &hkl) { return m * hkl; });
      TS_ASSERT_EQUALS(transformed.size(), integerHKLs.size());
    }
  }

private:
  std::vector<IntegerHKL> integerHKLs;
  std::vector<DoubleHKL> doubleHKLs;

  size_t numHKLs = 10000000;
};

#endif /* MANTID_GEOMETRY_ISHKLTEST_H_ */
