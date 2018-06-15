#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2TEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/ReflectometryReductionOneAuto2.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ReflectometryReductionOneAuto2;
using namespace Mantid::API;

class ReflectometryReductionOneAuto2Test : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_notTOF;
  MatrixWorkspace_sptr m_TOF;

  MatrixWorkspace_sptr loadRun(const std::string &run) {

    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->setChild(true);
    lAlg->initialize();
    lAlg->setProperty("Filename", run);
    lAlg->setPropertyValue("OutputWorkspace", "demo_ws");
    lAlg->execute();
    Workspace_sptr temp = lAlg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr matrixWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
    if (matrixWS)
      return matrixWS;

    WorkspaceGroup_sptr group =
        boost::dynamic_pointer_cast<WorkspaceGroup>(temp);
    if (group) {
      Workspace_sptr temp = group->getItem(0);
      MatrixWorkspace_sptr matrixWS =
          boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
      if (matrixWS)
        return matrixWS;
    }

    return MatrixWorkspace_sptr();
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneAuto2Test *createSuite() {
    return new ReflectometryReductionOneAuto2Test();
  }
  static void destroySuite(ReflectometryReductionOneAuto2Test *suite) {
    delete suite;
  }

  ReflectometryReductionOneAuto2Test() {
    FrameworkManager::Instance();

    m_notTOF =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            1, 10, 10);
    m_TOF = WorkspaceCreationHelper::
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
  }

  ~ReflectometryReductionOneAuto2Test() override {}

  void test_init() {
    ReflectometryReductionOneAuto2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_bad_input_workspace_units() {
    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_notTOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "0");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_wavelength_range() {
    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 15.0);
    alg.setProperty("WavelengthMax", 1.0);
    alg.setProperty("ProcessingInstructions", "0");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_background_range() {
    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 3.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 0.5);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_integration_range() {
    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "0");
    alg.setProperty("MonitorIntegrationWavelengthMin", 15.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.5);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_first_transmission_run_units() {
    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("FirstTransmissionRun", m_notTOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "0");
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_second_transmission_run_units() {
    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("FirstTransmissionRun", m_TOF);
    TS_ASSERT_THROWS_ANYTHING(
        alg.setProperty("SecondTransmissionRun", m_notTOF));
  }

  void test_bad_first_transmission_group_size() {
    MatrixWorkspace_sptr first = m_TOF->clone();
    MatrixWorkspace_sptr second = m_TOF->clone();
    MatrixWorkspace_sptr third = m_TOF->clone();
    MatrixWorkspace_sptr fourth = m_TOF->clone();

    WorkspaceGroup_sptr inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(first);
    inputWSGroup->addWorkspace(second);
    WorkspaceGroup_sptr transWSGroup = boost::make_shared<WorkspaceGroup>();
    transWSGroup->addWorkspace(first);
    transWSGroup->addWorkspace(second);
    transWSGroup->addWorkspace(third);
    transWSGroup->addWorkspace(fourth);
    AnalysisDataService::Instance().addOrReplace("input", inputWSGroup);
    AnalysisDataService::Instance().addOrReplace("trans", transWSGroup);

    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "input");
    alg.setPropertyValue("FirstTransmissionRun", "trans");
    alg.setPropertyValue("PolarizationAnalysis", "None");
    auto results = alg.validateInputs();
    TS_ASSERT(results.count("FirstTransmissionRun"));

    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove("input_1");
    AnalysisDataService::Instance().remove("input_2");
    AnalysisDataService::Instance().remove("trans");
    AnalysisDataService::Instance().remove("trans_3");
    AnalysisDataService::Instance().remove("trans_4");
  }

  void test_bad_second_transmission_group_size() {
    MatrixWorkspace_sptr first = m_TOF->clone();
    MatrixWorkspace_sptr second = m_TOF->clone();
    MatrixWorkspace_sptr third = m_TOF->clone();
    MatrixWorkspace_sptr fourth = m_TOF->clone();

    WorkspaceGroup_sptr inputWSGroup = boost::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(first);
    WorkspaceGroup_sptr firstWSGroup = boost::make_shared<WorkspaceGroup>();
    firstWSGroup->addWorkspace(second);
    WorkspaceGroup_sptr secondWSGroup = boost::make_shared<WorkspaceGroup>();
    secondWSGroup->addWorkspace(third);
    secondWSGroup->addWorkspace(fourth);
    AnalysisDataService::Instance().addOrReplace("input", inputWSGroup);
    AnalysisDataService::Instance().addOrReplace("first_trans", firstWSGroup);
    AnalysisDataService::Instance().addOrReplace("second_trans", secondWSGroup);

    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "input");
    alg.setPropertyValue("FirstTransmissionRun", "first_trans");
    alg.setPropertyValue("SecondTransmissionRun", "second_trans");
    alg.setPropertyValue("PolarizationAnalysis", "None");
    const auto results = alg.validateInputs();
    TS_ASSERT(!results.count("FirstTransmissionRun"));
    TS_ASSERT(results.count("SecondTransmissionRun"));

    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove("input_1");
    AnalysisDataService::Instance().remove("first_trans");
    AnalysisDataService::Instance().remove("first_trans_1");
    AnalysisDataService::Instance().remove("second_trans");
    AnalysisDataService::Instance().remove("second_trans_1");
    AnalysisDataService::Instance().remove("second_trans_2");
  }

  void test_correct_detector_position_INTER() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.setProperty("ProcessingInstructions", "3");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspaceBinned");

    // Check default rebin params
    const double qStep = alg.getProperty("MomentumTransferStep");
    const double qMin = alg.getProperty("MomentumTransferMin");
    const double qMax = alg.getProperty("MomentumTransferMax");
    TS_ASSERT_DELTA(qStep, 0.034028, 1e-6);
    TS_ASSERT_DELTA(qMin, out->x(0).front(), 1e-6);
    TS_ASSERT_DELTA(qMax, out->x(0).back(), 1e-6);

    // Compare instrument components before and after
    auto instIn = inter->getInstrument();
    auto instOut = out->getInstrument();

    // The following components should not have been moved
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor1")->getPos(),
                     instOut->getComponentByName("monitor1")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor2")->getPos(),
                     instOut->getComponentByName("monitor2")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor3")->getPos(),
                     instOut->getComponentByName("monitor3")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("linear-detector")->getPos(),
                     instOut->getComponentByName("linear-detector")->getPos());

    // Only 'point-detector' should have been moved vertically (along Y)

    auto point1In = instIn->getComponentByName("point-detector")->getPos();
    auto point1Out = instOut->getComponentByName("point-detector")->getPos();

    TS_ASSERT_EQUALS(point1In.X(), point1Out.X());
    TS_ASSERT_EQUALS(point1In.Z(), point1Out.Z());
    TS_ASSERT_DIFFERS(point1In.Y(), point1Out.Y());
    TS_ASSERT_DELTA(point1Out.Y() /
                        (point1Out.Z() - instOut->getSample()->getPos().Z()),
                    std::tan(theta * 2 * M_PI / 180), 1e-4);
  }

  void test_correct_detector_position_rotation_POLREF() {
    // Histograms in this run correspond to 'OSMOND' component
    auto polref = loadRun("POLREF00014966.raw");

    // Correct by rotating detectors around the sample
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", polref);
    alg.setProperty("ThetaIn", 1.5);
    alg.setProperty("DetectorCorrectionType", "RotateAroundSample");
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("MomentumTransferStep", 0.01);
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");

    // Compare instrument components before and after
    auto instIn = polref->getInstrument();
    auto instOut = out->getInstrument();

    // The following components should not have been moved
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor1")->getPos(),
                     instOut->getComponentByName("monitor1")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor2")->getPos(),
                     instOut->getComponentByName("monitor2")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor3")->getPos(),
                     instOut->getComponentByName("monitor3")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("point-detector")->getPos(),
                     instOut->getComponentByName("point-detector")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("lineardetector")->getPos(),
                     instOut->getComponentByName("lineardetector")->getPos());

    // Only 'OSMOND' should have been moved both vertically and in the beam
    // direction (along X and Z)

    auto detectorIn = instIn->getComponentByName("OSMOND")->getPos();
    auto detectorOut = instOut->getComponentByName("OSMOND")->getPos();

    TS_ASSERT_DELTA(detectorOut.X(), 25.99589, 1e-5);
    TS_ASSERT_EQUALS(detectorIn.Y(), detectorOut.Y());
    TS_ASSERT_DELTA(detectorOut.Z(), 0.1570, 1e-5);
  }

  void test_correct_detector_position_vertical_CRISP() {
    // Histogram in this run corresponds to 'point-detector' component
    auto polref = loadRun("CSP79590.raw");

    // Correct by shifting detectors vertically
    // Also explicitly pass CorrectDetectors=1
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", polref);
    alg.setProperty("ThetaIn", 0.25);
    alg.setProperty("CorrectDetectors", "1");
    alg.setProperty("DetectorCorrectionType", "VerticalShift");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("MomentumTransferStep", 0.01);
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");

    // Compare instrument components before and after
    auto instIn = polref->getInstrument();
    auto instOut = out->getInstrument();

    // The following components should not have been moved
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor1")->getPos(),
                     instOut->getComponentByName("monitor1")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor2")->getPos(),
                     instOut->getComponentByName("monitor2")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("linear-detector")->getPos(),
                     instOut->getComponentByName("linear-detector")->getPos());

    // Only 'point-detector' should have been moved vertically (along Y)

    auto detectorIn = instIn->getComponentByName("point-detector")->getPos();
    auto detectorOut = instOut->getComponentByName("point-detector")->getPos();

    TS_ASSERT_EQUALS(detectorIn.X(), detectorOut.X());
    TS_ASSERT_EQUALS(detectorIn.Z(), detectorOut.Z());
    TS_ASSERT_DELTA(detectorOut.Y() /
                        (detectorOut.Z() - instOut->getSample()->getPos().Z()),
                    std::tan(0.25 * 2 * M_PI / 180), 1e-4);
  }

  void test_correct_detector_position_from_logs() {
    auto inter = loadRun("INTER00013460.nxs");
    double theta = 0.7;

    // Use theta from the logs to correct detector positions
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaLogName", "theta");
    alg.setProperty("CorrectDetectors", "1");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.setProperty("ProcessingInstructions", "3");
    alg.execute();
    MatrixWorkspace_sptr corrected = alg.getProperty("OutputWorkspace");

    // Compare instrument components before and after
    auto instIn = inter->getInstrument();
    auto instOut = corrected->getInstrument();

    // The following components should not have been moved
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor1")->getPos(),
                     instOut->getComponentByName("monitor1")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor2")->getPos(),
                     instOut->getComponentByName("monitor2")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("monitor3")->getPos(),
                     instOut->getComponentByName("monitor3")->getPos());
    TS_ASSERT_EQUALS(instIn->getComponentByName("linear-detector")->getPos(),
                     instOut->getComponentByName("linear-detector")->getPos());

    // Only 'point-detector' should have been moved
    // vertically (along Y)

    auto point1In = instIn->getComponentByName("point-detector")->getPos();
    auto point1Out = instOut->getComponentByName("point-detector")->getPos();

    TS_ASSERT_EQUALS(point1In.X(), point1Out.X());
    TS_ASSERT_EQUALS(point1In.Z(), point1Out.Z());
    TS_ASSERT_DIFFERS(point1In.Y(), point1Out.Y());
    TS_ASSERT_DELTA(point1Out.Y() /
                        (point1Out.Z() - instOut->getSample()->getPos().Z()),
                    std::tan(theta * 2 * M_PI / 180), 1e-4);
  }

  void test_override_ThetaIn_without_correcting_detectors() {
    auto inter = loadRun("INTER00013460.nxs");

    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("CorrectDetectors", "0");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.setProperty("ProcessingInstructions", "3");
    alg.execute();
    MatrixWorkspace_sptr corrected = alg.getProperty("OutputWorkspace");

    // Compare instrument components before and after
    auto instIn = inter->getInstrument();
    auto instOut = corrected->getInstrument();

    // the detectors should not have been moved

    auto point1In = instIn->getComponentByName("point-detector")->getPos();
    auto point1Out = instOut->getComponentByName("point-detector")->getPos();

    TS_ASSERT_EQUALS(point1In, point1Out);
  }

  void test_sum_transmission_workspaces() {
    MatrixWorkspace_sptr first = m_TOF->clone();
    MatrixWorkspace_sptr second = m_TOF->clone();
    MatrixWorkspace_sptr third = m_TOF->clone();
    MatrixWorkspace_sptr fourth = m_TOF->clone();

    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(first);
    group->addWorkspace(second);
    group->addWorkspace(third);
    group->addWorkspace(fourth);

    ReflectometryReductionOneAuto2 alg;
    auto sum = alg.sumTransmissionWorkspaces(group);

    // Input workspaces remain the same
    TS_ASSERT_EQUALS(first->blocksize(), 20);
    TS_ASSERT_EQUALS(second->blocksize(), 20);
    TS_ASSERT_EQUALS(third->blocksize(), 20);
    TS_ASSERT_EQUALS(fourth->blocksize(), 20);
    TS_ASSERT_EQUALS(first->y(0)[0], 2);
    TS_ASSERT_EQUALS(second->y(0)[0], 2);
    TS_ASSERT_EQUALS(third->y(0)[0], 2);
    TS_ASSERT_EQUALS(fourth->y(0)[0], 2);

    // Output workspace
    TS_ASSERT_EQUALS(sum->blocksize(), 20);
    TS_ASSERT_DELTA(sum->y(0)[0], 4 * 2, 1e-6);
  }

  void test_IvsQ_linear_binning() {

    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("MomentumTransferMin", 1.0);
    alg.setProperty("MomentumTransferMax", 10.0);
    alg.setProperty("MomentumTransferStep", -0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQbinned = alg.getProperty("OutputWorkspaceBinned");

    // Check the rebin params have not changed
    const double qStep = alg.getProperty("MomentumTransferStep");
    const double qMin = alg.getProperty("MomentumTransferMin");
    const double qMax = alg.getProperty("MomentumTransferMax");
    TS_ASSERT_EQUALS(qStep, -0.04);
    TS_ASSERT_EQUALS(qMin, 1.0);
    TS_ASSERT_EQUALS(qMax, 10.0);

    TS_ASSERT_EQUALS(outQbinned->getNumberHistograms(), 1);
    // blocksize = (10.0 - 1.0) / 0.04
    TS_ASSERT_EQUALS(outQbinned->blocksize(), 225);
    TS_ASSERT_DELTA(outQbinned->x(0)[1] - outQbinned->x(0)[0], 0.04, 1e-6);
    TS_ASSERT_DELTA(outQbinned->x(0)[2] - outQbinned->x(0)[1], 0.04, 1e-6);
  }

  void test_IvsQ_logarithmic_binning() {

    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("MomentumTransferMin", 1.0);
    alg.setProperty("MomentumTransferMax", 10.0);
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQbinned = alg.getProperty("OutputWorkspaceBinned");

    TS_ASSERT_EQUALS(outQbinned->getNumberHistograms(), 1);
    TS_ASSERT_DIFFERS(outQbinned->blocksize(), 8);
    TS_ASSERT_DELTA(outQbinned->x(0)[1] - outQbinned->x(0)[0], 0.04, 1e-6);
    TS_ASSERT(outQbinned->x(0)[7] - outQbinned->x(0)[6] > 0.05);
  }

  void test_IvsQ_q_range() {

    ReflectometryReductionOneAuto2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outQ->blocksize(), 14);
    // X range in outLam
    TS_ASSERT_DELTA(outLam->x(0)[0], 1.7924, 0.0001);
    TS_ASSERT_DELTA(outLam->x(0)[7], 8.0658, 0.0001);
    // X range in outQ
    TS_ASSERT_DELTA(outQ->x(0)[0], 0.3353, 0.0001);
    TS_ASSERT_DELTA(outQ->x(0)[7], 0.5962, 0.0001);
  }

  void test_optional_outputs() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_optional_outputs_set() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_default_outputs_debug() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("Debug", true);
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_13460"));

    AnalysisDataService::Instance().clear();
  }

  void test_default_outputs_no_debug() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("Debug", false);
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned_13460"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsQ_13460"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam_13460"));

    AnalysisDataService::Instance().clear();
  }

  void test_default_outputs_no_run_number() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;
    inter->mutableRun().removeProperty("run_number");

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("Debug", true);
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_default_outputs_no_run_number_no_debug() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;
    inter->mutableRun().removeProperty("run_number");

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("Debug", false);
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2TEST_H_ */
