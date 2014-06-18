#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateTransmissionWorkspaceAuto.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"

using Mantid::Algorithms::CreateTransmissionWorkspaceAuto;
using namespace Mantid::API;

class CreateTransmissionWorkspaceAutoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspaceAutoTest *createSuite() { return new CreateTransmissionWorkspaceAutoTest(); }
  static void destroySuite( CreateTransmissionWorkspaceAutoTest *suite ) { delete suite; }

  CreateTransmissionWorkspaceAutoTest()
  {
    FrameworkManager::Instance();
  }

  void test_Init()
  {
    CreateTransmissionWorkspaceAuto alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("CreateTransmissionWorkspaceAutoTest_OutputWS");
    std::string inWSName("CreateTransmissionWorkspaceAutoTest_InputWS");
    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_THROWS_NOTHING( lAlg->initialize() );
    TS_ASSERT( lAlg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( lAlg->setPropertyValue("Filename", "INTER00013463.nxs"); );
    TS_ASSERT_THROWS_NOTHING( lAlg->setPropertyValue("OutputWorkspace", inWSName); );
    TS_ASSERT_THROWS_NOTHING( lAlg->execute(); );
    TS_ASSERT( lAlg->isExecuted() );
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    TS_ASSERT_THROWS_NOTHING( alg->initialize() );
    TS_ASSERT( alg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("FirstTransmissionRun", ws); );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", outWSName); );
    TS_ASSERT_THROWS_NOTHING( alg->execute(); );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING( outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName); );

    auto inst = ws->getInstrument();
    auto history = outWS->getHistory();
    auto algHist = history.getAlgorithmHistory(history.size()-1);
    auto historyAlg = algHist->getChildAlgorithm(0);
    
    double wavelengthMin = historyAlg->getProperty("WavelengthMin");
    double wavelengthMax = historyAlg->getProperty("WavelengthMax");
    double monitorBackgroundWavelengthMin = historyAlg->getProperty("MonitorBackgroundWavelengthMin");
    double monitorBackgroundWavelengthMax = historyAlg->getProperty("MonitorBackgroundWavelengthMax");
    double monitorIntegrationWavelengthMin = historyAlg->getProperty("MonitorIntegrationWavelengthMin");
    double monitorIntegrationWavelengthMax = historyAlg->getProperty("MonitorIntegrationWavelengthMax");
    int i0MonitorIndex = historyAlg->getProperty("I0MonitorIndex");
    std::string processingInstructions = historyAlg->getProperty("ProcessingInstructions");
    std::vector<std::string> pointDetectorStartStop;
    boost::split(pointDetectorStartStop,processingInstructions,boost::is_any_of(","));

    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMin").at(0), wavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMax").at(0), wavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMin").at(0), monitorBackgroundWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMax").at(0), monitorBackgroundWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMin").at(0), monitorIntegrationWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMax").at(0), monitorIntegrationWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("I0MonitorIndex").at(0), i0MonitorIndex);
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStart").at(0), boost::lexical_cast<double>(pointDetectorStartStop.at(0)));
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStop").at(0), boost::lexical_cast<double>(pointDetectorStartStop.at(1)));
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(inWSName);

  }
  
  void xtest_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_ */