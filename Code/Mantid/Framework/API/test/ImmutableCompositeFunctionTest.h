#ifndef IMMUTABLECOMPOSITEFUNCTIONTEST_H_
#define IMMUTABLECOMPOSITEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ImmutableCompositeFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"

using namespace Mantid;
using namespace Mantid::API;


class Linear: public ParamFunction, public IFunction1D
{
public:
  Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "Linear";}

  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(size_t i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    for(size_t i=0;i<nData;i++)
    {
      out->set(static_cast<int>(i),0,1.);
      out->set(static_cast<int>(i),1,xValues[i]);
    }
  }

};

class ImmutableCompositeFunctionTest_Function: public ImmutableCompositeFunction
{
public:
  ImmutableCompositeFunctionTest_Function(): ImmutableCompositeFunction()
  {
  }
};

class ImmutableCompositeFunctionTest : public CxxTest::TestSuite
{
public:
  void testAdd()
  {
    ImmutableCompositeFunctionTest_Function icf;
  }
};

#endif /*IMMUTABLECOMPOSITEFUNCTIONTEST_H_*/
