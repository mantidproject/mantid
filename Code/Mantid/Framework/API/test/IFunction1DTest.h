#ifndef IFUNCTION1DTEST_H_
#define IFUNCTION1DTEST_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

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

class IFunction1DTest : public CxxTest::TestSuite
{
public:

  void testIFunction()
  {
    IFunction1DTest_Function function;
    // What does this test do??
    int i = 1;
    TS_ASSERT(i);

    // const int nx = 10;
    // const int ny = 10;
    // Mantid::DataObjects::Workspace2D_sptr ws = Create2DWorkspace(nx,ny);

    // for(int i=0;i<nx;++i)
    // {
    //   for(int j=0;j<ny;++j)
    //   {
    //   }
    // }

    // std::cerr<<"\nn="<<ws->axes()<<"\n";
  }

};

#endif /*IFUNCTION1DTEST_H_*/
