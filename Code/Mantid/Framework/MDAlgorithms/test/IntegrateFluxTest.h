#ifndef MANTID_MDALGORITHMS_INTEGRATEFLUXTEST_H_
#define MANTID_MDALGORITHMS_INTEGRATEFLUXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/IntegrateFlux.h"
#include "MantidAPI/AlgorithmManager.h"

using Mantid::MDAlgorithms::IntegrateFlux;
using namespace Mantid::API;

class IntegrateFluxTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateFluxTest *createSuite() { return new IntegrateFluxTest(); }
  static void destroySuite( IntegrateFluxTest *suite ) 
  { 
    delete suite; 
  }


  void test_Init()
  {
    IntegrateFlux alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the input workspace.
    std::string inWSName("IntegrateFluxTest_InputWS");
    // Name of the output workspace.
    std::string outWSName("IntegrateFluxTest_OutputWS");

    // Create an input workspace
    auto inputWS = createInputWorkspace(inWSName);

    IntegrateFlux alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    auto inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>( inWSName );
    
    TS_ASSERT( ws->getAxis(0)->unit() == inWS->getAxis(0)->unit() );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 4 );

    auto &x = ws->readX(0);
    auto &y = ws->readY(0);
    
    TS_ASSERT_EQUALS( x.size(), 98 );
    TS_ASSERT_EQUALS( x.size(), y.size() );

    for(size_t i = 10; i < x.size(); ++i)
    {
      double t = x[i];
      TS_ASSERT_DELTA( t*(t+1.0) / y[i], 1.0, 0.1 );
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
  
  void test_two_interpolation_point()
  {
    // Name of the input workspace.
    std::string inWSName("IntegrateFluxTest_InputWS");
    // Name of the output workspace.
    std::string outWSName("IntegrateFluxTest_OutputWS");

    // Create an input workspace
    auto inputWS = createInputWorkspace(inWSName);

    IntegrateFlux alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inWSName);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setProperty("NPoints", 2);
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 4 );

    auto &x = ws->readX(0);
    auto &y = ws->readY(0);

    TS_ASSERT_EQUALS( x.size(), y.size() );
    TS_ASSERT_EQUALS( x.size(), 2 );

    double t = x[1];
    TS_ASSERT_DELTA( t*(t+1.0) / y[1], 1.0, 0.1 );

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
  
  void test_one_interpolation_point()
  {
    // Name of the input workspace.
    std::string inWSName("IntegrateFluxTest_InputWS");
    // Name of the output workspace.
    std::string outWSName("IntegrateFluxTest_OutputWS");

    // Create an input workspace
    auto inputWS = createInputWorkspace(inWSName);

    IntegrateFlux alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inWSName);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS( alg.setProperty("NPoints", 1), std::invalid_argument );

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
  
private:

  Workspace_sptr createInputWorkspace(const std::string& wsName)
  {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    alg->initialize();
    alg->setPropertyValue("WorkspaceType","Event");
    alg->setPropertyValue("Function","User Defined");
    alg->setPropertyValue("UserDefinedFunction","name=LinearBackground,A0=1,A1=2");
    alg->setProperty("NumEvents",10000);
    alg->setProperty("NumBanks",1);
    alg->setProperty("BankPixelWidth",2);
    alg->setProperty("XMin",0.0);
    alg->setProperty("XMax",100.0);
    alg->setPropertyValue("XUnit","Momentum");
    alg->setProperty("BinWidth",1.0);
    alg->setProperty("OutputWorkspace",wsName);
    alg->execute();

    alg = Mantid::API::AlgorithmManager::Instance().create("CompressEvents");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace",wsName);
    alg->setPropertyValue("OutputWorkspace",wsName);
    alg->setProperty("Tolerance",1.0);
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
    return ws;
  }


};


#endif /* MANTID_MDALGORITHMS_INTEGRATEFLUXTEST_H_ */