#ifndef RESOLUTIONTEST_H_
#define RESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Resolution.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/Convolution.h"
#include "MantidAPI/IPeakFunction.h"
#include <Poco/File.h>


#include <fstream>

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class ResolutionTest_Gauss: public IPeakFunction
{
public:
  ResolutionTest_Gauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "ResolutionTest_Gauss";}

  void functionLocal(double* out, const double* xValues, const int& nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-x*x*w);
    }
  }
  void functionDerivLocal(Jacobian* out, const double* xValues, const int& nData)
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      double e = h*exp(-x*x*w);
      out->set(i,0,x*h*e*w);
      out->set(i,1,e);
      out->set(i,2,-x*x*h*e);
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

DECLARE_FUNCTION(ResolutionTest_Gauss);

class ResolutionTest : public CxxTest::TestSuite
{
public:

  ResolutionTest()
    :resH(3),resS(acos(0.)),
    N(117),DX(10),X0(-DX/2),dX(DX/(N-1)),yErr(0),resFileName("ResolutionTestResolution.res")
  {
  }

  void setUp()
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

void tearDown()
  {
    Poco::File phandle(resFileName);
    if( phandle.exists() ) 
      {
	phandle.remove();
      }
  }

  void testIt()
  {
    Resolution res;
    res.setAttribute("FileName",IFitFunction::Attribute(resFileName));
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

  void testFit()
  {
    const int nX = 100;
    const int nY = nX - 1;

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D",1,nX,nY));

    double spec;
    double x;
    const double x0 = 0.;
    const double dx = DX/nY;

    const double pi = acos(0.)*2.;
    const double s1 = pi/3;
    const double h1 = 2.;
    const double c1 = 5.5;
    const double sp = s1*resS/(s1+resS);
    const double hp = h1*resH*sqrt(pi/(s1+resS));

    Mantid::MantidVec& X = ws->dataX(0);
    Mantid::MantidVec& Y = ws->dataY(0);
    Mantid::MantidVec& E = ws->dataE(0);

    for(int i=0;i<nY;i++)
    {
      X[i] = x0 + dx*i;
      x = X[i] - c1;
      Y[i] = hp*exp(-x*x*sp);
      E[i] = 1;
    }
    X.back() = X[nY-1] + dx;
    AnalysisDataService::Instance().add("ResolutionTest_WS",ws);

    Resolution* res = new Resolution;
    res->setAttribute("FileName",IFitFunction::Attribute(resFileName));

    ResolutionTest_Gauss* gauss = new ResolutionTest_Gauss;
    gauss->setParameter("c",5);
    gauss->setParameter("h",2);
    gauss->setParameter("s",1);
    
    Convolution* conv = new Convolution;
    conv->addFunction(res);
    conv->addFunction(gauss);

    Fit fit;
    fit.initialize();
    fit.setPropertyValue("InputWorkspace","ResolutionTest_WS");
    fit.setPropertyValue("WorkspaceIndex","0");
    //fit.setFunction(conv);
    fit.setPropertyValue("Function",*conv);
    fit.execute();

  }

private:
  const double resH,resS;
  const int N;
  const double DX,X0,dX;
  double yErr;
  const std::string resFileName;
};

#endif /*RESOLUTIONTEST_H_*/
