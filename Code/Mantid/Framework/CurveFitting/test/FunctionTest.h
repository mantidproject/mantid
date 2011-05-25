#ifndef FUNCTIONTEST_H_
#define FUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class FunctionTestGauss: public IPeakFunction
{
public:
  FunctionTestGauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }
  std::string name()const{return "FunctionTestGauss";}
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

  void testInit(Mantid::DataObjects::Workspace2D_const_sptr ws,int spec,int xMin,int xMax)
  {
    TS_ASSERT_EQUALS(ws.get(),m_workspace.get());
    TS_ASSERT_EQUALS(spec,m_workspaceIndex);
    TS_ASSERT_EQUALS(xMin,m_xMinIndex);
    TS_ASSERT_EQUALS(xMax,m_xMaxIndex);
  }
};

DECLARE_FUNCTION(FunctionTestGauss);

class Exp
{
public:
  double operator()(double x){return exp(-0.5*(x-5.)*(x-5.)*3);}
};

class FunctionTest : public CxxTest::TestSuite
{
public:
  FunctionTest()
  {
    FrameworkManager::Instance();
  }

  void testFit()
  {
    FunctionTestGauss *g = new FunctionTestGauss();

    TS_ASSERT_EQUALS(g->category(),"General");

    g->setParameter("c",5.5);
    g->setParameter("h",1.2);
    g->setParameter("s",1.);

    TS_ASSERT_EQUALS(g->nParams(),3);
    TS_ASSERT_EQUALS(g->nActive(),3);

    TS_ASSERT_EQUALS(g->getParameter(0),5.5);
    TS_ASSERT_EQUALS(g->getParameter(1),1.2);
    TS_ASSERT_EQUALS(g->getParameter(2),1.);

    WS_type ws = mkWS(Exp(),1,0,10,0.1);
    storeWS("Exp",ws);

    g->setMatrixWorkspace(ws,12,7,9);
    g->testInit(ws,12,7,9);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    alg.setPropertyValue("Function",*g);
    delete g;
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    for(size_t i=0;i<Y.size();i++)
    {
      TS_ASSERT_EQUALS(Y00[i],Y0[i]);
      TS_ASSERT_DELTA(Y0[i],Y[i],0.001);
      //TS_ASSERT_DIFFERS(R[i],0);  ???
    }

    IFitFunction *gout = FunctionFactory::Instance().createInitialized(alg.getPropertyValue("Function"));
    TS_ASSERT_EQUALS(gout->parameterName(0),"c");
    TS_ASSERT_DELTA(gout->getParameter(0),5,0.00001);

    TS_ASSERT_EQUALS(gout->parameterName(1),"h");
    TS_ASSERT_DELTA(gout->getParameter(1),1,0.00001);

    TS_ASSERT_EQUALS(gout->parameterName(2),"s");
    TS_ASSERT_DELTA(gout->getParameter(2),3,0.00001);


    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),4);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();

    TS_ASSERT_EQUALS(row.String(0),"c");
    TS_ASSERT_DELTA(row.Double(1),5,0.00001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"h");
    TS_ASSERT_DELTA(row.Double(1),1,0.000001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"s");
    TS_ASSERT_DELTA(row.Double(1),3,0.00001);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testActive()
  {
    FunctionTestGauss* g = new FunctionTestGauss();

    g->setParameter("c",5.5);
    g->setParameter("h",1.2);
    g->setParameter("s",2.);

    g->tie("s","2");

    TS_ASSERT_EQUALS(g->nParams(),3);
    TS_ASSERT_EQUALS(g->nActive(),2);

    TS_ASSERT(g->isActive(0));
    TS_ASSERT(g->isActive(1));
    TS_ASSERT(!g->isActive(2));

    WS_type ws = mkWS(Exp(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Function",*g);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    IFitFunction *gout = FunctionFactory::Instance().createInitialized(alg.getPropertyValue("Function"));
    TS_ASSERT_DELTA(gout->getParameter(0),5,0.0001);
    TS_ASSERT_DELTA(gout->getParameter(1),0.8944,0.0001);
    TS_ASSERT_DELTA(gout->getParameter(2),2,0.00001);

    removeWS("Exp");
  }

  void testFitString()
  {
    FunctionTestGauss g;

    g.setParameter("c",5.5);
    g.setParameter("h",1.2);
    g.setParameter("s",1.);

    TS_ASSERT_EQUALS(g.nParams(),3);
    TS_ASSERT_EQUALS(g.nActive(),3);

    TS_ASSERT_EQUALS(g.getParameter(0),5.5);
    TS_ASSERT_EQUALS(g.getParameter(1),1.2);
    TS_ASSERT_EQUALS(g.getParameter(2),1.);

    WS_type ws = mkWS(Exp(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    alg.setPropertyValue("Function",g);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    for(size_t i=0;i<Y.size();i++)
    {
      TS_ASSERT_EQUALS(Y00[i],Y0[i]);
      TS_ASSERT_DELTA(Y0[i],Y[i],0.001);
      //TS_ASSERT_DIFFERS(R[i],0);
    }

    TS_ASSERT_EQUALS(g.parameterName(0),"c");
    TS_ASSERT_DELTA(g.getParameter(0),5.5,0.00001);

    TS_ASSERT_EQUALS(g.parameterName(1),"h");
    TS_ASSERT_DELTA(g.getParameter(1),1.2,0.00001);

    TS_ASSERT_EQUALS(g.parameterName(2),"s");
    TS_ASSERT_DELTA(g.getParameter(2),1.,0.00001);


    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),4);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();

    TS_ASSERT_EQUALS(row.String(0),"c");
    TS_ASSERT_DELTA(row.Double(1),5,0.00001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"h");
    TS_ASSERT_DELTA(row.Double(1),1,0.000001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"s");
    TS_ASSERT_DELTA(row.Double(1),3,0.00001);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
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
    for(size_t iSpec=0;iSpec<ws->getNumberHistograms();iSpec++)
    {
      Mantid::MantidVec& Y = ws->dataY(iSpec);
      Mantid::MantidVec& E = ws->dataE(iSpec);
      for(size_t i=0;i<Y.size();i++)
      {
        Y[i] += noise*(-.5 + double(rand())/RAND_MAX);
        E[i] += noise;
      }
    }
  }

};

#endif /*FUNCTIONTEST_H_*/
