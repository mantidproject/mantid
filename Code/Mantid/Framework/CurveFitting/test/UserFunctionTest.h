#ifndef USERFUNCTIONTEST_H_
#define USERFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/UserFunction.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class UserFunctionTest : public CxxTest::TestSuite
{
public:

  class UserTestJacobian: public Jacobian
  {
    int m_nData,m_nParams;
    std::vector<double> m_buffer;
  public:
    UserTestJacobian(int nData,int nParams)
      :m_nData(nData),m_nParams(nParams)
    {
      m_buffer.resize(nData*nParams);
    }
    void set(size_t iY, size_t iP, double value)
    {
      m_buffer[iY*m_nParams + iP] = value;
    }
    double get(size_t iY, size_t iP)
    {
      return m_buffer[iY*m_nParams + iP];
    }
  };

  void testIt()
  {
    UserFunction fun;
    fun.setAttribute("Formula",IFitFunction::Attribute("h*sin(a*x-c)"));
    fun.setParameter("h",2.2);
    fun.setParameter("a",2.0);
    fun.setParameter("c",1.2);

    TS_ASSERT_EQUALS(fun.getParameter("h") , 2.2);
    TS_ASSERT_EQUALS(fun.getParameter("a") , 2.0);
    TS_ASSERT_EQUALS(fun.getParameter("c") , 1.2);
    TS_ASSERT_EQUALS(fun.asString(),"name=UserFunction,Formula=h*sin(a*x-c),h=2.2,a=2,c=1.2");
    TS_ASSERT_EQUALS(fun.getAttribute("Formula").asString(),"h*sin(a*x-c)");

    const size_t nParams = 3;
    const size_t nData = 10;
    std::vector<double> x(nData),y(nData);
    for(size_t i=0;i<nData;i++)
    {
      x[i] = 0.1*i;
    }
    fun.function(&y[0],&x[0],nData);
    for(size_t i=0;i<nData;i++)
    {
      TS_ASSERT_DELTA(y[i],2.2*sin(2*x[i]-1.2),0.000001);
    }

    UserTestJacobian J(nData,nParams);
    fun.functionDeriv(&J,&x[0],nData);
    for(size_t i=0;i<nData;i++)
    for(size_t j=0;j<nParams;j++)
    {
      double d = J.get(i,j);
      double dtrue;
      if (j == 0)
      {
        dtrue = sin(2*x[i]-1.2);
      }
      else if (j == 1)
      {
        dtrue = 2.2*cos(2*x[i]-1.2)*x[i];
      }
      else
      {
        dtrue = - 2.2*cos(2*x[i]-1.2);
      }
      TS_ASSERT_DELTA(d,dtrue,0.03);
    }

  }
};

#endif /*USERFUNCTIONTEST_H_*/
