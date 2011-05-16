#ifndef CONVOLUTIONTEST_H_
#define CONVOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/ProductFunctionMW.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFitFunction.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class ProductFunctionMWTest_Gauss: public IPeakFunction
{
public:
  ProductFunctionMWTest_Gauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "ProductFunctionMWTest_Gauss";}

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


class ProductFunctionMWTest_Linear: public ParamFunction, public IFunctionMW
{
public:
  ProductFunctionMWTest_Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "ProductFunctionMWTest_Linear";}

  void function(double* out, const double* xValues, const size_t nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(size_t i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv(Jacobian* out, const double* xValues, const size_t nData)
  {
    for(size_t i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
    }
  }

};

DECLARE_FUNCTION(ProductFunctionMWTest_Gauss);
DECLARE_FUNCTION(ProductFunctionMWTest_Linear);

class ProductFunctionMWTest : public CxxTest::TestSuite
{
public:

  void testFunction()
  {
    ProductFunctionMW prodF;

    ProductFunctionMWTest_Gauss* gauss1 = new ProductFunctionMWTest_Gauss();
    gauss1->setParameter(0,1.1);
    gauss1->setParameter(1,1.2);
    gauss1->setParameter(2,1.3);
    ProductFunctionMWTest_Gauss* gauss2 = new ProductFunctionMWTest_Gauss();
    gauss2->setParameter(0,2.1);
    gauss2->setParameter(1,2.2);
    gauss2->setParameter(2,2.3);
    ProductFunctionMWTest_Gauss* gauss3 = new ProductFunctionMWTest_Gauss();
    gauss3->setParameter(0,3.1);
    gauss3->setParameter(1,3.2);
    gauss3->setParameter(2,3.3);
    ProductFunctionMWTest_Linear* linear = new ProductFunctionMWTest_Linear();
    linear->setParameter(0,0.1);
    linear->setParameter(1,0.2);

    int iFun = -1;
    iFun = prodF.addFunction(linear);
    TS_ASSERT_EQUALS(iFun,0);
    iFun = prodF.addFunction(gauss1);
    TS_ASSERT_EQUALS(iFun,1);
    iFun = prodF.addFunction(gauss2);
    TS_ASSERT_EQUALS(iFun,2);
    iFun = prodF.addFunction(gauss3);
    TS_ASSERT_EQUALS(iFun,3);

    TS_ASSERT_EQUALS(prodF.nFunctions(),4);
    TS_ASSERT_EQUALS(prodF.name(),"ProductFunctionMW");

    CompositeFunction* cf = &prodF;
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(prodF.nParams(),11);
    TS_ASSERT_EQUALS(prodF.parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(prodF.getParameter(0),0.1);
    TS_ASSERT_EQUALS(prodF.parameterName(2),"f1.c");
    TS_ASSERT_EQUALS(prodF.getParameter(2),1.1);
    TS_ASSERT_EQUALS(prodF.parameterName(6),"f2.h");
    TS_ASSERT_EQUALS(prodF.getParameter(6),2.2);
    TS_ASSERT_EQUALS(prodF.parameterName(10),"f3.s");
    TS_ASSERT_EQUALS(prodF.getParameter(10),3.3);

    TS_ASSERT_EQUALS(prodF.nActive(),11);
    TS_ASSERT_EQUALS(prodF.nameOfActive(0),"f0.a");
    TS_ASSERT_EQUALS(prodF.activeParameter(0),0.1);
    TS_ASSERT_EQUALS(prodF.nameOfActive(4),"f1.s");
    TS_ASSERT_EQUALS(prodF.activeParameter(4),1.3);

    TS_ASSERT_EQUALS(prodF.parameterLocalName(0),"a");

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(prodF);
    TS_ASSERT(fun);

    ProductFunctionMW* prodF1 = dynamic_cast<ProductFunctionMW*>(fun);
    TS_ASSERT(prodF1);

    TS_ASSERT_EQUALS(prodF1->nFunctions(),4);
    TS_ASSERT_EQUALS(prodF1->name(),"ProductFunctionMW");

    CompositeFunction* cf1 = prodF1;
    TS_ASSERT(cf1);
    TS_ASSERT_EQUALS(prodF1->nParams(),11);
    TS_ASSERT_EQUALS(prodF1->parameterName(0),"f0.a");
    TS_ASSERT_EQUALS(prodF1->getParameter(0),0.1);
    TS_ASSERT_EQUALS(prodF1->parameterName(2),"f1.c");
    TS_ASSERT_EQUALS(prodF1->getParameter(2),1.1);
    TS_ASSERT_EQUALS(prodF1->parameterName(6),"f2.h");
    TS_ASSERT_EQUALS(prodF1->getParameter(6),2.2);
    TS_ASSERT_EQUALS(prodF1->parameterName(10),"f3.s");
    TS_ASSERT_EQUALS(prodF1->getParameter(10),3.3);

    TS_ASSERT_EQUALS(prodF1->nActive(),11);
    TS_ASSERT_EQUALS(prodF1->nameOfActive(0),"f0.a");
    TS_ASSERT_EQUALS(prodF1->activeParameter(0),0.1);
    TS_ASSERT_EQUALS(prodF1->nameOfActive(4),"f1.s");
    TS_ASSERT_EQUALS(prodF1->activeParameter(4),1.3);

    TS_ASSERT_EQUALS(prodF1->parameterLocalName(0),"a");

    delete fun;
  }

  void testProductFunctionMW()
  {
    ProductFunctionMW prodF;

    double c1 = 1.0;
    double h1 = 3.0;
    double s1 = 0.5;
    Gaussian* f0 = new Gaussian();
    f0->initialize();
    f0->setParameter("PeakCentre",c1);
    f0->setParameter("Height",h1);
    f0->setParameter("Sigma",s1);

    prodF.addFunction(f0);

    const int N = 30;
    double x[N],out[N],dx = 0.1;
    for(int i=0;i<N;i++)
    {
      x[i] = i*dx;
    }

    double c2 = 2;
    double h2 = 10.0;
    double s2 = 0.5;
    Gaussian* f1 = new Gaussian();
    f1->initialize();
    f1->setParameter("PeakCentre",c2);
    f1->setParameter("Height",h2);
    f1->setParameter("Sigma",s2);

    prodF.addFunction(f1);

    prodF.function(out,x,N);



    // a product of two gaussians is another gaussian

    for(int i=0;i<N;i++)
    {
      TS_ASSERT_DELTA(out[i],h1*exp(-0.5*(x[i]-c1)*(x[i]-c1)/(s1*s1))*
                             h2*exp(-0.5*(x[i]-c2)*(x[i]-c2)/(s2*s2)),1e-6);
    }

    // create dummy workspace to fit against    
    std::string wsName = "ProductFunctionMWTest_workspace";
    int histogramNumber = 1;
    int timechannels = 30;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    Mantid::MantidVec& xx = ws2D->dataX(0);
    Mantid::MantidVec& yy = ws2D->dataY(0); 
    Mantid::MantidVec& ee = ws2D->dataE(0); 

    for(int i=0;i<N;i++)
    {
      xx[i] = x[i];
      yy[i] = out[i];
      ee[i] = 0.1;
    }

    AnalysisDataService::Instance().add(wsName,ws);

    Fit fit;
    fit.initialize();
    fit.setPropertyValue("InputWorkspace",wsName);
    fit.setPropertyValue("WorkspaceIndex","0");

    f0->tie("PeakCentre","1.0");
    f0->tie("Height","3.0");
    f0->tie("Sigma","0.5");
    f1->setParameter("PeakCentre",c2+0.5);
    f1->setParameter("Height",h2+5.0);
    f1->tie("Sigma","0.5");
    fit.setPropertyValue("Function",prodF);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( fit.execute() )
    )    
    TS_ASSERT( fit.isExecuted() );

    // test the output from fit is what you expect

    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.0,0.01);

    IFitFunction *outF = FunctionFactory::Instance().createInitialized(fit.getPropertyValue("Function")); 
    //CompositeFunctionMW *pk = dynamic_cast<CompositeFunctionMW *>(out);

    TS_ASSERT_DELTA( outF->getParameter("f0.PeakCentre"), 1.0 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f0.Height"), 3.0 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f0.Sigma"), 0.5 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f1.PeakCentre"), 2.0 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f1.Height"), 10.0 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f1.Sigma"), 0.5 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }

private:

};

#endif /*CONVOLUTIONTEST_H_*/
