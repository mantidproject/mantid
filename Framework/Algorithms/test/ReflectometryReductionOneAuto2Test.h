#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2TEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryReductionOneAuto2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
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
    auto results = alg.validateInputs();
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

    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inter);
    alg.setProperty("ThetaIn", 0.7);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");

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

    // Only 'point-detector' and 'point-detector2' should have been moved
    // vertically (along Y)

    auto point1In = instIn->getComponentByName("point-detector")->getPos();
    auto point2In = instIn->getComponentByName("point-detector2")->getPos();
    auto point1Out = instOut->getComponentByName("point-detector")->getPos();
    auto point2Out = instOut->getComponentByName("point-detector2")->getPos();

    TS_ASSERT_EQUALS(point1In.X(), point1Out.X());
    TS_ASSERT_EQUALS(point1In.Z(), point1Out.Z());
    TS_ASSERT_EQUALS(point2In.X(), point2Out.X());
    TS_ASSERT_EQUALS(point2In.Z(), point2Out.Z());
    TS_ASSERT_DIFFERS(point1In.Y(), point1Out.Y());
    TS_ASSERT_DIFFERS(point2In.Y(), point2Out.Y());
    TS_ASSERT_DELTA(point1Out.Y() /
                        (point1Out.Z() - instOut->getSample()->getPos().Z()),
                    std::tan(0.7 * 2 * M_PI / 180), 1e-4);
    TS_ASSERT_DELTA(point2Out.Y() /
                        (point2Out.Z() - instOut->getSample()->getPos().Z()),
                    std::tan(0.7 * 2 * M_PI / 180), 1e-4);
  }

  void test_correct_detector_position_OFFSPEC_LinearDetector() {
    // Histograms in this run correspond to 'LinearDetector' component
    auto offspec = loadRun("OFFSPEC00004622.raw");

    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", offspec);
    alg.setProperty("ThetaIn", 0.64);
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
    alg.setProperty("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");

    // Compare instrument components before and after
    auto instIn = offspec->getInstrument();
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
    TS_ASSERT_EQUALS(instIn->getComponentByName("WLSFDetector")->getPos(),
                     instOut->getComponentByName("WLSFDetector")->getPos());

    // Only 'LinearDetector' should have been moved vertically (along Y)

    auto detectorIn = instIn->getComponentByName("LinearDetector")->getPos();
    auto detectorOut = instOut->getComponentByName("LinearDetector")->getPos();

    TS_ASSERT_EQUALS(detectorIn.X(), detectorOut.X());
    TS_ASSERT_EQUALS(detectorIn.Z(), detectorOut.Z());
    TS_ASSERT_DIFFERS(detectorIn.Y(), detectorOut.Y());
    TS_ASSERT_DELTA(detectorOut.Y() /
                        (detectorOut.Z() - instOut->getSample()->getPos().Z()),
                    std::tan(0.64 * 2 * M_PI / 180), 1e-4);
  }

  void test_correct_detector_position_POLREF() {
    // Histograms in this run correspond to 'OSMOND' component
    auto polref = loadRun("POLREF00014966.raw");

    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", polref);
    alg.setProperty("ThetaIn", 1.5);
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
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

    // Only 'OSMOND' should have been moved vertically (along Z)

    auto detectorIn = instIn->getComponentByName("OSMOND")->getPos();
    auto detectorOut = instOut->getComponentByName("OSMOND")->getPos();

    TS_ASSERT_EQUALS(detectorIn.X(), detectorOut.X());
    TS_ASSERT_EQUALS(detectorIn.Y(), detectorOut.Y());
    TS_ASSERT_DELTA(detectorOut.Z() /
                        (detectorOut.X() - instOut->getSample()->getPos().X()),
                    std::tan(1.5 * 2 * M_PI / 180), 1e-4);
  }

  void test_correct_detector_position_CRISP() {
    // Histogram in this run corresponds to 'point-detector' component
    auto polref = loadRun("CSP79590.raw");

    ReflectometryReductionOneAuto2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", polref);
    alg.setProperty("ThetaIn", 0.25);
    alg.setProperty("CorrectionAlgorithm", "None");
    alg.setProperty("OutputWorkspace", "IvsQ");
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
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2TEST_H_ */