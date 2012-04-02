#ifndef IFUNCTION1DTEST_H_
#define IFUNCTION1DTEST_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include <cxxtest/TestSuite.h>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;

const double A = 1.1;
const double B = 2.2;

class IFunction1DTest_Function: public virtual IFunction1D, public virtual ParamFunction
{
protected:
  virtual std::string name() const {return "IFunction1DTest_Function";}
  virtual void function1D(double* out, const double* xValues, const size_t nData)const
  {
    for(size_t i = 0; i < nData; ++i)
    {
      double x = xValues[i];
      out[i] = A * x + B;
    }
  }
  virtual void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    for(size_t i = 0; i < nData; ++i)
    {
      double x = xValues[i];
      out->set(i,0,x);
      out->set(i,1,1.0);
    }
  }
};

class IFunction1DTest_Jacobian: public Jacobian
{
public:
  IFunction1DTest_Jacobian(size_t ny, size_t np):
  m_np(np)
  {
    m_data.resize(ny * np);
  }
  virtual void set(size_t iY, size_t iP, double value)
  {
    m_data[iY*m_np + iP] = value;
  }
  virtual double get(size_t iY, size_t iP)
  {
    return m_data[iY*m_np + iP];
  }
private:
  size_t m_np;
  std::vector<double> m_data;
};

class IFunction1DTest : public CxxTest::TestSuite
{
public:

  void testIFunction()
  {
    IFunction1DTest_Function function;
    std::vector<double> x(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      x[i] = 1.0 + 0.1 * i;
    }
    FunctionDomain1DVector domain(x);
    FunctionValues values(domain);
    function.function(domain,values);
    for(size_t i = 0; i < domain.size(); ++i)
    {
      TS_ASSERT_EQUALS(values.getCalculated(i),A * (1.0 + 0.1 * i) + B);
    }
    
    IFunction1DTest_Jacobian jacobian(10,2);
    function.functionDeriv(domain,jacobian);
    for(size_t i = 0; i < domain.size(); ++i)
    {
      TS_ASSERT_EQUALS(jacobian.get(i,0), 1.0 + 0.1 * i);
      TS_ASSERT_EQUALS(jacobian.get(i,1), 1.0);
    }
  }

};

#endif /*IFUNCTION1DTEST_H_*/
