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

using namespace Mantid::CurveFitting;

namespace {
Eigen::MatrixXd GenerateMatrix(int i, int j, bool random = false) {
  std::srand((unsigned int)time(0));
  if (random) {
    return Eigen::MatrixXd::Random(i, j).unaryExpr([](double x) -> double { return round(abs(x * 10)); });
  } else {
    double n = 0;
    return Eigen::MatrixXd(i, j).unaryExpr([&n](double x) -> double { return x + n++; });
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
};