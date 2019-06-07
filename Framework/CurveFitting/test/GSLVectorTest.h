// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

  void test_create_from_initializer() {
    GSLVector gv({2.0, 4.0, 6.0});
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

  void test_assignment_operator_std_vector() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    GSLVector gc;
    gc = v;
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

    TS_ASSERT_THROWS(v1 += makeVector3(), const std::runtime_error &);
  }

  void test_minus_operator() {
    auto v1 = makeVector1();
    auto v2 = makeVector2();
    v1 -= v2;
    TS_ASSERT_EQUALS(v1.size(), 3);
    TS_ASSERT_EQUALS(v1[0], 2);
    TS_ASSERT_EQUALS(v1[1], 22);
    TS_ASSERT_EQUALS(v1[2], 222);

    TS_ASSERT_THROWS(v1 -= makeVector3(), const std::runtime_error &);
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
    TS_ASSERT_THROWS(v1.dot(makeVector3()), const std::runtime_error &);
  }

  void test_find_min_element() {
    GSLVector v(3);
    v[0] = 55;
    v[1] = 5;
    v[2] = 555;
    auto imin = v.indexOfMinElement();
    TS_ASSERT_EQUALS(imin, 1);
    v[2] = -555;
    imin = v.indexOfMinElement();
    TS_ASSERT_EQUALS(imin, 2);
  }

  void test_find_max_element() {
    GSLVector v(3);
    v[0] = 55;
    v[1] = 5;
    v[2] = 555;
    auto imax = v.indexOfMaxElement();
    TS_ASSERT_EQUALS(imax, 2);
    v[2] = -555;
    imax = v.indexOfMaxElement();
    TS_ASSERT_EQUALS(imax, 0);
  }

  void test_find_min_max_element() {
    GSLVector v(3);
    v[0] = 55;
    v[1] = 5;
    v[2] = 555;
    auto pit = v.indicesOfMinMaxElements();
    TS_ASSERT_EQUALS(pit.first, 1);
    TS_ASSERT_EQUALS(pit.second, 2);
  }

  void test_sort_indices_ascending() {
    GSLVector v(std::vector<double>{3.5, 5.9, 2.9, 0.5, 1.5});
    auto sorted = v.sortIndices();
    TS_ASSERT_EQUALS(sorted[0], 3);
    TS_ASSERT_EQUALS(sorted[1], 4);
    TS_ASSERT_EQUALS(sorted[2], 2);
    TS_ASSERT_EQUALS(sorted[3], 0);
    TS_ASSERT_EQUALS(sorted[4], 1);

    TS_ASSERT_EQUALS(v[0], 3.5);
    TS_ASSERT_EQUALS(v[1], 5.9);
    TS_ASSERT_EQUALS(v[2], 2.9);
    TS_ASSERT_EQUALS(v[3], 0.5);
    TS_ASSERT_EQUALS(v[4], 1.5);
    v.sort(sorted);
    TS_ASSERT_EQUALS(v[0], 0.5);
    TS_ASSERT_EQUALS(v[1], 1.5);
    TS_ASSERT_EQUALS(v[2], 2.9);
    TS_ASSERT_EQUALS(v[3], 3.5);
    TS_ASSERT_EQUALS(v[4], 5.9);
  }

  void test_sort_indices_descending() {
    GSLVector v(std::vector<double>{3.5, 5.9, 2.9, 0.5, 1.5});
    auto sorted = v.sortIndices(false);
    TS_ASSERT_EQUALS(sorted[0], 1);
    TS_ASSERT_EQUALS(sorted[1], 0);
    TS_ASSERT_EQUALS(sorted[2], 2);
    TS_ASSERT_EQUALS(sorted[3], 4);
    TS_ASSERT_EQUALS(sorted[4], 3);

    TS_ASSERT_EQUALS(v[0], 3.5);
    TS_ASSERT_EQUALS(v[1], 5.9);
    TS_ASSERT_EQUALS(v[2], 2.9);
    TS_ASSERT_EQUALS(v[3], 0.5);
    TS_ASSERT_EQUALS(v[4], 1.5);
    v.sort(sorted);
    TS_ASSERT_EQUALS(v[0], 5.9);
    TS_ASSERT_EQUALS(v[1], 3.5);
    TS_ASSERT_EQUALS(v[2], 2.9);
    TS_ASSERT_EQUALS(v[3], 1.5);
    TS_ASSERT_EQUALS(v[4], 0.5);
  }

  void test_move_std_vector() {
    std::vector<double> s{3.5, 5.9, 2.9, 0.5, 1.5};
    auto p0 = &s[0];
    GSLVector v(std::move(s));
    TS_ASSERT_EQUALS(v.size(), 5);
    TS_ASSERT_EQUALS(v[0], 3.5);
    TS_ASSERT_EQUALS(v[1], 5.9);
    TS_ASSERT_EQUALS(v[2], 2.9);
    TS_ASSERT_EQUALS(v[3], 0.5);
    TS_ASSERT_EQUALS(v[4], 1.5);
    TS_ASSERT_EQUALS(p0, &v[0]);
  }

  void test_toStdVector() {
    auto v = makeVector1();
    auto stdv = v.toStdVector();
    TS_ASSERT_EQUALS(v.size(), stdv.size());
    TS_ASSERT_EQUALS(v[0], stdv[0]);
    TS_ASSERT_EQUALS(v[1], stdv[1]);
    TS_ASSERT_EQUALS(v[2], stdv[2]);
  }

  void tst_add_constant() {
    auto v = makeVector1();
    v += 10;
    TS_ASSERT_EQUALS(v[0], 15.0);
    TS_ASSERT_EQUALS(v[1], 65.0);
    TS_ASSERT_EQUALS(v[2], 565.0);
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
