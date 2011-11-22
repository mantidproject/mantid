#ifndef TEMPFUNCTIONTEST_H_
#define TEMPFUNCTIONTEST_H_

#include "MantidAPI/TempFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMW.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

class TFT_Funct: public ParamFunction, public IFunctionMW
{
public:
  TFT_Funct()
  {
    declareParameter("c0", 0.0, "this is the famous c0 blah...");
    declareParameter("c1");
    declareParameter("c2");
    declareParameter("c3");
  }

  std::string name()const{return "TFT_Funct";}

  void functionMW(double* out, const double* xValues, const size_t nData)const
  {
    double c0 = getParameter("c0");
    double c1 = getParameter("c1");
    double c2 = getParameter("c2");
    double c3 = getParameter("c3");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i];
      out[i] = c0 + x*(c1 + x*(c2 + x*c3));
    }
  }
  void functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
  {
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i];
      out->set(i,0,1.);
      out->set(i,1,x);
      out->set(i,2,x*x);
      out->set(i,3,x*x*x);
    }
  }

};

class TempFunctionTest : public CxxTest::TestSuite
{
public:

  void testFunction()
  {
    TempFunction fun(new TFT_Funct);
    TS_ASSERT_EQUALS(fun.name(),"TFT_Funct");
    TS_ASSERT_EQUALS(fun.nParams(),4);
  }

};

#endif /*TEMPFUNCTIONTEST_H_*/
