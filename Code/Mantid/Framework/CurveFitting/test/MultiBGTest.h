#ifndef MULTIBGTEST_H_
#define MULTIBGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/MultiBG.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/IPeakFunction.h"

//#include <fstream>

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

/**
* A simple implementation of Jacobian.
*/
class SimpleJacobian: public Jacobian
{
public:
  /// Constructor
  SimpleJacobian(size_t nData,size_t nParams):m_nData(nData),m_nParams(nParams),m_data(nData*nParams){}
  /// Setter
  virtual void set(size_t iY, size_t iP, double value)
  {
    m_data[iY * m_nParams + iP] = value;
  }
  /// Getter
  virtual double get(size_t iY, size_t iP)
  {
    return m_data[iY * m_nParams + iP];
  }
  double *array(){return &m_data[0];}
  size_t n1()const{return m_nData;}
  size_t n2()const{return m_nParams;}
private:
  size_t m_nData; ///< size of the data / first dimension
  size_t m_nParams; ///< number of parameters / second dimension
  std::vector<double> m_data; ///< data storage
};

class MultiBGTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiBGTest *createSuite() { return new MultiBGTest(); }
  static void destroySuite( MultiBGTest *suite ) { delete suite; }

  MultiBGTest()
  {
    numBins = 31;
    x0 = -10.0;
    dx = 20.0 / static_cast<double>(numBins);

    a.resize(2);    b.resize(2);
    a[0] = 2.; b[0] = -.10;
    a[1] = -1.0; b[1] = .10;
    //a[1] = a[0] + b[0]*(x0 + numBins * dx * 1.5) ; b[1] = -.10;
    h = 20.0;
    s = 1.0;
    c = 0.1;
    Mantid::DataObjects::Workspace2D_sptr WS(new Mantid::DataObjects::Workspace2D);
    WS->initialize(2,numBins+1,numBins);
    
    for (size_t i = 0; i < numBins + 1; ++i)
    {
      WS->dataX(0)[i] = x0 + dx * static_cast<double>(i);
    }
    for(size_t spec = 0; spec < WS->getNumberHistograms(); ++spec)
    {
      WS->dataX(spec) = WS->readX(0);
      for (size_t i = 0; i < numBins; ++i)
      {
        const double x = (WS->readX(0)[i] + WS->readX(0)[i+1])/2;
        WS->dataY(spec)[i] = a[spec] + b[spec] * x + h * exp(-0.5 * (x - c) * (x - c) /s/s );// + noise();
        WS->dataE(spec)[i] = 1.0;//sqrt(fabs(WS->dataY(spec)[i]));
      }
    }
    //std::ofstream fil("MultiBGTestWS.txt");
    //for(size_t i = 0; i < numBins; ++i)
    //{
    //  fil << (WS->readX(0)[i] + WS->readX(0)[i+1])/2 << ',' << WS->readY(0)[i] << ',' << WS->readE(0)[i] << ',' << WS->readY(1)[i] << ',' << WS->readE(1)[i] << std::endl;
    //}
    AnalysisDataService::Instance().add("MultiBGTestWS",WS);
  }

  ~MultiBGTest()
  {
    AnalysisDataService::Instance().remove("MultiBGTestWS");
  }

  void testCorrectDataVectorConstrucion()
  {
    std::string funIni = "composite=MultiBG;name=Gaussian,Height=100.,Sigma=0.00100,PeakCentre=0;"
      "name=LinearBackground,A0=1,Workspace=MultiBGTestWS,WSParam=(WorkspaceIndex=0);"
      "name=LinearBackground,A0=2,Workspace=MultiBGTestWS,WSParam=(WorkspaceIndex=1)";

    Mantid::API::IFitFunction* fun = Mantid::API::FunctionFactory::Instance().createInitialized( funIni );
    fun->setWorkspace(Mantid::API::Workspace_sptr(),"",true);
    std::vector<double> out(2*numBins);

    fun->function(&out[0]);
    for(size_t i = 0; i < numBins/2; ++i)
    {
      TS_ASSERT_EQUALS(out[i],1.0);
    }
    TS_ASSERT_EQUALS(out[numBins/2],101.0);
    for(size_t i = numBins/2+1; i < numBins; ++i)
    {
      TS_ASSERT_EQUALS(out[i],1.0);
    }
    for(size_t i = numBins; i < numBins + numBins/2; ++i)
    {
      TS_ASSERT_EQUALS(out[i],2.0);
    }
    TS_ASSERT_EQUALS(out[numBins + numBins/2],102.0);
    for(size_t i = numBins + numBins/2+1; i < 2*numBins; ++i)
    {
      TS_ASSERT_EQUALS(out[i],2.0);
    }

    Mantid::API::IFitFunction* fun1 = Mantid::API::FunctionFactory::Instance().createInitialized( 
      "composite=MultiBG;name=Gaussian,Height=22.,Sigma=1,PeakCentre=0.2;"
      "name=LinearBackground,A0=2,A1=-0.1,Workspace=MultiBGTestWS,WSParam=(WorkspaceIndex=0);"
      "name=LinearBackground,A0=-1,A1=0.1,Workspace=MultiBGTestWS,WSParam=(WorkspaceIndex=1)"
      );
    fun1->setWorkspace(Mantid::API::Workspace_sptr(),"",true);
    SimpleJacobian J(fun1->dataSize(),fun1->nActive());
    fun1->functionDeriv(&J);
    double peakDeriv0 = 0;
    double peakDeriv1 = 0;
    double peakDeriv2 = 0;
    for(size_t i = 0; i < numBins; ++i)
    {
      peakDeriv0 += J.get(i,0);
      peakDeriv1 += J.get(i,1);
      peakDeriv2 += J.get(i,2);
      TS_ASSERT(J.get(i,3) != 0.0);
      if (i != numBins/2) TS_ASSERT(J.get(i,4) != 0.0);
      TS_ASSERT(J.get(i,5) == 0.0);
      TS_ASSERT(J.get(i,6) == 0.0);
    }
    TS_ASSERT(peakDeriv0 != 0.0);
    TS_ASSERT(peakDeriv1 != 0.0);
    TS_ASSERT(peakDeriv2 != 0.0);
    peakDeriv0 = 0;
    peakDeriv1 = 0;
    peakDeriv2 = 0;
    for(size_t i = numBins + 1; i < 2*numBins; ++i)
    {
      peakDeriv0 += J.get(i,0);
      peakDeriv1 += J.get(i,1);
      peakDeriv2 += J.get(i,2);
      TS_ASSERT(J.get(i,3) == 0.0);
      TS_ASSERT(J.get(i,4) == 0.0);
      TS_ASSERT(J.get(i,5) != 0.0);
      if (i != numBins + numBins/2) TS_ASSERT(J.get(i,6) != 0.0);
    }
    TS_ASSERT(peakDeriv0 != 0.0);
    TS_ASSERT(peakDeriv1 != 0.0);
    TS_ASSERT(peakDeriv2 != 0.0);

  }


  void testExists()
  {
    Mantid::API::IPeakFunction::setPeakRadius(1000);
    std::string funIni =       
      "composite=MultiBG;name=Gaussian,Height=22.,Sigma=1,PeakCentre=0.2;"
      "name=LinearBackground,A0=2,A1=-0.1,Workspace=MultiBGTestWS,WSParam=(WorkspaceIndex=0);"
      "name=LinearBackground,A0=-1,A1=0.1,Workspace=MultiBGTestWS,WSParam=(WorkspaceIndex=1)";

    Mantid::API::IFitFunction* fun = Mantid::API::FunctionFactory::Instance().createInitialized( funIni );
    TS_ASSERT(fun);
    
    MultiBG* mbg = dynamic_cast<MultiBG*>(fun);
    TS_ASSERT(mbg);
    
    std::vector<double> out(2*numBins);
    Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve("MultiBGTestWS"));
    fun->setWorkspace(ws,"",true);
    mbg->function(&out[0]);

    Mantid::API::IFunctionMW* bg1 = dynamic_cast<Mantid::API::IFunctionMW*>(mbg->getFunction(1));
    Mantid::API::IFunctionMW* bg2 = dynamic_cast<Mantid::API::IFunctionMW*>(mbg->getFunction(2));

    TS_ASSERT_EQUALS(bg1->getWorkspace()->getName(),"MultiBGTestWS");
    TS_ASSERT_EQUALS(bg1->getWorkspaceIndex(),0);
    TS_ASSERT_EQUALS(bg2->getWorkspace()->getName(),"MultiBGTestWS");
    TS_ASSERT_EQUALS(bg2->getWorkspaceIndex(),1);

    Fit fit;
    fit.initialize();
    fit.setPropertyValue("InputWorkspace","MultiBGTestWS");
    fit.setPropertyValue("Function",funIni);

    fit.execute();
    TS_ASSERT(fit.isExecuted());

    Mantid::API::IPeakFunction::setPeakRadius();
  }

  double noise()const
  {
    return (static_cast<double>(std::rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2)) * 0;
  }

  
  void testForCategories()
  {
    MultiBG forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Background" );
  }


private:
  double x0,dx;
  std::vector<double> a,b;
  double h,s,c;
  size_t numBins;
};

#endif /*MULTIBGTEST_H_*/
