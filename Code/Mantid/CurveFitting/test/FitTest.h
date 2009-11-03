#ifndef FITTEST_H_
#define FITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

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

class FitTest : public CxxTest::TestSuite
{
public:
  FitTest()
  {
    FrameworkManager::Instance();
  }

  void testFit()
  {

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "function=LinearBackground,A0=1,A1=0;";
    params += "function=Gaussian, PeakCentre=4.1,Height=1.1,Sigma=0.5;";
    params += "function=Gaussian, PeakCentre=6.1,Height=3.1,Sigma=0.5;";

    alg.setPropertyValue("InputParameters",params);
    //alg.setPropertyValue("Ties","f1.Sigma=f2.Sigma/3");

    alg.execute();
    WS_type outWS = getWS("out_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);
    for(int i=0;i<Y.size();i++)
    {
      TS_ASSERT_EQUALS(Y00[i],Y0[i]);
      TS_ASSERT_DELTA(Y0[i],Y[i],0.001);
      TS_ASSERT_DIFFERS(R[i],0);
    }

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),8);
    TS_ASSERT_EQUALS(outParams->columnCount(),2);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"A0");
    TS_ASSERT_DELTA(row.Double(1),1,0.00001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"A1");
    TS_ASSERT_DELTA(row.Double(1),0.3,0.00001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"Height");
    TS_ASSERT_DELTA(row.Double(1),1,0.00001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"PeakCentre");
    TS_ASSERT_DELTA(row.Double(1),4,0.00001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"Sigma");
    TS_ASSERT_DELTA(row.Double(1),0.7071,0.00001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"Height");
    TS_ASSERT_DELTA(row.Double(1),2,0.00001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"PeakCentre");
    TS_ASSERT_DELTA(row.Double(1),6,0.00001);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"Sigma");
    TS_ASSERT_DELTA(row.Double(1),0.57735,0.00001);

    removeWS("Exp");
    removeWS("out_Workspace");
    removeWS("out_Parameters");
  }

  void testTies()
  {

    WS_type ws = mkWS(FitExpression(),1,0,10,0.1);
    storeWS("Exp",ws);

    Fit alg;
    alg.initialize();

    alg.setPropertyValue("InputWorkspace","Exp");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setPropertyValue("Output","out");
    std::string params = "";
    params += "function=LinearBackground,A0=1,A1=0;";
    params += "function=Gaussian, PeakCentre=4.1,Height=1.1,Sigma=0.5;";
    params += "function=Gaussian, PeakCentre=6.1,Height=3.1,Sigma=0.5;";

    alg.setPropertyValue("InputParameters",params);
    alg.setPropertyValue("Ties","f1.Sigma=f2.Sigma/3");

    alg.execute();
    WS_type outWS = getWS("out_Workspace");

    TWS_type outParams = getTWS("out_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),7);
    TS_ASSERT_EQUALS(outParams->columnCount(),2);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"A0");
    TS_ASSERT_DELTA(row.Double(1),1.11869,0.00001);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"A1");
    TS_ASSERT_DELTA(row.Double(1),0.287874,0.00001);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"Height");
    TS_ASSERT_DELTA(row.Double(1),1.1314,0.00001);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"PeakCentre");
    TS_ASSERT_DELTA(row.Double(1),3.88745,0.00001);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"Height");
    TS_ASSERT_DELTA(row.Double(1),1.81892,0.00001);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"PeakCentre");
    TS_ASSERT_DELTA(row.Double(1),5.87412,0.00001);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"Sigma");
    TS_ASSERT_DELTA(row.Double(1),0.741036,0.00001);

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

};

#endif /*FITTEST_H_*/
