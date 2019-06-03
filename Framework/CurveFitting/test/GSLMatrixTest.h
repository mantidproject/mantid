// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GSLMATRIXTEST_H_
#define GSLMATRIXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/GSLMatrix.h"

using namespace Mantid::CurveFitting;

class GSLMatrixTest : public CxxTest::TestSuite {
public:
  void test_create_GSLMult2_plain_plain() {
    GSLMatrix m1(2, 2);
    GSLMatrix m2(2, 2);

    GSLMatrixMult2 mult2 = m1 * m2;

    TS_ASSERT_EQUALS(mult2.tr1, false);
    TS_ASSERT_EQUALS(mult2.tr2, false);
    TS_ASSERT_EQUALS(mult2.m_1.gsl(), m1.gsl());
    TS_ASSERT_EQUALS(mult2.m_2.gsl(), m2.gsl());
  }

  void test_create_GSLMult2_tr_plain() {
    GSLMatrix m1(2, 2);
    GSLMatrix m2(2, 2);

    GSLMatrixMult2 mult2 = m1.tr() * m2;

    TS_ASSERT_EQUALS(mult2.tr1, true);
    TS_ASSERT_EQUALS(mult2.tr2, false);
    TS_ASSERT_EQUALS(mult2.m_1.gsl(), m1.gsl());
    TS_ASSERT_EQUALS(mult2.m_2.gsl(), m2.gsl());
  }

  void test_create_GSLMult2_plain_tr() {
    GSLMatrix m1(2, 2);
    GSLMatrix m2(2, 2);

    GSLMatrixMult2 mult2 = m1 * m2.tr();

    TS_ASSERT_EQUALS(mult2.tr1, false);
    TS_ASSERT_EQUALS(mult2.tr2, true);
    TS_ASSERT_EQUALS(mult2.m_1.gsl(), m1.gsl());
    TS_ASSERT_EQUALS(mult2.m_2.gsl(), m2.gsl());
  }

  void test_create_GSLMult2_tr_tr() {
    GSLMatrix m1(2, 2);
    GSLMatrix m2(2, 2);

    GSLMatrixMult2 mult2 = m1.tr() * m2.tr();

    TS_ASSERT_EQUALS(mult2.tr1, true);
    TS_ASSERT_EQUALS(mult2.tr2, true);
    TS_ASSERT_EQUALS(mult2.m_1.gsl(), m1.gsl());
    TS_ASSERT_EQUALS(mult2.m_2.gsl(), m2.gsl());
  }

  void test_create_from_initializer_list() {
    GSLMatrix m({{1.0, 2.0}, {11.0, 12.0}, {21.0, 22.0}});
    TS_ASSERT_EQUALS(m.size1(), 3);
    TS_ASSERT_EQUALS(m.size2(), 2);
    TS_ASSERT_EQUALS(m(0, 0), 1.);
    TS_ASSERT_EQUALS(m(0, 1), 2.);
    TS_ASSERT_EQUALS(m(1, 0), 11.);
    TS_ASSERT_EQUALS(m(1, 1), 12.);
    TS_ASSERT_EQUALS(m(2, 0), 21.);
    TS_ASSERT_EQUALS(m(2, 1), 22.);
  }

