// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CURVEFITTING_TRUSTREGIONMINIMIZERTTEST_H_
#define CURVEFITTING_TRUSTREGIONMINIMIZERTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FuncMinimizers/TrustRegionMinimizer.h"
#include "MantidCurveFitting/Functions/UserFunction.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::FuncMinimisers;
using namespace Mantid::CurveFitting::CostFunctions;
using namespace Mantid::CurveFitting::Constraints;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class TrustRegionMinimizerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TrustRegionMinimizerTest *createSuite() {
    return new TrustRegionMinimizerTest();
  }
  static void destroySuite(TrustRegionMinimizerTest *suite) { delete suite; }

  void test_Linear() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(0.0, 10.0, 20));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", "a*x+b");
    dataMaker.setParameter("a", 1.1);
    dataMaker.setParameter("b", 2.2);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b");
    fun->setParameter("a", 1.);
    fun->setParameter("b", 2.);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 2);

    TrustRegionMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    TS_ASSERT_DELTA(fun->getParameter("a"), 1.1, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.2, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_Gaussian() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(0.0, 10.0, 20));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", "a*x+b+h*exp(-s*x^2)");
    dataMaker.setParameter("a", 1.1);
    dataMaker.setParameter("b", 2.2);
    dataMaker.setParameter("h", 3.3);
    dataMaker.setParameter("s", 0.2);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b+h*exp(-s*x^2)");
    fun->setParameter("a", 1.);
    fun->setParameter("b", 2.);
    fun->setParameter("h", 3.);
    fun->setParameter("s", 0.1);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);

    TrustRegionMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.0, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("a"), 1.1, 0.001);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.2, 0.001);
    TS_ASSERT_DELTA(fun->getParameter("h"), 3.3, 0.001);
    TS_ASSERT_DELTA(fun->getParameter("s"), 0.2, 0.001);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_Gaussian_fixed() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(0.0, 10.0, 20));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", "a*x+b+h*exp(-s*x^2)");
    dataMaker.setParameter("a", 1.1);
    dataMaker.setParameter("b", 2.2);
    dataMaker.setParameter("h", 3.3);
    dataMaker.setParameter("s", 0.2);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b+h*exp(-s*x^2)");
    fun->setParameter("a", 1.);
    fun->setParameter("b", 2.5);
    fun->setParameter("h", 3.);
    fun->setParameter("s", 0.1);
    fun->fix(0);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 3);

    TrustRegionMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize()); //
    TS_ASSERT_DELTA(costFun->val(), 0.2, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("a"), 1., 0.000001);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.90, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("h"), 2.67, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("s"), 0.27, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success"); // Failed to converge
  }

  void test_Gaussian_tied() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(0.0, 10.0, 20));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", "a*x+b+h*exp(-s*x^2)");
    dataMaker.setParameter("a", 1.1);
    dataMaker.setParameter("b", 2.2);
    dataMaker.setParameter("h", 3.3);
    dataMaker.setParameter("s", 0.2);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b+h*exp(-s*x^2)");
    fun->setParameter("a", 1.);
    fun->setParameter("b", 2.5);
    fun->setParameter("h", 3.);
    fun->setParameter("s", 0.1);
    fun->tie("a", "1");

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 3);

    TrustRegionMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize()); //
    TS_ASSERT_DELTA(costFun->val(), 0.2, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("a"), 1., 0.000001);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.90, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("h"), 2.67, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("s"), 0.27, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success"); // Failed to converge
  }

  void xtest_Linear_constrained() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(0.0, 10.0, 20));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", "a*x+b");
    dataMaker.setParameter("a", 1.1);
    dataMaker.setParameter("b", 2.2);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b");
    fun->setParameter("a", 1.);
    fun->setParameter("b", 2.);

    fun->addConstraint(
        std::make_unique<BoundaryConstraint>(fun.get(), "a", 0, 0.5));

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 2);

    TrustRegionMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    TS_ASSERT_DELTA(fun->getParameter("a"), 0.5, 0.1); // got 1.1
    TS_ASSERT_DELTA(fun->getParameter("b"), 5.2, 0.2); // got 2.2
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_Linear_constrained1() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(0.0, 10.0, 20));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", "a^2*x+b");
    dataMaker.setParameter("a", 1);
    dataMaker.setParameter("b", 2);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a^2*x+b");
    fun->setParameter("a", -0.5);
    fun->setParameter("b", 2.2);

    // lower bound is made > 0 because function's derivative over "a" at a=0 is
    // 0
    fun->addConstraint(
        std::make_unique<BoundaryConstraint>(fun.get(), "a", 0.001, 2.0));

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 2);

    TrustRegionMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    TS_ASSERT_DELTA(costFun->val(), 0.00, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("a"), 1.0, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.0, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }
};

#endif /*CURVEFITTING_TRUSTREGIONMINIMIZERTTEST_H_*/
