// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidCurveFitting/Functions/UserFunction.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class UserFunctionTest : public CxxTest::TestSuite {
public:
  class UserTestJacobian : public Jacobian {
    int m_nParams;
    std::vector<double> m_buffer;

  public:
    UserTestJacobian(int nData, int nParams) : m_nParams(nParams) { m_buffer.resize(nData * nParams); }
    void set(size_t iY, size_t iP, double value) override { m_buffer[iY * m_nParams + iP] = value; }
    double get(size_t iY, size_t iP) override { return m_buffer[iY * m_nParams + iP]; }
    void zero() override { m_buffer.assign(m_buffer.size(), 0.0); }
  };

  void testIt() {
    UserFunction fun;
    fun.setAttribute("Formula", UserFunction::Attribute("h*sin(a*x-c)"));
    fun.setParameter("h", 2.2);
    fun.setParameter("a", 2.0);
    fun.setParameter("c", 1.2);

    TS_ASSERT_EQUALS(fun.getParameter("h"), 2.2);
    TS_ASSERT_EQUALS(fun.getParameter("a"), 2.0);
    TS_ASSERT_EQUALS(fun.getParameter("c"), 1.2);
    TS_ASSERT_EQUALS(fun.asString(), "name=UserFunction,Formula=h*sin(a*x-c),h=2.2,a=2,c=1.2");
    TS_ASSERT_EQUALS(fun.getAttribute("Formula").asString(), "h*sin(a*x-c)");

    const size_t nParams = 3;
    const size_t nData = 10;
    std::vector<double> x(nData), y(nData);
    for (size_t i = 0; i < nData; i++) {
      x[i] = 0.1 * static_cast<double>(i);
    }
    fun.function1D(&y[0], &x[0], nData);
    for (size_t i = 0; i < nData; i++) {
      TS_ASSERT_DELTA(y[i], 2.2 * sin(2 * x[i] - 1.2), 0.000001);
    }

    FunctionDomain1DVector domain(x);
    UserTestJacobian J(nData, nParams);
    fun.functionDeriv(domain, J);

    for (size_t i = 0; i < nData; i++)
      for (size_t j = 0; j < nParams; j++) {
        double d = J.get(i, j);
        double dtrue;
        if (j == 0) {
          dtrue = sin(2 * x[i] - 1.2);
        } else if (j == 1) {
          dtrue = 2.2 * cos(2 * x[i] - 1.2) * x[i];
        } else {
          dtrue = -2.2 * cos(2 * x[i] - 1.2);
        }
        TS_ASSERT_DELTA(d, dtrue, 0.03);
      }

    // check its categories
    const std::vector<std::string> categories = fun.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "General");
  }

  void test_setAttribute_will_reevaluate_function_if_it_has_changed() {
    UserFunction fun;
    fun.setAttribute("Formula", UserFunction::Attribute("a*x"));
    fun.setParameter("a", 1.1);

    fun.setAttribute("Formula", UserFunction::Attribute("a*x+b"));

    // Check that the 'a' parameter has been reset
    TS_ASSERT_EQUALS(0.0, fun.getParameter("a"));
  }

  void test_setAttribute_will_not_reevaluate_function_if_the_function_has_not_changed() {
    UserFunction fun;
    fun.setAttribute("Formula", UserFunction::Attribute("a*x"));
    fun.setParameter("a", 1.1);

    fun.setAttribute("Formula", UserFunction::Attribute("a*x"));

    // Check that the 'a' parameter has not been reset
    TS_ASSERT_EQUALS(1.1, fun.getParameter("a"));
  }
};
