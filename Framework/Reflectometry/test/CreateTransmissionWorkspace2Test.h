// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"
#include "MantidReflectometry/CreateTransmissionWorkspace2.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Reflectometry;
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

  void test_one_run_stores_output_workspace() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces(alg);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    check_stored_lambda_workspace("outWS");
    check_output_not_set(alg, "OutputWorkspaceFirstTransmission",
                         "TRANS_LAM_1234");
  }

  void test_one_run_sets_output_workspace_when_child() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces(alg);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.setChild(true);
    alg.execute();
    check_output_lambda_workspace(alg, "OutputWorkspace", "outWS");
    check_output_not_set(alg, "OutputWorkspaceFirstTransmission",
                         "TRANS_LAM_1234");
  }

  void test_one_run_stores_output_workspace_with_default_name() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces(alg);
    alg.execute();
    check_stored_lambda_workspace("TRANS_LAM_1234");
  }

  void test_one_run_sets_output_workspace_with_default_name_when_child() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces(alg);
    alg.setChild(true);
    alg.execute();
    check_output_lambda_workspace(alg, "OutputWorkspace", "TRANS_LAM_1234");
  }

  void test_two_runs_stores_stitched_output_workspace_only() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    check_stored_lambda_workspace("outWS");
    check_output_not_set(alg, "OutputWorkspaceFirstTransmission",
                         "TRANS_LAM_1234");
    check_output_not_set(alg, "OutputWorkspaceSecondTransmission",
                         "TRANS_LAM_4321");
  }

  void test_two_runs_does_not_set_interim_output_workspaces_when_child() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.setChild(true);
    alg.execute();
    check_output_lambda_workspace(alg, "OutputWorkspace", "outWS");
    check_output_not_set(alg, "OutputWorkspaceFirstTransmission",
                         "TRANS_LAM_1234");
    check_output_not_set(alg, "OutputWorkspaceSecondTransmission",
                         "TRANS_LAM_4321");
  }

  void test_two_runs_sets_all_output_workspaces_when_child_with_debug() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.setChild(true);
    alg.setProperty("Debug", true);
    alg.execute();
    check_output_lambda_workspace(alg, "OutputWorkspace", "outWS");
    check_output_lambda_workspace(alg, "OutputWorkspaceFirstTransmission",
                                  "TRANS_LAM_1234");
    check_output_lambda_workspace(alg, "OutputWorkspaceSecondTransmission",
                                  "TRANS_LAM_4321");
  }

  void test_two_runs_stores_stitched_output_workspace_with_default_name() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.execute();
    check_stored_lambda_workspace("TRANS_LAM_1234_4321");
    check_output_not_set(alg, "OutputWorkspaceFirstTransmission",
                         "TRANS_LAM_1234");
    check_output_not_set(alg, "OutputWorkspaceSecondTransmission",
                         "TRANS_LAM_4321");
  }

  void
  test_two_runs_sets_stitched_output_workspace_with_default_name_when_child() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setChild(true);
    alg.execute();
    check_output_lambda_workspace(alg, "OutputWorkspace",
                                  "TRANS_LAM_1234_4321");
  }

  void test_two_runs_sets_all_output_workspaces_when_debug_enabled() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setProperty("Debug", true);
    alg.execute();
    check_stored_lambda_workspace("TRANS_LAM_1234_4321");
    check_stored_lambda_workspace("TRANS_LAM_1234");
    check_stored_lambda_workspace("TRANS_LAM_4321");
  }

  void test_two_runs_stores_no_lambda_workspaces_in_ADS_when_child() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setChild(true);
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    check_lambda_workspace(outWS);
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("TRANS_LAM_4321"));
    TS_ASSERT(
        !AnalysisDataService::Instance().doesExist("TRANS_LAM_1234_4321"));
  }

  void
  test_two_runs_stores_no_lambda_workspaces_in_ADS_when_child_with_debug() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setChild(true);
    alg.setProperty("Debug", true);
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    check_lambda_workspace(outWS);
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("TRANS_LAM_4321"));
    TS_ASSERT(
        !AnalysisDataService::Instance().doesExist("TRANS_LAM_1234_4321"));
  }

  void test_two_runs_sets_interim_lambda_output_workspaces_when_debug() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg);
    alg.setProperty("Debug", true);
    alg.execute();
    check_stored_lambda_workspace("TRANS_LAM_1234_4321");
    check_stored_lambda_workspace("TRANS_LAM_1234");
    check_stored_lambda_workspace("TRANS_LAM_4321");
  }

  void test_throws_if_first_trans_name_not_found() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg, false, true);
    alg.setProperty("Debug", true);
    alg.execute();
    TS_ASSERT_EQUALS(alg.isExecuted(), false);
  }

  void test_throws_if_first_trans_name_not_found_when_child() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg, false, true);
    alg.setProperty("Debug", true);
    alg.setChild(true);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_throws_if_second_trans_name_not_found() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg, true, false);
    alg.setProperty("Debug", true);
    alg.execute();
    TS_ASSERT_EQUALS(alg.isExecuted(), false);
  }

  void test_throws_if_second_trans_name_not_found_when_child() {
    CreateTransmissionWorkspace2 alg;
    setup_test_to_check_output_workspaces_with_2_inputs(alg, true, false);
    alg.setProperty("Debug", true);
    alg.setChild(true);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

private:
  void setup_test_to_check_output_workspaces(CreateTransmissionWorkspace2 &alg,
                                             bool hasRunNumber = true) {
    AnalysisDataService::Instance().clear();
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    if (hasRunNumber)
      inputWS->mutableRun().addProperty<std::string>("run_number", "1234");

    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 3.0);
    alg.setProperty("WavelengthMax", 12.0);
    alg.setPropertyValue("ProcessingInstructions", "2");
  }

  void setup_test_to_check_output_workspaces_with_2_inputs(
      CreateTransmissionWorkspace2 &alg, bool firstHasRunNumber = true,
      bool secondHasRunNumber = true) {
    setup_test_to_check_output_workspaces(alg, firstHasRunNumber);
    auto inputWS2 = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    if (secondHasRunNumber)
      inputWS2->mutableRun().addProperty<std::string>("run_number", "4321");
    alg.setProperty("SecondTransmissionRun", inputWS2);
  }

  void check_output_lambda_workspace(CreateTransmissionWorkspace2 const &alg,
                                     std::string const &propertyName,
                                     std::string const &name) {
    TS_ASSERT_EQUALS(alg.getPropertyValue(propertyName), name);
    MatrixWorkspace_sptr outWS = alg.getProperty(propertyName);
    check_lambda_workspace(outWS);
  }

  void check_stored_lambda_workspace(std::string const &name) {
    auto exists = AnalysisDataService::Instance().doesExist(name);
    TS_ASSERT(exists);
    if (exists) {
      auto ws =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
      check_lambda_workspace(ws);
    }
  }

  void check_lambda_workspace(const MatrixWorkspace_sptr &ws) {
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->unitID(), "Wavelength");
    TS_ASSERT(ws->x(0).front() >= 3.0);
    TS_ASSERT(ws->x(0).back() <= 12.0);
  }

  void check_output_not_set(CreateTransmissionWorkspace2 const &alg,
                            std::string const &propertyName,
                            std::string const &name) {
    TS_ASSERT(alg.isDefault(propertyName));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(name));
  }
};
