// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/EigenJacobian.h"
#include "MantidCurveFitting/EigenMatrix.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class EigenJacobianTest : public CxxTest::TestSuite {
public:
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
    const size_t size = 10;
    const double val = 5;

    EigenJacobian J(*test_fn, size);

    J.set(5, 1, val);
    J.set(9, 2, val * 3);
    TS_ASSERT_EQUALS(J.get(5, 1), val);
    TS_ASSERT_EQUALS(J.get(9, 2), val * 3);
  }

  void test_EigenJacobian_add_number_to_column() {
    auto test_fn = generate_tst_fn();
    const size_t size = 35;
    const double val = 5;

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
    TS_ASSERT_EQUALS(J.get((size_t)size - 1, 1), (size_t)val + 5);
  }

  void test_JacobianImpl1_get_and_set() {
    int size = 10;
    double val = 5;
    int n_params = 3;

    JacobianImpl1<EigenMatrix> J;

    J.m_index.reserve(n_params);
    for (int i = 0; i < n_params; ++i) {
      J.m_index.emplace_back(i);
    }

    EigenMatrix m(size, n_params);
    J.setJ(&m);

    J.set(5, 1, val);
    J.set(9, 2, val * 3);
    TS_ASSERT_EQUALS(J.get(5, 1), val);
    TS_ASSERT_EQUALS(J.get(9, 2), val * 3);
    TS_ASSERT_EQUALS(m(5, 1), val);
    TS_ASSERT_EQUALS(m(9, 2), val * 3);
  }

  void test_JacobianImpl1_add_number_to_column() {
    const size_t size = 35;
    const double val = 5;
    const int n_params = 3;

    JacobianImpl1<EigenMatrix> J;

    J.m_index.reserve(n_params);
    for (int i = 0; i < n_params; ++i) {
      J.m_index.emplace_back(i);
    }

    EigenMatrix m(size, n_params);
    J.setJ(&m);

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
