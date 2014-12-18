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

  void test_check_input_workpace_not_tof_or_wavelength_throws()
  {
    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_NotTOF);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
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
    boost::split(pointDetectorStartStop, processingInstructions, boost::is_any_of(":"));

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
        boost::lexical_cast<double>(pointDetectorStartStop[1]));

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);

  }



};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_ */
