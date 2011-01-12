#ifndef CURVEFITTING_COMPOSITEFUNCTIONTEST_H_
#define CURVEFITTING_COMPOSITEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class ITestFunction
{
public:
  virtual void testInit(Mantid::DataObjects::Workspace2D_const_sptr ws,int spec,int xMin,int xMax) = 0;
};

class CurveFittingGauss: public IPeakFunction, public ITestFunction
{
public:
  CurveFittingGauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "CurveFittingGauss";}

  void functionLocal(double* out, const double* xValues, const int& nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(int i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-0.5*x*x*w);
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

  void testInit(Mantid::DataObjects::Workspace2D_const_sptr ws,int spec,int xMin,int xMax)
  {
    TS_ASSERT_EQUALS(ws.get(),m_workspace.get());
    TS_ASSERT_EQUALS(spec,m_workspaceIndex);
    TS_ASSERT_EQUALS(xMin,m_xMinIndex);
    TS_ASSERT_EQUALS(xMax,m_xMaxIndex);
  }
};


class CurveFittingLinear: public Function, public ITestFunction
{
public:
  CurveFittingLinear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "CurveFittingLinear";}

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

  void testInit(Mantid::DataObjects::Workspace2D_const_sptr ws,int spec,int xMin,int xMax)
  {
    TS_ASSERT_EQUALS(ws.get(),m_workspace.get());
    TS_ASSERT_EQUALS(spec,m_workspaceIndex);
    TS_ASSERT_EQUALS(xMin,m_xMinIndex);
    TS_ASSERT_EQUALS(xMax,m_xMaxIndex);
  }
};

class CompFunction : public CompositeFunction
{
public:

  void testInit(Mantid::DataObjects::Workspace2D_const_sptr ws,int spec,int xMin,int xMax)
  {
    TS_ASSERT_EQUALS(ws.get(),m_workspace.get());
    TS_ASSERT_EQUALS(spec,m_workspaceIndex);
    TS_ASSERT_EQUALS(xMin,m_xMinIndex);
    TS_ASSERT_EQUALS(xMax,m_xMaxIndex);

    for(int i=0;i<nFunctions();i++)
    {
      ITestFunction* f = dynamic_cast<ITestFunction*>(getFunction(i));
      f->testInit(ws,spec,xMin,xMax);
    }
  }

};

DECLARE_FUNCTION(CurveFittingLinear);
DECLARE_FUNCTION(CurveFittingGauss);

class CompositeFunctionTest : public CxxTest::TestSuite
{
public:
  CompositeFunctionTest()
  {
    Kernel::ConfigService::Instance().setString("curvefitting.peakRadius","100");
    FrameworkManager::Instance();
  }

  void testFit()
  {
    CompFunction *mfun = new CompFunction();
    CurveFittingGauss *g1 = new CurveFittingGauss(),*g2 = new CurveFittingGauss();
    CurveFittingLinear *bk = new CurveFittingLinear();

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(g2);

    g1->setParameter("c",3.1);
    g1->setParameter("h",1.1);
    g1->setParameter("s",1.);

    g2->setParameter("c",7.1);
    g2->setParameter("h",1.1);
    g2->setParameter("s",1.);

    bk->setParameter("a",0.8);

    TS_ASSERT_EQUALS(mfun->nParams(),8);
    TS_ASSERT_EQUALS(mfun->nActive(),8);

    TS_ASSERT_EQUALS(mfun->getParameter(0),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.0);
    TS_ASSERT_EQUALS(mfun->getParameter(5),7.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(7),1.0);

    WS_type ws = mkWS(1,0,10,0.1);
    addNoise(ws,0.1);
    storeWS("mfun",ws);

    mfun->setMatrixWorkspace(ws,7,12,3);
    mfun->testInit(ws,7,12,3);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","mfun");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    //alg.setFunction(mfun);
    alg.setPropertyValue("Function",*mfun);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);
    for(int i=0;i<Y.size();i++)
    {
      TS_ASSERT_EQUALS(Y00[i],Y0[i]);
      TS_ASSERT_DELTA(Y0[i],Y[i],0.1);
      TS_ASSERT_DIFFERS(R[i],0);
    }

    IFunction* out = FunctionFactory::Instance().createInitialized(alg.getPropertyValue("Function"));
    TS_ASSERT_EQUALS(out->parameterName(0),"f0.a");
    TS_ASSERT_DELTA(out->getParameter(0),1,0.1);

    TS_ASSERT_EQUALS(out->parameterName(1),"f0.b");
    TS_ASSERT_DELTA(out->getParameter(1),0.1,0.1);

    TS_ASSERT_EQUALS(out->parameterName(2),"f1.c");
    TS_ASSERT_DELTA(out->getParameter(2),4,0.2);

    TS_ASSERT_EQUALS(out->parameterName(3),"f1.h");
    TS_ASSERT_DELTA(out->getParameter(3),1,0.2);

    TS_ASSERT_EQUALS(out->parameterName(4),"f1.s");
    TS_ASSERT_DELTA(out->getParameter(4),2.13,0.2);

    TS_ASSERT_EQUALS(out->parameterName(5),"f2.c");
    TS_ASSERT_DELTA(out->getParameter(5),6,0.2);

    TS_ASSERT_EQUALS(out->parameterName(6),"f2.h");
    TS_ASSERT_DELTA(out->getParameter(6),2,0.2);

    TS_ASSERT_EQUALS(out->parameterName(7),"f2.s");
    TS_ASSERT_DELTA(out->getParameter(7),3.0,0.2);


    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),1,0.1);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.1,0.1);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),4,0.2);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),1,0.2);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    TS_ASSERT_DELTA(row.Double(1),2.13,0.2);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6,0.2);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),2,0.2);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3.0,0.2);

    removeWS("mfun");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }
private:
  WS_type mkWS(int nSpec,double x0,double x1,double dx,bool isHist=false)
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
        double x1 = x-4;
        double x2 = x-6;
        Y[i] = 1. + 0.1*x + exp(-0.5*(x1*x1)*2)+2*exp(-0.5*(x2*x2)*3);
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

  void interrupt()
  {
    int iii;
    std::cerr<<"Enter a number:";
    std::cin>>iii;
  }
};

#endif /*CURVEFITTING_COMPOSITEFUNCTIONTEST_H_*/
