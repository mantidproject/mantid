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

class EigenViewTest : public CxxTest::TestSuite {
public:
  void test_EigenMatrixView() {
    Eigen::MatrixXd m = GenerateMatrix(4, 5, true);
    EigenMatrix_View test_m_1 = EigenMatrix_View(m);
    TS_ASSERT(m == test_m_1.matrix_mutator()); // test matrix are equal upon creation of view

    double d = 5;
    m(Eigen::Index(1), Eigen::Index(1)) = d;

    test_m_1.matrix_mutator()(0, 1) = -2.0;
    TS_ASSERT(m == test_m_1.matrix_mutator()); // test view and matrix reference same matrix

    EigenMatrix_View test_m_2 = test_m_1;
    TS_ASSERT(test_m_2.matrix_inspector() == test_m_1.matrix_inspector()); // test copy assignment operator

    m(Eigen::Index(0), Eigen::Index(0)) = -3.0;
    TS_ASSERT(test_m_2.matrix_mutator() == m &&
              test_m_1.matrix_mutator() == m); // test copied views still reference the original matrix.

    EigenMatrix_View test_m_3 =
        EigenMatrix_View(test_m_2.matrix_mutator().data(), test_m_2.rows(), test_m_2.cols(), 2, 2, 2, 2);
    TS_ASSERT(test_m_3.matrix_inspector()(0, 0) == test_m_1.matrix_inspector()(2, 2) &&
              test_m_3.matrix_inspector()(0, 1) == test_m_1.matrix_inspector()(2, 3) &&
              test_m_3.matrix_inspector()(1, 0) == test_m_1.matrix_inspector()(3, 2) &&
              test_m_3.matrix_inspector()(1, 1) ==
                  test_m_1.matrix_inspector()(3, 3)); // test view sub_matrix considers correct elements

    test_m_3.matrix_mutator()(0, 1) = -4.0;
    TS_ASSERT(m(Eigen::Index(3), Eigen::Index(3)) ==
              test_m_3.matrix_mutator()(1, 1)); // test sub-matrix still references original matrix.

    EigenMatrix_View test_m_4 = EigenMatrix_View(test_m_3.matrix_mutator());
    test_m_4.matrix_mutator()(1, 1) = -5.0;
    TS_ASSERT(test_m_4.matrix_inspector() == test_m_3.matrix_inspector()); // test creation of matrix with map.
  }

  void test_EigenVectorView() {
    Eigen::VectorXd v = GenerateVector(10, true);
    EigenVector_View test_v_1 = EigenVector_View(v);
    TS_ASSERT(test_v_1.vector_inspector() == v); // test matrix are equal upon creation of view

    test_v_1.vector_mutator()(0) = -1.0;
    TS_ASSERT(test_v_1.vector_inspector() == v); // test view and matrix reference same matrix

    EigenVector_View test_v_2 = test_v_1;
    test_v_1.vector_mutator()(1) = -2.0;
    TS_ASSERT(test_v_1.vector_inspector() == test_v_2.vector_inspector()); // test copy constructor

    EigenVector_View test_v_3 = EigenVector_View(test_v_1.vector_mutator().data(), 3, 4);
    test_v_3.vector_mutator()(0) = -3.0;

    TS_ASSERT(test_v_3.vector_inspector()(0) == test_v_1.vector_inspector()(4) &&
              test_v_3.vector_inspector()(1) == test_v_1.vector_inspector()(5) &&
              test_v_3.vector_inspector()(2) ==
                  test_v_1.vector_inspector()(6)); // test view sub_matrix considers correct elements

    EigenVector_View test_v_4 = EigenVector_View(test_v_1.vector_inspector(), 3, 4);
    TS_ASSERT(test_v_3.vector_inspector() == test_v_4.vector_inspector()); // test view creation using map
  }
};
