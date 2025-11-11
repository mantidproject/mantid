// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidReflectometry/ReflectometryReductionOneAuto3.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/ClearCache.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidFrameworkTestHelpers/ReflectometryHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Points.h"

namespace ReflAuto3Test {
constexpr std::string TEST_GROUP_NAME = "testGroup";
typedef std::variant<MatrixWorkspace_sptr, double, std::string, bool> propVariant;
} // namespace ReflAuto3Test

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::FrameworkTestHelpers;
using namespace Mantid::Reflectometry;
using namespace WorkspaceCreationHelper;
using namespace ReflAuto3Test;

class ReflectometryReductionOneAuto3Test : public CxxTest::TestSuite {
public:
  void tearDown() override {
    ADS.clear();
    clear_instrument_cache();
  }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneAuto3Test *createSuite() {
    ConfigService::Instance().setString("default.facility", "ISIS");
    return new ReflectometryReductionOneAuto3Test();
  }
  static void destroySuite(ReflectometryReductionOneAuto3Test *suite) {
    ConfigService::Instance().setString("default.facility", " ");
    delete suite;
  }

  ReflectometryReductionOneAuto3Test() {
    FrameworkManager::Instance();

    m_notTOF = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 10, 10);
    m_TOF = WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrumentMultiDetector();
  }

  ~ReflectometryReductionOneAuto3Test() override = default;

  void test_init() {
    ReflectometryReductionOneAuto3 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_bad_input_workspace_units() {
    const auto alg = create_refl_algorithm(m_notTOF, std::nullopt, "1");
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_bad_wavelength_range() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "1");

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_bad_monitor_background_range() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "1");
    setup_optional_properties(alg, {{"MonitorBackgroundWavelengthMin", 3.0}, {"MonitorBackgroundWavelengthMax", 0.5}});

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_bad_monitor_integration_range() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "1");
    setup_optional_properties(alg, {{"MonitorBackgroundWavelengthMin", 15.0}, {"MonitorBackgroundWavelengthMax", 1.5}});

    TS_ASSERT_THROWS_ANYTHING(alg->execute())
  }

  void test_bad_first_transmission_run_units() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "1");
    setup_optional_properties(alg, {{"MonitorBackgroundWavelengthMin", 1.0}, {"MonitorBackgroundWavelengthMax", 15.0}});
    alg->setProperty("FirstTransmissionRun", m_notTOF);

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_bad_second_transmission_run_units() {
    ReflectometryReductionOneAuto3 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_TOF);
    alg.setProperty("FirstTransmissionRun", m_TOF);
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("SecondTransmissionRun", m_notTOF));
  }

  void test_bad_first_transmission_group_size() {
    MatrixWorkspace_sptr first = m_TOF->clone();
    MatrixWorkspace_sptr second = m_TOF->clone();
    MatrixWorkspace_sptr third = m_TOF->clone();
    MatrixWorkspace_sptr fourth = m_TOF->clone();

    WorkspaceGroup_sptr inputWSGroup = std::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(first);
    inputWSGroup->addWorkspace(second);
    WorkspaceGroup_sptr transWSGroup = std::make_shared<WorkspaceGroup>();
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
  }

  void test_bad_second_transmission_group_size() {
    const MatrixWorkspace_sptr first = m_TOF->clone();
    const MatrixWorkspace_sptr second = m_TOF->clone();
    const MatrixWorkspace_sptr third = m_TOF->clone();
    const MatrixWorkspace_sptr fourth = m_TOF->clone();

    WorkspaceGroup_sptr inputWSGroup = std::make_shared<WorkspaceGroup>();
    inputWSGroup->addWorkspace(first);
    WorkspaceGroup_sptr firstWSGroup = std::make_shared<WorkspaceGroup>();
    firstWSGroup->addWorkspace(second);
    WorkspaceGroup_sptr secondWSGroup = std::make_shared<WorkspaceGroup>();
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
  }

  void test_correct_detector_position_INTER() {
    auto inter = loadRun("INTER00013460.nxs");
    const double theta = 0.7;

    // Use the default correction type, which is a vertical shift
    const auto alg = create_refl_algorithm(inter, theta, "4");
    alg->execute();
    MatrixWorkspace_sptr out = alg->getProperty("OutputWorkspaceBinned");

    // Check default rebin params
    const double qStep = alg->getProperty("MomentumTransferStep");
    const double qMin = alg->getProperty("MomentumTransferMin");
    const double qMax = alg->getProperty("MomentumTransferMax");
    TS_ASSERT_DELTA(qStep, 0.034028, 1e-6);
    TS_ASSERT_DELTA(qMin, out->x(0).front(), 1e-6);
    TS_ASSERT_DELTA(qMax, out->x(0).back(), 1e-6);

    // Compare instrument components before and after
    compare_detectors_in_out(inter->getInstrument(), out->getInstrument(),
                             {"monitor1", "monitor2", "monitor3", "linear-detector"}, "point-detector",
                             {std::tan(theta * 2 * M_PI / 180)});
  }

  void test_correct_detector_position_rotation_POLREF() {
    // Histograms in this run correspond to 'OSMOND' component
    auto polref = loadRun("POLREF00014966.nxs");

    // Correct by rotating detectors around the sample
    const auto alg = create_refl_algorithm(polref, 1.5);
    setup_optional_properties(alg, {{"DetectorCorrectionType", "RotateAroundSample"},
                                    {"AnalysisMode", "MultiDetectorAnalysis"},
                                    {"MomentumTransferStep", 0.01}});
    alg->execute();
    MatrixWorkspace_sptr out = alg->getProperty("OutputWorkspace");
    // Compare instrument components before and after
    compare_detectors_in_out(polref->getInstrument(), out->getInstrument(),
                             {"monitor1", "monitor2", "monitor3", "point-detector", "lineardetector"}, "OSMOND",
                             {25.99589, 0.1570});
  }

  void test_correct_detector_position_vertical_CRISP() {
    // Histogram in this run corresponds to 'point-detector' component
    auto crisp = loadRun("CSP79590.raw");
    const double theta = 0.25;

    // Correct by shifting detectors vertically
    // Also explicitly pass CorrectDetectors=1
    const auto alg = create_refl_algorithm(crisp, theta);
    setup_optional_properties(
        alg, {{"DetectorCorrectionType", "VerticalShift"}, {"CorrectDetectors", "1"}, {"MomentumTransferStep", 0.01}});
    alg->execute();
    MatrixWorkspace_sptr out = alg->getProperty("OutputWorkspace");

    // Compare instrument components before and after
    compare_detectors_in_out(crisp->getInstrument(), out->getInstrument(), {"monitor1", "monitor2", "linear-detector"},
                             "point-detector", {std::tan(theta * 2 * M_PI / 180)});
  }

  void test_correct_detector_position_from_logs() {
    auto inter = loadRun("INTER00013460.nxs");

    // Use theta from the logs to correct detector positions
    const auto alg = create_refl_algorithm(inter);
    setup_optional_properties(alg, {{"ThetaLogName", "theta"}, {"CorrectDetectors", "1"}});
    alg->execute();
    MatrixWorkspace_sptr corrected = alg->getProperty("OutputWorkspace");

    // Compare instrument components before and after
    compare_detectors_in_out(inter->getInstrument(), corrected->getInstrument(),
                             {"monitor1", "monitor2", "monitor3", "linear-detector"}, "point-detector",
                             {std::tan(0.7 * 2 * M_PI / 180)});
  }

  void test_override_ThetaIn_without_correcting_detectors() {
    auto inter = loadRun("INTER00013460.nxs");
    // Use theta from the logs to correct detector positions
    const auto alg = create_refl_algorithm(inter, 10.0, "4");
    alg->setProperty("CorrectDetectors", "0");
    alg->execute();
    MatrixWorkspace_sptr corrected = alg->getProperty("OutputWorkspace");
    // Compare instrument components before and after
    compare_detectors_in_out(inter->getInstrument(), corrected->getInstrument(), {"point-detector"});
  }

  void test_IvsQ_linear_binning() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "2");
    setup_optional_properties(
        alg, {{"MomentumTransferMin", 1.0}, {"MomentumTransferMax", 10.0}, {"MomentumTransferStep", -0.04}});

    alg->execute();
    MatrixWorkspace_sptr outQbinned = alg->getProperty("OutputWorkspaceBinned");
    // Check the rebin params have not changed
    const double qStep = alg->getProperty("MomentumTransferStep");
    const double qMin = alg->getProperty("MomentumTransferMin");
    const double qMax = alg->getProperty("MomentumTransferMax");
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
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "2");
    setup_optional_properties(
        alg, {{"MomentumTransferMin", 1.0}, {"MomentumTransferMax", 10.0}, {"MomentumTransferStep", 0.04}});

    alg->execute();
    MatrixWorkspace_sptr outQbinned = alg->getProperty("OutputWorkspaceBinned");
    TS_ASSERT_EQUALS(outQbinned->getNumberHistograms(), 1);
    TS_ASSERT_DIFFERS(outQbinned->blocksize(), 8);
    TS_ASSERT_DELTA(outQbinned->x(0)[1] - outQbinned->x(0)[0], 0.04, 1e-6);
    TS_ASSERT(outQbinned->x(0)[7] - outQbinned->x(0)[6] > 0.05);
  }

  void test_IvsLam_range() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    alg->setProperty("MomentumTransferStep", 0.04);

    alg->execute();
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outLam = alg->getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outQ->binEdges(0).size(), 15);
    // X range in outLam
    assert_bin_values(outLam, true, {0, 1, 7, 13, 14}, {1.7924, 2.6886, 8.0658, 13.4431, 14.3393});
  }

  void test_IvsQ_range() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    alg->setProperty("MomentumTransferStep", 0.04);

    alg->execute();
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outLam = alg->getProperty("OutputWorkspaceWavelength");

    assert_size(outQ, 1, std::nullopt, 15);
    // X range in outLam
    assert_bin_values(outLam, true, {0, 7}, {1.7924, 8.0658});
    // X range in outQ
    assert_bin_values(outQ, true, {0, 1, 6, 7, 12, 13, 14}, {0.3353, 0.3577, 0.5366, 0.5962, 1.3415, 1.7886, 2.6830});
  }

  void test_IvsQ_range_cropped() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    setup_optional_properties(
        alg, {{"MomentumTransferMin", 0.5}, {"MomentumTransferMax", 1.5}, {"MomentumTransferStep", 0.04}});
    alg->execute();
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outLam = alg->getProperty("OutputWorkspaceWavelength");

    // X range in outQ is cropped to momentum transfer limits
    assert_size(outQ, 1, std::nullopt, 7);
    assert_bin_values(outQ, true, {0, 1, 5, 6}, {0.5366, 0.5962, 1.0732, 1.3414});
  }

  void test_IvsQ_values() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    alg->execute();
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspace");

    assert_size(outQ, 1, std::nullopt, 14, false);
    // Y values in outQ
    assert_bin_values(outQ, false, {0, 13}, {2.0, 2.0});
  }

  void test_IvsQ_values_scaled() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    setup_optional_properties(alg, {{"ScaleFactor", 0.1}});
    alg->execute();
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspace");

    assert_size(outQ, 1, std::nullopt, 14, false);
    // Y values in outQ
    assert_bin_values(outQ, false, {0, 13}, {20.0, 20.0});
  }

  void test_IvsQ_binned_values() {
    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    setup_optional_properties(
        alg, {{"MomentumTransferMin", 0.0}, {"MomentumTransferMax", 7.0}, {"MomentumTransferStep", -1.0}});
    alg->execute();
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspaceBinned");

    assert_size(outQ, 1, std::nullopt, 7, false);
    // Y values in outQ
    assert_bin_values(outQ, false, {0, 1, 2, 3, 4, 5, 6}, {21.1817, 5.2910, 1.5273, 0.0, 0.0, 0.0, 0.0});
  }

  void test_IvsQ_binned_values_scaled() {

    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    setup_optional_properties(alg, {{"MomentumTransferMin", 0.0},
                                    {"MomentumTransferMax", 7.0},
                                    {"MomentumTransferStep", -1.0},
                                    {"ScaleFactor", 0.1}});
    alg->execute();
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspaceBinned");

    assert_size(outQ, 1, std::nullopt, 7, false);
    // Y values in outQ
    assert_bin_values(outQ, false, {0, 1, 2, 3, 4, 5, 6}, {211.8171, 52.9097, 15.2731, 0.0, 0.0, 0.0, 0.0});
  }

  void test_IvsLam_values() {

    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    alg->execute();
    MatrixWorkspace_sptr outLam = alg->getProperty("OutputWorkspaceWavelength");

    assert_size(outLam, 1, std::nullopt, 14, false);
    // Y values in outLam
    assert_bin_values(outLam, false, {0, 13}, {2.0, 2.0});
  }

  void test_IvsLam_values_are_not_scaled() {

    const auto alg = create_refl_algorithm(m_TOF, std::nullopt, "3");
    setup_optional_properties(alg, {{"ScaleFactor", 0.1}});
    alg->execute();
    MatrixWorkspace_sptr outLam = alg->getProperty("OutputWorkspaceWavelength");

    assert_size(outLam, 1, std::nullopt, 14, false);
    // Y values in outQ
    assert_bin_values(outLam, false, {0, 13}, {2.0, 2.0});
  }

  void test_optional_outputs() {
    auto inter = loadRun("INTER00013460.nxs");
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt, false, false);

    alg->execute();

    assert_ads_exists({"IvsQ_binned_13460", "IvsQ_13460"});
    TS_ASSERT(!ADS.doesExist("IvsLam_13460"));
  }

  void test_optional_outputs_binned() {
    auto inter = loadRun("INTER00013460.nxs");
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt, false, false);
    alg->setPropertyValue("OutputWorkspaceBinned", "IvsQ_Binned");

    alg->execute();

    TS_ASSERT(!ADS.doesExist("IvsLam_13460"));
    TS_ASSERT(!ADS.doesExist("IvsQ_binned_13460"));
    assert_ads_exists({"IvsQ_binned", "IvsQ_13460"});
  }

  void test_optional_outputs_set() {
    auto inter = loadRun("INTER00013460.nxs");
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt, true, false);

    alg->execute();

    assert_ads_exists({"IvsQ_binned", "IvsQ", "IvsLam"});
  }

  void test_default_outputs_debug() {
    auto inter = loadRun("INTER00013460.nxs");
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt, false, false);
    alg->setProperty("Debug", true);

    alg->execute();

    assert_ads_exists({"IvsQ_binned_13460", "IvsQ_13460", "IvsLam_13460"});
  }

  void test_default_outputs_no_debug() {
    auto inter = loadRun("INTER00013460.nxs");
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt, false, false);

    alg->execute();
    assert_ads_exists({"IvsQ_binned_13460", "IvsQ_13460"});
    TS_ASSERT(!ADS.doesExist("IvsLam_13460"));
  }

  void test_default_outputs_no_run_number() {
    auto inter = loadRun("INTER00013460.nxs");
    inter->mutableRun().removeProperty("run_number");
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt, false, false);
    alg->setProperty("Debug", true);

    alg->execute();

    assert_ads_exists({"IvsQ_binned", "IvsQ", "IvsLam"});
  }

  void test_default_outputs_no_run_number_no_debug() {
    auto inter = loadRun("INTER00013460.nxs");
    inter->mutableRun().removeProperty("run_number");
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt, false, false);

    alg->execute();

    assert_ads_exists({"IvsQ_binned", "IvsQ"});
    TS_ASSERT(!ADS.doesExist("IvsLam"));
  }

  void test_workspace_group_with_no_polarization_analysis_does_not_create_spin_state_sample_logs() {
    prepare_group_with_run_number(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 0.0000000001, 15.0, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}});

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    check_output_group_contains_sample_logs_for_spin_state_ORSO(retrieveOutWS("IvsQ_1234"), false);
    check_output_group_contains_sample_logs_for_spin_state_ORSO(retrieveOutWS("IvsQ_binned_1234"), false);
    check_output_group_contains_sample_logs_for_spin_state_ORSO(retrieveOutWS("IvsLam_1234"), false);
  }

  void test_workspace_group_with_polarization_analysis_creates_spin_state_sample_logs() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes");
    applyPolarizationEfficiencies(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}});

    alg->execute();

    check_output_group_contains_sample_logs_for_spin_state_ORSO(retrieveOutWS("IvsQ"), true);
    check_output_group_contains_sample_logs_for_spin_state_ORSO(retrieveOutWS("IvsQ_binned"), true);
    check_output_group_contains_sample_logs_for_spin_state_ORSO(retrieveOutWS("IvsLam"), true);
  }

  void test_polarization_correction() {
    prepareInputGroup(TEST_GROUP_NAME, "Fredrikze");
    applyPolarizationEfficiencies(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}});

    alg->execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);
    assert_spectra_in_group_values(outLamGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {0.9, 0.8, 0.7, 0.6});

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);
    assert_spectra_in_group_values(outQGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {0.9, 0.8, 0.7, 0.6});
  }

  void test_polarization_correction_with_background_subtraction() {
    prepareInputGroup(TEST_GROUP_NAME, "Fredrikze");
    applyPolarizationEfficiencies(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04},
                                    {"PolarizationAnalysis", true},
                                    {"SubtractBackground", true},
                                    {"BackgroundProcessingInstructions", "3-4"}});

    alg->execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);
  }

  void test_input_workspace_group_with_default_output_workspaces() {
    prepare_group_with_run_number(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}});
    alg->execute();

    // Mandatory workspaces should exist, IvsLam is always output for groups
    assert_ads_exists({"IvsQ_1234", "IvsQ_binned_1234", "IvsLam_1234"});

    auto outQGroup = retrieveOutWS("IvsQ_1234");
    auto outQGroupBinned = retrieveOutWS("IvsQ_binned_1234");
    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outQGroupBinned.size(), 4);
  }

  void test_input_workspace_group_with_default_output_workspaces_and_debug_on() {
    prepare_group_with_run_number(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"Debug", true}});
    alg->execute();

    // Mandatory workspaces should exist, IvsLam is always output for groups
    assert_ads_exists({"IvsQ_1234", "IvsQ_binned_1234", "IvsLam_1234"});

    auto outLamGroup = retrieveOutWS("IvsLam_1234");
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);
  }

  void test_input_workspace_group_with_named_output_workspaces() {
    prepare_group_with_run_number(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}});
    alg->setPropertyValue("OutputWorkspace", "testIvsQ");
    alg->setPropertyValue("OutputWorkspaceBinned", "testIvsQ_binned");
    alg->setPropertyValue("OutputWorkspaceWavelength", "testIvsLam");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    // Mandatory workspaces should exist, IvsLam is always output for groups
    assert_ads_exists({"testIvsQ", "testIvsQ_binned", "testIvsLam"});

    auto outQGroup = retrieveOutWS("testIvsQ");
    auto outQGroupBinned = retrieveOutWS("testIvsQ_binned");
    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outQGroupBinned.size(), 4);
  }

  void test_input_workspace_group_with_named_output_workspaces_and_debug_on() {
    prepare_group_with_run_number(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"Debug", true}});
    alg->setPropertyValue("OutputWorkspace", "testIvsQ");
    alg->setPropertyValue("OutputWorkspaceBinned", "testIvsQ_binned");
    alg->setPropertyValue("OutputWorkspaceWavelength", "testIvsLam");

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    // Mandatory workspaces should exist, IvsLam is always output for groups
    assert_ads_exists({"testIvsQ", "testIvsQ_binned", "testIvsLam"});

    auto outLamGroup = retrieveOutWS("testIvsLam");
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);
  }

  void test_one_transmissionrun() {
    const double startX = 1000;
    const int nBins = 3;
    const double deltaX = 1000;
    const std::vector<double> yValues1 = {1, 2, 3};
    const std::vector<double> yValues2 = {4, 5, 6};
    MatrixWorkspace_sptr input = createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("input", input);

    MatrixWorkspace_sptr first = createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first", first);
    MatrixWorkspace_sptr second = createWorkspaceSingle(startX, nBins, deltaX, yValues2);
    ADS.addOrReplace("second", second);

    GroupWorkspaces mkGroup;
    mkGroup.initialize();
    mkGroup.setProperty("InputWorkspaces", "input");
    mkGroup.setProperty("OutputWorkspace", "inputWSGroup");
    mkGroup.execute();

    mkGroup.setProperty("InputWorkspaces", "first,second");
    mkGroup.setProperty("OutputWorkspace", "transWSGroup");
    mkGroup.execute();

    const auto alg = create_refl_algorithm("inputWSGroup", 10.0, "2", 0.0000000001, 15.0);
    setup_optional_properties(
        alg, {{"MomentumTransferStep", 0.04}, {"Debug", true}, {"FirstTransmissionRun", "transWSGroup"}});

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");
    assert_spectra_in_group_values(outQGroup, {0, 0}, {0, 3}, {2.8022, 11.2088}, false);
    assert_spectra_in_group_values(outQGroup, {0, 0}, {0, 2}, {1.3484, 0.9207});
    assert_spectra_in_group_values(outLamGroup, {0, 0}, {0, 3}, {0.1946, 0.7787}, false);
    assert_spectra_in_group_values(outLamGroup, {0, 0}, {0, 2}, {0.9207, 1.3484});
  }

  void test_polarization_with_transmissionrun() {
    const double startX = 1000;
    const int nBins = 3;
    const double deltaX = 1000;
    const double endX = 4000;

    prepareInputGroup("inputWSGroup", "Fredrikze", 4, startX, endX, nBins);

    const std::vector<double> yValues1 = {1, 2, 3};
    const std::vector<double> yValues2 = {4, 5, 6};

    MatrixWorkspace_sptr first = createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first", first);
    MatrixWorkspace_sptr second = createWorkspaceSingle(startX, nBins, deltaX, yValues2);
    ADS.addOrReplace("second", second);

    GroupWorkspaces mkGroup;
    mkGroup.initialize();
    mkGroup.setProperty("InputWorkspaces", "first,second");
    mkGroup.setProperty("OutputWorkspace", "transWSGroup");
    mkGroup.execute();

    const auto alg = create_refl_algorithm("inputWSGroup", 10.0, "2", 0.0000000001, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"FirstTransmissionRun", "transWSGroup"}});
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");
    assert_spectra_in_group_values(outQGroup, {0, 0}, {0, 3}, {3.4710, 13.8841}, false);
    assert_spectra_in_group_values(outQGroup, {0, 0}, {0, 2}, {0.5810, 0.7785});
    assert_spectra_in_group_values(outLamGroup, {0, 0}, {0, 3}, {0.1430, 0.5719}, false);
    assert_spectra_in_group_values(outLamGroup, {0, 0}, {0, 2}, {0.7785, 0.5810});
  }

  void test_second_transmissionrun() {
    const double startX = 1000;
    const int nBins = 3;
    const double deltaX = 1000;
    const std::vector<double> yValues1 = {1, 2, 3};
    const std::vector<double> yValues2 = {4, 5, 6};
    MatrixWorkspace_sptr input = createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("input", input);

    MatrixWorkspace_sptr first = createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first", first);
    MatrixWorkspace_sptr second = createWorkspaceSingle(startX, nBins, deltaX, yValues2);
    ADS.addOrReplace("second", second);

    MatrixWorkspace_sptr first2 = createWorkspaceSingle(startX, nBins, deltaX, yValues1);
    ADS.addOrReplace("first2", first2);
    MatrixWorkspace_sptr second2 = createWorkspaceSingle(startX, nBins, deltaX, yValues2);
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

    const auto alg = create_refl_algorithm("inputWSGroup", 10.0, "2", 0.0000000001, 15.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04},
                                    {"Debug", true},
                                    {"FirstTransmissionRun", "transWSGroup"},
                                    {"SecondTransmissionRun", "transWSGroup2"}});

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");
    assert_spectra_in_group_values(outQGroup, {0, 0}, {0, 3}, {2.8022, 11.2088}, false);
    assert_spectra_in_group_values(outQGroup, {0, 0}, {0, 2}, {1.3484, 0.9207});
    assert_spectra_in_group_values(outLamGroup, {0, 0}, {0, 3}, {0.1946, 0.7787}, false);
    assert_spectra_in_group_values(outLamGroup, {0, 0}, {0, 2}, {0.9207, 1.3484});
  }

  void test_polarization_correction_default_Wildes() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes");
    applyPolarizationEfficiencies(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}});

    alg->execute();
    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);

    assert_spectra_in_group_values(outLamGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {0.9368, 0.7813, 0.6797, 0.5242});

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);
    assert_spectra_in_group_values(outQGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {0.9368, 0.7813, 0.6797, 0.5242});
  }

  void test_polarization_correction_with_efficiency_workspace() {
    prepareInputGroup(TEST_GROUP_NAME, "Fredrikze");
    auto efficiencies = createPolarizationEfficienciesWorkspace("Fredrikze");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", efficiencies}});

    alg->execute();
    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);
    assert_spectra_in_group_values(outLamGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.9267, 1.7838, -0.3231, -0.4659});

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);
    assert_spectra_in_group_values(outQGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {1.9267, 1.7838, -0.3231, -0.4659});
  }

  void test_polarization_correction_with_efficiency_workspace_Fredrikze_PNR() {
    prepareInputGroup(TEST_GROUP_NAME, "Fredrikze", 2);
    auto efficiencies = createPolarizationEfficienciesWorkspace("Fredrikze");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", efficiencies}});

    alg->execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 2);
    TS_ASSERT_EQUALS(outLamGroup.size(), 2);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 1.4062, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[1]->y(0)[0], 0.2813, 0.0001);

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 1.4062, 0.0001);
    TS_ASSERT_DELTA(outQGroup[1]->y(0)[0], 0.2813, 0.0001);
  }

  void test_polarization_correction_with_efficiency_workspace_Wildes() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes");
    auto efficiencies = createPolarizationEfficienciesWorkspace("Wildes");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", efficiencies}});

    alg->execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 4);
    TS_ASSERT_EQUALS(outLamGroup.size(), 4);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);
    assert_spectra_in_group_values(outLamGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {0.6552, 0.4330, 0.9766, 0.7544});

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);
    assert_spectra_in_group_values(outQGroup, {0, 1, 2, 3}, {0, 0, 0, 0}, {0.6552, 0.4330, 0.9766, 0.7544});
  }

  void test_parameter_file_used_with_efficiency_workspace_Wildes() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes");
    const auto input_group = retrieveOutWS(TEST_GROUP_NAME);
    // We're setting this to an invalid value to catch it on purpose later.
    input_group[0]->instrumentParameters().addString(input_group[0]->getInstrument()->getComponentID(),
                                                     "WildesFlipperConfig", "01,01,10");
    auto efficiencies = createPolarizationEfficienciesWorkspace("Wildes");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", efficiencies}});

    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::logic_error &e, std::string(e.what()),
                            "Invalid value for property Flippers (string) from string \"01,01,10\": When setting value "
                            "of property \"Flippers\": Each spin state must only appear once");
  }

  void test_error_occurs_when_set_spin_states_used_with_Wildes() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes");
    const auto input_group = retrieveOutWS(TEST_GROUP_NAME);
    // We're setting this to an invalid value to catch it on purpose later.
    input_group[0]->instrumentParameters().addString(input_group[0]->getInstrument()->getComponentID(),
                                                     "WildesFlipperConfig", "00,11,01,10");
    auto efficiencies = createPolarizationEfficienciesWorkspace("Wildes");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04},
                                    {"PolarizationAnalysis", true},
                                    {"PolarizationEfficiencies", efficiencies},
                                    {"FredrikzePolarizationSpinStateOrder", "01,10,11,00"}});

    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), const std::runtime_error &e, std::string(e.what()),
        "A custom spin state order cannot be entered using the FredrikzePolarizationSpinStateOrder property when "
        "performing a Wildes polarization correction. Check you don't have one assigned in the Experiment Settings. "
        "Modify the parameter file for your instrument to change the spin state order.");
  }

  void test_polarization_correction_with_efficiency_workspace_Fredrikze_custom() {
    prepareInputGroup(TEST_GROUP_NAME, "Fredrikze", 2);
    auto efficiencies = createPolarizationEfficienciesWorkspace("Fredrikze");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04},
                                    {"PolarizationAnalysis", true},
                                    {"PolarizationEfficiencies", efficiencies},
                                    {"FredrikzePolarizationSpinStateOrder", "a,p"}});
    alg->execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 2);
    TS_ASSERT_EQUALS(outLamGroup.size(), 2);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 0.2938, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[1]->y(0)[0], 1.4186, 0.0001);

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 0.2938, 0.0001);
    TS_ASSERT_DELTA(outQGroup[1]->y(0)[0], 1.4186, 0.0001);
  }

  void test_polarization_correction_with_efficiency_workspace_Wildes_no_analyser() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes", 2);
    auto efficiencies = createPolarizationEfficienciesWorkspace("Wildes");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", efficiencies}});
    alg->execute();

    auto outQGroup = retrieveOutWS("IvsQ");
    auto outLamGroup = retrieveOutWS("IvsLam");

    TS_ASSERT_EQUALS(outQGroup.size(), 2);
    TS_ASSERT_EQUALS(outLamGroup.size(), 2);

    TS_ASSERT_EQUALS(outLamGroup[0]->blocksize(), 9);
    // X range in outLam
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).front(), 2.0729661466, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[0]->x(0).back(), 14.2963182408, 0.0001);

    TS_ASSERT_DELTA(outLamGroup[0]->y(0)[0], 0.7554, 0.0001);
    TS_ASSERT_DELTA(outLamGroup[1]->y(0)[0], 0.9161, 0.0001);

    TS_ASSERT_EQUALS(outQGroup[0]->blocksize(), 9);

    TS_ASSERT_DELTA(outQGroup[0]->y(0)[0], 0.7554, 0.0001);
    TS_ASSERT_DELTA(outQGroup[1]->y(0)[0], 0.9161, 0.0001);
  }

  void test_polarization_correction_with_invalid_efficiencies_workspace_labels() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes");
    auto efficiencies = createPolarizationEfficienciesWorkspace("Wildes");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", efficiencies}});

    // Set some invalid labels on the efficiencies workspace
    auto &axis = dynamic_cast<TextAxis &>(*efficiencies->getAxis(1));
    for (size_t i = 0; i < axis.length(); ++i) {
      axis.setLabel(i, "test" + std::to_string(i));
    }

    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), std::runtime_error & e, std::string(e.what()),
        "Axes labels for efficiencies workspace do not match any supported polarization correction method");
  }

  void test_polarization_correction_with_invalid_efficiencies_workspace_format() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes");
    auto const inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TEST_GROUP_NAME + "_1");
    auto invalid_format = createFloodWorkspace(inputWS->getInstrument());
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", invalid_format}});

    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error & e, std::string(e.what()),
                            "Efficiencies workspace is not in a supported format");
  }

  void test_polarization_correction_with_efficiencies_workspace_and_invalid_num_input_workspaces() {
    prepareInputGroup(TEST_GROUP_NAME, "Wildes", 3);
    auto efficiencies = createPolarizationEfficienciesWorkspace("Wildes");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0);
    setup_optional_properties(
        alg,
        {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}, {"PolarizationEfficiencies", efficiencies}});

    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error & e, std::string(e.what()),
                            "Only input workspace groups with two or four periods are supported");
  }

  void test_monitor_index_in_group() {
    prepareInputGroup(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "1", 1.0, 5.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"PolarizationAnalysis", true}});

    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::invalid_argument & e, std::string(e.what()),
                            "A detector is expected at workspace index 0 (Was "
                            "converted from specnum), found a monitor");
  }

  void test_I0MonitorIndex_is_detector() {
    prepareInputGroup(TEST_GROUP_NAME);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 5.0);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04},
                                    {"PolarizationAnalysis", true},
                                    {"MonitorBackgroundWavelengthMin", 1.0},
                                    {"MonitorBackgroundWavelengthMax", 5.0},
                                    {"I0MonitorIndex", "1"}});

    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::invalid_argument & e, std::string(e.what()),
                            "A monitor is expected at spectrum index 1");
  }

  void test_QStep_QMin_and_QMax() {
    auto inter = loadRun("INTER00013460.nxs");
    const auto alg = create_refl_algorithm(inter, 0.7, "4");
    setup_optional_properties(
        alg, {{"MomentumTransferStep", 0.1}, {"MomentumTransferMin", 0.1}, {"MomentumTransferMax", 1.0}});

    alg->execute();

    MatrixWorkspace_sptr outQBin = alg->getProperty("OutputWorkspaceBinned");

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
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt);
    setup_optional_properties(alg, {{"CorrectionAlgorithm", "None"}, {"MomentumTransferMin", 0.1}});

    alg->execute();

    MatrixWorkspace_sptr outQbinned = alg->getProperty("OutputWorkspaceBinned");

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
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt);
    setup_optional_properties(alg, {{"CorrectionAlgorithm", "None"}, {"MomentumTransferMax", 0.1}});

    alg->execute();

    MatrixWorkspace_sptr outQBin = alg->getProperty("OutputWorkspaceBinned");

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
    const auto alg = create_refl_algorithm(inter, 0.7, "4");
    alg->setProperty("MomentumTransferMax", 1.0);
    alg->setProperty("MomentumTransferMin", 0.1);

    alg->execute();

    MatrixWorkspace_sptr outQBin = alg->getProperty("OutputWorkspaceBinned");

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
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt);
    setup_optional_properties(alg, {{"CorrectionAlgorithm", "None"}, {"MomentumTransferStep", 0.1}});

    alg->execute();

    MatrixWorkspace_sptr outQBin = alg->getProperty("OutputWorkspaceBinned");

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
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt);
    setup_optional_properties(
        alg, {{"CorrectionAlgorithm", "None"}, {"MomentumTransferStep", 0.1}, {"MomentumTransferMin", 0.1}});

    alg->execute();

    MatrixWorkspace_sptr outQBin = alg->getProperty("OutputWorkspaceBinned");

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
    const auto alg = create_refl_algorithm(inter, 0.7, "4", std::nullopt, std::nullopt);
    setup_optional_properties(
        alg, {{"CorrectionAlgorithm", "None"}, {"MomentumTransferStep", 0.1}, {"MomentumTransferMax", 0.1}});

    alg->execute();

    MatrixWorkspace_sptr outQBin = alg->getProperty("OutputWorkspaceBinned");

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
    auto inputWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto flood = createFloodWorkspace(inputWS->getInstrument());
    const auto alg = create_refl_algorithm(inputWS, 1.5, "2+3");
    // Correct by rotating detectors around the sample
    setup_optional_properties(alg, {{"CorrectionAlgorithm", "None"},
                                    {"MomentumTransferStep", 0.01},
                                    {"AnalysisMode", "MultiDetectorAnalysis"},
                                    {"DetectorCorrectionType", "RotateAroundSample"},
                                    {"FloodWorkspace", flood}});

    alg->execute();

    MatrixWorkspace_sptr out = alg->getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(out->y(0)[0], 4.5, 0.000001);
  }

  void test_flood_correction_transmission() {
    auto inputWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto transWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    for (size_t i = 0; i < transWS->getNumberHistograms(); ++i) {
      auto &y = transWS->mutableY(i);
      y.assign(y.size(), 10.0 * static_cast<double>(i + 1));
    }
    auto flood = createFloodWorkspace(inputWS->getInstrument());

    const auto alg = create_refl_algorithm(inputWS, 1.5, "2+3");
    // Correct by rotating detectors around the sample
    setup_optional_properties(alg, {{"CorrectionAlgorithm", "None"},
                                    {"MomentumTransferStep", 0.01},
                                    {"AnalysisMode", "MultiDetectorAnalysis"},
                                    {"DetectorCorrectionType", "RotateAroundSample"},
                                    {"FloodWorkspace", flood},
                                    {"FirstTransmissionRun", transWS}});

    alg->execute();

    MatrixWorkspace_sptr out = alg->getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(out->y(0)[0], 0.0782608695, 0.000001);
  }

  void test_flood_correction_group() {
    auto inputWS1 = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto inputWS2 = create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    inputWS2 *= 2.0;
    auto group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(inputWS1);
    group->addWorkspace(inputWS2);
    AnalysisDataService::Instance().addOrReplace(TEST_GROUP_NAME, group);
    auto flood = createFloodWorkspace(inputWS1->getInstrument());
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 1.5, "2+3", 1.0, 15.0, true, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.01},
                                    {"AnalysisMode", "MultiDetectorAnalysis"},
                                    {"DetectorCorrectionType", "RotateAroundSample"},
                                    {"FloodWorkspace", flood}});

    alg->execute();

    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IvsQ");
    auto out1 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0));
    TS_ASSERT_DELTA(out1->y(0)[0], 4.5, 0.000001);
    auto out2 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1));
    TS_ASSERT_DELTA(out2->y(0)[0], 9.0, 0.000001);
  }

  void test_flood_correction_polarization_correction() {
    prepareInputGroup(TEST_GROUP_NAME, "Fredrikze");
    applyPolarizationEfficiencies(TEST_GROUP_NAME);
    auto const inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TEST_GROUP_NAME + "_1");
    auto flood = createFloodWorkspace(inputWS->getInstrument(), 257);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0, true, false);
    setup_optional_properties(
        alg, {{"PolarizationAnalysis", true}, {"MomentumTransferStep", 0.04}, {"FloodWorkspace", flood}});

    alg->execute();
    TS_ASSERT(alg->isExecuted());
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IvsQ");
    TS_ASSERT(out);
    auto out1 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0));
    TS_ASSERT_DELTA(out1->y(0)[0], 90.0, 0.001);
    auto out2 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1));
    TS_ASSERT_DELTA(out2->y(0)[0], 80.0, 0.001);
    auto out3 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(2));
    TS_ASSERT_DELTA(out3->y(0)[0], 70.0, 0.003);
    auto out4 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(3));
    TS_ASSERT_DELTA(out4->y(0)[0], 60.0, 0.003);
  }

  void test_flood_correction_parameter_file() {
    prepareInputGroup(TEST_GROUP_NAME, "Flood");
    auto const inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(TEST_GROUP_NAME + "_1");
    auto flood = createFloodWorkspace(inputWS->getInstrument(), 257);
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0, true, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"FloodCorrection", "ParameterFile"}});

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    WorkspaceGroup_sptr out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IvsQ");
    TS_ASSERT(out);
    auto out1 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0));
    TS_ASSERT_DELTA(out1->y(0)[0], 90.0, 1e-15);
    auto out2 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1));
    TS_ASSERT_DELTA(out2->y(0)[0], 80.0, 1e-15);
    auto out3 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(2));
    TS_ASSERT_DELTA(out3->y(0)[0], 70.0, 1e-15);
    auto out4 = std::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(3));
    TS_ASSERT_DELTA(out4->y(0)[0], 60.0, 1e-14);
  }

  void test_flood_correction_parameter_file_no_flood_parameters() {
    prepareInputGroup(TEST_GROUP_NAME, "No_Flood");
    const auto alg = create_refl_algorithm(TEST_GROUP_NAME, 10.0, "2", 1.0, 15.0, true, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"FloodCorrection", "ParameterFile"}});

    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::invalid_argument & e, e.what(),
                            std::string("Instrument parameter file doesn't have the Flood_Run parameter."));
  }

  void test_output_workspace_is_given_informative_name_if_input_has_correct_form() {
    std::string const groupName = "TOF_1234_sliced";
    prepareInputGroup(groupName, "", 2);
    ADS.rename("TOF_1234_sliced_1", "TOF_1234_sliced_first");
    ADS.rename("TOF_1234_sliced_2", "TOF_1234_sliced_second");
    const auto alg = create_refl_algorithm(groupName, 10.0, "2", 1.0, 15.0, false, false);
    alg->setProperty("MomentumTransferStep", 0.04);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    assert_ads_exists({"IvsQ_1234_sliced_first", "IvsQ_1234_sliced_second", "IvsQ_binned_1234_sliced_first",
                       "IvsQ_binned_1234_sliced_second", "IvsLam_1234_sliced_first", "IvsLam_1234_sliced_second"});
  }

  void test_autodetect_on_instrument_with_polynomial_correction() {
    auto ws_1 = createREFL_WS(10, 5000, 10000, std::vector<double>(10, 1), "PolynomialCorrection");
    auto const polStringInter =
        std::string("35.5893,-24.5591,9.20375,-1.89265,0.222291,-0.0148746,0.00052709,-7.66807e-06");
    std::map<std::string, std::string> propertiesToAssert{{"CorrectionAlgorithm", "PolynomialCorrection"},
                                                          {"Polynomial", polStringInter}};
    const auto alg = create_refl_algorithm(ws_1, 0.7, "2", 1.0, 15.0, false, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"CorrectionAlgorithm", "AutoDetect"}});
    alg->execute();

    auto ws_out = ADS.retrieveWS<MatrixWorkspace>("IvsQ");
    check_algorithm_properties_in_child_histories(ws_out, 2, 1, propertiesToAssert);
  }

  void test_autodetect_on_instrument_with_exponential_correction() {
    auto const &ws_1 = createREFL_WS(10, 5000, 10000, std::vector<double>(10, 1), "ExponentialCorrection");
    std::map<std::string, std::string> propertiesToAssert{
        {"CorrectionAlgorithm", "ExponentialCorrection"}, {"C0", "36.568800000000003"}, {"C1", "0.18867600000000001"}};
    const auto alg = create_refl_algorithm(ws_1, 0.7, "2", 1.0, 15.0, false, false);
    setup_optional_properties(alg, {{"MomentumTransferStep", 0.04}, {"CorrectionAlgorithm", "AutoDetect"}});
    alg->execute();

    auto ws_out = ADS.retrieveWS<MatrixWorkspace>("IvsQ");
    check_algorithm_properties_in_child_histories(ws_out, 2, 1, propertiesToAssert);
  }

