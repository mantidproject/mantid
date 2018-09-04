#ifndef CURVEFITTING_LEASTSQUARESTEST_H_
#define CURVEFITTING_LEASTSQUARESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/CostFunctions/CostFuncRwp.h"
#include "MantidCurveFitting/FuncMinimizers/BFGS_Minimizer.h"
#include "MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/FuncMinimizers/SimplexMinimizer.h"
#include "MantidCurveFitting/Functions/ExpDecay.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/LinearBackground.h"
#include "MantidCurveFitting/Functions/UserFunction.h"

#include <gsl/gsl_blas.h>
#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::FuncMinimisers;
using namespace Mantid::CurveFitting::CostFunctions;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class LeastSquaresTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeastSquaresTest *createSuite() { return new LeastSquaresTest(); }
  static void destroySuite(LeastSquaresTest *suite) { delete suite; }

  void test_With_Simplex() {
    std::vector<double> x(10), y(10);
    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = 3.3 * x[i] + 4.4;
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b");
    fun->setParameter("a", 1.1);
    fun->setParameter("b", 2.2);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);

    SimplexMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.0000, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("a"), 3.3, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("b"), 4.4, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_With_Simplex_Rwp() {
    std::vector<double> x(10), y(10), sqrty(10);
    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = 3.3 * x[i] + 4.4;
      sqrty[i] = sqrt(fabs(y[i]));
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(sqrty);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b");
    fun->setParameter("a", 1.1);
    fun->setParameter("b", 2.2);

    boost::shared_ptr<CostFuncRwp> costFun = boost::make_shared<CostFuncRwp>();
    costFun->setFittingFunction(fun, domain, values);

    SimplexMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.0, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("a"), 3.3, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("b"), 4.4, 0.01);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_With_BFGS() {
    std::vector<double> x(10), y(10);
    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = 9.9 * exp(-(x[i]) / 0.5);
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", 1.);
    fun->setParameter("Lifetime", 1.);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);

    BFGS_Minimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.0, 1e-7);
    TS_ASSERT_DELTA(fun->getParameter("Height"), 9.9, 1e-4);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-4);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_val_deriv_valAndDeriv() {
    const double a = 1.0;
    const double b = 2.0;
    std::vector<double> x(3), y(3);
    x[0] = 0.;
    y[0] = a * x[0] + b; // == 2.0
    x[1] = 1.;
    y[1] = a * x[1] + b; // == 3.0
    x[2] = 2.;
    y[2] = a * x[2] + b; // == 4.0
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b");
    fun->setParameter("a", 1.1);
    fun->setParameter("b", 2.2);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_DELTA(costFun->val(), 0.145,
                    1e-10); // == 0.5 *( 0.2^2 + 0.3^2 + 0.4^2 )

    std::vector<double> der;
    costFun->deriv(der);
    TS_ASSERT_EQUALS(der.size(), 2);
    TS_ASSERT_DELTA(der[0], 1.1, 1e-10); // == 0 * 0.2 + 1 * 0.3 + 2 * 0.4
    TS_ASSERT_DELTA(der[1], 0.9, 1e-10); // == 1 * 0.2 + 1 * 0.3 + 1 * 0.4

    std::vector<double> der1;
    TS_ASSERT_DELTA(costFun->valAndDeriv(der1), 0.145, 1e-10);
    TS_ASSERT_EQUALS(der1.size(), 2);
    TS_ASSERT_DELTA(der1[0], 1.1, 1e-10);
    TS_ASSERT_DELTA(der1[1], 0.9, 1e-10);

    TS_ASSERT_DELTA(costFun->valDerivHessian(), 0.145, 1e-10);
    const GSLVector &g = costFun->getDeriv();
    // const GSLMatrix& H = costFun->getHessian();
    TS_ASSERT_DELTA(g.get(0), 1.1, 1e-10);
    TS_ASSERT_DELTA(g.get(1), 0.9, 1e-10);
  }

  void test_linear_correction_is_good_approximation() {
    const double a = 1.0;
    const double b = 2.0;
    std::vector<double> x(3), y(3);
    x[0] = 0.;
    y[0] = a * x[0] + b; // == 2.0
    x[1] = 1.;
    y[1] = a * x[1] + b; // == 3.0
    x[2] = 2.;
    y[2] = a * x[2] + b; // == 4.0
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun = boost::make_shared<UserFunction>();
    fun->setAttributeValue("Formula", "a*x+b");
    fun->setParameter("a", 1.1);
    fun->setParameter("b", 2.2);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);
    TS_ASSERT_DELTA(costFun->val(), 0.145,
                    1e-10); // == 0.5 *( 0.2^2 + 0.3^2 + 0.4^2 )

    TS_ASSERT_DELTA(costFun->valDerivHessian(), 0.145, 1e-10);
    GSLVector g = costFun->getDeriv();
    const GSLMatrix &H = costFun->getHessian();

    GSLVector dx(2);
    dx.set(0, -0.1);
    dx.set(1, -0.2);

    double L; // = d*dx + 0.5 * dx * H * dx
    gsl_blas_dgemv(CblasNoTrans, 0.5, H.gsl(), dx.gsl(), 1., g.gsl());
    gsl_blas_ddot(g.gsl(), dx.gsl(), &L);
    TS_ASSERT_DELTA(L, -0.145, 1e-10); // L + costFun->val() == 0
  }

  void test_Fixing_parameter() {
    std::vector<double> x(10), y(10);
    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = 9.9 * exp(-(x[i]) / 0.5);
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", 1.);
    fun->setParameter("Lifetime", 1.);
    fun->fix(1);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fun, domain, values);

    BFGS_Minimizer s;
    s.initialize(costFun);

    TS_ASSERT_DELTA(costFun->val(), 112.0, 0.1);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 7.84, 0.1);

    TS_ASSERT_DELTA(fun->getParameter("Height"), 7.6, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 1.0, 1e-9);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void test_With_LM_Rwp() {
    std::vector<double> x(10), y(10), e(10);
    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = 9.9 * exp(-(x[i]) / 0.5);
      e[i] = 1. / sqrt(y[i]);
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(e);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", 19.);
    fun->setParameter("Lifetime", 0.1);

    boost::shared_ptr<CostFuncRwp> costFun = boost::make_shared<CostFuncRwp>();
    costFun->setFittingFunction(fun, domain, values);

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);

    TS_ASSERT_DELTA(costFun->val(), 0.64, 0.05);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(), 0.0000, 0.00001);

    TS_ASSERT_DELTA(fun->getParameter("Height"), 9.9, 0.01);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 1e-9);
    TS_ASSERT_EQUALS(s.getError(), "success");
  }

  void testDerivatives() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(79300., 79600., 41));
    API::FunctionValues_sptr data(new API::FunctionValues(*domain));
    boost::shared_ptr<UserFunction> fun0 = boost::make_shared<UserFunction>();
    fun0->setAttributeValue("Formula", "b + h * exp(-((x-c)/s)^2)");
    fun0->setParameter("b", 9);
    fun0->setParameter("h", 224.);
    fun0->setParameter("c", 79430.1);
    fun0->setParameter("s", 27.4);
    fun0->function(*domain, *data);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(*data);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun1 = boost::make_shared<UserFunction>();
    fun1->setAttributeValue("Formula", "b + h * exp(-((x-c)/s)^2)");
    fun1->setParameter("b", 0);
    fun1->setParameter("h", 200.);
    fun1->setParameter("c", 79450.);
    fun1->setParameter("s", 300);
    fun1->function(*domain, *data);

    API::CompositeFunction_sptr fnWithBk(new API::CompositeFunction());

    boost::shared_ptr<LinearBackground> bk =
        boost::make_shared<LinearBackground>();
    bk->initialize();

    bk->setParameter("A0", 0.0);
    bk->setParameter("A1", 0.0);
    bk->tie("A1", "0");

    // set up Gaussian fitting function
    boost::shared_ptr<Gaussian> fn = boost::make_shared<Gaussian>();
    fn->initialize();
    fn->setParameter("PeakCentre", 79450.0);
    fn->setParameter("Height", 200.0);
    fn->setParameter("Sigma", 300);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    // costFun->setFittingFunction(fun1,domain,values);
    costFun->setFittingFunction(fnWithBk, domain, values);

    size_t n = costFun->nParams();
    // system("pause");
    // double f0 = costFun->val();
    // std::cerr << "fun=" << f0 << '\n';
    // const GSLVector& g = costFun->getDeriv();
    // const GSLMatrix& H = costFun->getHessian();

    for (size_t i = 0; i < n; ++i) {
      double dp = 0.000001;
      double p1 = costFun->getParameter(i) + dp;
      costFun->setParameter(i, p1);
      // double f1 = costFun->val();
      costFun->setParameter(i, p1 - dp);
      // std::cerr << "deriv " << i << ' ' << p1 << ' ' << (f1 - f0) / dp << ' '
      // << g.get(i)  << '\n';
      // for(size_t j = 0; j <= i; ++j)
      //{
      //}
    }
  }
};

#endif /*CURVEFITTING_LEASTSQUARESTEST_H_*/
