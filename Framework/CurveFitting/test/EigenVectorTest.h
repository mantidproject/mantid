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
  void test_create_GSLVector() {
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

  void test_zeros_complex() {
    ComplexMatrix m(10, 12);
    for (size_t i = 0; i < m.size1(); ++i) {
      for (size_t j = 0; j < m.size2(); ++j) {
        ComplexType value = m(i, j);
        TS_ASSERT_EQUALS(value.real(), 0.0);
        TS_ASSERT_EQUALS(value.imag(), 0.0);
      }
    }
  }

  void test_resize_complex() {
    ComplexMatrix m(5, 6);
    for (size_t i = 0; i < m.size1(); ++i) {
      for (size_t j = 0; j < m.size2(); ++j) {
        ComplexType value(static_cast<double>(i), static_cast<double>(j));
      }
    }

    m.resize(12, 10);
    for (size_t i = 0; i < m.size1(); ++i) {
      for (size_t j = 0; j < m.size2(); ++j) {
        ComplexType value = m(i, j);
        TS_ASSERT_EQUALS(value.real(), 0.0);
        TS_ASSERT_EQUALS(value.imag(), 0.0);
      }
    }

    m.resize(3, 4);
    for (size_t i = 0; i < m.size1(); ++i) {
      for (size_t j = 0; j < m.size2(); ++j) {
        ComplexType value = m(i, j);
        TS_ASSERT_EQUALS(value.real(), 0.0);
        TS_ASSERT_EQUALS(value.imag(), 0.0);
      }
    }
  }

  void test_multiply_two_matrices_complex() {
    ComplexMatrix m1(2, 2);
    m1.set(0, 0, v1);
    m1.set(0, 1, v2);
    m1.set(1, 0, v3);
    m1.set(1, 1, v4);
    ComplexMatrix m2(2, 2);
    m2.set(0, 0, v5);
    m2.set(0, 1, v6);
    m2.set(1, 0, v7);
    m2.set(1, 1, v8);

    ComplexMatrix m3;

    m3 = m1 * m2;

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 18.81, 3.8, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 21.78, 4.4, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 42.57, 8.6, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 49.5, 10, 1e-8);

    m3 = m1.tr() * m2;

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 25.74, 5.2, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 29.7, 6, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 37.62, 7.6, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 43.56, 8.8, 1e-8);

    m3 = m1 * m2.tr();

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 16.83, 3.4, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 22.77, 4.6, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 38.61, 7.8, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 52.47, 10.6, 1e-8);

    m3 = m1.tr() * m2.tr();

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 22.77, 4.6, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 30.69, 6.2, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 33.66, 6.8, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 45.54, 9.2, 1e-8);

    m3 = m1.ctr() * m2;

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 26.26, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 30.30, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 38.38, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 44.44, 0, 1e-8);

    m3 = m1 * m2.ctr();

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 17.17, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 23.23, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 39.39, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 53.53, 0, 1e-8);

    m3 = m1.ctr() * m2.tr();

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 23.23, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 31.31, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 34.34, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 46.46, 0, 1e-8);

    m3 = m1.tr() * m2.ctr();

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 23.23, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 31.31, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 34.34, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 46.46, 0, 1e-8);

    m3 = m1.ctr() * m2.ctr();

    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 0), 22.77, -4.6, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(0, 1), 30.69, -6.2, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 0), 33.66, -6.8, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(m3.get(1, 1), 45.54, -9.2, 1e-8);
  }

  void test_multiply_three_matrices_complex() {
    ComplexMatrix m1(2, 2);
    m1.set(0, 0, v1);
    m1.set(0, 1, v2);
    m1.set(1, 0, v3);
    m1.set(1, 1, v4);
    ComplexMatrix m2(2, 2);
    m2.set(0, 0, v5);
    m2.set(0, 1, v6);
    m2.set(1, 0, v7);
    m2.set(1, 1, v8);
    ComplexMatrix m3(2, 2);
    m3.set(0, 0, v9);
    m3.set(0, 1, v10);
    m3.set(1, 0, v11);
    m3.set(1, 1, v12);

    ComplexMatrix m;

    m = m1.tr() * m2 * m3;

    TS_ASSERT_EQUALS(m.size1(), 2);
    TS_ASSERT_EQUALS(m.size2(), 2);

    for (size_t i = 0; i < m.size1(); ++i)
      for (size_t j = 0; j < m.size2(); ++j) {
        ComplexType d{0.0, 0.0};
        for (size_t k = 0; k < m2.size1(); ++k)
          for (size_t l = 0; l < m2.size2(); ++l) {
            d += m1.get(k, i) * m2.get(k, l) * m3.get(l, j);
          }
        d -= m.get(i, j);
        TS_ASSERT_DELTA(std::norm(d), 0.0, 1e-8);
      }
  }

  void test_invert_complex() {
    ComplexMatrix m(2, 2);
    m.set(0, 0, 1);
    m.set(0, 1, 1);
    m.set(1, 0, 0);
    m.set(1, 1, 1);
    m.invert();
    TS_ASSERT_EQUALS(m.get(0, 0), 1.0);
    TS_ASSERT_EQUALS(m.get(0, 1), -1.0);
    TS_ASSERT_EQUALS(m.get(1, 0), 0.0);
    TS_ASSERT_EQUALS(m.get(1, 1), 1.0);
    m.set(0, 0, 2);
    m.set(0, 1, 0);
    m.set(1, 0, 0);
    m.set(1, 1, 2);
    m.invert();
    TS_ASSERT_EQUALS(m.get(0, 0), 0.5);
    TS_ASSERT_EQUALS(m.get(0, 1), 0.0);
    TS_ASSERT_EQUALS(m.get(1, 0), 0.0);
    TS_ASSERT_EQUALS(m.get(1, 1), 0.5);
  }

  void test_subMatrix_complex() {
    ComplexMatrix m(4, 4);
    m.set(0, 0, v0);
    m.set(0, 1, v1);
    m.set(0, 2, v2);
    m.set(0, 3, v3);
    m.set(1, 0, v10);
    m.set(1, 1, v11);
    m.set(1, 2, v12);
    m.set(1, 3, v13);
    m.set(2, 0, v20);
    m.set(2, 1, v21);
    m.set(2, 2, v22);
    m.set(2, 3, v23);
    m.set(3, 0, v30);
    m.set(3, 1, v31);
    m.set(3, 2, v32);
    m.set(3, 3, v33);

    ComplexMatrix subm(m, 1, 1, 2, 2);
    TS_ASSERT_EQUALS(subm.get(0, 0), v11);
    TS_ASSERT_EQUALS(subm.get(0, 1), v12);
    TS_ASSERT_EQUALS(subm.get(1, 0), v21);
    TS_ASSERT_EQUALS(subm.get(1, 1), v22);
  }

  void test_subMatrix_fail_complex() {
    ComplexMatrix m(4, 4);
    m.set(0, 0, 0);
    m.set(0, 1, 1);
    m.set(0, 2, 2);
    m.set(0, 3, 3);
    m.set(1, 0, 10);
    m.set(1, 1, 11);
    m.set(1, 2, 12);
    m.set(1, 3, 13);
    m.set(2, 0, 20);
    m.set(2, 1, 21);
    m.set(2, 2, 22);
    m.set(2, 3, 23);
    m.set(3, 0, 30);
    m.set(3, 1, 31);
    m.set(3, 2, 32);
    m.set(3, 3, 33);

    TS_ASSERT_THROWS(ComplexMatrix subm(m, 2, 2, 3, 3), const std::runtime_error &);
  }

  void test_eigenSystem_rectangular_throw_complex() {
    ComplexMatrix M(3, 4);
    ComplexVector v;
    ComplexMatrix Q;
    TS_ASSERT_THROWS(M.eigenSystemHermitian(v, Q), const std::runtime_error &);
  }

  void test_small_real_eigenSystem() {
    const size_t n = 2;
    ComplexMatrix m(n, n);
    m.set(0, 0, 0);
    m.set(0, 1, 1);
    m.set(1, 0, 1);
    m.set(1, 1, 11);

    ComplexVector v;
    ComplexMatrix Q;
    ComplexMatrix M = m;
    M.eigenSystemHermitian(v, Q);
    TS_ASSERT_EQUALS(v.size(), n);
    TS_ASSERT_EQUALS(Q.size1(), n);
    TS_ASSERT_EQUALS(Q.size2(), n);

    TS_ASSERT_COMPLEX_DELTA(v[0], -0.09016994, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(v[1], 11.09016994, 0, 1e-8);

    TS_ASSERT_COMPLEX_DELTA(Q.get(0, 0), -0.99595931, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(Q.get(0, 1), -0.0898056, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(Q.get(1, 0), 0.0898056, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(Q.get(1, 1), -0.99595931, 0, 1e-8);
  }

  void test_small_complex_eigenSystem() {
    const size_t n = 2;
    ComplexMatrix m(n, n);
    m.set(0, 0, 0);
    m.set(0, 1, v1);
    m.set(1, 0, conj(v1));
    m.set(1, 1, 11);

    ComplexVector v;
    ComplexMatrix Q;
    ComplexMatrix M = m;
    M.eigenSystemHermitian(v, Q);
    TS_ASSERT_EQUALS(v.size(), n);
    TS_ASSERT_EQUALS(Q.size1(), n);
    TS_ASSERT_EQUALS(Q.size2(), n);

    TS_ASSERT_COMPLEX_DELTA(v[0], -0.0910643, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(v[1], 11.0910643, 0, 1e-8);

    TS_ASSERT_COMPLEX_DELTA(Q.get(0, 0), -0.99591981, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(Q.get(0, 1), 0.09024265, 0, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(Q.get(1, 0), 0.08979479, -0.00897948, 1e-8);
    TS_ASSERT_COMPLEX_DELTA(Q.get(1, 1), 0.99097725, -0.09909772, 1e-8);
  }

  void test_eigenSystem_complex() {
    const size_t n = 4;
    ComplexMatrix m(n, n);
    m.set(0, 0, 0);
    m.set(0, 1, v1);
    m.set(0, 2, v2);
    m.set(0, 3, v3);
    m.set(1, 0, conj(v1));
    m.set(1, 1, 11);
    m.set(1, 2, v12);
    m.set(1, 3, v13);
    m.set(2, 0, conj(v2));
    m.set(2, 1, conj(v12));
    m.set(2, 2, 22);
    m.set(2, 3, v23);
    m.set(3, 0, conj(v3));
    m.set(3, 1, conj(v13));
    m.set(3, 2, conj(v23));
    m.set(3, 3, 33);
    ComplexVector v;
    ComplexMatrix Q;
    ComplexMatrix M = m;
    M.eigenSystemHermitian(v, Q);
    TS_ASSERT_EQUALS(v.size(), n);
    TS_ASSERT_EQUALS(Q.size1(), n);
    TS_ASSERT_EQUALS(Q.size2(), n);
    {
      ComplexMatrix D = Q.ctr() * m * Q;
      ComplexType trace_m = 0.0;
      ComplexType trace_D = 0.0;
      ComplexType det = 1.0;
      for (size_t i = 0; i < n; ++i) {
        TS_ASSERT_COMPLEX_DELTA_2(D.get(i, i), v.get(i), 1e-10);
        trace_m += m.get(i, i);
        trace_D += D.get(i, i);
        det *= D.get(i, i);
      }
      TS_ASSERT_COMPLEX_DELTA_2(trace_D, trace_m, 1e-10);
      TS_ASSERT_COMPLEX_DELTA_2(det, m.det(), 1e-10);
    }
    {
      ComplexMatrix D = Q.ctr() * Q;
      for (size_t i = 0; i < n; ++i) {
        TS_ASSERT_COMPLEX_DELTA_2(D.get(i, i), 1.0, 1e-10);
      }
    }
  }

  void test_copyColumn_complex() {

    ComplexMatrix m(4, 4);
    m.set(0, 0, v0);
    m.set(0, 1, v1);
    m.set(0, 2, v2);
    m.set(0, 3, v3);
    m.set(1, 0, v10);
    m.set(1, 1, v11);
    m.set(1, 2, v12);
    m.set(1, 3, v13);
    m.set(2, 0, v20);
    m.set(2, 1, v21);
    m.set(2, 2, v22);
    m.set(2, 3, v23);
    m.set(3, 0, v30);
    m.set(3, 1, v31);
    m.set(3, 2, v32);
    m.set(3, 3, v33);

    auto column = m.copyColumn(2);
    TS_ASSERT(column[0] == m.get(0, 2));
    TS_ASSERT(column[1] == m.get(1, 2));
    TS_ASSERT(column[2] == m.get(2, 2));
    TS_ASSERT(column[3] == m.get(3, 2));

    column[2] = 0;
    TS_ASSERT_EQUALS(column[2].real(), ComplexType(0, 0));
    TS_ASSERT_EQUALS(m.get(2, 2), v22);
  }

  void test_copyRow_complex() {

    ComplexMatrix m(4, 4);
    m.set(0, 0, v0);
    m.set(0, 1, v1);
    m.set(0, 2, v2);
    m.set(0, 3, v3);
    m.set(1, 0, v10);
    m.set(1, 1, v11);
    m.set(1, 2, v12);
    m.set(1, 3, v13);
    m.set(2, 0, v20);
    m.set(2, 1, v21);
    m.set(2, 2, v22);
    m.set(2, 3, v23);
    m.set(3, 0, v30);
    m.set(3, 1, v31);
    m.set(3, 2, v32);
    m.set(3, 3, v33);

    auto row = m.copyRow(1);

    TS_ASSERT(row[0] == m.get(1, 0));
    TS_ASSERT(row[1] == m.get(1, 1));
    TS_ASSERT(row[2] == m.get(1, 2));
    TS_ASSERT(row[3] == m.get(1, 3));
    row[2] = 0;
    TS_ASSERT_EQUALS(row[2], ComplexType(0, 0));
    TS_ASSERT_EQUALS(m.get(1, 2), v12);
  }

  void test_index_operator_complex() {
    ComplexMatrix m(2, 2);
    m(0, 0) = v11;
    m(0, 1) = v12;
    m(1, 0) = v21;
    m(1, 1) = v22;

    TS_ASSERT(m(0, 0) == v11);
    TS_ASSERT(m(0, 1) == v12);
    TS_ASSERT(m(1, 0) == v21);
    TS_ASSERT(m(1, 1) == v22);
  }

  void test_sort_columns_complex() {
    ComplexMatrix m(3, 3);
    m(0, 0) = v11;
    m(1, 0) = v11;
    m(2, 0) = v11;
    m(0, 1) = v22;
    m(1, 1) = v22;
    m(2, 1) = v22;
    m(0, 2) = v33;
    m(1, 2) = v33;
    m(2, 2) = v33;
    std::vector<size_t> indices{2, 0, 1};
    m.sortColumns(indices);
    TS_ASSERT_EQUALS(m(0, 0), v33);
    TS_ASSERT_EQUALS(m(1, 0), v33);
    TS_ASSERT_EQUALS(m(2, 0), v33);
    TS_ASSERT_EQUALS(m(0, 1), v11);
    TS_ASSERT_EQUALS(m(1, 1), v11);
    TS_ASSERT_EQUALS(m(2, 1), v11);
    TS_ASSERT_EQUALS(m(0, 2), v22);
    TS_ASSERT_EQUALS(m(1, 2), v22);
    TS_ASSERT_EQUALS(m(2, 2), v22);
  }

  void test_packing() {
    ComplexMatrix m(4, 3);
    m.set(0, 0, v0);
    m.set(0, 1, v1);
    m.set(0, 2, v2);
    m.set(1, 0, v10);
    m.set(1, 1, v11);
    m.set(1, 2, v12);
    m.set(2, 0, v20);
    m.set(2, 1, v21);
    m.set(2, 2, v22);
    m.set(3, 0, v30);
    m.set(3, 1, v31);
    m.set(3, 2, v32);

    std::vector<double> packed = m.packToStdVector();
    TS_ASSERT_EQUALS(packed.size(), 2 * m.size1() * m.size2());

    auto index = [&m](size_t i, size_t j) { return 2 * (i * m.size2() + j); };
    for (size_t i = 0; i < m.size1(); ++i) {
      for (size_t j = 0; j < m.size2(); ++j) {
        TS_ASSERT_EQUALS(packed[index(i, j)], m.get(i, j).real());
        TS_ASSERT_EQUALS(packed[index(i, j) + 1], m.get(i, j).imag());
      }
    }
  }

  void test_copy_constructor_complex() {
    ComplexMatrix a(2, 2);
    a(0, 0) = v11;
    a(0, 1) = v12;
    a(1, 0) = v21;
    a(1, 1) = v22;

    ComplexMatrix m(a);
    TS_ASSERT(m(0, 0) == v11);
    TS_ASSERT(m(0, 1) == v12);
    TS_ASSERT(m(1, 0) == v21);
    TS_ASSERT(m(1, 1) == v22);

    a(1, 1) = 0;
    TS_ASSERT_DIFFERS(a.eigen(), m.eigen());
  }

  void test_move_constructor_complex() {
    ComplexMatrix a(2, 2);
    a(0, 0) = v11;
    a(0, 1) = v12;
    a(1, 0) = v21;
    a(1, 1) = v22;
    ComplexMatrix m(std::move(a));
    // test that data has been transferred
    TS_ASSERT(m(0, 0) == v11);
    TS_ASSERT(m(0, 1) == v12);
    TS_ASSERT(m(1, 0) == v21);
    TS_ASSERT(m(1, 1) == v22);
    // Check data in has been deleted by move.
    TS_ASSERT_EQUALS(a.eigen().size(), 0);
  }

  void test_move_constructor_matrix_complex() {
    Eigen::MatrixXcd a(2, 2);
    a(0, 0) = v11;
    a(0, 1) = v12;
    a(1, 0) = v21;
    a(1, 1) = v22;
    ComplexMatrix m(std::move(a));
    // test that data has been transferred
    TS_ASSERT(m(0, 0) == v11);
    TS_ASSERT(m(0, 1) == v12);
    TS_ASSERT(m(1, 0) == v21);
    TS_ASSERT(m(1, 1) == v22);
    // Check data in has been deleted by move.
    TS_ASSERT_EQUALS(a.size(), 0);
  }

  void test_copy_assignment_complex() {
    ComplexMatrix a(2, 2);
    a(0, 0) = v11;
    a(0, 1) = v12;
    a(1, 0) = v21;
    a(1, 1) = v22;

    ComplexMatrix m;
    m = a;
    TS_ASSERT(m(0, 0) == v11);
    TS_ASSERT(m(0, 1) == v12);
    TS_ASSERT(m(1, 0) == v21);
    TS_ASSERT(m(1, 1) == v22);

    a(1, 1) = 0;
    TS_ASSERT_DIFFERS(&(m.eigen()), &(a.eigen()));
  }

  void test_move_assignment_complex() {
    ComplexMatrix a(2, 2);
    a(0, 0) = v11;
    a(0, 1) = v12;
    a(1, 0) = v21;
    a(1, 1) = v22;
    auto eigen = a.eigen();
    ComplexMatrix m;
    m = std::move(a);
    TS_ASSERT(m(0, 0) == v11);
    TS_ASSERT(m(0, 1) == v12);
    TS_ASSERT(m(1, 0) == v21);
    TS_ASSERT(m(1, 1) == v22);
    TS_ASSERT_EQUALS(m.eigen(), eigen);
  }
};
