#ifndef PLOTPEAKBYLOGVALUETEST_H_
#define PLOTPEAKBYLOGVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PlotPeakByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class PlotPeak_Expression
{
public:
  PlotPeak_Expression(const int i):m_ws(i){}
  double operator()(double x,int spec)
  {
    if (spec == 1)
    {
      const double a = 1. + 0.1*m_ws;
      const double b = 0.3 - 0.02*m_ws;
      const double h = 2. - 0.2*m_ws;
      const double c = 5. + 0.03*m_ws;
      const double s = 0.1 + 0.01*m_ws;
      return a+b*x+h*exp(-0.5*(x-c)*(x-c)/(s*s));
    }
    return 0.;
  }
private:
  const int m_ws;
};

class PlotPeakByLogValueTest : public CxxTest::TestSuite
{
public:
  PlotPeakByLogValueTest()
  {
    FrameworkManager::Instance();
  }

  void testWorkspaceGroup()
  {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PlotPeakGroup");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex","1");
    alg.setPropertyValue("LogValue","var");
    alg.setPropertyValue("Function","name=LinearBackground,A0=1,A1=0.3;name=Gaussian,PeakCentre=5,Height=2,Sigma=0.1");
    alg.execute();

    TWS_type result = getTWS("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),11);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(),11);
    TS_ASSERT_EQUALS(tnames[0],"var");
    TS_ASSERT_EQUALS(tnames[1],"f0.A0");
    TS_ASSERT_EQUALS(tnames[2],"f0.A0_Err");
    TS_ASSERT_EQUALS(tnames[3],"f0.A1");
    TS_ASSERT_EQUALS(tnames[4],"f0.A1_Err");
    TS_ASSERT_EQUALS(tnames[5],"f1.Height");
    TS_ASSERT_EQUALS(tnames[6],"f1.Height_Err");
    TS_ASSERT_EQUALS(tnames[7],"f1.PeakCentre");
    TS_ASSERT_EQUALS(tnames[8],"f1.PeakCentre_Err");
    TS_ASSERT_EQUALS(tnames[9],"f1.Sigma");
    TS_ASSERT_EQUALS(tnames[10],"f1.Sigma_Err");

    TS_ASSERT_DELTA(result->Double(0,0),1,1e-12);
    TS_ASSERT_DELTA(result->Double(0,1),1,1e-12);
    TS_ASSERT_DELTA(result->Double(0,3),0.3,1e-12);
    TS_ASSERT_DELTA(result->Double(0,5),2,1e-12);
    TS_ASSERT_DELTA(result->Double(0,7),5,1e-12);
    TS_ASSERT_DELTA(result->Double(0,9),0.1,1e-12);

    TS_ASSERT_DELTA(result->Double(1,0),1.3,1e-12);
    TS_ASSERT_DELTA(result->Double(1,1),1.1,1e-12);
    TS_ASSERT_DELTA(result->Double(1,3),0.28,1e-12);
    TS_ASSERT_DELTA(result->Double(1,5),1.8,1e-12);
    TS_ASSERT_DELTA(result->Double(1,7),5.03,1e-12);
    TS_ASSERT_DELTA(result->Double(1,9),0.11,1e-12);

    TS_ASSERT_DELTA(result->Double(2,0),1.6,1e-12);
    TS_ASSERT_DELTA(result->Double(2,1),1.2,1e-12);
    TS_ASSERT_DELTA(result->Double(2,3),0.26,1e-12);
    TS_ASSERT_DELTA(result->Double(2,5),1.6,1e-12);
    TS_ASSERT_DELTA(result->Double(2,7),5.06,1e-12);
    TS_ASSERT_DELTA(result->Double(2,9),0.12,1e-12);

    deleteData();
    AnalysisDataService::Instance().remove("PlotPeakResult");

  }

  void testWorkspaceList()
  {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PlotPeakGroup_0;PlotPeakGroup_1;PlotPeakGroup_2");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex","1");
    alg.setPropertyValue("LogValue","var");
    alg.setPropertyValue("Function","name=LinearBackground,A0=1,A1=0.3;name=Gaussian,PeakCentre=5,Height=2,Sigma=0.1");
    alg.execute();

