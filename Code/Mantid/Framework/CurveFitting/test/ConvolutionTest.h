#ifndef CONVOLUTIONTEST_H_
#define CONVOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/Convolution.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class ConvolutionExpression
{
public:
  double operator()(double x)
  {return 1+0.3*x+exp(-0.5*(x-4)*(x-4)*2)+2*exp(-0.5*(x-6)*(x-6)*3);}
};

class ConvolutionExp
{
public:
  double operator()(double x)
  {return exp(-0.5*(x-7)*(x-7)*2);}
};

class ConvolutionTest_Gauss: public IPeakFunction
{
public:
  ConvolutionTest_Gauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "ConvolutionTest_Gauss";}

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
    //throw Mantid::Kernel::Exception::NotImplementedError("");
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


class ConvolutionTest_Linear: public ParamFunction, public IFunctionMW
{
public:
  ConvolutionTest_Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "ConvolutionTest_Linear";}

  void function(double* out, const double* xValues, const int& nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(int i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    for(int i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
    }
  }

};

DECLARE_FUNCTION(ConvolutionTest_Gauss);
DECLARE_FUNCTION(ConvolutionTest_Linear);

class ConvolutionTest : public CxxTest::TestSuite
{
public:
  ConvolutionTest()
  {
    //FrameworkManager::Instance();
  }

