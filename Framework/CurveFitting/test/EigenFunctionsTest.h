// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EigenFunctions.h"

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit_nlin.h>
#include <math.h>

#include <iostream>

namespace {
static int is_zero(double x) { return x == 0.0; }

static int param_is_dropped(const gsl_matrix *covar, size_t i) {
  size_t n = covar->size1;
  for (size_t k = 0; k < n; ++k) {
    if (!is_zero(gsl_matrix_get(covar, i, k)))
      return 0; // row not all zero
    if (!is_zero(gsl_matrix_get(covar, k, i)))
      return 0; // col not all zero
  }
  return 1;
}

static int param_is_dropped(const Eigen::MatrixXd &covar, size_t i) {
  size_t n = covar.rows();
  for (size_t k = 0; k < n; ++k) {
    if (!is_zero(covar(i, k)))
      return 0; // row not all zero
    if (!is_zero(covar(k, i)))
      return 0; // col not all zero
  }
  return 1;
}

// As per the gsl docs, parameter i is considered dropped if its row and column are zero
// https://tool.oschina.net/uploads/apidocs/gsl/Computing-the-covariance-matrix-of-best-fit-parameters.html
static size_t count_dropped_params(const gsl_matrix *covar) {
  size_t n = covar->size1;
  size_t dropped = 0;
  for (size_t i = 0; i < n; ++i)
    dropped += (size_t)param_is_dropped(covar, i);
  return dropped;
}

static size_t count_dropped_params(const Eigen::MatrixXd &covar) {
  size_t n = covar.rows();
  size_t dropped = 0;
  for (size_t i = 0; i < n; ++i)
    dropped += (size_t)param_is_dropped(covar, i);
  return dropped;
}

/* Simple Jacobian with two columns that become dependent when J11 = 0 */
static gsl_matrix *make_J_2x2(double J11) {
  gsl_matrix *J = gsl_matrix_alloc(2, 2);
  gsl_matrix_set(J, 0, 0, 1.0);
  gsl_matrix_set(J, 1, 0, 1.0);
  gsl_matrix_set(J, 0, 1, 0.0);
  gsl_matrix_set(J, 1, 1, J11);
  return J;
}
} // namespace

class EigenFunctionsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EigenFunctionsTest *createSuite() { return new EigenFunctionsTest(); }
  static void destroySuite(EigenFunctionsTest *suite) { delete suite; }

  void assert_gsl_eigen_equal(const gsl_matrix *gslM, const Eigen::MatrixXd &eM) {
    if (gslM->size1 != (size_t)eM.rows() || gslM->size2 != (size_t)eM.cols()) {
      throw std::invalid_argument("Matricies are not of equal size");
    }
    for (size_t i = 0; i < gslM->size2; i++) {
      for (size_t j = 0; j < gslM->size1; j++) {
        TS_ASSERT_EQUALS(gsl_matrix_get(gslM, i, j), eM(i, j))
      }
    }
  }

  void eigen_gsl_equivalence_impl(double J11, double epsrel) {
    Mantid::CurveFitting::EigenMatrix J{{1, 0}, {1, J11}};

    gsl_matrix *JGsl = make_J_2x2(J11);
    gsl_matrix *covarGsl = gsl_matrix_calloc(2, 2);

    auto covar = Mantid::CurveFitting::covar_from_jacobian(J.mutator(), epsrel);
    gsl_multifit_covar(JGsl, epsrel, covarGsl);

    assert_gsl_eigen_equal(covarGsl, covar);
    TS_ASSERT_EQUALS(count_dropped_params(covarGsl), count_dropped_params(covar))

    gsl_matrix_free(covarGsl);
    gsl_matrix_free(JGsl);
  }

  void test_esprel_large() { eigen_gsl_equivalence_impl(1e-8, 1e-8); }

  void test_epsrel_small() { eigen_gsl_equivalence_impl(1e-8, 1e-9); }

  void test_epsrel_zero() { eigen_gsl_equivalence_impl(1e-8, 0); }

  void test_epsrel_zero_linear_dependence() { eigen_gsl_equivalence_impl(0, 0); }
};
