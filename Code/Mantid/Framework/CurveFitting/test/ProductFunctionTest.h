#ifndef PRODUCTFUNCTIONTEST_H_
#define PRODUCTFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ProductFunction.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Jacobian.h"

#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;

class ProductFunctionMWTest_Gauss: public Mantid::API::IPeakFunction
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
  void functionDerivLocal(Mantid::API::Jacobian* out, const double* xValues, const size_t nData)
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

};


class ProductFunctionMWTest_Linear: public Mantid::API::ParamFunction, public Mantid::API::IFunction1D
{
public:
  ProductFunctionMWTest_Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "ProductFunctionMWTest_Linear";}

  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(size_t i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv1D(Mantid::API::Jacobian* out, const double* xValues, const size_t nData)
  {
    for(size_t i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
    }
  }

};

DECLARE_FUNCTION(ProductFunctionMWTest_Gauss)
DECLARE_FUNCTION(ProductFunctionMWTest_Linear)

class ProductFunctionTest : public CxxTest::TestSuite
{
public:

  void testFunction()
  {
    Mantid::CurveFitting::ProductFunction prodF;

    Mantid::API::IFunction_sptr gauss1( new ProductFunctionMWTest_Gauss );
    gauss1->setParameter(0,1.1);
    gauss1->setParameter(1,1.2);
    gauss1->setParameter(2,1.3);
    Mantid::API::IFunction_sptr gauss2( new ProductFunctionMWTest_Gauss );
    gauss2->setParameter(0,2.1);
    gauss2->setParameter(1,2.2);
    gauss2->setParameter(2,2.3);
    Mantid::API::IFunction_sptr gauss3( new ProductFunctionMWTest_Gauss );
    gauss3->setParameter(0,3.1);
    gauss3->setParameter(1,3.2);
    gauss3->setParameter(2,3.3);
    Mantid::API::IFunction_sptr linear( new ProductFunctionMWTest_Linear );
    linear->setParameter(0,0.1);
    linear->setParameter(1,0.2);

    size_t iFun = 100000;
    iFun = prodF.addFunction(linear);
    TS_ASSERT_EQUALS(iFun,0);
    iFun = prodF.addFunction(gauss1);
    TS_ASSERT_EQUALS(iFun,1);
    iFun = prodF.addFunction(gauss2);
    TS_ASSERT_EQUALS(iFun,2);
    iFun = prodF.addFunction(gauss3);
    TS_ASSERT_EQUALS(iFun,3);

    TS_ASSERT_EQUALS(prodF.nFunctions(),4);
    TS_ASSERT_EQUALS(prodF.name(),"ProductFunction");

    Mantid::API::CompositeFunction* cf = &prodF;
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

    TS_ASSERT_EQUALS(prodF.nameOfActive(0),"f0.a");
    TS_ASSERT_EQUALS(prodF.activeParameter(0),0.1);
    TS_ASSERT_EQUALS(prodF.nameOfActive(4),"f1.s");
    TS_ASSERT_EQUALS(prodF.activeParameter(4),1.3);

    TS_ASSERT_EQUALS(prodF.parameterLocalName(0),"a");

    Mantid::API::IFunction_sptr fun = Mantid::API::FunctionFactory::Instance().createInitialized(prodF.asString());
    TS_ASSERT(fun);

    Mantid::CurveFitting::ProductFunction* prodF1 = dynamic_cast<Mantid::CurveFitting::ProductFunction*>(fun.get());
    TS_ASSERT(prodF1);

    TS_ASSERT_EQUALS(prodF1->nFunctions(),4);
    TS_ASSERT_EQUALS(prodF1->name(),"ProductFunction");

    Mantid::API::CompositeFunction* cf1 = prodF1;
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

    TS_ASSERT_EQUALS(prodF1->nameOfActive(0),"f0.a");
    TS_ASSERT_EQUALS(prodF1->activeParameter(0),0.1);
    TS_ASSERT_EQUALS(prodF1->nameOfActive(4),"f1.s");
    TS_ASSERT_EQUALS(prodF1->activeParameter(4),1.3);

    TS_ASSERT_EQUALS(prodF1->parameterLocalName(0),"a");

  }