  void test_multiply_two_matrices() {
    GSLMatrix m1(2, 2);
    m1.set(0, 0, 1);
    m1.set(0, 1, 2);
    m1.set(1, 0, 3);
    m1.set(1, 1, 4);
    GSLMatrix m2(2, 2);
    m2.set(0, 0, 5);
    m2.set(0, 1, 6);
    m2.set(1, 0, 7);
    m2.set(1, 1, 8);

    GSLMatrix m3;

    m3 = m1 * m2;

    TS_ASSERT_EQUALS(m3.get(0, 0), 19.);
    TS_ASSERT_EQUALS(m3.get(0, 1), 22.);
    TS_ASSERT_EQUALS(m3.get(1, 0), 43.);
    TS_ASSERT_EQUALS(m3.get(1, 1), 50.);

    m3 = m1.tr() * m2;

    TS_ASSERT_EQUALS(m3.get(0, 0), 26.);
    TS_ASSERT_EQUALS(m3.get(0, 1), 30.);
    TS_ASSERT_EQUALS(m3.get(1, 0), 38.);
    TS_ASSERT_EQUALS(m3.get(1, 1), 44.);

    m3 = m1 * m2.tr();

    TS_ASSERT_EQUALS(m3.get(0, 0), 17.);
    TS_ASSERT_EQUALS(m3.get(0, 1), 23.);
    TS_ASSERT_EQUALS(m3.get(1, 0), 39.);
    TS_ASSERT_EQUALS(m3.get(1, 1), 53.);

    m3 = m1.tr() * m2.tr();

    TS_ASSERT_EQUALS(m3.get(0, 0), 23.);
    TS_ASSERT_EQUALS(m3.get(0, 1), 31.);
    TS_ASSERT_EQUALS(m3.get(1, 0), 34.);
    TS_ASSERT_EQUALS(m3.get(1, 1), 46.);
  }

  void test_multiply_three_matrices() {
    GSLMatrix m1(2, 2);
    m1.set(0, 0, 1);
    m1.set(0, 1, 2);
    m1.set(1, 0, 3);
    m1.set(1, 1, 4);
    GSLMatrix m2(2, 2);
    m2.set(0, 0, 5);
    m2.set(0, 1, 6);
    m2.set(1, 0, 7);
    m2.set(1, 1, 8);
    GSLMatrix m3(2, 2);
    m3.set(0, 0, 9);
    m3.set(0, 1, 10);
    m3.set(1, 0, 11);
    m3.set(1, 1, 12);

    GSLMatrix m;

    m = m1.tr() * m2 * m3;

    TS_ASSERT_EQUALS(m.size1(), 2);
    TS_ASSERT_EQUALS(m.size2(), 2);

    for (size_t i = 0; i < m.size1(); ++i)
      for (size_t j = 0; j < m.size2(); ++j) {
        double d = 0.0;
        for (size_t k = 0; k < m2.size1(); ++k)
          for (size_t l = 0; l < m2.size2(); ++l) {
            d += m1.get(k, i) * m2.get(k, l) * m3.get(l, j);
          }
        TS_ASSERT_DELTA(d, m.get(i, j), 1e-8);
      }
  }

  void test_invert() {
    GSLMatrix m(2, 2);
    m.set(0, 0, 1);
    m.set(0, 1, 1);
    m.set(1, 0, 0);
    m.set(1, 1, 1);
    m.invert();
    TS_ASSERT_EQUALS(m.get(0, 0), 1);
    TS_ASSERT_EQUALS(m.get(0, 1), -1);
    TS_ASSERT_EQUALS(m.get(1, 0), 0);
    TS_ASSERT_EQUALS(m.get(1, 1), 1);
    m.set(0, 0, 2);
    m.set(0, 1, 0);
    m.set(1, 0, 0);
    m.set(1, 1, 2);
    m.invert();
    TS_ASSERT_EQUALS(m.get(0, 0), 0.5);
    TS_ASSERT_EQUALS(m.get(0, 1), 0);
    TS_ASSERT_EQUALS(m.get(1, 0), 0);
    TS_ASSERT_EQUALS(m.get(1, 1), 0.5);
  }

  void test_subMatrix() {
    GSLMatrix m(4, 4);
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

    GSLMatrix subm(m, 1, 1, 2, 2);
    TS_ASSERT_EQUALS(subm.get(0, 0), 11);
    TS_ASSERT_EQUALS(subm.get(0, 1), 12);
    TS_ASSERT_EQUALS(subm.get(1, 0), 21);
    TS_ASSERT_EQUALS(subm.get(1, 1), 22);
  }

