// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_COSTFUNCFITTINGTEST_H_
#define MANTID_CURVEFITTING_COSTFUNCFITTINGTEST_H_

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include <boost/make_shared.hpp>

using Mantid::CurveFitting::CostFunctions::CostFuncFitting;
using namespace Mantid::API;

namespace {

class CostFuncMock : public CostFuncFitting {
public:
  std::string name() const override { return "CostFuncMock"; }
  void addVal(FunctionDomain_sptr domain,
              FunctionValues_sptr values) const override {}
  void addValDerivHessian(IFunction_sptr function, FunctionDomain_sptr domain,
                          FunctionValues_sptr values, bool evalDeriv = true,
                          bool evalHessian = true) const override {}
  double val() const override { return 0.0; }
  void deriv(std::vector<double> &) const override {}
  double valAndDeriv(std::vector<double> &) const override { return 0.0; }
};
} // namespace

class CostFuncFittingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CostFuncFittingTest *createSuite() {
    return new CostFuncFittingTest();
  }
  static void destroySuite(CostFuncFittingTest *suite) { delete suite; }

  void test_parameterName() {
    CostFuncMock costFun;
    auto fun = FunctionFactory::Instance().createInitialized(
        "name=LinearBackground;name=ExpDecay");
    auto domain = boost::make_shared<FunctionDomain1DVector>(0);
    auto values = boost::make_shared<FunctionValues>(*domain);
    costFun.setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun.nParams(), 4);
    TS_ASSERT_EQUALS(costFun.parameterName(0), "f0.A0");
    TS_ASSERT_EQUALS(costFun.parameterName(2), "f1.Height");
    TS_ASSERT_EQUALS(costFun.parameterName(3), "f1.Lifetime");
    fun->fix(1);
    costFun.reset();
    TS_ASSERT_EQUALS(costFun.nParams(), 3);
    TS_ASSERT_EQUALS(costFun.parameterName(0), "f0.A0");
    TS_ASSERT_EQUALS(costFun.parameterName(1), "f1.Height");
    TS_ASSERT_EQUALS(costFun.parameterName(2), "f1.Lifetime");
  }
};

#endif /* MANTID_CURVEFITTING_COSTFUNCFITTINGTEST_H_ */
