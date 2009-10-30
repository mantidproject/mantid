#ifndef PARAMETERTIETEST_H_
#define PARAMETERTIETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterTie.h"

using namespace Mantid;
using namespace Mantid::API;

class Gauss: public IPeakFunction
{
public:
  void init()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }
  void function(double* out, const double* xValues, const int& nData)
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-0.5*x*x*w);
    }
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      double e = h*exp(-0.5*x*x*w);
      out->set(i,0,x*h*e*w);
      out->set(i,1,e);
      out->set(i,2,-0.5*x*x*h*e);
    }
  }

  double centre()const
  {
    return parameter(0);
  }

  double height()const
  {
    return parameter(1);
  }

  double width()const
  {
    return parameter(2);
  }

  void setCentre(const double c)
  {
    parameter(0) = c;
  }
  void setHeight(const double h)
  {
    parameter(1) = h;
  }

  void setWidth(const double w)
  {
    parameter(2) = w;
  }
};


class Linear: public IFunction
{
public:
  void init()
  {
    declareParameter("a");
    declareParameter("b");
  }
  void function(double* out, const double* xValues, const int& nData)
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(int i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    for(int i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
    }
  }
};

class ParameterTieTest : public CxxTest::TestSuite
{
public:

  void testComposite()
  {
    CompositeFunction mfun;
    Gauss *g1 = new Gauss(),*g2 = new Gauss();
    Linear *bk = new Linear();

    g1->init();
    g2->init();
    bk->init();

    mfun.addFunction(bk);
    mfun.addFunction(g1);
    mfun.addFunction(g2);

    g1->getParameter("c") = 3.1;
    g1->getParameter("h") = 1.1;
    g1->getParameter("s") = 1.;

    g2->getParameter("c") = 7.1;
    g2->getParameter("h") = 1.1;
    g2->getParameter("s") = 2.;

    bk->getParameter("a") = 0.8;

    ParameterTie tie(&mfun,"f1.s");
    tie.set("f2.s^2+f0.a+1");

    TS_ASSERT_DELTA(tie.eval(),5.8,0.00001);

    TS_ASSERT_THROWS(mustThrow1(&mfun),std::invalid_argument);
    TS_ASSERT_THROWS(mustThrow2(&mfun),std::invalid_argument);
    TS_ASSERT_THROWS(mustThrow3(&mfun),std::out_of_range);

    TS_ASSERT_THROWS(tie.set("a+b"),std::invalid_argument);

  }

  void testSimple()
  {
    Linear bk;
    bk.init();

    bk.getParameter("a") = 0.8;
    bk.getParameter("b") = 0.;

    ParameterTie tie(&bk,"b");
    tie.set("2*a-1");

    TS_ASSERT_DELTA(tie.eval(),0.6,0.00001);
    TS_ASSERT_THROWS( mustThrow4(&bk),std::invalid_argument);
    TS_ASSERT_THROWS( mustThrow5(&bk),std::invalid_argument);
    TS_ASSERT_THROWS( tie.set("q+p"),std::invalid_argument);

    TS_ASSERT_THROWS(tie.set(""),std::runtime_error);

  }
private:
  void mustThrow1(CompositeFunction* fun)
  {
    ParameterTie tie(fun,"s");
  }
  void mustThrow2(CompositeFunction* fun)
  {
    ParameterTie tie(fun,"g1.s");
  }
  void mustThrow3(CompositeFunction* fun)
  {
    ParameterTie tie(fun,"f10.s");
  }
  void mustThrow4(IFunction* fun)
  {
    ParameterTie tie(fun,"f1.a");
  }
  void mustThrow5(IFunction* fun)
  {
    ParameterTie tie(fun,"c");
  }
};

#endif /*PARAMETERTIETEST_H_*/