  void test_subMatrix_fail() {
    GSLMatrix m(4, 4);
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

    TS_ASSERT_THROWS(GSLMatrix subm(m, 2, 2, 3, 3), const std::runtime_error &);
  }

  void test_eigenSystem_rectangular_throw() {
    GSLMatrix M(3, 4);
    GSLVector v;
    GSLMatrix Q;
    TS_ASSERT_THROWS(M.eigenSystem(v, Q), const std::runtime_error &);
  }

  void test_eigenSystem() {
    const size_t n = 4;
    GSLMatrix m(n, n);
    m.set(0, 0, 0);
    m.set(0, 1, 1);
    m.set(0, 2, 2);
    m.set(0, 3, 3);
    m.set(1, 0, 1);
    m.set(1, 1, 11);
    m.set(1, 2, 12);
    m.set(1, 3, 13);
    m.set(2, 0, 2);
    m.set(2, 1, 12);
    m.set(2, 2, 22);
    m.set(2, 3, 23);
    m.set(3, 0, 3);
    m.set(3, 1, 13);
    m.set(3, 2, 23);
    m.set(3, 3, 33);
    GSLVector v;
    GSLMatrix Q;
    GSLMatrix M = m;
    M.eigenSystem(v, Q);
    TS_ASSERT_EQUALS(v.size(), n);
    TS_ASSERT_EQUALS(Q.size1(), n);
    TS_ASSERT_EQUALS(Q.size2(), n);
    {
      GSLMatrix D = Q.tr() * m * Q;
      double trace_m = 0.0;
      double trace_D = 0.0;
      double det = 1.0;
      for (size_t i = 0; i < n; ++i) {
        TS_ASSERT_DELTA(D.get(i, i), v.get(i), 1e-10);
        trace_m += m.get(i, i);
        trace_D += D.get(i, i);
        det *= D.get(i, i);
      }
      TS_ASSERT_DELTA(trace_D, trace_m, 1e-10);
      TS_ASSERT_DELTA(det, m.det(), 1e-10);
    }
    {
      GSLMatrix D = Q.tr() * Q;
      for (size_t i = 0; i < n; ++i) {
        TS_ASSERT_DELTA(D.get(i, i), 1.0, 1e-10);
      }
    }
  }

  void test_copyColumn() {

    GSLMatrix m(4, 4);
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

    auto column = m.copyColumn(2);
    TS_ASSERT_EQUALS(column[0], m.get(0, 2));
    TS_ASSERT_EQUALS(column[1], m.get(1, 2));
    TS_ASSERT_EQUALS(column[2], m.get(2, 2));
    TS_ASSERT_EQUALS(column[3], m.get(3, 2));

    column[2] = 0;
    TS_ASSERT_EQUALS(m.get(2, 2), 22);
  }

  void test_copyRow() {

    GSLMatrix m(4, 4);
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

    auto row = m.copyRow(1);
    TS_ASSERT_EQUALS(row[0], m.get(1, 0));
    TS_ASSERT_EQUALS(row[1], m.get(1, 1));
    TS_ASSERT_EQUALS(row[2], m.get(1, 2));
    TS_ASSERT_EQUALS(row[3], m.get(1, 3));
    row[2] = 0;
    TS_ASSERT_EQUALS(m.get(1, 2), 12);
  }

