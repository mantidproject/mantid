#ifndef PLOTPEAKBYLOGVALUETEST_H_
#define PLOTPEAKBYLOGVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PlotPeakByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
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
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotPeakByLogValueTest *createSuite() { return new PlotPeakByLogValueTest(); }
  static void destroySuite( PlotPeakByLogValueTest *suite ) { delete suite; }

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
    TS_ASSERT(alg.isExecuted());

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),12);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(),12);
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
    TS_ASSERT_EQUALS(tnames[11],"Chi_squared");

    TS_ASSERT_DELTA(result->Double(0,0),1,1e-10);
    TS_ASSERT_DELTA(result->Double(0,1),1,1e-10);
    TS_ASSERT_DELTA(result->Double(0,3),0.3,1e-10);
    TS_ASSERT_DELTA(result->Double(0,5),2,1e-10);
    TS_ASSERT_DELTA(result->Double(0,7),5,1e-10);
    TS_ASSERT_DELTA(result->Double(0,9),0.1,1e-10);

    TS_ASSERT_DELTA(result->Double(1,0),1.3,1e-10);
    TS_ASSERT_DELTA(result->Double(1,1),1.1,1e-10);
    TS_ASSERT_DELTA(result->Double(1,3),0.28,1e-10);
    TS_ASSERT_DELTA(result->Double(1,5),1.8,1e-10);
    TS_ASSERT_DELTA(result->Double(1,7),5.03,1e-10);
    TS_ASSERT_DELTA(result->Double(1,9),0.11,1e-10);

    TS_ASSERT_DELTA(result->Double(2,0),1.6,1e-10);
    TS_ASSERT_DELTA(result->Double(2,1),1.2,1e-10);
    TS_ASSERT_DELTA(result->Double(2,3),0.26,1e-10);
    TS_ASSERT_DELTA(result->Double(2,5),1.6,1e-10);
    TS_ASSERT_DELTA(result->Double(2,7),5.06,1e-10);
    TS_ASSERT_DELTA(result->Double(2,9),0.12,1e-10);

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");

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

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),12);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(),12);
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
    TS_ASSERT_EQUALS(tnames[11],"Chi_squared");

    TS_ASSERT_DELTA(result->Double(0,0),1,1e-10);
    TS_ASSERT_DELTA(result->Double(0,1),1,1e-10);
    TS_ASSERT_DELTA(result->Double(0,3),0.3,1e-10);
    TS_ASSERT_DELTA(result->Double(0,5),2,1e-10);
    TS_ASSERT_DELTA(result->Double(0,7),5,1e-10);
    TS_ASSERT_DELTA(result->Double(0,9),0.1,1e-10);

    TS_ASSERT_DELTA(result->Double(1,0),1.3,1e-10);
    TS_ASSERT_DELTA(result->Double(1,1),1.1,1e-10);
    TS_ASSERT_DELTA(result->Double(1,3),0.28,1e-10);
    TS_ASSERT_DELTA(result->Double(1,5),1.8,1e-10);
    TS_ASSERT_DELTA(result->Double(1,7),5.03,1e-10);
    TS_ASSERT_DELTA(result->Double(1,9),0.11,1e-10);

    TS_ASSERT_DELTA(result->Double(2,0),1.6,1e-10);
    TS_ASSERT_DELTA(result->Double(2,1),1.2,1e-10);
    TS_ASSERT_DELTA(result->Double(2,3),0.26,1e-10);
    TS_ASSERT_DELTA(result->Double(2,5),1.6,1e-10);
    TS_ASSERT_DELTA(result->Double(2,7),5.06,1e-10);
    TS_ASSERT_DELTA(result->Double(2,9),0.12,1e-10);

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");

  }

  void testWorkspaceList_plotting_against_ws_names()
  {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PlotPeakGroup_0;PlotPeakGroup_1;PlotPeakGroup_2");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex","1");
    alg.setPropertyValue("LogValue","SourceName");
    alg.setPropertyValue("Function","name=LinearBackground,A0=1,A1=0.3;name=Gaussian,PeakCentre=5,Height=2,Sigma=0.1");
    alg.execute();

    TWS_type result =  WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),12);

    std::vector<std::string> tnames = result->getColumnNames();
    TS_ASSERT_EQUALS(tnames.size(),12);
    TS_ASSERT_EQUALS(tnames[0],"Source name");

    TS_ASSERT_EQUALS(result->String(0,0),"PlotPeakGroup_0");
    TS_ASSERT_EQUALS(result->String(1,0),"PlotPeakGroup_1");
    TS_ASSERT_EQUALS(result->String(2,0),"PlotPeakGroup_2");

    deleteData();
    WorkspaceCreationHelper::removeWS("PlotPeakResult");

  }

private:

  WorkspaceGroup_sptr m_wsg;

  void createData()
  {
    m_wsg.reset(new WorkspaceGroup);
    AnalysisDataService::Instance().add("PlotPeakGroup",m_wsg);
    const int N = 3;
    for(int iWS=0;iWS<N;++iWS)
    {
      auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(PlotPeak_Expression(iWS),3,0,10,0.005);
      for( int i=0; i < 3; ++i)
      {
        ws->getAxis(1)->setValue(i, 0);
      }
      Kernel::TimeSeriesProperty<double>* logd = new Kernel::TimeSeriesProperty<double>("var");
      logd->addValue("2007-11-01T18:18:53",1+iWS*0.3);
      ws->mutableRun().addLogData(logd);
      std::ostringstream wsName;
      wsName << "PlotPeakGroup_" << iWS ;
      WorkspaceCreationHelper::storeWS(wsName.str(), ws);
      m_wsg->add(wsName.str());
    }
  }

  void deleteData()
  {
    FrameworkManager::Instance().deleteWorkspace(m_wsg->getName());
    m_wsg.reset();
  }
};

#endif /*PLOTPEAKBYLOGVALUETEST_H_*/
