// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO3TEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO3TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryReductionOneAuto3.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/ReflectometryHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::TestHelpers;
using namespace WorkspaceCreationHelper;

class ReflectometryReductionOneAuto3Test : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_notTOF;
  MatrixWorkspace_sptr m_TOF;
  AnalysisDataServiceImpl &ADS = AnalysisDataService::Instance();

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

  void momentumTransferHelper(ReflectometryReductionOneAuto3 &alg,
                              MatrixWorkspace_sptr &inter,
                              const double &theta) {
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
    alg.setProperty("Debug", false);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneAuto3Test *createSuite() {
    return new ReflectometryReductionOneAuto3Test();
  }
  static void destroySuite(ReflectometryReductionOneAuto3Test *suite) {
    delete suite;
  }

  ReflectometryReductionOneAuto3Test() {
    FrameworkManager::Instance();

    m_notTOF =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            1, 10, 10);
    m_TOF = WorkspaceCreationHelper::
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
  }

  ~ReflectometryReductionOneAuto3Test() override {}

  void test_init() {
    ReflectometryReductionOneAuto3 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_bad_input_workspace_units() {
    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_notTOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_wavelength_range() {
    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 15.0);
    alg.setProperty("WavelengthMax", 1.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_background_range() {
    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("MonitorBackgroundWavelengthMin", 3.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 0.5);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_integration_range() {
    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("MonitorIntegrationWavelengthMin", 15.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 1.5);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_first_transmission_run_units() {
    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("FirstTransmissionRun", m_notTOF);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_second_transmission_run_units() {
    ReflectometryReductionOneAuto3 alg;
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

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "input");
    alg.setPropertyValue("FirstTransmissionRun", "trans");
    alg.setProperty("PolarizationAnalysis", false);
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

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "input");
    alg.setPropertyValue("FirstTransmissionRun", "first_trans");
    alg.setPropertyValue("SecondTransmissionRun", "second_trans");
    alg.setProperty("PolarizationAnalysis", false);
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
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.setProperty("ProcessingInstructions", "4");
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
    ReflectometryReductionOneAuto3 alg;
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
    ReflectometryReductionOneAuto3 alg;
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
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaLogName", "theta");
    alg.setProperty("CorrectDetectors", "1");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.setProperty("ProcessingInstructions", "4");
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

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("CorrectDetectors", "0");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.setProperty("ProcessingInstructions", "4");
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

  void test_IvsQ_linear_binning() {

    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
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

    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
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

  void test_IvsLam_range() {

    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outQ->binEdges(0).size(), 15);
    // X range in outLam
    TS_ASSERT_DELTA(outLam->binEdges(0)[0], 1.7924, 0.0001);
    TS_ASSERT_DELTA(outLam->binEdges(0)[1], 2.6886, 0.0001);
    TS_ASSERT_DELTA(outLam->binEdges(0)[7], 8.0658, 0.0001);
    TS_ASSERT_DELTA(outLam->binEdges(0)[13], 13.4431, 0.0001);
    TS_ASSERT_DELTA(outLam->binEdges(0)[14], 14.3393, 0.0001);
  }

  void test_IvsQ_range() {

    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outQ->binEdges(0).size(), 15);
    // X range in outLam
    TS_ASSERT_DELTA(outLam->binEdges(0)[0], 1.7924, 0.0001);
    TS_ASSERT_DELTA(outLam->binEdges(0)[7], 8.0658, 0.0001);
    // X range in outQ
    TS_ASSERT_DELTA(outQ->binEdges(0)[0], 0.3353, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[1], 0.3577, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[6], 0.5366, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[7], 0.5962, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[12], 1.3415, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[13], 1.7886, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[14], 2.6830, 0.0001);
  }

  void test_IvsQ_range_cropped() {

    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("MomentumTransferMin", 0.5);
    alg.setProperty("MomentumTransferMax", 1.5);
    alg.setProperty("ProcessingInstructions", "3");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), 1);
    // X range in outQ is cropped to momentum transfer limits
    TS_ASSERT_EQUALS(outQ->binEdges(0).size(), 7);
    TS_ASSERT_DELTA(outQ->binEdges(0)[0], 0.5366, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[1], 0.5962, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[5], 1.0732, 0.0001);
    TS_ASSERT_DELTA(outQ->binEdges(0)[6], 1.3414, 0.0001);
  }

  void test_optional_outputs() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam_13460"));

    AnalysisDataService::Instance().clear();
  }

  void test_optional_outputs_binned() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsQ_binned_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam_13460"));

    AnalysisDataService::Instance().clear();
  }

  void test_optional_outputs_set() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
    alg.setProperty("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_default_outputs_debug() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
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
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
    alg.setProperty("Debug", false);
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned_13460"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_13460"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam_13460"));

    AnalysisDataService::Instance().clear();
  }

  void test_default_outputs_no_run_number() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;
    inter->mutableRun().removeProperty("run_number");

    // Use the default correction type, which is a vertical shift
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
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
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", theta);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("ProcessingInstructions", "4");
    alg.setProperty("Debug", false);
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_binned"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_polarization_correction() {

    std::string const name = "input";
    prepareInputGroup(name, "Fredrikze");
    applyPolarizationEfficiencies(name);

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setProperty("PolarizationAnalysis", true);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 0.9, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[1]->y(0)[0], 0.8, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[2]->y(0)[0], 0.7, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[3]->y(0)[0], 0.6, 0.0001);

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 0.9, 0.0001);
    TS_ASSERT_DELTA(outQGroup[1]->y(0)[0], 0.8, 0.0001);
    TS_ASSERT_DELTA(outQGroup[2]->y(0)[0], 0.7, 0.0001);
    TS_ASSERT_DELTA(outQGroup[3]->y(0)[0], 0.6, 0.0001);

    ADS.clear();
  }

  void test_input_workspace_group_with_default_output_workspaces() {
    ReflectometryReductionOneAuto3 alg;
    setup_alg_on_input_workspace_group_with_run_number(alg);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Mandatory workspaces should exist
    TS_ASSERT_EQUALS(ADS.doesExist("IvsQ_1234"), true);
    TS_ASSERT_EQUALS(ADS.doesExist("IvsQ_binned_1234"), true);
    // IvsLam is currently always output for group workspaces
    TS_ASSERT_EQUALS(ADS.doesExist("IvsLam_1234"), true);

    auto outQGroup = retrieveOutWS("IvsQ_1234");
    auto outQGroupBinned = retrieveOutWS("IvsQ_binned_1234");
    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outQGroupBinned.size(), 4);

    ADS.clear();
  }

  void
  test_input_workspace_group_with_default_output_workspaces_and_debug_on() {
    ReflectometryReductionOneAuto3 alg;
    setup_alg_on_input_workspace_group_with_run_number(alg);
    alg.setProperty("Debug", true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Mandatory workspaces should exist
    TS_ASSERT_EQUALS(ADS.doesExist("IvsQ_1234"), true);
    TS_ASSERT_EQUALS(ADS.doesExist("IvsQ_binned_1234"), true);
    TS_ASSERT_EQUALS(ADS.doesExist("IvsLam_1234"), true);

    auto outLamGroup = retrieveOutWS("IvsLam_1234");
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    ADS.clear();
  }

  void test_input_workspace_group_with_named_output_workspaces() {
    ReflectometryReductionOneAuto3 alg;
    setup_alg_on_input_workspace_group_with_run_number(alg);
    alg.setPropertyValue("OutputWorkspace", "testIvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "testIvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "testIvsLam");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Mandatory workspaces should exist
    TS_ASSERT_EQUALS(ADS.doesExist("testIvsQ"), true);
    TS_ASSERT_EQUALS(ADS.doesExist("testIvsQ_binned"), true);
    // IvsLam is currently always output for group workspaces
    TS_ASSERT_EQUALS(ADS.doesExist("testIvsLam"), true);

    auto outQGroup = retrieveOutWS("testIvsQ");
    auto outQGroupBinned = retrieveOutWS("testIvsQ_binned");
    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outQGroupBinned.size(), 4);

    ADS.clear();
  }

  void test_input_workspace_group_with_named_output_workspaces_and_debug_on() {
    ReflectometryReductionOneAuto3 alg;
    setup_alg_on_input_workspace_group_with_run_number(alg);
    alg.setPropertyValue("OutputWorkspace", "testIvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "testIvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "testIvsLam");
    alg.setProperty("Debug", true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Mandatory workspaces should exist
    TS_ASSERT_EQUALS(ADS.doesExist("testIvsQ"), true);
    TS_ASSERT_EQUALS(ADS.doesExist("testIvsQ_binned"), true);
    TS_ASSERT_EQUALS(ADS.doesExist("testIvsLam"), true);

    auto outLamGroup = retrieveOutWS("testIvsLam");
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    ADS.clear();
  }

  void test_one_transmissionrun() {
    const double startX = 1000;
    const int nBins = 3;
    const double deltaX = 1000;
    const std::vector<double> yValues1 = {1, 2, 3};
    const std::vector<double> yValues2 = {4, 5, 6};
    MatrixWorkspace_sptr input =
        createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("input", input);

    MatrixWorkspace_sptr first =
        createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first", first);
    MatrixWorkspace_sptr second =
        createWorkspaceSingle(startX, nBins, deltaX, yValues2);
    ADS.addOrReplace("second", second);

    GroupWorkspaces mkGroup;
    mkGroup.initialize();
    mkGroup.setProperty("InputWorkspaces", "input");
    mkGroup.setProperty("OutputWorkspace", "inputWSGroup");
    mkGroup.execute();

    mkGroup.setProperty("InputWorkspaces", "first,second");
    mkGroup.setProperty("OutputWorkspace", "transWSGroup");
    mkGroup.execute();

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("InputWorkspace", "inputWSGroup");
    alg.setProperty("WavelengthMin", 0.0000000001);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.setPropertyValue("FirstTransmissionRun", "transWSGroup");
    alg.setProperty("Debug", true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_DELTA(outQGroup[0]->x(0)[0], 2.8022, 0.0001);
    TS_ASSERT_DELTA(outQGroup[0]->x(0)[3], 11.2088, 0.0001);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 1.3484, 0.0001);
    TS_ASSERT_DELTA(outQGroup[0]->y(0)[2], 0.9207, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->x(0)[0], 0.1946, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0)[3], 0.7787, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 0.9207, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[2], 1.3484, 0.0001);

    ADS.clear();
  }

  void test_polarization_with_transmissionrun() {
    const double startX = 1000;
    const int nBins = 3;
    const double deltaX = 1000;
    const double endX = 4000;

    prepareInputGroup("inputWSGroup", "Fredrikze", 4, startX, endX, nBins);

    const std::vector<double> yValues1 = {1, 2, 3};
    const std::vector<double> yValues2 = {4, 5, 6};

    MatrixWorkspace_sptr first =
        createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first", first);
    MatrixWorkspace_sptr second =
        createWorkspaceSingle(startX, nBins, deltaX, yValues2);
    ADS.addOrReplace("second", second);

    GroupWorkspaces mkGroup;
    mkGroup.initialize();
    mkGroup.setProperty("InputWorkspaces", "first,second");
    mkGroup.setProperty("OutputWorkspace", "transWSGroup");
    mkGroup.execute();

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("InputWorkspace", "inputWSGroup");
    alg.setProperty("WavelengthMin", 0.0000000001);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setProperty("PolarizationAnalysis", true);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.setPropertyValue("FirstTransmissionRun", "transWSGroup");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_DELTA(outQGroup[0]->x(0)[0], 3.4710, 0.0001);
    TS_ASSERT_DELTA(outQGroup[0]->x(0)[3], 13.8841, 0.0001);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 0.5810, 0.0001);
    TS_ASSERT_DELTA(outQGroup[0]->y(0)[2], 0.7785, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->x(0)[0], 0.1430, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0)[3], 0.5719, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 0.7785, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[2], 0.5810, 0.0001);

    ADS.clear();
  }

  void test_second_transmissionrun() {
    const double startX = 1000;
    const int nBins = 3;
    const double deltaX = 1000;
    const std::vector<double> yValues1 = {1, 2, 3};
    const std::vector<double> yValues2 = {4, 5, 6};
    MatrixWorkspace_sptr input =
        createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("input", input);

    MatrixWorkspace_sptr first =
        createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first", first);
    MatrixWorkspace_sptr second =
        createWorkspaceSingle(startX, nBins, deltaX, yValues2);
    ADS.addOrReplace("second", second);

    MatrixWorkspace_sptr first2 =
        createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first2", first2);
    MatrixWorkspace_sptr second2 =
        createWorkspaceSingle(startX, nBins, deltaX, yValues2);
    ADS.addOrReplace("second2", second2);

    GroupWorkspaces mkGroup;
    mkGroup.initialize();
    mkGroup.setProperty("InputWorkspaces", "input");
    mkGroup.setProperty("OutputWorkspace", "inputWSGroup");
    mkGroup.execute();

    mkGroup.setProperty("InputWorkspaces", "first,second");
    mkGroup.setProperty("OutputWorkspace", "transWSGroup");
    mkGroup.execute();

    mkGroup.setProperty("InputWorkspaces", "first2,second2");
    mkGroup.setProperty("OutputWorkspace", "transWSGroup2");
    mkGroup.execute();

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("InputWorkspace", "inputWSGroup");
    alg.setProperty("WavelengthMin", 0.0000000001);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.setPropertyValue("FirstTransmissionRun", "transWSGroup");
    alg.setPropertyValue("SecondTransmissionRun", "transWSGroup2");
    alg.setProperty("Debug", true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_DELTA(outQGroup[0]->x(0)[0], 2.8022, 0.0001);
    TS_ASSERT_DELTA(outQGroup[0]->x(0)[3], 11.2088, 0.0001);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 1.3484, 0.0001);
    TS_ASSERT_DELTA(outQGroup[0]->y(0)[2], 0.9207, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->x(0)[0], 0.1946, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0)[3], 0.7787, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 0.9207, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[2], 1.3484, 0.0001);

    ADS.clear();
  }

  void test_polarization_correction_default_Wildes() {

    std::string const name = "input";
    prepareInputGroup(name, "Wildes");
    applyPolarizationEfficiencies(name);

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setProperty("PolarizationAnalysis", true);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 0.9368, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[1]->y(0)[0], 0.7813, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[2]->y(0)[0], 0.6797, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[3]->y(0)[0], 0.5242, 0.0001);

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 0.9368, 0.0001);
    TS_ASSERT_DELTA(outQGroup[1]->y(0)[0], 0.7813, 0.0001);
    TS_ASSERT_DELTA(outQGroup[2]->y(0)[0], 0.6797, 0.0001);
    TS_ASSERT_DELTA(outQGroup[3]->y(0)[0], 0.5242, 0.0001);

    ADS.clear();
  }

  void test_monitor_index_in_group() {
    std::string const name = "input";
    prepareInputGroup(name);

    ReflectometryReductionOneAuto3 alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 5.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setProperty("PolarizationAnalysis", true);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::invalid_argument & e,
                            std::string(e.what()),
                            "A detector is expected at workspace index 0 (Was "
                            "converted from specnum), found a monitor");
  }

  void test_I0MonitorIndex_is_detector() {
    std::string const name = "input";
    prepareInputGroup(name);

    ReflectometryReductionOneAuto3 alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 5.0);
    alg.setProperty("MonitorBackgroundWavelengthMin", 1.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 5.0);
    alg.setPropertyValue("I0MonitorIndex", "1");
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::invalid_argument & e,
                            std::string(e.what()),
                            "A monitor is expected at spectrum index 1");
  }

  void test_QStep_QMin_and_QMax() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    momentumTransferHelper(alg, inter, theta);
    alg.setProperty("MomentumTransferStep", 0.1);
    alg.setProperty("MomentumTransferMin", 0.1);
    alg.setProperty("MomentumTransferMax", 1.0);
    alg.execute();

    MatrixWorkspace_sptr outQBin = alg.getProperty("OutputWorkspaceBinned");

    const auto &outX = outQBin->x(0);
    const auto &outY = outQBin->y(0);

    TS_ASSERT_DELTA(outX[0], 0.1, 0.0001);
    TS_ASSERT_DELTA(outY[0], 0.0, 0.0001);

    TS_ASSERT_DELTA(outX[24], 1.0, 0.0001);
    TS_ASSERT_DELTA(outY[23], 0, 0.0001);

    TS_ASSERT_EQUALS(outX.size(), 25);
    TS_ASSERT_EQUALS(outY.size(), 24);
  }

  void test_QMin_alone() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    momentumTransferHelper(alg, inter, theta);
    alg.setProperty("MomentumTransferMin", 0.1);
    alg.execute();

    MatrixWorkspace_sptr outQbinned = alg.getProperty("OutputWorkspaceBinned");

    const auto &outX = outQbinned->x(0);
    const auto &outY = outQbinned->y(0);

    TS_ASSERT_DELTA(outX[0], 0.1, 0.0001);
    TS_ASSERT_DELTA(outY[0], 0.0, 0.0001);

    TS_ASSERT_DELTA(outX[1], 0.1018, 0.0001);

    TS_ASSERT_EQUALS(outX.size(), 2);
    TS_ASSERT_EQUALS(outY.size(), 1);
  }

  void test_QMax_alone() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    momentumTransferHelper(alg, inter, theta);
    alg.setProperty("MomentumTransferMax", 0.1);
    alg.execute();

    MatrixWorkspace_sptr outQBin = alg.getProperty("OutputWorkspaceBinned");

    const auto &outX = outQBin->x(0);
    const auto &outY = outQBin->y(0);

    TS_ASSERT_DELTA(outX[0], 0.009, 0.0001);
    TS_ASSERT_DELTA(outY[0], 0.0006, 0.0001);

    TS_ASSERT_DELTA(outX[72], 0.1, 0.0001);
    TS_ASSERT_DELTA(outY[71], 3.8e-06, 0.0001);

    TS_ASSERT_EQUALS(outX.size(), 73);
    TS_ASSERT_EQUALS(outY.size(), 72);
  }

  void test_QMax_and_QMin() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    momentumTransferHelper(alg, inter, theta);
    alg.setProperty("MomentumTransferMin", 0.1);
    alg.setProperty("MomentumTransferMax", 1.0);
    alg.execute();

    MatrixWorkspace_sptr outQBin = alg.getProperty("OutputWorkspaceBinned");

    const auto &outX = outQBin->x(0);
    const auto &outY = outQBin->y(0);

    TS_ASSERT_DELTA(outX[0], 0.1, 0.0001);
    TS_ASSERT_DELTA(outY[0], 0.0, 0.0001);

    TS_ASSERT_DELTA(outX[69], 1.0, 0.0001);
    TS_ASSERT_DELTA(outY[68], 0.0, 0.0001);

    TS_ASSERT_EQUALS(outX.size(), 70);
    TS_ASSERT_EQUALS(outY.size(), 69);
  }

  void test_QStep_alone() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    momentumTransferHelper(alg, inter, theta);
    alg.setProperty("MomentumTransferStep", 0.1);
    alg.execute();

    MatrixWorkspace_sptr outQBin = alg.getProperty("OutputWorkspaceBinned");

    const auto &outX = outQBin->x(0);
    const auto &outY = outQBin->y(0);

    TS_ASSERT_DELTA(outX[0], 0.009, 0.0001);
    TS_ASSERT_DELTA(outY[0], 0.0021, 0.0001);

    TS_ASSERT_DELTA(outX[26], 0.1018, 0.0001);
    TS_ASSERT_DELTA(outY[25], 4.4e-06, 0.0001);

    TS_ASSERT_EQUALS(outX.size(), 27);
    TS_ASSERT_EQUALS(outY.size(), 26);
  }

  void test_QStep_QMin_alone() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    momentumTransferHelper(alg, inter, theta);
    alg.setProperty("MomentumTransferStep", 0.1);
    alg.setProperty("MomentumTransferMin", 0.1);
    alg.execute();

    MatrixWorkspace_sptr outQBin = alg.getProperty("OutputWorkspaceBinned");

    const auto &outX = outQBin->x(0);
    const auto &outY = outQBin->y(0);

    TS_ASSERT_DELTA(outX[0], 0.1, 0.0001);
    TS_ASSERT_DELTA(outY[0], 0.0, 0.0001);

    TS_ASSERT_DELTA(outX[1], 0.1018, 0.0001);

    TS_ASSERT_EQUALS(outX.size(), 2);
    TS_ASSERT_EQUALS(outY.size(), 1);
  }

  void test_QStep_QMax_alone() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    momentumTransferHelper(alg, inter, theta);
    alg.setProperty("MomentumTransferStep", 0.1);
    alg.setProperty("MomentumTransferMax", 0.1);
    alg.execute();

    MatrixWorkspace_sptr outQBin = alg.getProperty("OutputWorkspaceBinned");

    const auto &outX = outQBin->x(0);
    const auto &outY = outQBin->y(0);

    TS_ASSERT_DELTA(outX[0], 0.009, 0.0001);
    TS_ASSERT_DELTA(outY[0], 0.0021, 0.0001);

    TS_ASSERT_DELTA(outX[25], 0.1, 0.0001);
    TS_ASSERT_DELTA(outY[24], 2.3e-05, 0.0001);

    TS_ASSERT_EQUALS(outX.size(), 26);
    TS_ASSERT_EQUALS(outY.size(), 25);
  }

  void test_flood_correction() {
    auto inputWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto flood = createFloodWorkspace(inputWS->getInstrument());

    // Correct by rotating detectors around the sample
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("FloodWorkspace", flood);
    alg.setProperty("ThetaIn", 1.5);
    alg.setProperty("DetectorCorrectionType", "RotateAroundSample");
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("MomentumTransferStep", 0.01);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2+3");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(out->y(0)[0], 4.5, 0.000001);
    AnalysisDataService::Instance().clear();
  }

  void test_flood_correction_transmission() {
    auto inputWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto transWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    for (size_t i = 0; i < transWS->getNumberHistograms(); ++i) {
      auto &y = transWS->mutableY(i);
      y.assign(y.size(), 10.0 * double(i + 1));
    }
    auto flood = createFloodWorkspace(inputWS->getInstrument());

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("FloodWorkspace", flood);
    alg.setProperty("ThetaIn", 1.5);
    alg.setProperty("DetectorCorrectionType", "RotateAroundSample");
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("MomentumTransferStep", 0.01);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2+3");
    alg.setProperty("FirstTransmissionRun", transWS);
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(out->y(0)[0], 0.0782608695, 0.000001);
    AnalysisDataService::Instance().clear();
  }

  void test_flood_correction_group() {
    auto inputWS1 =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto inputWS2 =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    inputWS2 *= 2.0;
    auto group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(inputWS1);
    group->addWorkspace(inputWS2);
    AnalysisDataService::Instance().addOrReplace("input", group);
    auto flood = createFloodWorkspace(inputWS1->getInstrument());

    // Correct by rotating detectors around the sample
    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", "input");
    alg.setProperty("FloodWorkspace", flood);
    alg.setProperty("ThetaIn", 1.5);
    alg.setProperty("DetectorCorrectionType", "RotateAroundSample");
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("MomentumTransferStep", 0.01);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2+3");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceBinned", "IvsQb");
    alg.execute();
    WorkspaceGroup_sptr out =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IvsQ");
    auto out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0));
    TS_ASSERT_DELTA(out1->y(0)[0], 4.5, 0.000001);
    auto out2 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1));
    TS_ASSERT_DELTA(out2->y(0)[0], 9.0, 0.000001);
    AnalysisDataService::Instance().clear();
  }

  void test_flood_correction_polarization_correction() {
    std::string const name = "input";
    prepareInputGroup(name, "Fredrikze");
    applyPolarizationEfficiencies(name);
    auto const inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name +
                                                                    "_1");
    auto flood = createFloodWorkspace(inputWS->getInstrument(), 257);

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("FloodWorkspace", flood);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setProperty("PolarizationAnalysis", true);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IvsQ");
    TS_ASSERT(out);
    auto out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0));
    TS_ASSERT_DELTA(out1->y(0)[0], 90.0, 0.001);
    auto out2 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1));
    TS_ASSERT_DELTA(out2->y(0)[0], 80.0, 0.001);
    auto out3 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(2));
    TS_ASSERT_DELTA(out3->y(0)[0], 70.0, 0.003);
    auto out4 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(3));
    TS_ASSERT_DELTA(out4->y(0)[0], 60.0, 0.003);

    AnalysisDataService::Instance().clear();
  }

  void test_flood_correction_parameter_file() {

    std::string const name = "input";
    prepareInputGroup(name, "Flood");
    auto const inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name +
                                                                    "_1");
    auto flood = createFloodWorkspace(inputWS->getInstrument(), 257);

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("FloodCorrection", "ParameterFile");
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    WorkspaceGroup_sptr out =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IvsQ");
    TS_ASSERT(out);
    auto out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0));
    TS_ASSERT_DELTA(out1->y(0)[0], 90.0, 1e-15);
    auto out2 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1));
    TS_ASSERT_DELTA(out2->y(0)[0], 80.0, 1e-15);
    auto out3 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(2));
    TS_ASSERT_DELTA(out3->y(0)[0], 70.0, 1e-15);
    auto out4 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(3));
    TS_ASSERT_DELTA(out4->y(0)[0], 60.0, 1e-14);
    AnalysisDataService::Instance().clear();
  }

  void test_flood_correction_parameter_file_no_flood_parameters() {

    std::string const name = "input";
    prepareInputGroup(name, "No_Flood");

    ReflectometryReductionOneAuto3 alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("FloodCorrection", "ParameterFile");
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("WavelengthMin", 1.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_EQUALS(
        alg.execute(), std::invalid_argument & e, e.what(),
        std::string(
            "Instrument parameter file doesn't have the Flood_Run parameter."));
    AnalysisDataService::Instance().clear();
  }

private:
  MatrixWorkspace_sptr
  createFloodWorkspace(Mantid::Geometry::Instrument_const_sptr instrument,
                       size_t n = 4) {
    size_t detid = 1;
    auto flood = create2DWorkspace(int(n), 1);
    if (n == 4) {
      flood->mutableY(0)[0] = 0.7;
      flood->mutableY(1)[0] = 1.0;
      flood->mutableY(2)[0] = 0.8;
      flood->mutableY(3)[0] = 0.9;
    } else {
      for (size_t i = 0; i < n; ++i) {
        flood->mutableY(i)[0] = double(i) * 0.01;
      }
      detid = 1000;
    }
    flood->setInstrument(instrument);
    for (size_t i = 0; i < flood->getNumberHistograms(); ++i) {
      flood->getSpectrum(i).setDetectorID(Mantid::detid_t(i + detid));
    }
    flood->getAxis(0)->setUnit("TOF");
    return flood;
  }

  void setup_alg_on_input_workspace_group_with_run_number(
      ReflectometryReductionOneAuto3 &alg) {
    std::string const name = "input";
    prepareInputGroup(name);
    WorkspaceGroup_sptr group = ADS.retrieveWS<WorkspaceGroup>("input");
    MatrixWorkspace_sptr ws =
        ADS.retrieveWS<MatrixWorkspace>(group->getNames()[0]);
    ws->mutableRun().addProperty<std::string>("run_number", "1234");

    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("InputWorkspace", name);
    alg.setProperty("WavelengthMin", 0.0000000001);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ThetaIn", 10.0);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("MomentumTransferStep", 0.04);
  }
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO3TEST_H_ */
