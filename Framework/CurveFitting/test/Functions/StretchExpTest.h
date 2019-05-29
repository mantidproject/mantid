// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef STRETCHEXPTEST_H_
#define STRETCHEXPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/StretchExp.h"

using namespace Mantid::CurveFitting::Functions;
class StretchExpTest_Jacobian : public Mantid::API::Jacobian {
  std::vector<double> m_values;

public:
  StretchExpTest_Jacobian() { m_values.resize(3); }
  void set(size_t, size_t iP, double value) override { m_values[iP] = value; }
  double get(size_t, size_t iP) override { return m_values[iP]; }
  void zero() override { m_values.assign(m_values.size(), 0.0); }
};

class StretchExpTest : public CxxTest::TestSuite {
public:
  void test_derivative_at_0() {
    Mantid::API::FunctionDomain1DVector x(0);
    StretchExpTest_Jacobian jac;
    StretchExp fn;
    fn.initialize();
    fn.setParameter("Height", 1.5);
    fn.setParameter("Lifetime", 5.0);
    fn.setParameter("Stretching", 0.4);
    fn.functionDeriv(x, jac);
    TS_ASSERT_EQUALS(jac.get(0, 2), 0.0);

    fn.setParameter("Stretching", 0.0);
    fn.functionDeriv(x, jac);
    TS_ASSERT_EQUALS(jac.get(0, 2), 0.0);

    Mantid::API::FunctionDomain1DVector x1(0.001);
    fn.functionDeriv(x1, jac);
    TS_ASSERT_DIFFERS(jac.get(0, 2), 0.0);
    fn.setParameter("Stretching", 0.4);
    fn.functionDeriv(x1, jac);
    TS_ASSERT_DIFFERS(jac.get(0, 2), 0.0);
  }

  void test_negative_x() {
    Mantid::API::FunctionDomain1DVector x(-0.001);
    Mantid::API::FunctionValues y(x);

    StretchExp fn;
    fn.initialize();
    fn.setParameter("Height", 1.5);
    fn.setParameter("Lifetime", 5.0);
    fn.setParameter("Stretching", 0.4);
    TS_ASSERT_THROWS(fn.function(x, y), const std::runtime_error &);
  }
};

#endif /*STRETCHEXPTEST_H_*/
