#ifndef DELTAFUNCTIONTEST_H
#define DELTAFUNCTIONTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/Fit.h"
#include "MantidDataHandling/LoadRaw.h"

#include "MantidCurveFitting/DeltaFunction.h"
#include "MantidCurveFitting/Convolution.h"



using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

//same class as ConvolutionTest_Gauss in ConvolutionTest.h
class DeltaFunctionTest_Gauss: public IPeakFunction
{
public:
	DeltaFunctionTest_Gauss()
  {
    declareParameter("c");     // center of the peak
    declareParameter("h",1.);  // height of the peak
    declareParameter("s",1.);  // 1/(2*sigma^2)
  }

  std::string name()const{return "DeltaFunctionTest_Gauss";}

  void functionLocal(double* out, const double* xValues, const size_t nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-x*x*w);
    }
  }
  void functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
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

  double fwhm()const
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

  void setFwhm(const double w)
  {
    setParameter(2,w);
  }

}; // end of DeltaFunctionTest_Gauss

// A derived class from DeltaFunction
class DeltaFunctionTest_Delta: public DeltaFunction
{
public:
  DeltaFunctionTest_Delta()
  {
    declareParameter("p1");
    declareParameter("p2");
  }

  double HeightPrefactor() const{
    const double& p1 = getParameter("p1");
    const double& p2 = getParameter("p2");
    return p1*p2; // simple operation
  }

  std::string name()const{return "DeltaFunctionTest_Delta";}

}; // end of DeltaFunctionTest_Delta

class DeltaFunctionTest : public CxxTest::TestSuite
{
public:
  void testDeltaFunction()
  {
    Convolution conv;
    //set the resolution function
    double h = 3.0;  // height
    double a = 1.3;  // 1/(2*sigma^2)
    DeltaFunctionTest_Gauss* res = new DeltaFunctionTest_Gauss();
    res->setParameter("c",0);
    res->setParameter("h",h);
    res->setParameter("s",a);
    conv.addFunction(res);

    //set the "structure factor"
    double H=1.5, p1=2.6, p2=0.7;
    DeltaFunctionTest_Delta* eds = new DeltaFunctionTest_Delta();
    eds->setParameter("Height",H);
    eds->setParameter("p1",p1);
    eds->setParameter("p2",p2);
    conv.addFunction(eds);

    //set up some frequency values centered around zero
    const int N = 117;
    double w[N],out[N],w0,dw = 0.13;
    w0=-dw*int(N/2);
    for(int i=0;i<N;i++) w[i] = w0 + i*dw;

    //convolve. The result must be the resolution multiplied by factor H*p1*p2);
    conv.functionMW(out,w,N);
    for(int i=0;i<N;i++){
      TS_ASSERT_DELTA(out[i],H*p1*p2*h*exp(-w[i]*w[i]*a),1e-10);
    }
  }

}; // end of DeltaFunctionTest


#endif /* DELTAFUNCTIONTEST_H */
