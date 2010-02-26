#ifndef PARAMETERTIETEST_H_
#define PARAMETERTIETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterTie.h"

using namespace Mantid;
using namespace Mantid::API;

class ParameterTieTest_Gauss: public IPeakFunction
{
public:
  ParameterTieTest_Gauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }
  std::string name()const{return "ParameterTieTest_Gauss";}
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
    return getParameter(0);
  }

  double height()const
  {
    return getParameter(1);
  }

  double width()const
  {
    return getParameter(2);
  }

  void setCentre(const double c)
  {
    setParameter(0,c);
  }
  void setHeight(const double h)
  {
    setParameter(1,h);
  }

  void setWidth(const double w)
  {
    setParameter(2,w);
  }
};


class ParameterTieTest_Linear: public Function
{
public:
  ParameterTieTest_Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }
  std::string name()const{return "ParameterTieTest_Linear";}
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
    ParameterTieTest_Gauss *g1 = new ParameterTieTest_Gauss(),*g2 = new ParameterTieTest_Gauss();
    ParameterTieTest_Linear *bk = new ParameterTieTest_Linear();

    mfun.addFunction(bk);
    mfun.addFunction(g1);
    mfun.addFunction(g2);

    g1->setParameter("c",3.1);
    g1->setParameter("h",1.1);
    g1->setParameter("s",1.);

    g2->setParameter("c",7.1);
    g2->setParameter("h",1.1);
    g2->setParameter("s",2.);

    bk->setParameter("a",0.8);

    ParameterTie tie(&mfun,"f1.s");
    tie.set("f2.s^2+f0.a+1");

    TS_ASSERT_DELTA(tie.eval(),5.8,0.00001);
    TS_ASSERT_EQUALS(tie.getFunction(),g1);
    TS_ASSERT_EQUALS(tie.getIndex(),2);

    TS_ASSERT_THROWS(mustThrow1(&mfun),std::invalid_argument);
    TS_ASSERT_THROWS(mustThrow2(&mfun),std::invalid_argument);
    TS_ASSERT_THROWS(mustThrow3(&mfun),std::out_of_range);

    TS_ASSERT_THROWS(tie.set("a+b"),std::invalid_argument);

  }

  void testSimple()
  {
    ParameterTieTest_Linear bk;

    bk.setParameter("a",0.8);
    bk.setParameter("b",0.);

    ParameterTie tie(&bk,"b");
    tie.set("2*a-1");

    TS_ASSERT_EQUALS(tie.getIndex(),1);
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
