#ifndef NEWFITTEST_H_
#define NEWFITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidCurveFitting/NewFit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class FitExpression
{
public:
  double operator()(double x)
  {return 1+0.3*x+exp(-0.5*(x-4)*(x-4)*2)+2*exp(-0.5*(x-6)*(x-6)*3);}
};

class FitExpression1
{
public:
  double operator()(double x)
  {return 1+0.3*x+0.01*exp(-0.5*(x-4)*(x-4)*2);}
};

class FitExp
{
public:
  double operator()(double x)
  {return exp(-0.5*(x-5)*(x-5)*2);}
};

class NewFitTest_Gauss: public IPeakFunction
{
public:
  NewFitTest_Gauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "NewFitTest_Gauss";}

  void functionLocal(double* out, const double* xValues, const size_t nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-0.5*x*x*w);
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
      double e = h*exp(-0.5*x*x*w);
      out->set(i,0,x*h*e*w);
      out->set(i,1,e);
      out->set(i,2,-0.5*x*x*h*e);
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


class NewFitTest_Linear: public ParamFunction, public IFunctionMW
{
public:
  NewFitTest_Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "NewFitTest_Linear";}

  void functionMW(double* out, const double* xValues, const size_t nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(size_t i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    for(size_t i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
    }
  }

};

DECLARE_FUNCTION(NewFitTest_Gauss);
DECLARE_FUNCTION(NewFitTest_Linear);

class NewFitTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NewFitTest *createSuite() { return new NewFitTest(); }
  static void destroySuite( NewFitTest *suite ) { delete suite; }

  NewFitTest()
  {
    Kernel::ConfigService::Instance().setString("curvefitting.peakRadius","100");
    FrameworkManager::Instance();
  }

  void testFit()
  {
    NewFit fit;
    fit.initialize();
    fit.setPropertyValue("InputWorkspace","");
    //fit.setProperty("WorkspaceIndex",0);
    fit.setPropertyValue("Function","name=NewFitTest_Linear");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
    TS_ASSERT(fit.isExecuted());
  }

};

#endif /*NEWFITTEST_H_*/
