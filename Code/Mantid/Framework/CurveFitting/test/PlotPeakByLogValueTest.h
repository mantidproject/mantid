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
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <sstream>
#include <algorithm>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

namespace
{
  struct Fun
  {
    double operator()(double, int i)
    {
        return double(i + 1);
    }
  };

  class PLOTPEAKBYLOGVALUETEST_Fun: public IFunction1D, public ParamFunction
  {
  public:
      PLOTPEAKBYLOGVALUETEST_Fun():IFunction1D(),ParamFunction()
      {
          declareParameter("A");
          declareAttribute("WorkspaceIndex", Attribute(0));
      }
      std::string name() const {return "PLOTPEAKBYLOGVALUETEST_Fun";}
      void function1D(double *out, const double *, const size_t nData) const
      {
          if ( nData == 0 ) return;
          const double a = getParameter("A") + static_cast<double>( getAttribute("WorkspaceIndex").asInt() );
          std::fill_n( out, nData, a );
      }
  };

  DECLARE_FUNCTION(PLOTPEAKBYLOGVALUETEST_Fun)

  class PropertyNameIs
  {
    public:
      PropertyNameIs(std::string name) : m_name(name) {};

      bool operator()(Mantid::Kernel::PropertyHistory_sptr p)
      {
        return p->name() == m_name;
      }

