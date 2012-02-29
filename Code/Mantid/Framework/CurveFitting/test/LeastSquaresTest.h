#ifndef CURVEFITTING_LEASTSQUARESTEST_H_
#define CURVEFITTING_LEASTSQUARESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidCurveFitting/BFGS_Minimizer.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/ExpDecay.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include <gsl/gsl_blas.h>
#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class LeastSquaresTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeastSquaresTest *createSuite() { return new LeastSquaresTest(); }
  static void destroySuite( LeastSquaresTest *suite ) { delete suite; }

  void test_With_Simplex()
  {
    std::vector<double> x(10),y(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      x[i] = 0.1 * i;
      y[i] = 3.3 * x[i] + 4.4;
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun(new UserFunction);
    fun->setAttributeValue("Formula","a*x+b");
    fun->setParameter("a",1.1);
    fun->setParameter("b",2.2);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(fun,domain,values);

    SimplexMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(),0.0,0.0001);
    TS_ASSERT_DELTA(fun->getParameter("a"),3.3,0.01);
    TS_ASSERT_DELTA(fun->getParameter("b"),4.4,0.01);
    TS_ASSERT_EQUALS(s.getError(),"success");
  }

  void test_With_BFGS()
  {
    std::vector<double> x(10),y(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      x[i] = 0.1 * i;
      y[i] =  9.9 * exp( -(x[i])/0.5 );
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",1.);
    fun->setParameter("Lifetime",1.);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(fun,domain,values);

    BFGS_Minimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(),0.0,1e-7);
    TS_ASSERT_DELTA(fun->getParameter("Height"),9.9,1e-4);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"),0.5,1e-4);
    TS_ASSERT_EQUALS(s.getError(),"success");

  }

  void test_val_deriv_valAndDeriv()
  {
    const double a = 1.0;
    const double b = 2.0;
    std::vector<double> x(3),y(3);
    x[0] = 0.; y[0] = a * x[0] + b; // == 2.0
    x[1] = 1.; y[1] = a * x[1] + b; // == 3.0
    x[2] = 2.; y[2] = a * x[2] + b; // == 4.0
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun(new UserFunction);
    fun->setAttributeValue("Formula","a*x+b");
    fun->setParameter("a",1.1);
    fun->setParameter("b",2.2);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(fun,domain,values);
    TS_ASSERT_DELTA(costFun->val(), 0.145, 1e-10); // == 0.5 *( 0.2^2 + 0.3^2 + 0.4^2 )

    std::vector<double> der;
    costFun->deriv(der);
    TS_ASSERT_EQUALS(der.size(),2);
    TS_ASSERT_DELTA(der[0], 1.1, 1e-10); // == 0 * 0.2 + 1 * 0.3 + 2 * 0.4
    TS_ASSERT_DELTA(der[1], 0.9, 1e-10); // == 1 * 0.2 + 1 * 0.3 + 1 * 0.4

    std::vector<double> der1;
    TS_ASSERT_DELTA(costFun->valAndDeriv(der1),0.145,1e-10);
    TS_ASSERT_EQUALS(der1.size(),2);
    TS_ASSERT_DELTA(der1[0], 1.1, 1e-10);
    TS_ASSERT_DELTA(der1[1], 0.9, 1e-10);

    GSLVector g(2);
    GSLMatrix H(2,2);
    TS_ASSERT_DELTA(costFun->valDerivHessian(g,H),0.145,1e-10);
    TS_ASSERT_DELTA(g.get(0), 1.1, 1e-10);
    TS_ASSERT_DELTA(g.get(1), 0.9, 1e-10);
  }

  void test_linear_correction_is_good_approximation()
  {
    const double a = 1.0;
    const double b = 2.0;
    std::vector<double> x(3),y(3);
    x[0] = 0.; y[0] = a * x[0] + b; // == 2.0
    x[1] = 1.; y[1] = a * x[1] + b; // == 3.0
    x[2] = 2.; y[2] = a * x[2] + b; // == 4.0
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<UserFunction> fun(new UserFunction);
    fun->setAttributeValue("Formula","a*x+b");
    fun->setParameter("a",1.1);
    fun->setParameter("b",2.2);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(fun,domain,values);
    TS_ASSERT_DELTA(costFun->val(), 0.145, 1e-10); // == 0.5 *( 0.2^2 + 0.3^2 + 0.4^2 )

    GSLVector g(2);
    GSLMatrix H(2,2);
    TS_ASSERT_DELTA(costFun->valDerivHessian(g,H),0.145,1e-10);

    GSLVector dx(2);
    dx.set(0,-0.1);
    dx.set(1,-0.2);

    double L; // = d*dx + 0.5 * dx * H * dx
    gsl_blas_dgemv( CblasNoTrans,0.5,H.gsl(),dx.gsl(),1.,g.gsl() );
    gsl_blas_ddot( g.gsl(), dx.gsl(), &L );
    TS_ASSERT_DELTA(L, -0.145, 1e-10); // L + costFun->val() == 0
  }

  void test_Fixing_parameter()
  {
    std::vector<double> x(10),y(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      x[i] = 0.1 * i;
      y[i] =  9.9 * exp( -(x[i])/0.5 );
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",1.);
    fun->setParameter("Lifetime",1.);
    fun->fix(1);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(fun,domain,values);

    BFGS_Minimizer s;
    s.initialize(costFun);

    TS_ASSERT_DELTA(costFun->val(),112.0,0.1);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(),7.84,0.1);

    TS_ASSERT_DELTA(fun->getParameter("Height"),7.6,0.01);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"),1.0,1e-9);
    TS_ASSERT_EQUALS(s.getError(),"success");

  }

};

#endif /*CURVEFITTING_LEASTSQUARESTEST_H_*/
