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

class IFunction1DTest_Function: public virtual IFitFunction, public virtual ParamFunction
{
protected:
  virtual void function1D(FunctionDomain1D&)const
  {
  }
};

class IFunction1DTest : public CxxTest::TestSuite
{
public:

  void testIFunction()
  {
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
