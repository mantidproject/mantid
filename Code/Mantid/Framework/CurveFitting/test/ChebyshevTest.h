#ifndef CHEBYSHEVTEST_H_
#define CHEBYSHEVTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Chebyshev.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class ChebyshevTest : public CxxTest::TestSuite
{
public:
  void testValues()
  {
    const int N = 11;
    double y[N],x[N];
    for(int i=0;i<N;++i)
    {
      x[i] = i*0.1;
    }
    Chebyshev cheb;
    cheb.setAttribute("n",IFitFunction::Attribute(10));
    for(int n=0;n<=10;++n)
    {
      cheb.setParameter(n,1.);
      if (n > 0)
      {
        cheb.setParameter(n-1,0.);
      }
      cheb.function(&y[0],&x[0],N);
      for(int i=0;i<N;++i)
      {
        TS_ASSERT_DELTA(y[i],cos(n*acos(x[i])),1e-12);
      }
    }
  }

  void testFit()
  {
    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,11,11);

    Mantid::MantidVec& X = ws->dataX(0);
    Mantid::MantidVec& Y = ws->dataY(0);
    Mantid::MantidVec& E = ws->dataE(0);
    for(size_t i=0;i<Y.size();i++)
    {
      double x = -1. + 0.1*i;
      X[i] = x;
      Y[i] = x*x*x;
      E[i] = 1;
    }
    //X.back() = 1.;

    AnalysisDataService::Instance().add("ChebyshevTest_ws",ws);

    Fit fit;
    fit.initialize();

    fit.setPropertyValue("InputWorkspace","ChebyshevTest_ws");
    fit.setPropertyValue("WorkspaceIndex","0");

    Chebyshev* cheb = new Chebyshev();
    cheb->setAttribute("n",IFitFunction::Attribute(3));
    fit.setPropertyValue("Function",*cheb);

    fit.execute();
    IFitFunction::Attribute StartX = cheb->getAttribute("StartX");
    TS_ASSERT_EQUALS(StartX.asDouble(),-1);
    IFitFunction::Attribute EndX = cheb->getAttribute("EndX");
    TS_ASSERT_EQUALS(EndX.asDouble(),1);
    TS_ASSERT(fit.isExecuted());

    IFitFunction *out = FunctionFactory::Instance().createInitialized(fit.getPropertyValue("Function"));
    TS_ASSERT_DELTA(out->getParameter("A0"),0,1e-12);
    TS_ASSERT_DELTA(out->getParameter("A1"),0.75,1e-12);
    TS_ASSERT_DELTA(out->getParameter("A2"),0,1e-12);
    TS_ASSERT_DELTA(out->getParameter("A3"),0.25,1e-12);

  }
};

#endif /*CHEBYSHEVTEST_H_*/
