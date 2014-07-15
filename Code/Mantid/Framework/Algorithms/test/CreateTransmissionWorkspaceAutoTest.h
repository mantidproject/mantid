#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateTransmissionWorkspaceAuto.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <boost/assign/list_of.hpp>

using Mantid::Algorithms::CreateTransmissionWorkspaceAuto;
using namespace Mantid::API;
using namespace boost::assign;
using Mantid::MantidVec;

class CreateTransmissionWorkspaceAutoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspaceAutoTest *createSuite() { return new CreateTransmissionWorkspaceAutoTest(); }
  static void destroySuite( CreateTransmissionWorkspaceAutoTest *suite ) { delete suite; }

  MatrixWorkspace_sptr m_TOF;
  MatrixWorkspace_sptr m_NotTOF;
  const std::string outWSName;
  const std::string inWSName;
  const std::string transWSName;

  CreateTransmissionWorkspaceAutoTest(): outWSName("CreateTransmissionWorkspaceAutoTest_OutputWS_Q"),
    inWSName("CreateTransmissionWorkspaceAutoTest_InputWS"),
    transWSName("CreateTransmissionWorkspaceAutoTest_TransWS")
  {
    FrameworkManager::Instance();
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

  ~CreateTransmissionWorkspaceAutoTest()
  {
    AnalysisDataService::Instance().remove("TOF");
    AnalysisDataService::Instance().remove("NotTOF");
  }

  IAlgorithm_sptr construct_standard_algorithm()
  {
    auto alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    alg->initialize();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("WavelengthMin", 0.0);
    alg->setProperty("WavelengthMax", 1.0);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setPropertyValue("ProcessingInstructions", "0, 1");
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setRethrows(true);
    return alg;
  }

  void test_Init()
  {
    CreateTransmissionWorkspaceAuto alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->initialize();
    lAlg->setPropertyValue("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", inWSName);
    lAlg->execute(); 
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

  void test_check_first_transmission_workspace_not_tof_or_wavelength_throws()
  {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("FirstTransmissionRun", m_NotTOF), std::invalid_argument);
  }

  void test_check_second_transmission_workspace_not_tof_throws()
  {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("SecondTransmissionRun", m_NotTOF), std::invalid_argument);
  }

  void test_end_overlap_must_be_greater_than_start_overlap_or_throw()
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

  void test_must_provide_wavelengths()
  {
    auto alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    alg->initialize();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    alg->setProperty("WavelengthMax", 1.0);
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setRethrows(true);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);

    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setRethrows(true);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_wavelength_min_greater_wavelength_max_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_monitor_background_wavelength_min_greater_monitor_background_wavelength_max_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("MonitorBackgroundWavelengthMin", 1.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_monitor_integration_wavelength_min_greater_monitor_integration_wavelength_max_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("MonitorIntegrationWavelengthMin", 1.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_monitor_index_positive()
  {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("I0MonitorIndex", -1), std::invalid_argument);
  }

  void test_workspace_index_list_throw_if_not_pairs()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("ProcessingInstructions", "0");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_workspace_index_list_values_not_positive_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("ProcessingInstructions", "-1, 0"); //-1 is not acceptable.
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_workspace_index_list_min_max_pairs_throw_if_min_greater_than_max()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("ProcessingInstructions", "1, 0"); //1 > 0.
    TS_ASSERT_THROWS(alg->execute(), std::out_of_range);
  }

  void test_spectrum_map_mismatch_throws()
  {
    // Name of the output workspace.
    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->initialize();
    lAlg->setPropertyValue("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", inWSName);
    lAlg->execute(); 
    MatrixWorkspace_sptr trans1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    TS_ASSERT_THROWS_NOTHING( alg->initialize() );
    TS_ASSERT( alg->isInitialized() );
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("FirstTransmissionRun", trans1); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SecondTransmissionRun", m_TOF); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("ProcessingInstructions", "3,4"); );
    MantidVec params = boost::assign::list_of(0.0)(0.1)(1.0).convert_to_container<MantidVec>();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("Params", params); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("StartOverlap", 1.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("EndOverlap", 2.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", outWSName); );
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_execute_one_tranmission()
  {
    // Name of the output workspace.
    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->initialize();
    lAlg->setPropertyValue("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", inWSName);
    lAlg->execute(); 
    MatrixWorkspace_sptr trans1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    TS_ASSERT_THROWS_NOTHING( alg->initialize() );
    TS_ASSERT( alg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("FirstTransmissionRun", trans1); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("ProcessingInstructions", "3,4"); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("I0MonitorIndex",0 ); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthMin", 0.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthMax", 17.9); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthStep", 0.5); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorBackgroundWavelengthMin", 15.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorBackgroundWavelengthMax", 17.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorIntegrationWavelengthMin", 4.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorIntegrationWavelengthMax", 10.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", outWSName); );
    TS_ASSERT_THROWS_NOTHING( alg->execute(); );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING( outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName); );

    TS_ASSERT_EQUALS("Wavelength", outWS->getAxis(0)->unit()->unitID());

    //Because we have one transmission workspace, binning should come from the WavelengthStep.
    auto x = outWS->readX(0);
    auto actual_binning = x[1] - x[0];
    double step = alg->getProperty("WavelengthStep");
    TS_ASSERT_DELTA(step, actual_binning, 0.0000001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(inWSName);
  }

  void test_execute_two_tranmissions()
  {
    // Name of the output workspace.
    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->initialize();
    lAlg->setPropertyValue("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", inWSName + "1");
    lAlg->execute(); 
    MatrixWorkspace_sptr trans1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName + "1");
    lAlg->setPropertyValue("Filename", "INTER00013464.nxs");
    lAlg->setPropertyValue("OutputWorkspace", inWSName + "2");
    lAlg->execute();
    MatrixWorkspace_sptr trans2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName + "2");

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    TS_ASSERT_THROWS_NOTHING( alg->initialize() );
    TS_ASSERT( alg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("ProcessingInstructions", "3,4"); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("FirstTransmissionRun", trans1); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SecondTransmissionRun", trans2); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("I0MonitorIndex", 0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthMin", 0.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthMax", 17.9); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthStep", 0.5); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorBackgroundWavelengthMin", 15.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorBackgroundWavelengthMax", 17.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorIntegrationWavelengthMin", 4.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorIntegrationWavelengthMax", 10.0); );
    MantidVec params = boost::assign::list_of(1.5)(0.02)(17).convert_to_container<MantidVec>();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("Params", params); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("StartOverlap", 10.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("EndOverlap", 12.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", outWSName); );
    TS_ASSERT_THROWS_NOTHING( alg->execute(); );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING( outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName); );

    TS_ASSERT_EQUALS("Wavelength", outWS->getAxis(0)->unit()->unitID());

    //Because we have two transmission workspaces, binning should come from the Params for stitching.
    auto x = outWS->readX(0);
    auto actual_binning = x[1] - x[0];
    MantidVec outparams = alg->getProperty("Params");
    TS_ASSERT_DELTA(actual_binning, params[1], 0.0000001);
    TS_ASSERT_DELTA(1.5, params[0], 0.0000001);
    TS_ASSERT_DELTA(17, params[2], 0.0000001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(inWSName + "1");
    AnalysisDataService::Instance().remove(inWSName + "2");
  }

  void test_execute_two_tranmissions_with_minimal_property()
  {
    // Name of the output workspace.
    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->initialize();
    lAlg->setPropertyValue("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", inWSName + "1");
    lAlg->execute();
    MatrixWorkspace_sptr trans1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName + "1");
    lAlg->setPropertyValue("Filename", "INTER00013464.nxs");
    lAlg->setPropertyValue("OutputWorkspace", inWSName + "2");
    lAlg->execute();
    MatrixWorkspace_sptr trans2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inWSName + "2");

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    TS_ASSERT_THROWS_NOTHING( alg->initialize() );
    TS_ASSERT( alg->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("ProcessingInstructions", "3,4"); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("FirstTransmissionRun", trans1); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("SecondTransmissionRun", trans2); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("I0MonitorIndex", 0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthMin", 0.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthMax", 17.9); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("WavelengthStep", 0.5); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorBackgroundWavelengthMin", 15.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorBackgroundWavelengthMax", 17.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorIntegrationWavelengthMin", 4.0); );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("MonitorIntegrationWavelengthMax", 10.0); );
    MantidVec params = boost::assign::list_of(1.5)(0.02)(17).convert_to_container<MantidVec>();
    // TS_ASSERT_THROWS_NOTHING( alg->setProperty("Params", params); ); // DO NOT SPECIFY PARAMS
    //TS_ASSERT_THROWS_NOTHING( alg->setProperty("StartOverlap", 10.0); ); // DO NOT SPECIFY STARTOVERLAP
    //TS_ASSERT_THROWS_NOTHING( alg->setProperty("EndOverlap", 12.0); ); // DO NOT SPECIFY END OVERLAP
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", outWSName); );
    TS_ASSERT_THROWS_NOTHING( alg->execute(); );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING( outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName); );

    TS_ASSERT_EQUALS("Wavelength", outWS->getAxis(0)->unit()->unitID());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(inWSName + "1");
    AnalysisDataService::Instance().remove(inWSName + "2");
  }

};

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_ */
