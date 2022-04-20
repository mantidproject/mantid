// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EigenComplexMatrix.h"

using namespace Mantid::CurveFitting;

namespace {
ComplexType v0{0, 0};
ComplexType v1{1, 0.1};
ComplexType v2{2, 0.2};
ComplexType v3{3, 0.3};
ComplexType v4{4, 0.4};
ComplexType v5{5, 0.5};
ComplexType v6{6, 0.6};
ComplexType v7{7, 0.7};
ComplexType v8{8, 0.8};
ComplexType v9{9, 0.9};
ComplexType v10{10, 0.1};
ComplexType v11{11, 0.11};
ComplexType v12{12, 0.12};
ComplexType v13{13, 0.13};
ComplexType v20{20, 0.20};
ComplexType v21{21, 0.21};
ComplexType v22{22, 0.22};
ComplexType v23{23, 0.23};
ComplexType v30{30, 0.30};
ComplexType v31{31, 0.31};
ComplexType v32{32, 0.32};
ComplexType v33{33, 0.33};

#define TS_ASSERT_COMPLEX_DELTA(v1, r2, i2, d)                                                                         \
  TS_ASSERT_DELTA(ComplexType(v1).real(), r2, d);                                                                      \
  TS_ASSERT_DELTA(ComplexType(v1).imag(), i2, d);

#define TS_ASSERT_COMPLEX_DELTA_2(v1, v2, d)                                                                           \
  TS_ASSERT_DELTA(ComplexType(v1).real(), ComplexType(v2).real(), d);                                                  \
  TS_ASSERT_DELTA(ComplexType(v1).imag(), ComplexType(v2).imag(), d);
} // namespace

class ComplexMatrixTest : public CxxTest::TestSuite {
public:
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
    EigenVector v;
    ComplexMatrix Q;
    TS_ASSERT_THROWS(M.eigenSystemHermitian(v, Q), const std::runtime_error &);
  }

  void test_small_real_eigenSystem() {
    // this replicates a portion of the CrystalFieldTest. This ensures consistency of results at the
    // ComplexMatrix level.

    const size_t n = 2;
    ComplexMatrix m(n, n);
    m.set(0, 0, 0);
    m.set(0, 1, 1);
    m.set(1, 0, 1);
    m.set(1, 1, 11);

    EigenVector v;
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

    EigenVector v;
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
    EigenVector v;
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

  void test_crystal_eigenSystem() {
    // this replicates a portion of the CrystalFieldTest. This ensures consistency of results at the
    // ComplexMatrix level.

    Eigen::MatrixXcd m{{27.737, 0, -85.3224, 0, -62.279, 0}, {0, -73.789, 0, 112.995, 0, -62.279},
                       {-85.3224, 0, 46.052, 0, 112.995, 0}, {0, 112.995, 0, 46.052, 0, -85.3224},
                       {-62.279, 0, 112.995, 0, -73.789, 0}, {0, -62.279, 0, -85.3224, 0, 27.737}};
    ComplexMatrix m_c(std::move(m));

    EigenVector v;
    ComplexMatrix m_cr;

    m_c.eigenSystemHermitian(v, m_cr);

    auto indicies = v.sortIndices();
    v.sort(indicies);
    m_cr.sortColumns(indicies);

    auto res = m_cr.ctr() * m_cr;

    for (size_t i = 0; i < res.size1(); ++i) {
      for (size_t j = 0; j < res.size2(); ++j) {
        ComplexType value = res(i, j);
        if (i == j) {
          TS_ASSERT_DELTA(value.real(), 1.0, 1e-10);
          TS_ASSERT_DELTA(value.imag(), 0.0, 1e-10);
        } else {
          TS_ASSERT_DELTA(value.real(), 0.0, 1e-10);
          TS_ASSERT_DELTA(value.imag(), 0.0, 1e-10);
        }
      }
    }

    Eigen::VectorXd ans_v(6);
    ans_v << -142.461, -142.461, -42.2269, -42.2269, 184.688, 184.688;

    for (size_t i = 0; i < v.size(); i++) {
      TS_ASSERT_DELTA(v.get(i), ans_v(i), 1e-1);
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