  void testProductFunction()
  {
    Mantid::CurveFitting::ProductFunction prodF;

    double c1 = 1.0;
    double h1 = 3.0;
    double s1 = 0.5;
    Mantid::API::IFunction_sptr f0( new Mantid::CurveFitting::Gaussian );
    f0->initialize();
    f0->setParameter("PeakCentre",c1);
    f0->setParameter("Height",h1);
    f0->setParameter("Sigma",s1);

    prodF.addFunction(f0);

    const int N = 30;
    std::vector<double> x(N);
    const double dx = 0.1;
    for(int i=0;i<N;i++)
    {
      x[i] = i*dx;
    }

    Mantid::API::FunctionDomain1DVector domain(x);
    Mantid::API::FunctionValues out(domain);

    double c2 = 2;
    double h2 = 10.0;
    double s2 = 0.5;
    Mantid::API::IFunction_sptr f1( new Mantid::CurveFitting::Gaussian );
    f1->initialize();
    f1->setParameter("PeakCentre",c2);
    f1->setParameter("Height",h2);
    f1->setParameter("Sigma",s2);

    prodF.addFunction(f1);

    prodF.function(domain,out);

    // a product of two gaussians is another gaussian

    for(int i=0;i<N;i++)
    {
      TS_ASSERT_DELTA(out.getCalculated(i),h1*exp(-0.5*(x[i]-c1)*(x[i]-c1)/(s1*s1))*
                             h2*exp(-0.5*(x[i]-c2)*(x[i]-c2)/(s2*s2)),1e-6);
    }

    // create dummy workspace to fit against    
    std::string wsName = "ProductFunctionMWTest_workspace";
    int histogramNumber = 1;
    int timechannels = 30;
    Mantid::API::Workspace_sptr ws = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Mantid::DataObjects::Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);
    Mantid::MantidVec& xx = ws2D->dataX(0);
    Mantid::MantidVec& yy = ws2D->dataY(0); 
    Mantid::MantidVec& ee = ws2D->dataE(0); 

    for(int i=0;i<N;i++)
    {
      xx[i] = x[i];
      yy[i] = out.getCalculated(i);
      ee[i] = 0.1;
    }

    Mantid::API::AnalysisDataService::Instance().add(wsName,ws);

    Mantid::CurveFitting::Fit fit;
    fit.initialize();

    f0->tie("PeakCentre","1.0");
    f0->tie("Height","3.0");
    f0->tie("Sigma","0.5");
    f1->setParameter("PeakCentre",c2+0.5);
    f1->setParameter("Height",h2+5.0);
    f1->tie("Sigma","0.5");
    fit.setPropertyValue("Function",prodF.asString());
    fit.setPropertyValue("InputWorkspace",wsName);
    fit.setPropertyValue("WorkspaceIndex","0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( fit.execute() )
    )    
    TS_ASSERT( fit.isExecuted() );

    // test the output from fit is what you expect

    double dummy = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.0,0.01);

    Mantid::API::IFunction_sptr outF = fit.getProperty("Function"); 

    TS_ASSERT_DELTA( outF->getParameter("f0.PeakCentre"), 1.0 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f0.Height"), 3.0 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f0.Sigma"), 0.5 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f1.PeakCentre"), 2.0 ,0.001);
    TS_ASSERT_DELTA( outF->getParameter("f1.Height"), 10.0 ,0.01);
    TS_ASSERT_DELTA( outF->getParameter("f1.Sigma"), 0.5 ,0.001);

    Mantid::API::AnalysisDataService::Instance().remove(wsName);
  }
 
  void testForCategories()
  {
    Mantid::CurveFitting::ProductFunction forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "General" );
  }

  void testDerivatives()
  {
    Mantid::CurveFitting::ProductFunction prodF;

    Mantid::API::IFunction_sptr linear1( new ProductFunctionMWTest_Linear );
    linear1->setParameter(0, 1.0);
    linear1->setParameter(1, 2.0);

    Mantid::API::IFunction_sptr linear2( new ProductFunctionMWTest_Linear );
    linear2->setParameter(0, 3.0);
    linear2->setParameter(1, 4.0);

    prodF.addFunction( linear1 );
    prodF.addFunction( linear2 );

    Mantid::API::FunctionDomain1DVector domain(3.0);
    Mantid::API::FunctionValues out(domain);

    prodF.function( domain, out );

    TS_ASSERT_EQUALS( out.getCalculated(0), 105.0);

    Mantid::CurveFitting::Jacobian jacobian(1,4);
    prodF.functionDeriv( domain, jacobian );

    TS_ASSERT_DELTA( jacobian.get(0,0), 15, 1e-9 );
    TS_ASSERT_DELTA( jacobian.get(0,1), 45, 1e-9 );
    TS_ASSERT_DELTA( jacobian.get(0,2), 7, 1e-9 );
    TS_ASSERT_DELTA( jacobian.get(0,3), 21, 1e-9 );

  }

private:

};

#endif /*PRODUCTFUNCTIONTEST_H_*/