private:
  MatrixWorkspace_sptr m_notTOF;
  MatrixWorkspace_sptr m_TOF;
  AnalysisDataServiceImpl &ADS = AnalysisDataService::Instance();

  MatrixWorkspace_sptr loadRun(const std::string &run) {
    const auto lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->setChild(true);
    lAlg->initialize();
    lAlg->setProperty("Filename", run);
    lAlg->setPropertyValue("OutputWorkspace", "demo_ws");
    lAlg->execute();
    Workspace_sptr temp = lAlg->getProperty("OutputWorkspace");
    if (temp->isGroup()) {
      const WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(temp);
      temp = group->getItem(0);
    }

    if (const MatrixWorkspace_sptr matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(temp); matrixWS) {
      return matrixWS;
    }
    return {};
  };

  MatrixWorkspace_sptr createFloodWorkspace(const Mantid::Geometry::Instrument_const_sptr &instrument, size_t n = 4) {
    size_t detid = 1;
    auto flood = create2DWorkspace(static_cast<int>(n), 1);
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

  MatrixWorkspace_sptr createPolarizationEfficienciesWorkspace(const std::string &correctionMethod) {
    // The workspace created here takes most of its values from the test parameter files, however the P1/Pp
    // efficiency factor has been changed so that we can distinguish between tests that use a workspace vs those
    // that use the test parameter file
    std::vector<std::string> axisLabels;
    if (correctionMethod == "Wildes") {
      axisLabels = {"P1", "P2", "F1", "F2"};
    } else {
      axisLabels = {"Pp", "Ap", "Rho", "Alpha"};
    }

    std::vector<double> lambda{0, 3, 6, 10, 15, 20};

    std::map<std::string, std::vector<double>> efficiencyFactors;
    efficiencyFactors[axisLabels[0]] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
    efficiencyFactors[axisLabels[1]] = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8};
    efficiencyFactors[axisLabels[2]] = {0.778, 0.778, 0.778, 0.778, 0.778, 0.778};
    efficiencyFactors[axisLabels[3]] = {0.75, 0.75, 0.75, 0.75, 0.75, 0.75};

    auto join_alg = AlgorithmManager::Instance().create("JoinISISPolarizationEfficiencies");
    join_alg->setChild(true);
    join_alg->initialize();

    for (const auto &label : axisLabels) {
      Points xVals(lambda);
      Counts yVals(efficiencyFactors[label]);
      CountStandardDeviations eVals(std::vector<double>(efficiencyFactors[label].size()));
      auto factorWs = std::make_shared<Workspace2D>();
      factorWs->initialize(1, Histogram(xVals, yVals, eVals));

      join_alg->setProperty(label, factorWs);
    }
    join_alg->setPropertyValue("OutputWorkspace", "efficiencies");
    join_alg->execute();
    return join_alg->getProperty("OutputWorkspace");
  }

  void prepare_group_with_run_number(const std::string &groupName = "testGroup",
                                     const std::string &runNumber = "1234") {
    prepareInputGroup(groupName);
    WorkspaceGroup_sptr group = ADS.retrieveWS<WorkspaceGroup>(groupName);
    MatrixWorkspace_sptr ws = ADS.retrieveWS<MatrixWorkspace>(group->getNames()[0]);
    ws->mutableRun().addProperty<std::string>("run_number", runNumber);
  }

  IAlgorithm_sptr create_refl_algorithm(const std::variant<MatrixWorkspace_sptr, std::string> &ws,
                                        const std::optional<double> &theta = std::nullopt,
                                        const std::optional<const char *> &processingInstructions = std::nullopt,
                                        const std::optional<double> &wavMin = 1.0,
                                        const std::optional<double> &wavMax = 15.0, const bool setDefaultOutput = true,
                                        const bool isChild = true) {
    const auto alg = std::make_shared<ReflectometryReductionOneAuto3>();
    alg->initialize();
    alg->setChild(isChild);

    if (std::holds_alternative<MatrixWorkspace_sptr>(ws)) {
      alg->setProperty("InputWorkspace", std::get<MatrixWorkspace_sptr>(ws));
    } else {
      alg->setPropertyValue("InputWorkspace", std::get<std::string>(ws));
    }

    if (theta) {
      alg->setProperty("ThetaIn", *theta);
    }

    if (processingInstructions) {
      alg->setProperty("ProcessingInstructions", *processingInstructions);
    }

    if (wavMin && wavMax) {
      alg->setProperty("WavelengthMin", *wavMin);
      alg->setProperty("WavelengthMax", *wavMax);
    }

    if (setDefaultOutput) {
      alg->setPropertyValue("OutputWorkspace", "IvsQ");
      alg->setPropertyValue("OutputWorkspaceBinned", "IvsQ_binned");
      alg->setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    }

    return alg;
  }

  void compare_detectors_in_out(const Instrument_const_sptr &instIn, const Instrument_const_sptr &instOut,
                                const std::vector<std::string> &unmovedComponents,
                                const std::optional<std::string> &movedComponent = std::nullopt,
                                const std::vector<double> &movedPositions = {0.0}) {
    // The following components should not have been moved
    for (const auto &componentName : unmovedComponents) {
      TS_ASSERT_EQUALS(instIn->getComponentByName(componentName)->getPos(),
                       instOut->getComponentByName(componentName)->getPos())
    }
    if (movedComponent) {
      const auto movedIn = instIn->getComponentByName(*movedComponent)->getPos();
      const auto movedOut = instOut->getComponentByName(*movedComponent)->getPos();
      if (movedComponent == "point-detector") {
        // Only 'point-detector' should have been moved vertically (along Y)
        TS_ASSERT_EQUALS(movedIn.X(), movedOut.X());
        TS_ASSERT_EQUALS(movedIn.Z(), movedOut.Z());
        TS_ASSERT_DIFFERS(movedIn.Y(), movedOut.Y());
        TS_ASSERT_DELTA(movedIn.Y() / (movedOut.Z() - instOut->getSample()->getPos().Z()), movedPositions.at(0), 1e-4);
      } else if (movedComponent == "OSMOND") {
        // 'OSMOND' should have been moved both vertically and in the beam direction (along X and Z)
        TS_ASSERT_DELTA(movedOut.X(), movedPositions.at(0), 1e-4);
        TS_ASSERT_EQUALS(movedIn.Y(), movedOut.Y());
        TS_ASSERT_DELTA(movedOut.Z(), movedPositions.at(1), 1e-4);
      }
    }
  }

  void setup_optional_properties(const IAlgorithm_sptr &alg, const std::map<std::string, propVariant> &props) {
    for (const auto &[name, value] : props) {
      if (std::holds_alternative<double>(value)) {
        alg->setProperty(name, std::get<double>(value));
      } else if (std::holds_alternative<std::string>(value)) {
        alg->setProperty(name, std::get<std::string>(value));
      } else if (std::holds_alternative<bool>(value)) {
        alg->setProperty(name, std::get<bool>(value));
      } else if (std::holds_alternative<MatrixWorkspace_sptr>(value)) {
        alg->setProperty(name, std::get<MatrixWorkspace_sptr>(value));
      }
    }
  }

  void assert_size(const MatrixWorkspace_sptr &ws, const std::optional<int> &expectedHistNo = std::nullopt,
                   const std::optional<int> &expectedBlockSize = std::nullopt,
                   const std::optional<int> &expectedSize = std::nullopt, const bool assertEdges = true) {
    if (expectedHistNo) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), *expectedHistNo);
    }
    if (expectedBlockSize) {
      TS_ASSERT_EQUALS(ws->blocksize(), *expectedBlockSize);
    }
    if (expectedSize) {
      const auto refSize = assertEdges ? ws->binEdges(0).size() : ws->counts(0).size();
      TS_ASSERT_EQUALS(refSize, *expectedSize);
    }
  }

  void assert_bin_values(const MatrixWorkspace_sptr &ws, bool compareEdges, const std::vector<size_t> &indexList,
                         const std::vector<double> &values, const int wsIndex = 0) {
    for (size_t index = 0; index < indexList.size(); ++index) {
      const auto refValue =
          compareEdges ? ws->binEdges(wsIndex)[indexList.at(index)] : ws->counts(wsIndex)[indexList.at(index)];
      TS_ASSERT_DELTA(refValue, values.at(index), 1e-4);
    }
  }

  void assert_spectra_in_group_values(const std::vector<MatrixWorkspace_sptr> &group, const std::vector<int> &grpIdx,
                                      const std::vector<int> &spIdx, const std::vector<double> &expectedValues,
                                      bool testY = true) {
    for (auto index = 0; index < static_cast<int>(expectedValues.size()); index++) {
      const auto refValue =
          testY ? group.at(grpIdx[index])->y(0)[spIdx[index]] : group.at(grpIdx[index])->x(0)[spIdx[index]];
      TS_ASSERT_DELTA(refValue, expectedValues[index], 1e-4);
    }
  }

  void assert_ads_exists(const std::vector<std::string> &wsNames) {
    for (const auto &name : wsNames) {
      TS_ASSERT(ADS.doesExist(name));
    }
  }

  void clear_instrument_cache() {
    // REFL instrument keeps cache of parameters that are needed only for specific tests
    ClearCache clearAlg;
    clearAlg.initialize();
    clearAlg.setProperty("InstrumentCache", true);
    clearAlg.execute();
  }
  void check_algorithm_properties_in_child_histories(MatrixWorkspace_sptr &workspace, int topLevelIdx,
                                                     int childLevelIdx,
                                                     std::map<std::string, std::string> const &propValues) {
    auto const selectedParentHistory = workspace->getHistory().getAlgorithmHistory(topLevelIdx);
    auto const selectedChildHistories = selectedParentHistory->getChildHistories()[childLevelIdx];
    for (const auto &[prop, value] : propValues) {
      TS_ASSERT_EQUALS(selectedChildHistories->getPropertyValue(prop), value);
    }
  }

  void check_output_group_contains_sample_logs_for_spin_state_ORSO(std::vector<MatrixWorkspace_sptr> const &wsGroup,
                                                                   bool has_sample_logs = false) {
    for (auto const &ws : wsGroup) {
      TS_ASSERT_EQUALS(ws->mutableRun().hasProperty(SpinStatesORSO::LOG_NAME), has_sample_logs);
    }
  }
};