    private:
      std::string m_name;
  };

}

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

  void test_passWorkspaceIndexToFunction()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),3,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "PLOTPEAKBYLOGVALUETEST_WS", ws );
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction",true);
    alg.setPropertyValue("Function","name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    TWS_type result =  WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT( result );

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    TableRow row = result->getFirstRow();
    do{
        TS_ASSERT_DELTA( row.Double(1), 1.0, 1e-15 );
    }
    while( row.next() );

    AnalysisDataService::Instance().clear();
  }

  void test_dont_passWorkspaceIndexToFunction()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),3,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "PLOTPEAKBYLOGVALUETEST_WS", ws );
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction",false);
    alg.setPropertyValue("Function","name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    TWS_type result =  WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT( result );

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    double a = 1.0;
    TableRow row = result->getFirstRow();
    do{
        TS_ASSERT_DELTA( row.Double(1), a, 1e-15 );
        a += 1.0;
    }
    while( row.next() );

    AnalysisDataService::Instance().clear();
  }

  void test_passWorkspaceIndexToFunction_composit_function_case()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),3,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "PLOTPEAKBYLOGVALUETEST_WS", ws );
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction",true);
    alg.setPropertyValue("Function","name=FlatBackground,ties=(A0=0.5);name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    TWS_type result =  WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT( result );

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    TableRow row = result->getFirstRow();
    do{
        TS_ASSERT_DELTA( row.Double(1), 0.5, 1e-15 );
    }
    while( row.next() );

    AnalysisDataService::Instance().clear();
  }

  void test_createOutputOption()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(Fun(),3,-5.0,5.0,0.1,false);
    AnalysisDataService::Instance().add( "PLOTPEAKBYLOGVALUETEST_WS", ws );
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PLOTPEAKBYLOGVALUETEST_WS,v1:3");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction",true);
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function","name=FlatBackground,ties=(A0=0.5);name=PLOTPEAKBYLOGVALUETEST_Fun");
    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    TWS_type result =  WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT( result );

    // each spectrum contains values equal to its spectrum number (from 1 to 3)
    TableRow row = result->getFirstRow();
    do{
        TS_ASSERT_DELTA( row.Double(1), 0.5, 1e-15 );
    }
    while( row.next() );

    auto matrices = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_NormalisedCovarianceMatrices");
    auto params = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Parameters");
    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");

    TS_ASSERT( matrices );
    TS_ASSERT( params );
    TS_ASSERT( fits );

    TS_ASSERT( matrices->getNames().size() == 3 );
    TS_ASSERT( params->getNames().size() == 3 );
    TS_ASSERT( fits->getNames().size() == 3 );

    AnalysisDataService::Instance().clear();
  }

  void test_createOutputOptionMultipleWorkspaces()
  {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PlotPeakGroup_0;PlotPeakGroup_1;PlotPeakGroup_2");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setPropertyValue("WorkspaceIndex","1");
    alg.setPropertyValue("LogValue","var");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("Function","name=LinearBackground,A0=1,A1=0.3;name=Gaussian,PeakCentre=5,Height=2,Sigma=0.1");
    TS_ASSERT( alg.execute() );

    TWS_type result = WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT_EQUALS(result->columnCount(),12);

    auto matrices = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_NormalisedCovarianceMatrices");
    auto params = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Parameters");
    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");

    TS_ASSERT( matrices );
    TS_ASSERT( params );
    TS_ASSERT( fits );

    TS_ASSERT( matrices->getNames().size() == 3 );
    TS_ASSERT( params->getNames().size() == 3 );
    TS_ASSERT( fits->getNames().size() == 3 );
  }


  void test_createOutputWithExtraOutputOptions()
  {
    auto ws = createTestWorkspace();
    AnalysisDataService::Instance().add( "PLOTPEAKBYLOGVALUETEST_WS", ws );
    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PLOTPEAKBYLOGVALUETEST_WS,v0:2");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setProperty("PassWSIndexToFunction",true);
    alg.setProperty("CreateOutput", true);
    alg.setProperty("OutputCompositeMembers", true);
    alg.setProperty("ConvolveMembers", true);
    alg.setPropertyValue("Function","name=LinearBackground,A0=0,A1=0;"
      "(composite=Convolution,FixResolution=true,NumDeriv=true;"
      "name=Resolution,Workspace=PLOTPEAKBYLOGVALUETEST_WS,WorkspaceIndex=0;"
      "name=Gaussian,Height=3000,PeakCentre=6493,Sigma=50;);");
    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    TWS_type result =  WorkspaceCreationHelper::getWS<TableWorkspace>("PlotPeakResult");
    TS_ASSERT( result );

    auto matrices = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_NormalisedCovarianceMatrices");
    auto params = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Parameters");
    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");

    TS_ASSERT( matrices );
    TS_ASSERT( params );
    TS_ASSERT( fits );

    TS_ASSERT( matrices->getNames().size() == 2 );
    TS_ASSERT( params->getNames().size() == 2 );
    TS_ASSERT( fits->getNames().size() == 2 );

    auto wsNames = fits->getNames();
    for (size_t i=0; i< wsNames.size(); ++i)
    {
      auto fit = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsNames[i]);
      TS_ASSERT( fit );
      TS_ASSERT( fit->getNumberHistograms() == 5);
    }

    AnalysisDataService::Instance().clear();
  }


  void testMinimizer()
  {
    createData();

    PlotPeakByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("Input","PlotPeakGroup_0;PlotPeakGroup_1;PlotPeakGroup_2");
    alg.setPropertyValue("OutputWorkspace","PlotPeakResult");
    alg.setProperty("CreateOutput", true);
    alg.setPropertyValue("WorkspaceIndex","1");
    alg.setPropertyValue("Function","name=LinearBackground,A0=1,A1=0.3;name=Gaussian,PeakCentre=5,Height=2,Sigma=0.1");
    alg.setPropertyValue("MaxIterations", "50");
    alg.setPropertyValue("Minimizer", "Levenberg-Marquardt,AbsError=0.01,RelError=0.01");

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    auto fits = AnalysisDataService::Instance().retrieveWS<const WorkspaceGroup>("PlotPeakResult_Workspaces");
    TS_ASSERT(fits);

    for (size_t i = 0; i < fits->size(); ++i)
    {
      // Get the Fit algorithm history
      auto fit = fits->getItem(i);
      const auto & wsHistory = fit->getHistory();
      const auto & child = wsHistory.getAlgorithmHistory(0);
      TS_ASSERT_EQUALS(child->name(), "Fit");
      const auto & properties = child->getProperties();

      // Check max iterations property
      PropertyNameIs maxIterationsCheck("MaxIterations");
      auto prop = std::find_if(properties.begin(), properties.end(), maxIterationsCheck);
      TS_ASSERT_EQUALS((*prop)->value(), "50");

      // Check minimizer property
      PropertyNameIs minimizerCheck("Minimizer");
      prop = std::find_if(properties.begin(), properties.end(), minimizerCheck);
      TS_ASSERT_EQUALS((*prop)->value(), "Levenberg-Marquardt,AbsError=0.01,RelError=0.01");
    }
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
        ws->getSpectrum(i)->setSpectrumNo(0);
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

  MatrixWorkspace_sptr createTestWorkspace()
  {
    const int numHists(2);
    const int numBins(2000);
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numHists, numBins, true);
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    MantidVecPtr xdata;
    xdata.access().resize(numBins+1);
    // Update X data  to a sensible values. Looks roughly like the MARI binning
    // Update the Y values. We don't care about errors here

    // We'll simply use a gaussian as a test
    const double peakOneCentre(6493.0), sigmaSqOne(250*250.), peakTwoCentre(10625.), sigmaSqTwo(50*50);
    const double peakOneHeight(3000.), peakTwoHeight(1000.);
    for( int i = 0; i <= numBins; ++i)
    {
      const double xValue = 5.0 + 5.5*i;
      if( i < numBins )
      {
        testWS->dataY(0)[i] = peakOneHeight * exp(-0.5*pow(xValue - peakOneCentre, 2.)/sigmaSqOne);
        testWS->dataY(1)[i] = peakTwoHeight * exp(-0.5*pow(xValue - peakTwoCentre, 2.)/sigmaSqTwo);

      }
      xdata.access()[i] = xValue;
    }
    testWS->setX(0, xdata);
    testWS->setX(1, xdata);
    return testWS;
  }

  void deleteData()
  {
    FrameworkManager::Instance().deleteWorkspace(m_wsg->getName());
    m_wsg.reset();
  }
};

#endif /*PLOTPEAKBYLOGVALUETEST_H_*/
