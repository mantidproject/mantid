#ifndef MANTID_GEOMETRY_ISHKLTEST_H_
#define MANTID_GEOMETRY_ISHKLTEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "MantidGeometry/Crystal/IsHKL.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

using Mantid::Geometry::IsHKL;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::IntMatrix;
using Mantid::Kernel::V3D;

template <typename NumericType, typename Derived>
Derived transform(const IsHKL<NumericType, Derived> &hkl) {
  if (!hkl.isZero()) {
    return static_cast<const Derived &>(hkl);
  }

  return Derived(static_cast<NumericType>(4), static_cast<NumericType>(5),
                 static_cast<NumericType>(6));
}

template <typename NumericType, typename Derived>
Derived higherOrderHKL(const IsHKL<NumericType, Derived> &hkl,
                       const NumericType &order) {
  Derived higherOrder;
  std::transform(hkl.cbegin(), hkl.cend(), higherOrder.begin(),
                 [=](const NumericType &index) { return index * order; });

  return higherOrder;
}

class IntegerHKL : public IsHKL<int, IntegerHKL> {
public:
  using IsHKL<int, IntegerHKL>::IsHKL;

  bool operator<(const IntegerHKL &other) const {
    auto mismatch = std::mismatch(cbegin(), cend(), other.cbegin());

    return mismatch.first != cend() && *(mismatch.first) < *(mismatch.second);
  }
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

    TS_ASSERT_EQUALS(higherOrderHKL(IntegerHKL(3, 4, 5), 2),
                     IntegerHKL(6, 8, 10));
    TS_ASSERT_EQUALS(higherOrderHKL(DoubleHKL(3, 4, 5), 2.0),
                     DoubleHKL(6, 8, 10));
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
    std::fill_n(std::back_inserter(v3dHKLs), numHKLs, V3D(2, 1, 2));
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

  void test_less_than_double() {
    DoubleHKL hkl(2, 1, -3);
    bool test = std::all_of(doubleHKLs.cbegin(), doubleHKLs.cend(),
                            [&hkl](const DoubleHKL &lhs) { return hkl < lhs; });

    TS_ASSERT(test);
  }

  void test_equals_V3D() {
    V3D hkl(2, 1, 3);
    bool test = std::all_of(v3dHKLs.cbegin(), v3dHKLs.cend(),
                            [&hkl](const V3D &lhs) { return lhs != hkl; });

    TS_ASSERT(test);
  }

  void test_less_than_V3D() {
    V3D hkl(2, 1, -3);
    bool test = std::all_of(v3dHKLs.cbegin(), v3dHKLs.cend(),
                            [&hkl](const V3D &lhs) { return hkl < lhs; });

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

  void test_matrix_multiply_v3d() {
    DblMatrix m(3, 3, false);
    m[0][1] = 1.0;
    m[1][0] = -1.0;
    m[2][2] = 1.0;

    std::vector<V3D> transformed(v3dHKLs.size());

    for (size_t i = 0; i < 10; ++i) {
      std::transform(v3dHKLs.cbegin(), v3dHKLs.cend(), transformed.begin(),
                     [&m](const V3D &hkl) { return m * hkl; });
      TS_ASSERT_EQUALS(transformed.size(), v3dHKLs.size());
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
  std::vector<V3D> v3dHKLs;

  size_t numHKLs = 10000000;
};

#endif /* MANTID_GEOMETRY_ISHKLTEST_H_ */
