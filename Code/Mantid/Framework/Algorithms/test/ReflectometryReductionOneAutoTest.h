#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryReductionOneAuto.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <boost/assign/list_of.hpp>

using Mantid::Algorithms::ReflectometryReductionOneAuto;
using namespace Mantid::API;
using namespace boost::assign;
using Mantid::MantidVec;

class ReflectometryReductionOneAutoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneAutoTest *createSuite() { return new ReflectometryReductionOneAutoTest(); }
  static void destroySuite( ReflectometryReductionOneAutoTest *suite ) { delete suite; }

  MatrixWorkspace_sptr m_TOF;
  MatrixWorkspace_sptr m_NotTOF;
  const std::string outWSQ;
  const std::string outWSLam;
  const std::string inWSName;
  const std::string transWSName;

  ReflectometryReductionOneAutoTest(): outWSQ("ReflectometryReductionOneAutoTest_OutputWS_Q"),
    outWSLam("ReflectometryReductionOneAutoTest_OutputWS_Lam"),
    inWSName("ReflectometryReductionOneAutoTest_InputWS"),
    transWSName("ReflectometryReductionOneAutoTest_TransWS")
  {
    MantidVec xData = boost::assign::list_of(0)(0)(0)(0).convert_to_container<MantidVec>();
    MantidVec yData = boost::assign::list_of(0)(0)(0).convert_to_container<MantidVec>();

    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("UnitX", "1/q");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setPropertyValue("OutputWorkspace", "NotTOF");
    createWorkspace->execute();
    m_NotTOF = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("NotTOF");

    createWorkspace->setProperty("UnitX", "TOF");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setPropertyValue("OutputWorkspace", "TOF");
    createWorkspace->execute();
    m_TOF = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("TOF");
  }

  ~ReflectometryReductionOneAutoTest()
  {

  }

  IAlgorithm_sptr construct_standard_algorithm()
  {
    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
    alg->initialize();
    alg->setProperty("InputWorkspace", m_TOF);
    alg->setProperty("WavelengthMin", 0.0);
    alg->setProperty("WavelengthMax", 1.0);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg->setPropertyValue("ProcessingInstructions", "0, 1");
    alg->setPropertyValue("OutputWorkspace", "out_ws_Q");
    alg->setPropertyValue("OutputWorkspaceWavelength", "out_ws_Lam");
    alg->setRethrows(true);
    return alg;
  }

  void test_Init()
  {
    ReflectometryReductionOneAuto alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
  }

  void test_exec()
  {
    // Name of the output workspace.

    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lalg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( lAlg->initialize() );
    TS_ASSERT( lAlg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( lAlg->setPropertyValue("Filename", "INTER00013463.nxs"); );
    TS_ASSERT_THROWS_NOTHING( lAlg->setPropertyValue("OutputWorkspace", transWSName); );
    TS_ASSERT_THROWS_NOTHING( lAlg->execute(); );
    TS_ASSERT_THROWS_NOTHING( lAlg->setPropertyValue("Filename", "INTER00013460.nxs"); );
    TS_ASSERT_THROWS_NOTHING( lAlg->setPropertyValue("OutputWorkspace", inWSName); );
    TS_ASSERT_THROWS_NOTHING( lAlg->execute(); );
    TS_ASSERT( lAlg->isExecuted() );
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName);
    MatrixWorkspace_sptr tws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(transWSName);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg->initialize() );
    TS_ASSERT( alg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", ws); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("AnalysisMode", "PointDetectorAnalysis"); );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", outWSQ); );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspaceWavelength", outWSLam); );
    TS_ASSERT_THROWS_NOTHING( alg->execute(); );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING( outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQ); );

    auto inst = tws->getInstrument();
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
    AnalysisDataService::Instance().remove(outWSQ);
    AnalysisDataService::Instance().remove(outWSLam);
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(transWSName);

  }

  void test_check_input_workpace_not_tof_throws()
  {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", m_NotTOF), std::invalid_argument);
  }

  void test_check_first_transmission_workspace_not_tof_or_wavelength_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_NotTOF);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_check_second_transmission_workspace_not_tof_throws()
  {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("SecondTransmissionRun", m_NotTOF), std::invalid_argument);
  }

  void test_proivde_second_transmission_run_without_first_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("SecondTransmissionRun", m_TOF);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_provide_second_transmission_run_without_params_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_provide_second_transmission_run_without_start_overlap_q_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    MantidVec params = boost::assign::list_of(0.0)(0.1)(1.0).convert_to_container<MantidVec>();
    alg->setProperty("Params", params);
    alg->setProperty("EndOverlap", 0.4);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_provide_end_transmission_run_without_end_overlap_q_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    MantidVec params = boost::assign::list_of(0.0)(0.1)(1.0).convert_to_container<MantidVec>();
    alg->setProperty("Params", params);
    alg->setProperty("StartOverlap", 0.4);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_end_overlap_q_must_be_greater_than_start_overlap_q_or_throw()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    MantidVec params = boost::assign::list_of(0.0)(0.1)(1.0).convert_to_container<MantidVec>();
    alg->setProperty("Params", params);
    alg->setProperty("StartOverlap", 0.6);
    alg->setProperty("EndOverlap", 0.4);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

};


#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_ */