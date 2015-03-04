#ifndef CHEBYSHEVTEST_H_
#define CHEBYSHEVTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ChebfunBase.h"
#include <cmath>

#include "C:/Users/hqs74821/Work/Mantid_stuff/Testing/class/MyTestDef.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

double Sin(double x) { return sin(x); }
double MinusSin(double x) { return -sin(x); }
double Cos(double x) { return cos(x); }
double SinCos(double x) { return sin(x) + cos(x); }
double DSinCos(double x) { return -sin(x) + cos(x); }
double Linear(double x) { return 3.3 + 2.6 * x; }
double Quadratic(double x) { return 33 + 2.6 * x - 3 * x * x; }

class ChebfunBaseTest : public CxxTest::TestSuite {
public:

  void testConstructor()
  {
    ChebfunBase base(10,-1.0,1.0);
    TS_ASSERT_EQUALS(base.order(),10);
    TS_ASSERT_EQUALS(base.size(),11);
    TS_ASSERT_EQUALS(base.startX(),-1);
    TS_ASSERT_EQUALS(base.endX(),1);
    TS_ASSERT_EQUALS(base.xPoints().size(),11);
    TS_ASSERT_EQUALS(base.width(),2);
  }

  void testFit()
  {
    ChebfunBase base( 10, -M_PI, M_PI );
    auto p = base.fit(Sin);
    for(size_t i = 0; i < p.size(); ++i)
    {
      TS_ASSERT_EQUALS(p[i], sin(base.xPoints()[i]));
    }
  }

  void testEval_Sin()
  {
    do_test_eval( Sin, -M_PI, M_PI, 10 );
  }

  void testEval_Cos()
  {
    do_test_eval( Cos, -M_PI, M_PI, 10 );
  }

  void testEval_SinCos()
  {
    do_test_eval( SinCos, -M_PI, M_PI, 10 );
  }

  void testEvalVector_1()
  {
    double x[] = {-M_PI, -1.5, 0., 1.5, M_PI};
    do_test_eval_vector(SinCos, 10, -M_PI, M_PI, x, sizeof(x)/sizeof(double));
  }

  void testEvalVector_2()
  {
    double x[] = {-M_PI, -M_PI, -1.5, -1.5, 0., 0., 1.5, 1.5, M_PI, M_PI};
    do_test_eval_vector(SinCos, 10, -M_PI, M_PI, x, sizeof(x)/sizeof(double));
  }

  void testEvalVector_3() {
    double x[] = {-3., -2.45454545, -1.90909091, -1.36363636,
                  -0.81818182, -0.27272727, 0.27272727,  0.81818182,
                  1.36363636,  1.90909091,  2.45454545,  3.};
    do_test_eval_vector(SinCos, 10, -M_PI, M_PI, x, sizeof(x) / sizeof(double));
  }

  void testEvalVector_4()
  {
    double x[] = {-2*M_PI, -M_PI, -1.5, 0., 1.5, M_PI, 2*M_PI};
    do_test_eval_vector(SinCos, 10, -M_PI, M_PI, x, sizeof(x)/sizeof(double));
  }

  void test_bestFit_Sin()
  {
    do_test_bestFit( Sin, -M_PI, M_PI, 20 );
  }

  void test_bestFit_Cos()
  {
    do_test_bestFit( Cos, -M_PI, M_PI, 21 );
  }

  void test_bestFit_SinCos()
  {
    do_test_bestFit( SinCos, -M_PI, M_PI, 21 );
  }

  void test_bestFit_Linear()
  {
    do_test_bestFit( Linear, -2, 10, 2 );
  }

  void test_bestFit_Quadratic()
  {
    do_test_bestFit( Quadratic, -4, 4, 3 );
  }

  void test_integrate_Sin()
  {
    do_test_integrate( Sin, -M_PI, M_PI, 0.0 );
    do_test_integrate( Sin, 0.0, M_PI, 2.0 );
  }

  void test_integrate_Cos()
  {
    do_test_integrate( Cos, -M_PI, M_PI, 0.0 );
    do_test_integrate( Cos, 0.0, M_PI, 0.0 );
    do_test_integrate( Cos, 0.0, M_PI/2, 1.0 );
  }

  void test_derivative_Sin()
  {
    do_test_derivative(Sin,-M_PI, M_PI, Cos);
  }

