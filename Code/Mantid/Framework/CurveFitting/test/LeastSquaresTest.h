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
    TS_ASSERT_DELTA(costFun->val(),0.0,1e-10);
    TS_ASSERT_DELTA(fun->getParameter("Height"),3.3,1e-10);
    TS_ASSERT_DELTA(fun->getParameter("Lifetime"),4.4,1e-10);
    TS_ASSERT_EQUALS(s.getError(),"success");
  }

};

#endif /*CURVEFITTING_LEASTSQUARESTEST_H_*/
