#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryReductionOneAuto.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <boost/assign/list_of.hpp>

using Mantid::Algorithms::ReflectometryReductionOneAuto;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace boost::assign;
using Mantid::MantidVec;

namespace
{
  class PropertyFinder
  {
  private:
    const std::string m_propertyName;
  public:
    PropertyFinder(const std::string& propertyName) :
        m_propertyName(propertyName)
    {
    }
    bool operator()(const PropertyHistories::value_type& candidate) const
    {
      return candidate->name() == m_propertyName;
    }
  };

  template<typename T>
  T findPropertyValue(PropertyHistories& histories, const std::string& propertyName)
  {
    PropertyFinder finder(propertyName);
    auto it = std::find_if(histories.begin(), histories.end(), finder);
    return boost::lexical_cast<T>((*it)->value());
  }

}

class ReflectometryReductionOneAutoTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneAutoTest *createSuite()
  {
    return new ReflectometryReductionOneAutoTest();
  }
  static void destroySuite(ReflectometryReductionOneAutoTest *suite)
  {
    delete suite;
  }

  MatrixWorkspace_sptr m_TOF;
  MatrixWorkspace_sptr m_NotTOF;
  MatrixWorkspace_sptr m_dataWorkspace;
  MatrixWorkspace_sptr m_transWorkspace1;
  MatrixWorkspace_sptr m_transWorkspace2;
  WorkspaceGroup_sptr m_multiDetectorWorkspace;
  const std::string outWSQName;
  const std::string outWSLamName;
  const std::string inWSName;
  const std::string transWSName;

  ReflectometryReductionOneAutoTest() :
      outWSQName("ReflectometryReductionOneAutoTest_OutputWS_Q"), outWSLamName(
          "ReflectometryReductionOneAutoTest_OutputWS_Lam"), inWSName(
          "ReflectometryReductionOneAutoTest_InputWS"), transWSName(
          "ReflectometryReductionOneAutoTest_TransWS")
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

    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->setChild(true);
    lAlg->initialize();
    lAlg->setProperty("Filename", "INTER00013460.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "demo_ws");
    lAlg->execute();
    Workspace_sptr temp = lAlg->getProperty("OutputWorkspace");
    m_dataWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
    //m_dataWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("data_ws");

    lAlg->setProperty("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "trans_ws_1");
    lAlg->execute();
    temp = lAlg->getProperty("OutputWorkspace");
    m_transWorkspace1 = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
    //m_transWorkspace1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("trans_ws_1");

    lAlg->setProperty("Filename", "INTER00013464.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "trans_ws_2");
    lAlg->execute();
    temp = lAlg->getProperty("OutputWorkspace");
    m_transWorkspace2 = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
    //m_transWorkspace2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("trans_ws_2");

    lAlg->setPropertyValue("Filename", "POLREF00004699.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "multidetector_ws_1");
    lAlg->execute();
    temp = lAlg->getProperty("OutputWorkspace");
    m_multiDetectorWorkspace = boost::dynamic_pointer_cast<WorkspaceGroup>(temp);
    //m_multiDetectorWorkspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>( "multidetector_ws_1");
  }
  ~ReflectometryReductionOneAutoTest()
  {
    AnalysisDataService::Instance().remove("TOF");
    AnalysisDataService::Instance().remove("NotTOF");
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
    alg->setPropertyValue("OutputWorkspace", outWSQName);
    alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName);
    alg->setRethrows(true);
    return alg;
  }

  void test_Init()
  {
    ReflectometryReductionOneAuto alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize());
    TS_ASSERT( alg.isInitialized());
  }

  void test_exec()
  {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg->initialize());
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", m_dataWorkspace));
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("AnalysisMode", "PointDetectorAnalysis"));
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", outWSQName));
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName));
    alg->execute();
    TS_ASSERT( alg->isExecuted());

    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQName);

    auto inst = m_dataWorkspace->getInstrument();
    auto workspaceHistory = outWS->getHistory();
    AlgorithmHistory_const_sptr workerAlgHistory =
        workspaceHistory.getAlgorithmHistory(0)->getChildAlgorithmHistory(0);
    auto vecPropertyHistories = workerAlgHistory->getProperties();

    const double wavelengthMin = findPropertyValue<double>(vecPropertyHistories, "WavelengthMin");
    const double wavelengthMax = findPropertyValue<double>(vecPropertyHistories, "WavelengthMax");
    const double monitorBackgroundWavelengthMin = findPropertyValue<double>(vecPropertyHistories,
        "MonitorBackgroundWavelengthMin");
    const double monitorBackgroundWavelengthMax = findPropertyValue<double>(vecPropertyHistories,
        "MonitorBackgroundWavelengthMax");
    const double monitorIntegrationWavelengthMin = findPropertyValue<double>(vecPropertyHistories,
        "MonitorIntegrationWavelengthMin");
    const double monitorIntegrationWavelengthMax = findPropertyValue<double>(vecPropertyHistories,
        "MonitorIntegrationWavelengthMax");
    const int i0MonitorIndex = findPropertyValue<int>(vecPropertyHistories, "I0MonitorIndex");
    std::string processingInstructions = findPropertyValue<std::string>(vecPropertyHistories,
        "ProcessingInstructions");
    std::vector<std::string> pointDetectorStartStop;
    boost::split(pointDetectorStartStop, processingInstructions, boost::is_any_of(","));

    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMin")[0], wavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMax")[0], wavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMin")[0],
        monitorBackgroundWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMax")[0],
        monitorBackgroundWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMin")[0], monitorIntegrationWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMax")[0], monitorIntegrationWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("I0MonitorIndex")[0], i0MonitorIndex);
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStart")[0],
        boost::lexical_cast<double>(pointDetectorStartStop[0]));
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStop")[0],
        boost::lexical_cast<double>(pointDetectorStartStop.at(1)));

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);

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
    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
    alg->initialize();
    alg->setProperty("InputWorkspace", m_TOF);
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    alg->setProperty("WavelengthMax", 1.0);
    alg->setPropertyValue("OutputWorkspace", "out_ws_Q");
    alg->setPropertyValue("OutputWorkspaceWavelength", "out_ws_Lam");
    alg->setRethrows(true);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);

    alg->setProperty("InputWorkspace", m_TOF);
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setPropertyValue("OutputWorkspace", "out_ws_Q");
    alg->setPropertyValue("OutputWorkspaceWavelength", "out_ws_Lam");
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

  void test_cannot_set_direct_beam_region_of_interest_without_multidetector_run()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("AnalysisMode", "PointDetectorAnalysis");
    std::vector<int> RegionOfDirectBeam = boost::assign::list_of(1)(2).convert_to_container<
        std::vector<int> >();
    alg->setProperty("RegionOfDirectBeam", RegionOfDirectBeam);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_region_of_direct_beam_indexes_cannot_be_negative_or_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("AnalysisMode", "MultiDetectorAnalysis");
    std::vector<int> RegionOfDirectBeam = boost::assign::list_of(0)(-1).convert_to_container<
        std::vector<int> >();
    alg->setProperty("RegionOfDirectBeam", RegionOfDirectBeam);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_region_of_direct_beam_indexes_must_be_provided_as_min_max_order_or_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("AnalysisMode", "MultiDetectorAnalysis");
    std::vector<int> RegionOfDirectBeam = boost::assign::list_of(1)(0).convert_to_container<
        std::vector<int> >();
    alg->setProperty("RegionOfDirectBeam", RegionOfDirectBeam);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_bad_detector_component_name_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("DetectorComponentName", "made-up");
    TS_ASSERT_THROWS(alg->execute(), std::out_of_range);
  }

  void test_bad_sample_component_name_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("SampleComponentName", "made-up");
    TS_ASSERT_THROWS(alg->execute(), std::out_of_range);
  }

  void test_point_detector_run_with_single_transmission_workspace()
  {

    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_dataWorkspace);
    alg->setProperty("ProcessingInstructions", "3,4");
    alg->setProperty("FirstTransmissionRun", m_transWorkspace1);
    alg->setProperty("ThetaIn", 0.2); // Currently a requirement that one transmisson correction is provided.
    TS_ASSERT_THROWS_NOTHING(alg->execute(););

    MatrixWorkspace_sptr outWSlam;
    MatrixWorkspace_sptr outWSq;
    double outTheta = 0;
    TS_ASSERT_THROWS_NOTHING(
        outWSlam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSLamName););
    TS_ASSERT_THROWS_NOTHING(
        outWSq = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQName););
    TS_ASSERT_THROWS_NOTHING( outTheta = alg->getProperty("ThetaOut"););
    TSM_ASSERT_EQUALS("Theta in and out should be the same", 0.2, outTheta);
    TS_ASSERT_EQUALS("Wavelength", outWSlam->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS("MomentumTransfer", outWSq->getAxis(0)->unit()->unitID());

    TS_ASSERT_EQUALS(2, outWSlam->getNumberHistograms());
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
  }

  void test_point_detector_run_with_two_transmission_workspaces()
  {
    auto alg = construct_standard_algorithm();

    alg->setProperty("InputWorkspace", m_dataWorkspace);
    alg->setProperty("ProcessingInstructions", "3,4");
    alg->setProperty("FirstTransmissionRun", m_transWorkspace1);
    alg->setProperty("SecondTransmissionRun", m_transWorkspace2);
    alg->setProperty("ThetaIn", 0.2); // Currently a requirement that one transmisson correction is provided.
    MantidVec params = boost::assign::list_of(0.0)(0.02)(5).convert_to_container<MantidVec>();
    TS_ASSERT_THROWS_NOTHING(alg->execute(););

    MatrixWorkspace_sptr outWSlam;
    MatrixWorkspace_sptr outWSq;
    double outTheta = 0;
    TS_ASSERT_THROWS_NOTHING(
        outWSlam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSLamName););
    TS_ASSERT_THROWS_NOTHING(
        outWSq = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQName););
    TS_ASSERT_THROWS_NOTHING( outTheta = alg->getProperty("ThetaOut"););
    TS_ASSERT_DELTA(outTheta, 0.2, 0.0000001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
  }

  void test_spectrum_map_mismatch_throws_when_strict()
  {
    /*
     Here we convert the transmission run to Lam. The workspace will NOT have the same spectra map as the input workspace,
     and strict checking is turned on, so this will throw upon execution.
     */
    IAlgorithm_sptr convAlg = AlgorithmManager::Instance().create("ConvertUnits");
    convAlg->setProperty("InputWorkspace", m_transWorkspace1);
    convAlg->setProperty("Target", "Wavelength");
    convAlg->setProperty("OutputWorkspace", transWSName + "Lam");
    convAlg->execute();
    MatrixWorkspace_sptr trans_run1_lam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        transWSName + "Lam");

    IAlgorithm_sptr cropAlg = AlgorithmManager::Instance().create("CropWorkspace");
    cropAlg->setProperty("InputWorkspace", trans_run1_lam);
    cropAlg->setProperty("EndWorkspaceIndex", 1);
    cropAlg->setProperty("OutputWorkspace", transWSName + "Lam");
    cropAlg->execute();
    trans_run1_lam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(transWSName + "Lam");

    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_dataWorkspace);
    alg->setProperty("ProcessingInstructions", "3,4"); // This will make spectrum numbers in input workspace different from denominator
    alg->setProperty("FirstTransmissionRun", trans_run1_lam);
    alg->setProperty("StrictSpectrumChecking", true); //Will not crash-out on spectrum checking.

    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument&);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(transWSName + "Lam");
  }

  void test_spectrum_map_mismatch_doesnt_throw_when_not_strict()
  {

    /*
     Here we convert the transmission run to Lam. The workspace will NOT have the same spectra map as the input workspace,
     and strict checking is turned on, so this will throw upon execution.
     */
    IAlgorithm_sptr convAlg = AlgorithmManager::Instance().create("ConvertUnits");
    convAlg->setProperty("InputWorkspace", m_transWorkspace1);
    convAlg->setProperty("Target", "Wavelength");
    convAlg->setProperty("OutputWorkspace", transWSName + "Lam");
    convAlg->execute();
    MatrixWorkspace_sptr trans_run1_lam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        transWSName + "Lam");

    IAlgorithm_sptr cropAlg = AlgorithmManager::Instance().create("CropWorkspace");
    cropAlg->setProperty("InputWorkspace", trans_run1_lam);
    cropAlg->setProperty("EndWorkspaceIndex", 1);
    cropAlg->setProperty("OutputWorkspace", transWSName + "Lam");
    cropAlg->execute();
    trans_run1_lam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(transWSName + "Lam");

    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_dataWorkspace);
    alg->setProperty("ProcessingInstructions", "3,4"); // This will make spectrum numbers in input workspace different from denominator
    alg->setProperty("FirstTransmissionRun", trans_run1_lam);
    alg->setProperty("StrictSpectrumChecking", false); //Will not crash-out on spectrum checking.

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    // Should not throw

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(transWSName + "Lam");
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
  }

  void test_calculate_theta()
  {

    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_dataWorkspace);
    alg->setProperty("ProcessingInstructions", "3,4");
    alg->setProperty("FirstTransmissionRun", m_transWorkspace1); // Currently a requirement that one transmisson correction is provided.

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    // Should not throw

    MatrixWorkspace_sptr outWSlam;
    MatrixWorkspace_sptr outWSq;
    double outTheta = 0;
    TS_ASSERT_THROWS_NOTHING(
        outWSlam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSLamName););
    TS_ASSERT_THROWS_NOTHING(
        outWSq = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQName););
    TS_ASSERT_THROWS_NOTHING( outTheta = alg->getProperty("ThetaOut"););

    TS_ASSERT_DELTA(0.70969419, outTheta, 0.00001);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
  }

  void test_correct_positions_point_detector()
  {

    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_dataWorkspace);
    alg->setProperty("ProcessingInstructions", "3,4");
    alg->setProperty("ThetaIn", 0.4); //Low angle
    alg->setProperty("CorrectDetectorPositions", true);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    // Should not throw

    MatrixWorkspace_sptr outWSlam1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        outWSQName);
    MatrixWorkspace_sptr outWSq1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        outWSQName);
    const double outTheta1 = alg->getProperty("ThetaOut");

    TS_ASSERT_DELTA(outTheta1, 0.4, 0.0000001);

    auto pos1 = outWSlam1->getInstrument()->getComponentByName("point-detector")->getPos();

    alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_dataWorkspace);
    alg->setProperty("ProcessingInstructions", "3,4");
    alg->setProperty("ThetaIn", 0.8); // Repeat with greater incident angle
    alg->setPropertyValue("OutputWorkspace", outWSQName + "2");
    alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName + "2");
    alg->setProperty("CorrectDetectorPositions", true);
    alg->execute();

    MatrixWorkspace_sptr outWSlam2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        outWSQName + "2");
    MatrixWorkspace_sptr outWSq2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        outWSQName + "2");
    double outTheta2 = alg->getProperty("ThetaOut");
    TS_ASSERT_DELTA(outTheta2, 0.8, 0.0000001);

    auto pos2 = outWSlam2->getInstrument()->getComponentByName("point-detector")->getPos();

    TSM_ASSERT_LESS_THAN("Greater incident angle so greater height.", pos1.Y(), pos2.Y());
    TS_ASSERT_EQUALS(pos2.X(), pos1.X());
    TS_ASSERT_EQUALS(pos2.Z(), pos1.Z());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
    AnalysisDataService::Instance().remove(outWSQName + "2");
    AnalysisDataService::Instance().remove(outWSLamName + "2");
  }

  void test_multidetector_run()
  {
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        m_multiDetectorWorkspace->getItem(0));

    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ProcessingInstructions", "3,10"); //Fictional values
    alg->setProperty("RegionOfDirectBeam", "20, 30"); //Fictional values
    alg->setProperty("ThetaIn", 0.1); //Fictional values
    alg->setProperty("CorrectDetectorPositions", false);
    alg->setProperty("AnalysisMode", "MultiDetectorAnalysis");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    // Should not throw

    MatrixWorkspace_sptr outWSlam;
    MatrixWorkspace_sptr outWSq;
    double outTheta = 0;
    TS_ASSERT_THROWS_NOTHING(
        outWSlam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSLamName););
    TS_ASSERT_THROWS_NOTHING(
        outWSq = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQName););
    TS_ASSERT_THROWS_NOTHING( outTheta = alg->getProperty("ThetaOut"););
    TS_ASSERT_DELTA(outTheta, 0.1, 0.0000001);

    TS_ASSERT_EQUALS("Wavelength", outWSlam->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS("MomentumTransfer", outWSq->getAxis(0)->unit()->unitID());
  }

  void test_correct_positions_multi_detector()
  {
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        m_multiDetectorWorkspace->getItem(0));

    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("ProcessingInstructions", "73"); //Fictional values
    alg->setProperty("RegionOfDirectBeam", "28, 29"); //Fictional values
    alg->setProperty("ThetaIn", 0.49 / 2); //Fictional values
    alg->setProperty("CorrectDetectorPositions", true);
    alg->setProperty("AnalysisMode", "MultiDetectorAnalysis");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    // Should not throw

    MatrixWorkspace_sptr outWSlam;
    MatrixWorkspace_sptr outWSq;
    double outTheta = 0;
    TS_ASSERT_THROWS_NOTHING(
        outWSlam = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSLamName););
    TS_ASSERT_THROWS_NOTHING(
        outWSq = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQName););
    TS_ASSERT_THROWS_NOTHING( outTheta = alg->getProperty("ThetaOut"););
    TS_ASSERT_DELTA(outTheta, 0.49/2, 0.0000001);

    auto pos = outWSlam->getInstrument()->getComponentByName("lineardetector")->getPos();

    TS_ASSERT_DELTA(-0.05714, pos.Z(), 0.0001);
  }

};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_ */