  void test_derivative_Cos()
  {
    do_test_derivative(Cos,-M_PI, M_PI, MinusSin);
  }

  void test_derivative_SinCos()
  {
    do_test_derivative(SinCos,-M_PI, M_PI, DSinCos);
  }

  void test_roots_Linear()
  {
    do_test_roots(Linear,-4,4,1);
    do_test_roots(Linear,0,4,0);
  }

  void test_roots_Quadratic()
  {
    do_test_roots(Quadratic,-4,4,2);
  }

  void test_roots_Sin()
  {
    do_test_roots(Sin,-M_PI,M_PI,3, 1e-5);
  }

  void test_roots_Cos()
  {
    do_test_roots(Cos,-M_PI,M_PI,2, 1e-9);
  }

  void test_roots_SinCos()
  {
    do_test_roots(SinCos,-M_PI,M_PI,2, 1e-10);
  }

private:

  void do_test_eval(std::function<double(double)> fun, double start, double end, size_t n)
  {
    ChebfunBase base( n, start, end );
    auto p = base.fit(fun);
    auto x = base.linspace(2*n);
    for(size_t i = 0; i < x.size(); ++i)
    {
      double xi = x[i];
      TS_ASSERT_DELTA(base.eval(xi,p), fun(xi), 1e-4);
    }
  }

  void do_test_eval_vector(std::function<double(double)> fun, size_t n, double start, double end, const double* xarr, size_t narr)
  {
    std::vector<double> x;
    x.assign(xarr,xarr+narr);

    ChebfunBase base( n, start, end );
    auto p = base.fit(fun);
    auto y = base.evalVector(x,p);
    TS_ASSERT_EQUALS(y.size(), x.size());
    for(size_t i = 0; i < x.size(); ++i)
    {
      double xi = x[i];
      if ( xi < base.startX() || xi > base.endX() )
      {
        TS_ASSERT_EQUALS( y[i], 0.0 );
      }
      else
      {
        //std::cerr << xi << ' ' << y[i] << ' ' << sin(xi) + cos(xi) << std::endl;
        TS_ASSERT_DELTA(y[i], sin(xi) + cos(xi), 1e-4);
      }
    }
  }

  void do_test_bestFit(std::function<double(double)> fun, double start, double end, size_t expected_n)
  {
    std::vector<double> p,a;
    auto base = ChebfunBase::bestFit(start, end, fun, p, a);
    auto x = base->linspace(2*base->size());
    for(size_t i = 0; i < x.size(); ++i)
    {
      double xi = x[i];
      TS_ASSERT_DELTA(base->eval(xi,p), fun(xi), 1e-14);
    }
    TS_ASSERT_EQUALS( base->size(), expected_n );
  }

  void do_test_integrate(std::function<double(double)> fun, double start, double end, double expected_integral)
  {
    std::vector<double> p,a;
    auto base = ChebfunBase::bestFit(start, end, fun, p, a);
    TS_ASSERT_DELTA( base->integrate(p), expected_integral, 1e-14 );
  }

  void do_test_derivative(std::function<double(double)> fun, double start, double end,std::function<double(double)> deriv)
  {
    std::vector<double> p,a,dp,da;
    auto base = ChebfunBase::bestFit(start, end, fun, p, a);
    base->derivative(a,da);
    dp = base->calcP(da);
    auto x = base->linspace(2*base->size());
    for(size_t i = 0; i < x.size(); ++i)
    {
      double xi = x[i];
      //std::cerr << xi << ' ' << base->eval(xi,dp) - Cos(xi) << std::endl;
      TS_ASSERT_DELTA(base->eval(xi,dp), deriv(xi), 1e-13);
    }
  }

  void do_test_roots(std::function<double(double)> fun, double start, double end, size_t n_roots, double tol = 1e-13)
  {
    std::vector<double> p,a;
    auto base = ChebfunBase::bestFit(start, end, fun, p, a);
    auto roots = base->roots(a);
    TS_ASSERT_EQUALS( n_roots, roots.size() );
    for(size_t i = 0; i < roots.size(); ++i)
    {
      TS_ASSERT_DELTA(base->eval(roots[i],p), 0.0, tol);
    }
  }
};

#endif /*CHEBYSHEVTEST_H_*/
