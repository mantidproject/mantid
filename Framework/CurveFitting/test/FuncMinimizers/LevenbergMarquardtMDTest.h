// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CURVEFITTING_LEVENBERGMARQUARDMDTTEST_H_
#define CURVEFITTING_LEVENBERGMARQUARDMDTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/Functions/BSpline.h"
#include "MantidCurveFitting/Functions/UserFunction.h"

#include "MantidTestHelpers/MultiDomainFunctionHelper.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::FuncMinimisers;
using namespace Mantid::CurveFitting::CostFunctions;
using namespace Mantid::CurveFitting::Constraints;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class LevenbergMarquardtMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LevenbergMarquardtMDTest *createSuite() {
    return new LevenbergMarquardtMDTest();
  }
  static void destroySuite(LevenbergMarquardtMDTest *suite) { delete suite; }

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

    LevenbergMarquardtMDMinimizer s;
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
    fun->setParameter("b", 2.);
    fun->setParameter("h", 3.);
    fun->setParameter("s", 0.1);
    fun->fix(0);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 3);

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.2, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("a"), 1., 0.000001);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.90, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("h"), 2.67, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("s"), 0.27, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
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
    fun->setParameter("b", 2.);
    fun->setParameter("h", 3.);
    fun->setParameter("s", 0.1);
    fun->tie("a", "1");

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 3);

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.2, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("a"), 1., 0.000001);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.90, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("h"), 2.67, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("s"), 0.27, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_Gaussian_tied_with_formula() {
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
    fun->tie("b", "2*a+0.1");

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 3);

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.002, 0.01);
    double a = fun->getParameter("a");
    TS_ASSERT_DELTA(a, 1.0895, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2 * a + 0.1, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("h"), 3.23, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("s"), 0.207, 0.001);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_Linear_constrained() {
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

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    TS_ASSERT_DELTA(fun->getParameter("a"), 0.5, 0.1);
    TS_ASSERT_DELTA(fun->getParameter("b"), 5.0, 0.1);
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
    fun->setConstraintPenaltyFactor("a", 1.e20);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 2);

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    // std::cerr << "a=" << fun->getParameter("a") << '\n';
    // std::cerr << "b=" << fun->getParameter("b") << '\n';

    TS_ASSERT_DELTA(costFun->val(), 0.00, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("a"), 1.0, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("b"), 2.0, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_BSpline_fit_uniform() {
    double startx = -3.14;
    double endx = 3.14;

    boost::shared_ptr<BSpline> bsp = boost::make_shared<BSpline>();
    bsp->setAttributeValue("Order", 3);
    bsp->setAttributeValue("NBreak", 10);
    bsp->setAttributeValue("StartX", startx);
    bsp->setAttributeValue("EndX", endx);

    double chi2 = fitBSpline(bsp, "sin(x)");
    TS_ASSERT_DELTA(chi2, 1e-4, 1e-5);

    FunctionDomain1DVector x(startx, endx, 100);
    FunctionValues y(x);
    bsp->function(x, y);

    for (size_t i = 0; i < x.size(); ++i) {
      double xx = x[i];
      TS_ASSERT_DELTA(y[i], sin(xx), 0.003);
    }
  }

  void test_BSpline_fit_uniform_finer() {
    double startx = -3.14;
    double endx = 3.14;

    boost::shared_ptr<BSpline> bsp = boost::make_shared<BSpline>();
    bsp->setAttributeValue("Order", 3);
    bsp->setAttributeValue("NBreak", 20);
    bsp->setAttributeValue("StartX", startx);
    bsp->setAttributeValue("EndX", endx);

    double chi2 = fitBSpline(bsp, "sin(x)");
    TS_ASSERT_DELTA(chi2, 1e-6, 1e-7);

    FunctionDomain1DVector x(startx, endx, 100);
    FunctionValues y(x);
    bsp->function(x, y);

    for (size_t i = 0; i < x.size(); ++i) {
      double xx = x[i];
      TS_ASSERT_DELTA(y[i], sin(xx), 0.0003);
    }
  }

  void test_BSpline_fit_nonuniform() {
    double startx = 0.0;
    double endx = 6.28;

    boost::shared_ptr<BSpline> bsp = boost::make_shared<BSpline>();
    bsp->setAttributeValue("Order", 3);
    bsp->setAttributeValue("NBreak", 10);
    bsp->setAttributeValue("StartX", startx);
    bsp->setAttributeValue("EndX", endx);

    // this function changes faster at the lower end
    // fit it with uniform break points first
    double chi2 = fitBSpline(bsp, "sin(10/(x+1))");
    TS_ASSERT_DELTA(chi2, 0.58, 0.005);

    // now do a nonuniform fit. increase density of break points at lower end
    std::vector<double> breaks = bsp->getAttribute("BreakPoints").asVector();
    breaks[1] = 0.3;
    breaks[2] = 0.5;
    breaks[3] = 1.0;
    breaks[4] = 1.5;
    breaks[5] = 2.0;
    breaks[6] = 3.0;
    bsp->setAttributeValue("Uniform", false);
    bsp->setAttributeValue("BreakPoints", breaks);
    chi2 = fitBSpline(bsp, "sin(10/(x+1))");
    TS_ASSERT_DELTA(chi2, 0.0055, 5e-5);
  }

  void test_BSpline_derivative() {

    double startx = -3.14;
    double endx = 3.14;

    boost::shared_ptr<BSpline> bsp = boost::make_shared<BSpline>();
    bsp->setAttributeValue("Order", 3);
    bsp->setAttributeValue("NBreak", 30);
    bsp->setAttributeValue("StartX", startx);
    bsp->setAttributeValue("EndX", endx);

    double chi2 = fitBSpline(bsp, "sin(x)");
    TS_ASSERT_DELTA(chi2, 1e-7, 5e-8);

    FunctionDomain1DVector x(startx, endx, 100);
    FunctionValues y(x);
    bsp->derivative(x, y); // first derivative

    for (size_t i = 0; i < x.size(); ++i) {
      double xx = x[i];
      TS_ASSERT_DELTA(y[i], cos(xx), 0.005);
    }
  }

  void test_BSpline_derivative_2() {

    double startx = -3.14;
    double endx = 3.14;

    boost::shared_ptr<BSpline> bsp = boost::make_shared<BSpline>();
    bsp->setAttributeValue("Order", 4);
    bsp->setAttributeValue("NBreak", 30);
    bsp->setAttributeValue("StartX", startx);
    bsp->setAttributeValue("EndX", endx);

    double chi2 = fitBSpline(bsp, "sin(x)");
    TS_ASSERT_DELTA(chi2, 2e-10, 1e-10);

    FunctionDomain1DVector x(startx, endx, 100);
    FunctionValues y(x);
    bsp->derivative(x, y, 2); // second derivative

    for (size_t i = 0; i < x.size(); ++i) {
      double xx = x[i];
      TS_ASSERT_DELTA(y[i], -sin(xx), 0.005);
    }
  }

  void test_BSpline_derivative_3() {

    double startx = -3.14;
    double endx = 3.14;

    boost::shared_ptr<BSpline> bsp = boost::make_shared<BSpline>();
    bsp->setAttributeValue("Order", 5);
    bsp->setAttributeValue("NBreak", 20);
    bsp->setAttributeValue("StartX", startx);
    bsp->setAttributeValue("EndX", endx);

    double chi2 = fitBSpline(bsp, "sin(x)");
    TS_ASSERT_DELTA(chi2, 1e-11, 5e-12);

    FunctionDomain1DVector x(startx, endx, 100);
    FunctionValues y(x);
    bsp->derivative(x, y, 3); // third derivative

    for (size_t i = 0; i < x.size(); ++i) {
      double xx = x[i];
      TS_ASSERT_DELTA(y[i], -cos(xx), 0.012);
    }
  }

  void test_Multidomain() {
    auto domain = Mantid::TestHelpers::makeMultiDomainDomain3();

    auto values = boost::make_shared<FunctionValues>(*domain);
    const double A0 = 0, A1 = 1, A2 = 2;
    const double B0 = 1, B1 = 2, B2 = 3;

    auto &d0 = static_cast<const FunctionDomain1D &>(domain->getDomain(0));
    for (size_t i = 0; i < d0.size(); ++i) {
      values->setFitData(i, A0 + A1 + A2 + (B0 + B1 + B2) * d0[i]);
    }

    auto &d1 = static_cast<const FunctionDomain1D &>(domain->getDomain(1));
    for (size_t i = 0; i < d1.size(); ++i) {
      values->setFitData(9 + i, A0 + A1 + (B0 + B1) * d1[i]);
    }

    auto &d2 = static_cast<const FunctionDomain1D &>(domain->getDomain(2));
    for (size_t i = 0; i < d2.size(); ++i) {
      values->setFitData(19 + i, A0 + A2 + (B0 + B2) * d2[i]);
    }
    values->setFitWeights(1);

    auto multi = Mantid::TestHelpers::makeMultiDomainFunction3();

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(multi, domain, values);
    TS_ASSERT_EQUALS(costFun->nParams(), 6);

    FuncMinimisers::LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    TS_ASSERT_EQUALS(s.getError(), "success");
    TS_ASSERT_DELTA(s.costFunctionVal(), 0, 1e-4);

    TS_ASSERT_DELTA(multi->getFunction(0)->getParameter("A"), 0, 1e-8);
    TS_ASSERT_DELTA(multi->getFunction(0)->getParameter("B"), 1, 1e-8);
    TS_ASSERT_DELTA(multi->getFunction(1)->getParameter("A"), 1, 1e-8);
    TS_ASSERT_DELTA(multi->getFunction(1)->getParameter("B"), 2, 1e-8);
    TS_ASSERT_DELTA(multi->getFunction(2)->getParameter("A"), 2, 1e-8);
    TS_ASSERT_DELTA(multi->getFunction(2)->getParameter("B"), 3, 1e-8);
  }

private:
  double fitBSpline(boost::shared_ptr<IFunction> bsp, std::string func) {
    const double startx = bsp->getAttribute("StartX").asDouble();
    const double endx = bsp->getAttribute("EndX").asDouble();

    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(startx, endx, 100));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", func);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(bsp, domain, values);

    FuncMinimisers::LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    return costFun->val();
  }
};

#endif /*CURVEFITTING_LevenbergMarquardtMDTest_H_*/
