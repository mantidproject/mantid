#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/ReflectometryReductionOneAuto.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ReflectometryReductionOneAuto;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::MantidVec;
using Mantid::MantidVecPtr;
using Mantid::HistogramData::BinEdges;

namespace {
class PropertyFinder {
private:
  const std::string m_propertyName;

public:
  PropertyFinder(const std::string &propertyName)
      : m_propertyName(propertyName) {}
  bool operator()(const PropertyHistories::value_type &candidate) const {
    return candidate->name() == m_propertyName;
  }
};

template <typename T>
T findPropertyValue(PropertyHistories &histories,
                    const std::string &propertyName) {
  PropertyFinder finder(propertyName);
  auto it = std::find_if(histories.begin(), histories.end(), finder);
  return boost::lexical_cast<T>((*it)->value());
}
}

class ReflectometryReductionOneAutoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneAutoTest *createSuite() {
    return new ReflectometryReductionOneAutoTest();
  }
  static void destroySuite(ReflectometryReductionOneAutoTest *suite) {
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

  ReflectometryReductionOneAutoTest()
      : outWSQName("ReflectometryReductionOneAutoTest_OutputWS_Q"),
        outWSLamName("ReflectometryReductionOneAutoTest_OutputWS_Lam"),
        inWSName("ReflectometryReductionOneAutoTest_InputWS"),
        transWSName("ReflectometryReductionOneAutoTest_TransWS") {
    MantidVec xData = {0, 0, 0, 0};
    MantidVec yData = {0, 0, 0};

    auto createWorkspace =
        AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("UnitX", "1/q");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setPropertyValue("OutputWorkspace", "NotTOF");
    createWorkspace->execute();
    m_NotTOF =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("NotTOF");

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
    // m_dataWorkspace =
    // AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("data_ws");

    lAlg->setProperty("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "trans_ws_1");
    lAlg->execute();
    temp = lAlg->getProperty("OutputWorkspace");
    m_transWorkspace1 = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
    // m_transWorkspace1 =
    // AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("trans_ws_1");

    lAlg->setProperty("Filename", "INTER00013464.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "trans_ws_2");
    lAlg->execute();
    temp = lAlg->getProperty("OutputWorkspace");
    m_transWorkspace2 = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
    // m_transWorkspace2 =
    // AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("trans_ws_2");

    lAlg->setPropertyValue("Filename", "POLREF00004699.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "multidetector_ws_1");
    lAlg->execute();
    temp = lAlg->getProperty("OutputWorkspace");
    m_multiDetectorWorkspace =
        boost::dynamic_pointer_cast<WorkspaceGroup>(temp);
    AnalysisDataService::Instance().addOrReplace("multidetector_group",
                                                 m_multiDetectorWorkspace);
  }
  ~ReflectometryReductionOneAutoTest() override {
    AnalysisDataService::Instance().remove("TOF");
    AnalysisDataService::Instance().remove("NotTOF");
    AnalysisDataService::Instance().remove("multidetector_group");
    AnalysisDataService::Instance().remove("multidetector_group_1");
    AnalysisDataService::Instance().remove("multidetector_group_2");
  }

  IAlgorithm_sptr construct_standard_algorithm() {
    auto alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOneAuto", 1);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_TOF);
    alg->setProperty("WavelengthMin", 0.0);
    alg->setProperty("WavelengthMax", 1.0);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg->setProperty("MomentumTransferStep", 0.1);
    alg->setPropertyValue("ProcessingInstructions", "0");
    alg->setPropertyValue("OutputWorkspace", outWSQName);
    alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName);
    alg->setRethrows(true);
    return alg;
  }

  void test_Init() {
    ReflectometryReductionOneAuto alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_check_input_workpace_not_tof_or_wavelength_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("InputWorkspace", m_NotTOF);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_check_first_transmission_workspace_not_tof_or_wavelength_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_NotTOF);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_check_second_transmission_workspace_not_tof_throws() {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("SecondTransmissionRun", m_NotTOF),
                     std::invalid_argument);
  }

  void test_proivde_second_transmission_run_without_first_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("SecondTransmissionRun", m_TOF);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_end_overlap_must_be_greater_than_start_overlap_or_throw() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    MantidVec params = {0.0, 0.1, 1.0};
    alg->setProperty("Params", params);
    alg->setProperty("StartOverlap", 0.6);
    alg->setProperty("EndOverlap", 0.4);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_must_provide_wavelengths() {
    auto algWithMax =
        AlgorithmManager::Instance().create("ReflectometryReductionOneAuto", 1);
    algWithMax->initialize();
    algWithMax->setProperty("InputWorkspace", m_TOF);
    algWithMax->setProperty("FirstTransmissionRun", m_TOF);
    algWithMax->setProperty("SecondTransmissionRun", m_TOF);
    algWithMax->setProperty("ProcessingInstructions", "3:4");
    algWithMax->setProperty("MomentumTransferStep", 0.1);
    algWithMax->setProperty("WavelengthMax", 1.0);
    algWithMax->setPropertyValue("OutputWorkspace", "out_ws_Q");
    algWithMax->setPropertyValue("OutputWorkspaceWavelength", "out_ws_Lam");
    algWithMax->setRethrows(true);
    TS_ASSERT_THROWS(algWithMax->execute(), std::runtime_error);

    auto algWithMin =
        AlgorithmManager::Instance().create("ReflectometryReductionOneAuto", 1);
    algWithMin->initialize();
    algWithMin->setProperty("InputWorkspace", m_TOF);
    algWithMin->setProperty("FirstTransmissionRun", m_TOF);
    algWithMin->setProperty("SecondTransmissionRun", m_TOF);
    algWithMin->setProperty("ProcessingInstructions", "3:4");
    algWithMin->setProperty("MomentumTransferStep", 0.1);
    algWithMin->setProperty("WavelengthMin", 1.0);
    algWithMin->setPropertyValue("OutputWorkspace", "out_ws_Q");
    algWithMin->setPropertyValue("OutputWorkspaceWavelength", "out_ws_Lam");
    algWithMin->setRethrows(true);
    TS_ASSERT_THROWS(algWithMin->execute(), std::runtime_error);
  }

  void test_wavelength_min_greater_wavelength_max_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void
  test_monitor_background_wavelength_min_greater_monitor_background_wavelength_max_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("MonitorBackgroundWavelengthMin", 1.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void
  test_monitor_integration_wavelength_min_greater_monitor_integration_wavelength_max_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("MonitorIntegrationWavelengthMin", 1.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_monitor_index_positive() {
    auto alg = construct_standard_algorithm();
    auto tempInst = m_TOF->getInstrument();
    m_TOF->setInstrument(m_dataWorkspace->getInstrument());
    alg->setProperty("InputWorkspace", m_TOF);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    m_TOF->setInstrument(tempInst);
  }
  void
  test_cannot_set_direct_beam_region_of_interest_without_multidetector_run() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("AnalysisMode", "PointDetectorAnalysis");
    std::vector<int> RegionOfDirectBeam = {1, 2};
    alg->setProperty("RegionOfDirectBeam", RegionOfDirectBeam);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_region_of_direct_beam_indexes_cannot_be_negative_or_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("AnalysisMode", "MultiDetectorAnalysis");
    std::vector<int> RegionOfDirectBeam = {0, -1};
    alg->setProperty("RegionOfDirectBeam", RegionOfDirectBeam);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void
  test_region_of_direct_beam_indexes_must_be_provided_as_min_max_order_or_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("AnalysisMode", "MultiDetectorAnalysis");
    std::vector<int> RegionOfDirectBeam = {1, 0};
    alg->setProperty("RegionOfDirectBeam", RegionOfDirectBeam);
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_bad_detector_component_name_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("DetectorComponentName", "made-up");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_bad_sample_component_name_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("SampleComponentName", "made-up");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }
  void test_exec() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOneAuto", 1);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_dataWorkspace));
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("AnalysisMode", "PointDetectorAnalysis"));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", outWSQName));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("MomentumTransferStep", 0.1));
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSQName);

    auto inst = m_dataWorkspace->getInstrument();
    auto workspaceHistory = outWS->getHistory();
    AlgorithmHistory_const_sptr workerAlgHistory =
        workspaceHistory.getAlgorithmHistory(0)->getChildAlgorithmHistory(0);
    auto vecPropertyHistories = workerAlgHistory->getProperties();

    const double wavelengthMin =
        findPropertyValue<double>(vecPropertyHistories, "WavelengthMin");
    const double wavelengthMax =
        findPropertyValue<double>(vecPropertyHistories, "WavelengthMax");
    const double monitorBackgroundWavelengthMin = findPropertyValue<double>(
        vecPropertyHistories, "MonitorBackgroundWavelengthMin");
    const double monitorBackgroundWavelengthMax = findPropertyValue<double>(
        vecPropertyHistories, "MonitorBackgroundWavelengthMax");
    const double monitorIntegrationWavelengthMin = findPropertyValue<double>(
        vecPropertyHistories, "MonitorIntegrationWavelengthMin");
    const double monitorIntegrationWavelengthMax = findPropertyValue<double>(
        vecPropertyHistories, "MonitorIntegrationWavelengthMax");
    const int i0MonitorIndex =
        findPropertyValue<int>(vecPropertyHistories, "I0MonitorIndex");
    std::string processingInstructions = findPropertyValue<std::string>(
        vecPropertyHistories, "ProcessingInstructions");
    std::vector<std::string> pointDetectorStartStop;
    boost::split(pointDetectorStartStop, processingInstructions,
                 boost::is_any_of(":"));

    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMin")[0], wavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMax")[0], wavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMin")[0],
                     monitorBackgroundWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMax")[0],
                     monitorBackgroundWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMin")[0],
                     monitorIntegrationWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMax")[0],
                     monitorIntegrationWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("I0MonitorIndex")[0],
                     i0MonitorIndex);
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStart")[0],
                     boost::lexical_cast<double>(pointDetectorStartStop[0]));
    TS_ASSERT_EQUALS(pointDetectorStartStop.size(), 1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
  }

  void test_missing_instrument_parameters_throws() {
    auto tinyWS =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
    auto inst = tinyWS->getInstrument();

    inst->getParameterMap()->addDouble(inst.get(), "I0MonitorIndex", 1.0);

    tinyWS->mutableRun().addLogData(
        new PropertyWithValue<double>("Theta", 0.12345));
    tinyWS->mutableRun().addLogData(
        new PropertyWithValue<std::string>("run_number", "12345"));

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOneAuto", 1);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", tinyWS));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", outWSQName));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("MomentumTransferStep", 0.1));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
  }

  void test_normalize_by_detmon() {
    // Prepare workspace
    //-----------------
    // Single bin from 1->2
    BinEdges x{1, 2};

    // 2 spectra, 2 x values, 1 y value per spectra
    auto tinyWS = createWorkspace<Workspace2D>(2, 2, 1);
    tinyWS->setBinEdges(0, x);
    tinyWS->setBinEdges(1, x);
    tinyWS->setCounts(0, 1, 10.0);
    tinyWS->setCountStandardDeviations(0, 1, 0.0);
    tinyWS->setCounts(1, 1, 5.0);
    tinyWS->setCountStandardDeviations(1, 1, 0.0);

    tinyWS->setTitle("Test histogram");
    tinyWS->getAxis(0)->setUnit("Wavelength");
    tinyWS->setYUnit("Counts");

    // Prepare instrument
    //------------------
    Instrument_sptr instrument = boost::make_shared<Instrument>();
    instrument->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 0));
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *sample = new ObjComponent("some-surface-holder");
    source->setPos(V3D(15, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector *det = new Detector("point-detector", 1, NULL);
    det->setPos(20, (20 - sample->getPos().X()), 0);
    instrument->add(det);
    instrument->markAsDetector(det);

    Detector *monitor = new Detector("Monitor", 2, NULL);
    monitor->setPos(14, 0, 0);
    instrument->add(monitor);
    instrument->markAsMonitor(monitor);

    // Add instrument to workspace
    tinyWS->setInstrument(instrument);
    tinyWS->getSpectrum(0).addDetectorID(det->getID());
    tinyWS->getSpectrum(1).addDetectorID(monitor->getID());

    // Now we can parameterize the instrument
    auto tinyInst = tinyWS->getInstrument();
    ParameterMap_sptr params = tinyInst->getParameterMap();
    params->addDouble(tinyInst.get(), "PointDetectorStart", 0.0);
    params->addDouble(tinyInst.get(), "PointDetectorStop", 0.0);
    params->addDouble(tinyInst.get(), "I0MonitorIndex", 1.0);
    params->addDouble(tinyInst.get(), "LambdaMin", 0.0);
    params->addDouble(tinyInst.get(), "LambdaMax", 10.0);
    params->addDouble(tinyInst.get(), "MonitorBackgroundMin", 0.0);
    params->addDouble(tinyInst.get(), "MonitorBackgroundMax", 0.0);
    params->addDouble(tinyInst.get(), "MonitorIntegralMin", 0.0);
    params->addDouble(tinyInst.get(), "MonitorIntegralMax", 10.0);

    tinyWS->mutableRun().addLogData(
        new PropertyWithValue<double>("Theta", 0.1));

    // Run the required algorithms
    //---------------------------

    // Convert units
    IAlgorithm_sptr conv = AlgorithmManager::Instance().create("ConvertUnits");
    conv->initialize();
    conv->setProperty("InputWorkspace", tinyWS);
    conv->setProperty("OutputWorkspace", inWSName);
    conv->setProperty("Target", "TOF");
    conv->execute();
    TS_ASSERT(conv->isExecuted());

    // Reduce
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOneAuto", 1);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("NormalizeByIntegratedMonitors", false));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("MomentumTransferStep", 0.1));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", outWSQName));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName));
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    // Get results
    MatrixWorkspace_sptr outWSLam =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSLamName);

    // Check results (10 / 5 = 2)
    TS_ASSERT_DELTA(outWSLam->readY(0)[0], 2, 1e-5);
    TS_ASSERT_DELTA(outWSLam->readX(0)[0], 1, 1e-5);
    TS_ASSERT_DELTA(outWSLam->readX(0)[1], 2, 1e-5);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);
  }

  void test_i0_monitor_index_not_required() {
    // Prepare instrument
    //------------------
    // hold the current instrument assigned to m_dataWorkspace
    auto m_dataWorkspaceInstHolder = m_dataWorkspace->getInstrument();
    Instrument_sptr instrument = boost::make_shared<Instrument>();
    instrument->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 0));
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *sample = new ObjComponent("some-surface-holder");
    source->setPos(V3D(15, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector *det = new Detector("point-detector", 1, NULL);
    det->setPos(20, (20 - sample->getPos().X()), 0);
    instrument->add(det);
    instrument->markAsDetector(det);

    Detector *monitor = new Detector("Monitor", 2, NULL);
    monitor->setPos(14, 0, 0);
    instrument->add(monitor);
    instrument->markAsMonitor(monitor);

    // Add new instrument to workspace
    m_dataWorkspace->setInstrument(instrument);

    // Now we can parameterize the instrument
    // without setting the I0MonitorIndex
    auto tinyInst = m_dataWorkspace->getInstrument();
    ParameterMap_sptr params = tinyInst->getParameterMap();
    params->addDouble(tinyInst.get(), "PointDetectorStart", 0.0);
    params->addDouble(tinyInst.get(), "PointDetectorStop", 0.0);
    params->addDouble(tinyInst.get(), "LambdaMin", 0.0);
    params->addDouble(tinyInst.get(), "LambdaMax", 10.0);
    params->addDouble(tinyInst.get(), "MonitorBackgroundMin", 0.0);
    params->addDouble(tinyInst.get(), "MonitorBackgroundMax", 0.0);
    params->addDouble(tinyInst.get(), "MonitorIntegralMin", 0.0);
    params->addDouble(tinyInst.get(), "MonitorIntegralMax", 10.0);

    // Reduce
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOneAuto", 1);
    alg->setRethrows(true);
    alg->setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_dataWorkspace));
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("I0MonitorIndex", Mantid::EMPTY_INT()));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("MomentumTransferStep", 0.1));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", outWSQName));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspaceWavelength", outWSLamName));
    // we have intentionally not set
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSQName);
    AnalysisDataService::Instance().remove(outWSLamName);

    // reset the instrument associated with m_dataWorkspace
    m_dataWorkspace->setInstrument(m_dataWorkspaceInstHolder);
  }
  void test_point_detector_run_with_single_transmission_workspace() {
    ReflectometryReductionOneAuto alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 2.0);
    alg.setProperty("I0MonitorIndex", 0);
    alg.setProperty("ProcessingInstructions", "3,4");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg.setProperty("InputWorkspace", m_dataWorkspace);
    alg.setProperty("FirstTransmissionRun", m_transWorkspace1);
    alg.setProperty("ThetaIn", 0.2);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    double thetaOut = alg.getProperty("ThetaOut");
    TSM_ASSERT_EQUALS("Theta in and out should be the same", thetaOut, 0.2);

    MatrixWorkspace_sptr IvsLam = alg.getProperty("OutputWorkspaceWavelength");
    TSM_ASSERT("OutputWorkspaceWavelength should be a valid matrix workspace",
               IvsLam);

    MatrixWorkspace_sptr IvsQ = alg.getProperty("OutputWorkspace");
    TSM_ASSERT("OutputWorkspace should be a valid matrix workspace", IvsQ);

    TSM_ASSERT_EQUALS("OutputWorkspaceWavelength should have two histograms",
                      IvsLam->getNumberHistograms(), 2);
  }

  void test_point_detector_run_with_two_transmission_workspaces() {
    ReflectometryReductionOneAuto alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 2.0);
    alg.setProperty("I0MonitorIndex", 0);
    alg.setProperty("ProcessingInstructions", "3, 4");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg.setProperty("InputWorkspace", m_dataWorkspace);
    alg.setProperty("FirstTransmissionRun", m_transWorkspace1);
    alg.setProperty("SecondTransmissionRun", m_transWorkspace2);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_spectrum_map_mismatch_throws_when_strict() {
    ReflectometryReductionOneAuto alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 2.0);
    alg.setProperty("I0MonitorIndex", 0);
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");

    alg.setProperty("InputWorkspace", m_dataWorkspace);
    alg.execute();

    MatrixWorkspace_sptr trans = alg.getProperty("OutputWorkspaceWavelength");
    alg.setProperty("FirstTransmissionRun", trans);
    alg.setProperty("ProcessingInstructions", "4");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_spectrum_map_mismatch_doesnt_throw_when_not_strict() {
    ReflectometryReductionOneAuto alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 2.0);
    alg.setProperty("I0MonitorIndex", 0);
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");

    alg.setProperty("InputWorkspace", m_dataWorkspace);
    alg.execute();

    MatrixWorkspace_sptr trans = alg.getProperty("OutputWorkspaceWavelength");
    alg.setProperty("FirstTransmissionRun", trans);
    alg.setProperty("ProcessingInstructions", "4");
    alg.setProperty("StrictSpectrumChecking", "0");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_multidetector_run() {
    ReflectometryReductionOneAuto alg;
    alg.initialize();
    alg.setChild(false);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 2.0);
    alg.setProperty("I0MonitorIndex", 0);
    alg.setProperty("ProcessingInstructions", "10");
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectDetectorPositions", "0");
    alg.setProperty("DetectorComponentName", "lineardetector");
    alg.setProperty("RegionOfDirectBeam", "20, 30");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg.setProperty("ThetaIn", 0.1);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.setPropertyValue("InputWorkspace", "multidetector_group");

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr IvsLam_1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsLam_1");
    TSM_ASSERT("OutputWorkspaceWavelength should be a matrix workspace",
               IvsLam_1);
    TS_ASSERT_EQUALS("Wavelength", IvsLam_1->getAxis(0)->unit()->unitID());
    MatrixWorkspace_sptr IvsLam_2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsLam_2");
    TSM_ASSERT("OutputWorkspaceWavelength should be a matrix workspace",
               IvsLam_2);
    TS_ASSERT_EQUALS("Wavelength", IvsLam_2->getAxis(0)->unit()->unitID());

    MatrixWorkspace_sptr IvsQ_1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsQ_1");
    TSM_ASSERT("OutputWorkspace should be a matrix workspace", IvsQ_1);
    TS_ASSERT_EQUALS("MomentumTransfer", IvsQ_1->getAxis(0)->unit()->unitID());
    MatrixWorkspace_sptr IvsQ_2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsQ_2");
    TSM_ASSERT("OutputWorkspace should be a matrix workspace", IvsQ_2);
    TS_ASSERT_EQUALS("MomentumTransfer", IvsQ_2->getAxis(0)->unit()->unitID());

    MatrixWorkspace_sptr tempWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        m_multiDetectorWorkspace->getItem(0));
    auto instrBefore = tempWS->getInstrument();
    auto detectorBefore = instrBefore->getComponentByName("lineardetector");
    auto instrAfter = IvsLam_1->getInstrument();
    auto detectorAfter = instrAfter->getComponentByName("lineardetector");
    TS_ASSERT_DELTA(detectorBefore->getPos().Z(), detectorAfter->getPos().Z(),
                    0.0001)

    AnalysisDataService::Instance().remove("IvsLam");
    AnalysisDataService::Instance().remove("IvsLam_1");
    AnalysisDataService::Instance().remove("IvsLam_2");
    AnalysisDataService::Instance().remove("IvsQ");
    AnalysisDataService::Instance().remove("IvsQ_1");
    AnalysisDataService::Instance().remove("IvsQ_2");
  }

  void test_multidetector_run_correct_positions() {
    ReflectometryReductionOneAuto alg;
    alg.initialize();
    alg.setChild(false);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 2.0);
    alg.setProperty("I0MonitorIndex", 0);
    alg.setProperty("ProcessingInstructions", "73");
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectDetectorPositions", "1");
    alg.setProperty("DetectorComponentName", "lineardetector");
    alg.setProperty("RegionOfDirectBeam", "28, 29");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg.setProperty("ThetaIn", 0.49);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.setPropertyValue("InputWorkspace", "multidetector_group");

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr IvsLam_1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsLam_1");
    TSM_ASSERT("OutputWorkspaceWavelength should be a matrix workspace",
               IvsLam_1);
    TS_ASSERT_EQUALS("Wavelength", IvsLam_1->getAxis(0)->unit()->unitID());
    MatrixWorkspace_sptr IvsLam_2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsLam_2");
    TSM_ASSERT("OutputWorkspaceWavelength should be a matrix workspace",
               IvsLam_2);
    TS_ASSERT_EQUALS("Wavelength", IvsLam_2->getAxis(0)->unit()->unitID());

    MatrixWorkspace_sptr IvsQ_1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsQ_1");
    TSM_ASSERT("OutputWorkspace should be a matrix workspace", IvsQ_1);
    TS_ASSERT_EQUALS("MomentumTransfer", IvsQ_1->getAxis(0)->unit()->unitID());
    MatrixWorkspace_sptr IvsQ_2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsQ_2");
    TSM_ASSERT("OutputWorkspace should be a matrix workspace", IvsQ_2);
    TS_ASSERT_EQUALS("MomentumTransfer", IvsQ_2->getAxis(0)->unit()->unitID());

    auto instr = IvsLam_1->getInstrument();
    auto detectorPos = instr->getComponentByName("lineardetector");
    TS_ASSERT_DELTA(-0.05714, detectorPos->getPos().Z(), 0.0001)

    AnalysisDataService::Instance().remove("IvsLam");
    AnalysisDataService::Instance().remove("IvsLam_1");
    AnalysisDataService::Instance().remove("IvsLam_2");
    AnalysisDataService::Instance().remove("IvsQ");
    AnalysisDataService::Instance().remove("IvsQ_1");
    AnalysisDataService::Instance().remove("IvsQ_2");
  }
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTOTEST_H_ */
