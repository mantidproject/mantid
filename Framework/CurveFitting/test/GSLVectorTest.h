#ifndef GSLVECTORTEST_H_
#define GSLVECTORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/GSLVector.h"

using namespace Mantid::CurveFitting;

class GSLVectorTest : public CxxTest::TestSuite {
public:
  void test_create_GSLVector() {
    {
      GSLVector v;
      TS_ASSERT_EQUALS(v.size(), 1);
    }
    {
      GSLVector v(1);
      TS_ASSERT_EQUALS(v.size(), 1);
    }
    {
      GSLVector v(2);
      TS_ASSERT_EQUALS(v.size(), 2);
    }
  }

  void test_create_from_std_vector() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    GSLVector gv(v);
    TS_ASSERT_EQUALS(gv.size(), 3);
    TS_ASSERT_EQUALS(gv[0], 2);
    TS_ASSERT_EQUALS(gv[1], 4);
    TS_ASSERT_EQUALS(gv[2], 6);
  }

  void test_copy_constructor() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    GSLVector gv(v);
    GSLVector gc(gv);
    TS_ASSERT_EQUALS(gc.size(), 3);
    TS_ASSERT_EQUALS(gc[0], 2);
    TS_ASSERT_EQUALS(gc[1], 4);
    TS_ASSERT_EQUALS(gc[2], 6);
  }

  void test_assignment_operator() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    GSLVector gv(v);
    GSLVector gc;
    gc = gv;
    TS_ASSERT_EQUALS(gc.size(), 3);
    TS_ASSERT_EQUALS(gc[0], 2);
    TS_ASSERT_EQUALS(gc[1], 4);
    TS_ASSERT_EQUALS(gc[2], 6);
  }

  void test_zero() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    GSLVector gv(v);
    gv.zero();
    TS_ASSERT_EQUALS(gv[0], 0);
    TS_ASSERT_EQUALS(gv[1], 0);
    TS_ASSERT_EQUALS(gv[2], 0);
  }

  void test_set_get() {
    GSLVector gv(3);
    gv.set(0, 9.9);
    gv.set(1, 7.7);
    gv.set(2, 3.3);
    TS_ASSERT_EQUALS(gv.get(0), 9.9);
    TS_ASSERT_EQUALS(gv.get(1), 7.7);
    TS_ASSERT_EQUALS(gv.get(2), 3.3);
  }

  void test_square_brackets() {
    GSLVector gv(3);
    gv.set(0, 9.9);
    gv.set(1, 7.7);
    gv.set(2, 3.3);
    TS_ASSERT_EQUALS(gv[0], 9.9);
    TS_ASSERT_EQUALS(gv[1], 7.7);
    TS_ASSERT_EQUALS(gv[2], 3.3);
    gv[0] = 3.3;
    gv[1] = 9.9;
    gv[2] = 7.7;
    TS_ASSERT_EQUALS(gv[1], 9.9);
    TS_ASSERT_EQUALS(gv[2], 7.7);
    TS_ASSERT_EQUALS(gv[0], 3.3);
  }

  void test_gsl() {
    GSLVector gv(3);
    gv.set(0, 9.9);
    gv.set(1, 7.7);
    gv.set(2, 3.3);

    auto gslVec = gv.gsl();

    TS_ASSERT_EQUALS(gsl_vector_get(gslVec, 0), 9.9);
    TS_ASSERT_EQUALS(gsl_vector_get(gslVec, 1), 7.7);
    TS_ASSERT_EQUALS(gsl_vector_get(gslVec, 2), 3.3);
  }

  void test_resize() {
    GSLVector gv(3);
    gv.set(0, 9.9);
    gv.set(1, 7.7);
    gv.set(2, 3.3);

    gv.resize(5);
    TS_ASSERT_EQUALS(gv.size(), 5);
    TS_ASSERT_EQUALS(gv.get(0), 9.9);
    TS_ASSERT_EQUALS(gv.get(1), 7.7);
    TS_ASSERT_EQUALS(gv.get(2), 3.3);
    TS_ASSERT_EQUALS(gv.get(3), 0);
    TS_ASSERT_EQUALS(gv.get(4), 0);

    gv[3] = 22;
    gv[4] = 33;
    TS_ASSERT_EQUALS(gv.get(3), 22);
    TS_ASSERT_EQUALS(gv.get(4), 33);

    gv.resize(2);
    TS_ASSERT_EQUALS(gv.size(), 2);
    TS_ASSERT_EQUALS(gv.get(0), 9.9);
    TS_ASSERT_EQUALS(gv.get(1), 7.7);
  }

  void test_plus_operator() {
    auto v1 = makeVector1();
    auto v2 = makeVector2();
    v1 += v2;
    TS_ASSERT_EQUALS(v1.size(), 3);
    TS_ASSERT_EQUALS(v1[0], 8);
    TS_ASSERT_EQUALS(v1[1], 88);
    TS_ASSERT_EQUALS(v1[2], 888);

    TS_ASSERT_THROWS(v1 += makeVector3(), std::runtime_error);
  }

  void test_minus_operator() {
    auto v1 = makeVector1();
    auto v2 = makeVector2();
    v1 -= v2;
    TS_ASSERT_EQUALS(v1.size(), 3);
    TS_ASSERT_EQUALS(v1[0], 2);
    TS_ASSERT_EQUALS(v1[1], 22);
    TS_ASSERT_EQUALS(v1[2], 222);

    TS_ASSERT_THROWS(v1 -= makeVector3(), std::runtime_error);
  }

  void test_times_operator() {
    auto v1 = makeVector1();
    v1 *= 2.2;
    TS_ASSERT_EQUALS(v1.size(), 3);
    TS_ASSERT_EQUALS(v1[0], 11);
    TS_ASSERT_DELTA(v1[1], 121, 1e-13);
    TS_ASSERT_EQUALS(v1[2], 1221);
  }

  void test_norm() {
    auto v = makeVector1();
    TS_ASSERT_DELTA(v.norm2(), 5.0 * 5.0 + 55.0 * 55.0 + 555.0 * 555.0, 1e-10);
    TS_ASSERT_DELTA(v.norm(), sqrt(5.0 * 5.0 + 55.0 * 55.0 + 555.0 * 555.0),
                    1e-10);
    v.normalize();
    TS_ASSERT_DELTA(v.norm(), 1.0, 1e-10);
  }

  void test_dot() {
    auto v1 = makeVector1();
    auto v2 = makeVector2();
    TS_ASSERT_DELTA(v1.dot(v2), 3.0 * 5.0 + 33.0 * 55.0 + 333.0 * 555.0, 1e-10);
    TS_ASSERT_THROWS(v1.dot(makeVector3()), std::runtime_error);
  }

private:
  GSLVector makeVector1() {
    GSLVector v(3);
    v[0] = 5;
    v[1] = 55;
    v[2] = 555;
    return v;
  }

  GSLVector makeVector2() {
    GSLVector v(3);
    v[0] = 3;
    v[1] = 33;
    v[2] = 333;
    return v;
  }

  GSLVector makeVector3() {
    GSLVector v(2);
    v[0] = 1;
    v[1] = 11;
    return v;
  }
};

#endif /*GSLVECTORTEST_H_*/
