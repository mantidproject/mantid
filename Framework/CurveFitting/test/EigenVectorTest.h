// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EigenVector.h"
#include <MantidCurveFitting/EigenComplexMatrix.h>
#include <MantidCurveFitting/EigenComplexVector.h>

using namespace Mantid::CurveFitting;

class EigenVectorTest : public CxxTest::TestSuite {
public:
  void test_create_EigenVector() {
    {
      EigenVector v;
      TS_ASSERT_EQUALS(v.size(), 1);
    }
    {
      EigenVector v(1);
      TS_ASSERT_EQUALS(v.size(), 1);
    }
    {
      EigenVector v(2);
      TS_ASSERT_EQUALS(v.size(), 2);
    }
  }

  void test_create_from_std_vector() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    EigenVector ev(v);
    TS_ASSERT_EQUALS(ev.size(), 3);
    TS_ASSERT_EQUALS(ev[0], 2);
    TS_ASSERT_EQUALS(ev[1], 4);
    TS_ASSERT_EQUALS(ev[2], 6);
  }

  void test_create_from_initializer() {
    EigenVector ev({2.0, 4.0, 6.0});
    TS_ASSERT_EQUALS(ev.size(), 3);
    TS_ASSERT_EQUALS(ev[0], 2);
    TS_ASSERT_EQUALS(ev[1], 4);
    TS_ASSERT_EQUALS(ev[2], 6);
  }

  void test_copy_constructor() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    EigenVector ev(v);
    EigenVector ec(ev);
    TS_ASSERT_EQUALS(ec.size(), 3);
    TS_ASSERT_EQUALS(ec[0], 2);
    TS_ASSERT_EQUALS(ec[1], 4);
    TS_ASSERT_EQUALS(ec[2], 6);
  }

  void test_assignment_operator() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    EigenVector ev(v);
    EigenVector ec;
    ec = ev;
    TS_ASSERT_EQUALS(ec.size(), 3);
    TS_ASSERT_EQUALS(ec[0], 2);
    TS_ASSERT_EQUALS(ec[1], 4);
    TS_ASSERT_EQUALS(ec[2], 6);
  }

  void test_assignment_operator_std_vector() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    EigenVector ec;
    ec = v;
    TS_ASSERT_EQUALS(ec.size(), 3);
    TS_ASSERT_EQUALS(ec[0], 2);
    TS_ASSERT_EQUALS(ec[1], 4);
    TS_ASSERT_EQUALS(ec[2], 6);
  }

  void test_zero() {
    std::vector<double> v(3);
    v[0] = 2;
    v[1] = 4;
    v[2] = 6;
    EigenVector ev(v);
    ev.zero();
    TS_ASSERT_EQUALS(ev[0], 0);
    TS_ASSERT_EQUALS(ev[1], 0);
    TS_ASSERT_EQUALS(ev[2], 0);
  }

  void test_set_get() {
    EigenVector ev(3);
    ev.set(0, 9.9);
    ev.set(1, 7.7);
    ev.set(2, 3.3);
    TS_ASSERT_EQUALS(ev.get(0), 9.9);
    TS_ASSERT_EQUALS(ev.get(1), 7.7);
    TS_ASSERT_EQUALS(ev.get(2), 3.3);
  }

  void test_square_brackets() {
    EigenVector ev(3);
    ev.set(0, 9.9);
    ev.set(1, 7.7);
    ev.set(2, 3.3);
    TS_ASSERT_EQUALS(ev[0], 9.9);
    TS_ASSERT_EQUALS(ev[1], 7.7);
    TS_ASSERT_EQUALS(ev[2], 3.3);
    ev[0] = 3.3;
    ev[1] = 9.9;
    ev[2] = 7.7;
    TS_ASSERT_EQUALS(ev[1], 9.9);
    TS_ASSERT_EQUALS(ev[2], 7.7);
    TS_ASSERT_EQUALS(ev[0], 3.3);
  }

  void test_eigen() {
    EigenVector ev(3);
    ev.set(0, 9.9);
    ev.set(1, 7.7);
    ev.set(2, 3.3);

    auto eigenVec = ev.inspector();

    TS_ASSERT_EQUALS(eigenVec(0), 9.9);
    TS_ASSERT_EQUALS(eigenVec(1), 7.7);
    TS_ASSERT_EQUALS(eigenVec(2), 3.3);
  }

  void test_resize() {
    EigenVector ev(3);
    ev.set(0, 9.9);
    ev.set(1, 7.7);
    ev.set(2, 3.3);

    ev.resize(5);
    TS_ASSERT_EQUALS(ev.size(), 5);
    TS_ASSERT_EQUALS(ev.get(0), 9.9);
    TS_ASSERT_EQUALS(ev.get(1), 7.7);
    TS_ASSERT_EQUALS(ev.get(2), 3.3);
    TS_ASSERT_EQUALS(ev.get(3), 0);
    TS_ASSERT_EQUALS(ev.get(4), 0);

    ev[3] = 22;
    ev[4] = 33;
    TS_ASSERT_EQUALS(ev.get(3), 22);
    TS_ASSERT_EQUALS(ev.get(4), 33);

    ev.resize(2);
    TS_ASSERT_EQUALS(ev.size(), 2);
    TS_ASSERT_EQUALS(ev.get(0), 9.9);
    TS_ASSERT_EQUALS(ev.get(1), 7.7);
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
    TS_ASSERT_DELTA(v.norm(), sqrt(5.0 * 5.0 + 55.0 * 55.0 + 555.0 * 555.0), 1e-10);
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
    EigenVector v(3);
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
    EigenVector v(3);
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
    EigenVector v(3);
    v[0] = 55;
    v[1] = 5;
    v[2] = 555;
    auto pit = v.indicesOfMinMaxElements();
    TS_ASSERT_EQUALS(pit.first, 1);
    TS_ASSERT_EQUALS(pit.second, 2);
  }

  void test_sort_indices_ascending() {
    EigenVector v(std::vector<double>{3.5, 5.9, 2.9, 0.5, 1.5});
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
    EigenVector v(std::vector<double>{3.5, 5.9, 2.9, 0.5, 1.5});
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
    EigenVector v(std::move(s));
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

  EigenVector makeVector1() {
    EigenVector v(3);
    v[0] = 5;
    v[1] = 55;
    v[2] = 555;

    return v;
  }

  EigenVector makeVector2() {
    EigenVector v(3);
    v[0] = 3;
    v[1] = 33;
    v[2] = 333;
    return v;
  }

  EigenVector makeVector3() {
    EigenVector v(2);
    v[0] = 1;
    v[1] = 11;
    return v;
  }
};
