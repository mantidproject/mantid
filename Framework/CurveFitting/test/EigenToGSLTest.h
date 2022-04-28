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
    return Eigen::MatrixXd::Random(i, j).unaryExpr([](double x) { return round(abs(x * 10)); });
  } else {
    double n = 0;
    return Eigen::MatrixXd(i, j).unaryExpr([&n](double x) { return ++n; });
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

    EigenMatrix m_tr = m.tr();

    gsl_matrix *m_gsl = &getGSLMatrixView(m.mutator()).matrix;

    for (int i = 0; i < m.size1(); i++) {
      for (int j = 0; j < m.size2(); j++) {
        TS_ASSERT_EQUALS(gsl_matrix_get(m_gsl, j, i), m_tr(j, i));
      }
    }
  }

  void test_EigenMatrix_to_GSL_const() {
    EigenMatrix m(10, 5);
    m = GenerateMatrix(10, 5);

    EigenMatrix m_tr = m.tr();
    const gsl_matrix *m_gsl = &getGSLMatrixView_const(m_tr.inspector()).matrix;

    for (int i = 0; i < m.size1(); i++) {
      for (int j = 0; j < m.size2(); j++) {
        TS_ASSERT_EQUALS(gsl_matrix_get(m_gsl, i, j), m(i, j));
      }
    }
  }

  void test_EigenVector_to_GSL() {
    EigenVector v(&GenerateVector(10));

    gsl_vector *v_gsl = &getGSLVectorView(v.mutator()).vector;

    for (int i = 0; i < v.size(); i++) {
      TS_ASSERT_EQUALS(gsl_vector_get(v_gsl, i), v[i]);
    }
  }

  void test_EigenVector_to_GSL_const() {
    const EigenVector v(&GenerateVector(10));

    const gsl_vector *v_gsl = &getGSLVectorView_const(v.inspector()).vector;

    for (int i = 0; i < v.size(); i++) {
      TS_ASSERT_EQUALS(gsl_vector_get(v_gsl, i), v[i]);
    }
  }
};