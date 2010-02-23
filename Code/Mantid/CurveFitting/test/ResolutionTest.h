#ifndef RESOLUTIONTEST_H_
#define RESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Resolution.h"

#include <fstream>

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class ResolutionTest : public CxxTest::TestSuite
{
public:

  ResolutionTest()
    :resH(3),resS(acos(0.)),
    N(117),DX(10),X0(-DX/2),dX(DX/(N-1)),yErr(0),resFileName("ResolutionTestResolution.res")
  {
    std::ofstream fil(resFileName.c_str());

    double y0 = 0;
    for(int i=0;i<N;i++)
    {
      double x = X0 + i*dX;
      double y = resH*exp(-x*x*resS);
      double err = fabs(y-y0)/10;
      if (err > yErr) yErr = err;
      fil<<x<<' '<<y<<" 0\n";
      y0 = y;
    }

  }

  void testIt()
  {
    Resolution res;
    res.setAttribute("FileName",resFileName);
    const int n = 50;
    double x[n];
    double y[n];
    double xStart = -2;
    double xEnd = 3.;
    double dx = (xEnd - xStart)/(n-1);
    for(int i=0;i<n;i++)
    {
      x[i] = xStart + dx*i;
    }
    res.function(y,x,n);
    for(int i=0;i<n;i++)
    {
      double xi = x[i];
      TS_ASSERT_DELTA(y[i],resH*exp(-xi*xi*resS),yErr);
    }
  }
private:
  const double resH,resS;
  const int N;
  const double DX,X0,dX;
  double yErr;
  const std::string resFileName;
};

#endif /*RESOLUTIONTEST_H_*/