  void test_index_operator() {
    GSLMatrix m(3, 3);
    m(0, 0) = 0.0;
    m(0, 1) = 1.0;
    m(0, 2) = 2.0;
    m(1, 0) = 10.0;
    m(1, 1) = 11.0;
    m(1, 2) = 12.0;
    m(2, 0) = 20.0;
    m(2, 1) = 21.0;
    m(2, 2) = 22.0;
    TS_ASSERT_EQUALS(m.get(0, 0), 0.0);
    TS_ASSERT_EQUALS(m.get(0, 1), 1.0);
    TS_ASSERT_EQUALS(m.get(0, 2), 2.0);
    TS_ASSERT_EQUALS(m.get(1, 0), 10.0);
    TS_ASSERT_EQUALS(m.get(1, 1), 11.0);
    TS_ASSERT_EQUALS(m.get(1, 2), 12.0);
    TS_ASSERT_EQUALS(m.get(2, 0), 20.0);
    TS_ASSERT_EQUALS(m.get(2, 1), 21.0);
    TS_ASSERT_EQUALS(m.get(2, 2), 22.0);

    TS_ASSERT_EQUALS(m(0, 0), 0.0);
    TS_ASSERT_EQUALS(m(0, 1), 1.0);
    TS_ASSERT_EQUALS(m(0, 2), 2.0);
    TS_ASSERT_EQUALS(m(1, 0), 10.0);
    TS_ASSERT_EQUALS(m(1, 1), 11.0);
    TS_ASSERT_EQUALS(m(1, 2), 12.0);
    TS_ASSERT_EQUALS(m(2, 0), 20.0);
    TS_ASSERT_EQUALS(m(2, 1), 21.0);
    TS_ASSERT_EQUALS(m(2, 2), 22.0);
  }

  void test_initializer_list() {
    gsl_set_error_handler_off();
    GSLMatrix m({{1.0, 2.0}, {4.0, 2.0}, {-1.0, -3.0}});
    TS_ASSERT_EQUALS(m.size1(), 3);
    TS_ASSERT_EQUALS(m.size2(), 2);
    TS_ASSERT_EQUALS(m(0, 0), 1.0);
    TS_ASSERT_EQUALS(m(1, 0), 4.0);
    TS_ASSERT_EQUALS(m(2, 0), -1.0);
    TS_ASSERT_EQUALS(m(0, 1), 2.0);
    TS_ASSERT_EQUALS(m(1, 1), 2.0);
    TS_ASSERT_EQUALS(m(2, 1), -3.0);

    TS_ASSERT_THROWS(GSLMatrix({{1.0, 2.0}, {4.0, 2.0, 0.0}, {-1.0, -3.0}}),
                     const std::runtime_error &);
  }

  void test_vector_mul() {
    gsl_set_error_handler_off();
    GSLMatrix m({{1.0, 2.0}, {4.0, 2.0}, {-1.0, -3.0}});
    GSLVector b({5.0, 2.0});
    GSLVector x = m * b;
    TS_ASSERT_EQUALS(x.size(), 3);
    TS_ASSERT_EQUALS(x[0], 9.0);
    TS_ASSERT_EQUALS(x[1], 24.0);
    TS_ASSERT_EQUALS(x[2], -11.0);
  }

  void test_solve_singular() {
    gsl_set_error_handler_off();
    GSLMatrix m({{0.0, 0.0}, {0.0, 0.0}});
    GSLVector b({1.0, 2.0});
    GSLVector x;
    TS_ASSERT_THROWS(m.solve(b, x), const std::runtime_error &);
  }

  void test_solve_singular_1() {
    gsl_set_error_handler_off();
    GSLMatrix m({{1.0, 2.0}, {2.0, 4.0}});
    GSLVector b({1.0, 2.0});
    GSLVector x;
    TS_ASSERT_THROWS(m.solve(b, x), const std::runtime_error &);
  }

  void test_solve() {
    gsl_set_error_handler_off();
    GSLMatrix m({{1.0, 2.0}, {4.0, 2.0}});
    GSLVector b({5.0, 2.0});
    GSLVector x;
    GSLMatrix mm = m;
    mm.solve(b, x);
    TS_ASSERT_EQUALS(x.size(), 2);
    TS_ASSERT_EQUALS(x[0], -1.0);
    TS_ASSERT_EQUALS(x[1], 3.0);
    GSLVector bb = m * x;
    TS_ASSERT_EQUALS(bb[0], 5.0);
    TS_ASSERT_EQUALS(bb[1], 2.0);
  }
};

#endif /*GSLMATRIXTEST_H_*/