    TWS_type result = getTWS("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),11);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(),11);
    TS_ASSERT_EQUALS(tnames[0],"var");
    TS_ASSERT_EQUALS(tnames[1],"f0.A0");
    TS_ASSERT_EQUALS(tnames[2],"f0.A0_Err");
    TS_ASSERT_EQUALS(tnames[3],"f0.A1");
    TS_ASSERT_EQUALS(tnames[4],"f0.A1_Err");
    TS_ASSERT_EQUALS(tnames[5],"f1.Height");
    TS_ASSERT_EQUALS(tnames[6],"f1.Height_Err");
    TS_ASSERT_EQUALS(tnames[7],"f1.PeakCentre");
    TS_ASSERT_EQUALS(tnames[8],"f1.PeakCentre_Err");
    TS_ASSERT_EQUALS(tnames[9],"f1.Sigma");
    TS_ASSERT_EQUALS(tnames[10],"f1.Sigma_Err");

    TS_ASSERT_DELTA(result->Double(0,0),1,1e-12);
    TS_ASSERT_DELTA(result->Double(0,1),1,1e-12);
    TS_ASSERT_DELTA(result->Double(0,3),0.3,1e-12);
    TS_ASSERT_DELTA(result->Double(0,5),2,1e-12);
    TS_ASSERT_DELTA(result->Double(0,7),5,1e-12);
    TS_ASSERT_DELTA(result->Double(0,9),0.1,1e-12);

    TS_ASSERT_DELTA(result->Double(1,0),1.3,1e-12);
    TS_ASSERT_DELTA(result->Double(1,1),1.1,1e-12);
    TS_ASSERT_DELTA(result->Double(1,3),0.28,1e-12);
    TS_ASSERT_DELTA(result->Double(1,5),1.8,1e-12);
    TS_ASSERT_DELTA(result->Double(1,7),5.03,1e-12);
    TS_ASSERT_DELTA(result->Double(1,9),0.11,1e-12);

    TS_ASSERT_DELTA(result->Double(2,0),1.6,1e-12);
    TS_ASSERT_DELTA(result->Double(2,1),1.2,1e-12);
    TS_ASSERT_DELTA(result->Double(2,3),0.26,1e-12);
    TS_ASSERT_DELTA(result->Double(2,5),1.6,1e-12);
    TS_ASSERT_DELTA(result->Double(2,7),5.06,1e-12);
    TS_ASSERT_DELTA(result->Double(2,9),0.12,1e-12);

    deleteData();
    AnalysisDataService::Instance().remove("PlotPeakResult");

  }

  // LoadNexus doesn't exist
  void t1estNexusFiles()
  {

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","../../../../Test/Nexus/MUSR00015189.nxs,sp3;"
      "../../../../Test/Nexus/MUSR00015190.nxs,sp3;"
      "../../../../Test/Nexus/MUSR00015191.nxs,sp3");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    //alg.setPropertyValue("WorkspaceIndex","1");
    alg.setPropertyValue("StartX","0");
    alg.setPropertyValue("EndX","8");
    alg.setPropertyValue("LogValue","Field_Danfysik");
    alg.setPropertyValue("Function","name=UserFunction,Formula=h*exp(-a*x)");
    alg.execute();

    TWS_type result = getTWS("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),5);
    TS_ASSERT_EQUALS(result->rowCount(),3);

    AnalysisDataService::Instance().remove("PlotPeakResult");
  }

  void t1estEmptyLog()
  {

    createData();
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PlotPeakGroup_0,v1:2");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setPropertyValue("StartX","0");
    alg.setPropertyValue("EndX","8");
    alg.setPropertyValue("Function","name=LinearBackground,A0=1,A1=0.3;name=Gaussian,PeakCentre=5,Height=2,Sigma=0.1");
    alg.execute();

    TWS_type result = getTWS("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),11);
    TS_ASSERT_EQUALS(result->rowCount(),3);

    AnalysisDataService::Instance().remove("PlotPeakResult");
    deleteData();
  }

  

private:

  WorkspaceGroup_sptr m_wsg;

  void createData()
  {
    m_wsg.reset(new WorkspaceGroup);
    m_wsg->add("PlotPeakGroup");
    const int N = 3;
    for(int iWS=0;iWS<N;++iWS)
    {
      WS_type ws = mkWS(PlotPeak_Expression(iWS),3,0,10,0.005);
      //addNoise(ws,0.01);
      Kernel::TimeSeriesProperty<double>* logd = new Kernel::TimeSeriesProperty<double>("var");
      logd->addValue("2007-11-01T18:18:53",1+iWS*0.3);
      ws->mutableRun().addLogData(logd);
      std::ostringstream wsName;
      wsName << "PlotPeakGroup_" << iWS ;
      AnalysisDataService::Instance().add(wsName.str(),ws);
      m_wsg->add(wsName.str());
    }
    AnalysisDataService::Instance().add("PlotPeakGroup",m_wsg);
  }

  void deleteData()
  {
    // Remove the group
    FrameworkManager::Instance().deleteWorkspace(m_wsg->getName());
    m_wsg.reset();
  }

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

    //Mantid::API::Axis* y  = new Mantid::API::SpectraAxis(3);
    //ws->replaceAxis(1,y);
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
        Y[i] = f(x,iSpec);
        E[i] = 1;
      }
      if (isHist)
        X.back() = X[nY-1] + dx;
      //y->setValue(iSpec,iSpec+1);
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

  void press_return()
  {
    std::cerr<<"Press Return";
    std::string str;
    getline(std::cin,str);
  }

};

#endif /*PLOTPEAKBYLOGVALUETEST_H_*/