  void testFunction()
  {
    Convolution conv;

    ConvolutionTest_Gauss* gauss1 = new ConvolutionTest_Gauss();
    gauss1->setParameter(0,1.1);
    gauss1->setParameter(1,1.2);
    gauss1->setParameter(2,1.3);
    ConvolutionTest_Gauss* gauss2 = new ConvolutionTest_Gauss();
    gauss2->setParameter(0,2.1);
    gauss2->setParameter(1,2.2);
    gauss2->setParameter(2,2.3);
    ConvolutionTest_Gauss* gauss3 = new ConvolutionTest_Gauss();
    gauss3->setParameter(0,3.1);
    gauss3->setParameter(1,3.2);
    gauss3->setParameter(2,3.3);
    ConvolutionTest_Linear* linear = new ConvolutionTest_Linear();
    linear->setParameter(0,0.1);
    linear->setParameter(1,0.2);

    int iFun = -1;
    iFun = conv.addFunction(linear);
    TS_ASSERT_EQUALS(iFun,0);
    iFun = conv.addFunction(gauss1);
    TS_ASSERT_EQUALS(iFun,1);
    iFun = conv.addFunction(gauss2);
    TS_ASSERT_EQUALS(iFun,1);
    iFun = conv.addFunction(gauss3);
    TS_ASSERT_EQUALS(iFun,1);

    TS_ASSERT_EQUALS(conv.nFunctions(),2);
    TS_ASSERT_EQUALS(conv.name(),"Convolution");

    CompositeFunction* cf = dynamic_cast<CompositeFunction*>(conv.getFunction(1));
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(conv.nParams(),11);
    TS_ASSERT_EQUALS(conv.parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(conv.getParameter(0),0.1);
    TS_ASSERT_EQUALS(conv.parameterName(2),"f1.f0.c");
    TS_ASSERT_EQUALS(conv.getParameter(2),1.1);
    TS_ASSERT_EQUALS(conv.parameterName(6),"f1.f1.h");
    TS_ASSERT_EQUALS(conv.getParameter(6),2.2);
    TS_ASSERT_EQUALS(conv.parameterName(10),"f1.f2.s");
    TS_ASSERT_EQUALS(conv.getParameter(10),3.3);

    TS_ASSERT_EQUALS(conv.nActive(),9);
    TS_ASSERT_EQUALS(conv.nameOfActive(0),"f1.f0.c");
    TS_ASSERT_EQUALS(conv.activeParameter(0),1.1);
    TS_ASSERT_EQUALS(conv.nameOfActive(4),"f1.f1.h");
    TS_ASSERT_EQUALS(conv.activeParameter(4),2.2);
    TS_ASSERT_EQUALS(conv.nameOfActive(8),"f1.f2.s");
    TS_ASSERT_EQUALS(conv.activeParameter(8),3.3);

    TS_ASSERT_EQUALS(conv.parameterLocalName(0),"a");
    TS_ASSERT_EQUALS(conv.parameterLocalName(2),"f0.c");
    TS_ASSERT_EQUALS(conv.parameterLocalName(6),"f1.h");
    TS_ASSERT_EQUALS(conv.parameterLocalName(10),"f2.s");

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(conv);
    TS_ASSERT(fun);

    Convolution* conv1 = dynamic_cast<Convolution*>(fun);
    TS_ASSERT(conv1);

    TS_ASSERT_EQUALS(conv1->nFunctions(),2);
    TS_ASSERT_EQUALS(conv1->name(),"Convolution");

    CompositeFunction* cf1 = dynamic_cast<CompositeFunction*>(conv1->getFunction(1));
    TS_ASSERT(cf1);
    TS_ASSERT_EQUALS(conv1->nParams(),11);
    TS_ASSERT_EQUALS(conv1->parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(conv1->getParameter(0),0.1);
    TS_ASSERT_EQUALS(conv1->parameterName(2),"f1.f0.c");
    TS_ASSERT_EQUALS(conv1->getParameter(2),1.1);
    TS_ASSERT_EQUALS(conv1->parameterName(6),"f1.f1.h");
    TS_ASSERT_EQUALS(conv1->getParameter(6),2.2);
    TS_ASSERT_EQUALS(conv1->parameterName(10),"f1.f2.s");
    TS_ASSERT_EQUALS(conv1->getParameter(10),3.3);

    TS_ASSERT_EQUALS(conv1->nActive(),9);
    TS_ASSERT_EQUALS(conv1->nameOfActive(0),"f1.f0.c");
    TS_ASSERT_EQUALS(conv1->activeParameter(0),1.1);
    TS_ASSERT_EQUALS(conv1->nameOfActive(4),"f1.f1.h");
    TS_ASSERT_EQUALS(conv1->activeParameter(4),2.2);
    TS_ASSERT_EQUALS(conv1->nameOfActive(8),"f1.f2.s");
    TS_ASSERT_EQUALS(conv1->activeParameter(8),3.3);

    TS_ASSERT_EQUALS(conv1->parameterLocalName(0),"a");
    TS_ASSERT_EQUALS(conv1->parameterLocalName(2),"f0.c");
    TS_ASSERT_EQUALS(conv1->parameterLocalName(6),"f1.h");
    TS_ASSERT_EQUALS(conv1->parameterLocalName(10),"f2.s");

    delete fun;
  }

  void testResolution()
  {
    Convolution conv;

    double a = 1.3;
    double h = 3.;
    ConvolutionTest_Gauss* res = new ConvolutionTest_Gauss();
    res->setParameter("c",0);
    res->setParameter("h",h);
    res->setParameter("s",a);

    conv.addFunction(res);

    const int N = 116;
    double x[N],xr[N],out[N],x0 = 0, dx = 0.3;
    double Dx = dx*N;
    for(int i=0;i<N;i++)
    {
      x[i] = x0 + i*dx;
      xr[i] = x[i] - x0 - Dx/2;
    }

    res->function(out,xr,N);

    // When called with only 1 function attached returns its fourier transform
    conv.function(out,x,N);

    // Check that the transform is correct: F( exp(-a*x^2) ) == sqrt(pi/a)*exp(-(pi*x)^2/a)
    Convolution::HalfComplex hout(out,N);
    double df = 1./Dx; // this is the x-step of the transformed data
    double pi= acos(0.)*2;
    double cc = pi*pi*df*df/a;
    for(int i=0;i<hout.size();i++)
    {
      TS_ASSERT_DELTA(hout.real(i),h*sqrt(pi/a)*exp(-cc*i*i),1e-7);
    }

    //std::ofstream fres("fres.txt");
    //for(int i=0;i<hout.size();i++)
    //{
    //  double f = df*i;
    //  fres<<f<<' '<<hout.real(i)<<' '<<0
    //    <<' '<<h*sqrt(pi/a)*exp(-pi*pi*f*f/a)<<" 0"<<'\n';
    //}

  }

  void testConvolution()
  {
    Convolution conv;

    double pi= acos(0.)*2;
    double c1 = 0.;
    double h1 = 3;
    double s1 = pi/2;
    ConvolutionTest_Gauss* res = new ConvolutionTest_Gauss();
    res->setParameter("c",c1);
    res->setParameter("h",h1);
    res->setParameter("s",s1);

    conv.addFunction(res);

    const int N = 116;
    double x[N],xr[N],out[N],x0 = 0, dx = 0.13;
    double Dx = dx*N;
    for(int i=0;i<N;i++)
    {
      x[i] = x0 + i*dx;
      xr[i] = x[i] - x0 - Dx/2;
    }

    double c2 = x0 + Dx/2;
    double h2 = 10.;
    double s2 = pi/3;
    ConvolutionTest_Gauss* fun = new ConvolutionTest_Gauss();
    fun->setParameter("c",c2);
    fun->setParameter("h",h2);
    fun->setParameter("s",s2);

    conv.addFunction(fun);

    conv.function(out,x,N);

    // a convolution of two gaussians is a gaussian with h == hp and s == sp
    double sp = s1*s2/(s1+s2);
    double hp = h1*h2*sqrt(pi/(s1+s2));

    double df = 1/Dx;
    //std::cerr<<hp<<' '<<sp<<'\n';
    //std::cerr<<"test.df="<<df<<'\n';
    //std::ofstream fconv("conv.txt");
    for(int i=0;i<N;i++)
    {
      double xi = x[i] - c2;
      TS_ASSERT_DELTA(out[i],hp*exp(-sp*xi*xi),1e-10);
      //fconv<<x[i]<<' '<<out[i]<<' '<< 0/*pi/a/sqrt(2.)*exp(-0.)*/<<'\n';
      
      //double  f = i*df;
      //fconv<<f<<' '<<h1*h2*pi/sqrt(s1*s2)*exp(-pi*pi*f*f*(1./s1+1./s2))<<" 0"<<'\n';
    }

  }

  void testFit()
  {
    Fit fit;
    WS_type ws = mkWS(ConvolutionExp(),1,10,24,0.13);
  }

private:

  template<class Funct>
  WS_type mkWS(Funct f,int nSpec,double x0,double x1,double dx,bool isHist=false)
  {
    int nX = int((x1 - x0)/dx) + 1;
    int nY = nX - (isHist?1:0);
    if (nY <= 0)
      throw std::invalid_argument("Cannot create an empty workspace");

    Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
      (WorkspaceFactory::Instance().create("Workspace2D",nSpec,nX,nY));

    double spec;
    double x;

    for(int iSpec=0;iSpec<nSpec;iSpec++)
    {
      spec = iSpec;
      Mantid::MantidVec& X = ws->dataX(iSpec);
      Mantid::MantidVec& Y = ws->dataY(iSpec);
      Mantid::MantidVec& E = ws->dataE(iSpec);
      for(int i=0;i<nY;i++)
      {
        x = x0 + dx*i;
        X[i] = x;
        Y[i] = f(x);
        E[i] = 1;
      }
      if (isHist)
        X.back() = X[nY-1] + dx;
    }
    return ws;
  }

  void storeWS(const std::string& name,WS_type ws)
  {
    AnalysisDataService::Instance().add(name,ws);
  }

  void removeWS(const std::string& name)
  {
    AnalysisDataService::Instance().remove(name);
  }

  WS_type getWS(const std::string& name)
  {
    return boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(name));
  }

  TWS_type getTWS(const std::string& name)
  {
    return boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(name));
  }

  void addNoise(WS_type ws,double noise)
  {
    for(int iSpec=0;iSpec<ws->getNumberHistograms();iSpec++)
    {
      Mantid::MantidVec& Y = ws->dataY(iSpec);
      Mantid::MantidVec& E = ws->dataE(iSpec);
      for(int i=0;i<Y.size();i++)
      {
        Y[i] += noise*(-.5 + double(rand())/RAND_MAX);
        E[i] += noise;
      }
    }
  }

  void press_return()
  {
    std::cerr<<"Press Return";
    std::string str;
    getline(std::cin,str);
  }

};

#endif /*CONVOLUTIONTEST_H_*/
