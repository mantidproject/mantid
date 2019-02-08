// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACE2TEST_H_
#define ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACE2TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateTransmissionWorkspace2.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace WorkspaceCreationHelper;

class CreateTransmissionWorkspace2Test : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_multiDetectorWS;
  MatrixWorkspace_sptr m_wavelengthWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspace2Test *createSuite() {
    return new CreateTransmissionWorkspace2Test();
  }
  static void destroySuite(CreateTransmissionWorkspace2Test *suite) {
    delete suite;
  }

  CreateTransmissionWorkspace2Test() {
    FrameworkManager::Instance();
    // A multi detector ws
    m_multiDetectorWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
    // A workspace in wavelength
    m_wavelengthWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
    m_wavelengthWS->getAxis(0)->setUnit("Wavelength");
  }

  void test_execute() {
    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_trans_run_in_wavelength_throws() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_ANYTHING(
        alg.setProperty("FirstTransmissionRun", m_wavelengthWS));
    TS_ASSERT_THROWS_ANYTHING(
        alg.setProperty("SecondTransmissionRun", m_wavelengthWS));
  }

  void test_wavelength_min_is_mandatory() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_wavelength_max_is_mandatory() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_processing_instructions_is_mandatory() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_wavelength_range() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("WavelengthMin", 15.0);
    alg.setProperty("WavelengthMax", 1.5);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_range() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("MonitorBackgroundWavelengthMin", 15.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 10.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_integration_range() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "2");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 0.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_one_tranmission_run() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outLam);
    TS_ASSERT_EQUALS("Wavelength", outLam->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
  }

  void test_one_run_processing_instructions() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "2+3");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outLam);
    TS_ASSERT_EQUALS("Wavelength", outLam->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Y counts, should be 2.0000 * 2
    TS_ASSERT_DELTA(outLam->y(0)[0], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 4.0000, 0.0001);
  }

  void test_one_run_monitor_normalization() {
    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // Normalize by integrated monitors : No

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = inputWS->mutableY(0);
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 0.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("NormalizeByIntegratedMonitors", false);
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 10);
    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 2.4996 = 3.15301 (detectors) / 1.26139 (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[2], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[4], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.4996, 0.0001);
  }

  void test_one_run_integrated_monitor_normalization() {
    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // MonitorIntegrationWavelengthMin : 1.5
    // MonitorIntegrationWavelengthMax : 15.0
    // Normalize by integrated monitors : Yes

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = inputWS->mutableY(0);
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 0.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.5);
    alg.setProperty("MonitorIntegrationWavelengthMax", 15.0);
    alg.setProperty("NormalizeByIntegratedMonitors", true);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 16);
    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 0.1981 = 2.0000 (detectors) / (1.26139*8) (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.1981, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.1981, 0.0001);
  }

  void test_one_run_NormalizeByIntegratedMonitors_is_false() {
    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // MonitorIntegrationWavelengthMin : 1.5
    // MonitorIntegrationWavelengthMax : 15.0
    // Normalize by integrated monitors : No

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = inputWS->mutableY(0);
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 0.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("NormalizeByIntegratedMonitors", false);
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.5);
    alg.setProperty("MonitorIntegrationWavelengthMax", 15.0);
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 10);
    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 2.4996 = 3.15301 (detectors) / 1.26139 (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[2], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[4], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.4996, 0.0001);
  }

  void test_two_transmission_runs() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("SecondTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
  }

  void test_two_transmission_runs_stitch_params() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("SecondTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("Params", "0.1");
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 126);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->x(0)[0], 1.7924, 0.0001);
    TS_ASSERT_DELTA(outLam->x(0)[1], 1.8924, 0.0001);
    TS_ASSERT_DELTA(outLam->x(0)[2], 1.9924, 0.0001);
    TS_ASSERT_DELTA(outLam->x(0)[3], 2.0924, 0.0001);
  }

  void test_two_transmission_runs_stitch_ScaleRHSWorkspace() {

    auto lhsWS = m_multiDetectorWS;
    auto rhsWS = m_multiDetectorWS;

    // Modify counts for rhsWS (only for this test)
    auto &Y = rhsWS->mutableY(1);
    std::fill(Y.begin(), Y.end(), 3.0);

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", lhsWS);
    alg.setProperty("SecondTransmissionRun", rhsWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ScaleRHSWorkspace", false);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[14] <= 15.0);

    // No monitors considered because MonitorBackgroundWavelengthMin
    // and MonitorBackgroundWavelengthMax were not set
    // Y counts must be all 3.0000
    const auto &y_counts = outLam->counts(0);
    for (auto itr = y_counts.begin(); itr != y_counts.end(); ++itr) {
      TS_ASSERT_DELTA(3.0, *itr, 0.000001);
    }
  }

  void test_one_run_store_in_ADS() {
    AnalysisDataService::Instance().clear();
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS->mutableRun().addProperty<std::string>("run_number", "1234");

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 3.0);
    alg.setProperty("WavelengthMax", 12.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();

    TS_ASSERT(!AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("outWS"));

    MatrixWorkspace_sptr firstLam =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");

    TS_ASSERT(firstLam);
    TS_ASSERT_EQUALS(firstLam->getAxis(0)->unit()->unitID(), "Wavelength");
    TS_ASSERT(firstLam->x(0).front() >= 3.0);
    TS_ASSERT(firstLam->x(0).back() <= 12.0);
  }

  void test_one_run_store_in_ADS_default() {
    AnalysisDataService::Instance().clear();
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS->mutableRun().addProperty<std::string>("run_number", "1234");

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 3.0);
    alg.setProperty("WavelengthMax", 12.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));

    MatrixWorkspace_sptr firstLam =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "TRANS_LAM_1234");

    TS_ASSERT(firstLam);
    TS_ASSERT_EQUALS(firstLam->getAxis(0)->unit()->unitID(), "Wavelength");
    TS_ASSERT(firstLam->x(0).front() >= 3.0);
    TS_ASSERT(firstLam->x(0).back() <= 12.0);
  }

  void test_two_runs_store_in_ADS() {
    AnalysisDataService::Instance().clear();
    auto inputWS1 = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS1->mutableRun().addProperty<std::string>("run_number", "1234");
    auto inputWS2 = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS2->mutableRun().addProperty<std::string>("run_number", "4321");

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS1);
    alg.setProperty("SecondTransmissionRun", inputWS2);
    alg.setProperty("WavelengthMin", 3.0);
    alg.setProperty("WavelengthMax", 12.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr firstLam =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "TRANS_LAM_1234");
    MatrixWorkspace_sptr secondLam =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "TRANS_LAM_4321");

    TS_ASSERT(firstLam);
    TS_ASSERT_EQUALS(firstLam->getAxis(0)->unit()->unitID(), "Wavelength");
    TS_ASSERT(firstLam->x(0).front() >= 3.0);
    TS_ASSERT(firstLam->x(0).back() <= 12.0);

    TS_ASSERT(secondLam);
    TS_ASSERT_EQUALS(secondLam->getAxis(0)->unit()->unitID(), "Wavelength");
    TS_ASSERT(secondLam->x(0).front() >= 3.0);
    TS_ASSERT(secondLam->x(0).back() <= 12.0);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_4321"));
    TS_ASSERT(
        !AnalysisDataService::Instance().doesExist("TRANS_LAM_1234_4321"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("outWS"));
  }

  void test_two_runs_store_in_ADS_default() {
    AnalysisDataService::Instance().clear();
    auto inputWS1 = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS1->mutableRun().addProperty<std::string>("run_number", "1234");
    auto inputWS2 = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS2->mutableRun().addProperty<std::string>("run_number", "4321");

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS1);
    alg.setProperty("SecondTransmissionRun", inputWS2);
    alg.setProperty("WavelengthMin", 3.0);
    alg.setProperty("WavelengthMax", 12.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.execute();
    MatrixWorkspace_sptr firstLam =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "TRANS_LAM_1234");
    MatrixWorkspace_sptr secondLam =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "TRANS_LAM_4321");

    TS_ASSERT(firstLam);
    TS_ASSERT_EQUALS(firstLam->getAxis(0)->unit()->unitID(), "Wavelength");
    TS_ASSERT(firstLam->x(0).front() >= 3.0);
    TS_ASSERT(firstLam->x(0).back() <= 12.0);

    TS_ASSERT(secondLam);
    TS_ASSERT_EQUALS(secondLam->getAxis(0)->unit()->unitID(), "Wavelength");
    TS_ASSERT(secondLam->x(0).front() >= 3.0);
    TS_ASSERT(secondLam->x(0).back() <= 12.0);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_4321"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_1234_4321"));
  }

  void test_two_runs_store_in_ADS_default_child() {
    AnalysisDataService::Instance().clear();
    auto inputWS1 = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS1->mutableRun().addProperty<std::string>("run_number", "1234");
    auto inputWS2 = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS2->mutableRun().addProperty<std::string>("run_number", "4321");

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS1);
    alg.setProperty("SecondTransmissionRun", inputWS2);
    alg.setProperty("WavelengthMin", 3.0);
    alg.setProperty("WavelengthMax", 12.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TRANS_LAM_4321"));
    TS_ASSERT(
        !AnalysisDataService::Instance().doesExist("TRANS_LAM_1234_4321"));
  }
};

#endif /* ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACE2TEST_H_ */
