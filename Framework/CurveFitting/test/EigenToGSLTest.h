// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <MantidCurveFitting/EigenMatrix.h>
#include <MantidCurveFitting/EigenVector.h>
#include <MantidCurveFitting/GSLFunctions.h>
#include <gsl/gsl_blas.h>

// JACOBIAN TEST INCLUDES
#include "MantidCurveFitting/Functions/Gaussian.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

namespace {
Eigen::MatrixXd GenerateMatrix(int i, int j, bool random = false) {
  std::srand((unsigned int)time(0));
  if (random) {
    return Eigen::MatrixXd::Random(i, j).unaryExpr([](double x) -> double { return round(abs(x * 10)); });
  } else {
    double n = 0;
    return Eigen::MatrixXd::Zero(i, j).unaryExpr([&n](double x) -> double { return x + n++; });
  }
}

double *createArray(const int &nArraySize, bool random) {
  std::srand((unsigned int)time(0));
  double *array = new double[nArraySize];
  for (int i = 0; i < nArraySize; ++i)
    random ? array[i] = rand() % nArraySize : array[i] = i;
  return array;
}

Eigen::VectorXd createVecFromArray(double *array, const int &nElements) {
  Eigen::Map<Eigen::VectorXd> vec(array, nElements, 1);
  return vec;
}

Eigen::VectorXd GenerateVector(int i, bool random = false) { return createVecFromArray(createArray(i, random), i); }

} // namespace

class EigenToGSLTest : public CxxTest::TestSuite {
public:
  void test_EigenMatrix_to_GSL() {
    EigenMatrix m(10, 5);
    m = GenerateMatrix(10, 5);

    gsl_matrix_view m_gsl_view = getGSLMatrixView(m.mutator());
    gsl_matrix *m_gsl = &m_gsl_view.matrix;

    EigenMatrix m_tr = m.tr();

    for (size_t i = 0; i < m.size1(); i++) {
      for (size_t j = 0; j < m.size2(); j++) {
        TS_ASSERT_EQUALS(gsl_matrix_get(m_gsl, j, i), m_tr(j, i));
      }
    }

    // check reference is not broken
    gsl_matrix_set(m_gsl, 0, 0, -1.);
    TS_ASSERT_EQUALS(gsl_matrix_get(m_gsl, 0, 0), m(0, 0));
  }

  void test_EigenMatrix_to_GSL_const() {
    EigenMatrix m(10, 5);
    m = GenerateMatrix(10, 5);

    EigenMatrix m_tr = m.tr();

    const gsl_matrix_const_view m_gsl_view = getGSLMatrixView_const(m_tr.inspector());
    const gsl_matrix *m_gsl = &m_gsl_view.matrix;

    for (size_t i = 0; i < m.size1(); i++) {
      for (size_t j = 0; j < m.size2(); j++) {
        TS_ASSERT_EQUALS(gsl_matrix_get(m_gsl, i, j), m(i, j));
      }
    }
  }

  void test_EigenVector_to_GSL() {
    auto vec = GenerateVector(10);
    EigenVector v(&vec);

    gsl_vector_view v_gsl = getGSLVectorView(v.mutator());

    for (size_t i = 0; i < v.size(); i++) {
      TS_ASSERT_EQUALS(gsl_vector_get(&v_gsl.vector, i), v[i]);
    }

    // check reference is not broken
    gsl_vector_set(&v_gsl.vector, 0, -1.);
    TS_ASSERT_EQUALS(gsl_vector_get(&v_gsl.vector, 0), v[0]);
  }

  void test_EigenVector_to_GSL_const() {
    auto vec = GenerateVector(10);

    const EigenVector v(&vec);

    const gsl_vector_const_view v_gsl = getGSLVectorView_const(v.inspector());

    for (size_t i = 0; i < v.size(); i++) {
      TS_ASSERT_EQUALS(gsl_vector_get(&v_gsl.vector, i), v[i]);
    }
  }

  std::shared_ptr<Functions::Gaussian> generate_tst_fn() {
    std::shared_ptr<Functions::Gaussian> fn = std::make_shared<Functions::Gaussian>();
    fn->initialize();
    fn->setParameter("PeakCentre", 79440.0);
    fn->setParameter("Height", 200.0);
    fn->setParameter("Sigma", 30.0);
    return fn;
  }

  void test_EigenJacobian_initialise() {
    auto test_fn = generate_tst_fn();
    int size = 10;

    EigenJacobian J(*test_fn, size);
    TS_ASSERT_EQUALS(J.matrix().size1(), size);
    TS_ASSERT_EQUALS(J.matrix().size2(), test_fn->nParams());
  }

  void test_EigenJacobian_get_and_set() {
    auto test_fn = generate_tst_fn();
    int size = 10;
    int val = 5;

    EigenJacobian J(*test_fn, size);

    J.set(5, 1, val);
    J.set(9, 2, val * 3);
    TS_ASSERT_EQUALS(J.get(5, 1), val);
    TS_ASSERT_EQUALS(J.get(9, 2), val * 3);
  }

  void test_EigenJacobian_add_number_to_column() {
    auto test_fn = generate_tst_fn();
    int size = 35;
    int val = 5;

    EigenJacobian J(*test_fn, size);
    J.addNumberToColumn(val, 0);
    TS_ASSERT_EQUALS(J.get(0, 0), val);
    TS_ASSERT_EQUALS(J.get(9, 0), val);
    TS_ASSERT_EQUALS(J.get(19, 0), val);
    TS_ASSERT_EQUALS(J.get(29, 0), val);
    TS_ASSERT_EQUALS(J.get(size - 1, 0), val);

    J.addNumberToColumn(val + 5, 1);
    TS_ASSERT_EQUALS(J.get(0, 1), val + 5);
    TS_ASSERT_EQUALS(J.get(9, 1), val + 5);
    TS_ASSERT_EQUALS(J.get(19, 1), val + 5);
    TS_ASSERT_EQUALS(J.get(29, 1), val + 5);
    TS_ASSERT_EQUALS(J.get(size - 1, 1), val + 5);
  }
};