// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/PolarizationCorrections/DepolarizedAnalyserTransmission.h"
#include "MantidKernel/Strings.h"

#include "PolarizationCorrectionsTestUtils.h"

namespace {
constexpr double PXD_VALUE = 9.32564;
constexpr double PXD_ERROR = 7.92860;
constexpr double COST_FUNC_MAX = 3.3e-5;

constexpr double FIT_DELTA = 1e-6;
constexpr double X_MIN = 3.5;
constexpr double X_MAX = 16.5;
} // namespace

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::DepolarizedAnalyserTransmission;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using namespace PolCorrTestUtils;

class DepolarizedAnalyserTransmissionTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_parameters = TestWorkspaceParameters("__mt", "name=LinearBackground, A0=0.112, A1=-0.004397", "Wavelength",
                                           N_SPECS, X_MIN, X_MAX, 0.1);
    auto const &mtWs = generateFunctionDefinedWorkspace(m_parameters);
    m_parameters.updateNameAndFunc("__dep", "name=ExpDecay, Height=0.1239, Lifetime=1.338");
    auto const &depWs = generateFunctionDefinedWorkspace(m_parameters);
    m_testWs = std::make_pair(mtWs, depWs);
  }

  void test_name() {
    DepolarizedAnalyserTransmission const alg;
    TS_ASSERT_EQUALS(alg.name(), "DepolarizedAnalyserTransmission");
  }

  void test_version() {
    DepolarizedAnalyserTransmission const alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_normal_exec() {
    // GIVEN
    auto const &[mtWs, depWs] = m_testWs;

    auto alg = createAlgorithm(mtWs, depWs);
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    ITableWorkspace_sptr const &outputWs = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr const &fitWs = alg->getProperty("OutputFitCurves");
    validateOutputParameters(outputWs);
    TS_ASSERT_EQUALS(fitWs, nullptr)
  }

  void test_normal_exec_with_file() {
    // GIVEN
    auto const &[mtWs, depWs] = m_testWs;
    auto alg = createAlgorithmUsingFilename(mtWs, depWs);
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    ITableWorkspace_sptr const &outputWs = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr const &fitWs = alg->getProperty("OutputFitCurves");
    validateOutputParameters(outputWs);
    TS_ASSERT_EQUALS(fitWs, nullptr)
  }

  void test_fit_ws_is_output_when_optional_prop_set() {
    // GIVEN
    auto const &[mtWs, depWs] = m_testWs;
    auto alg = createAlgorithm(mtWs, depWs);

    // WHEN
    alg->setPropertyValue("OutputFitCurves", "__unused_for_child");
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    ITableWorkspace_sptr const &outputWs = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr const &fitWs = alg->getProperty("OutputFitCurves");
    validateOutputParameters(outputWs);
    TS_ASSERT_EQUALS(fitWs->getNumberHistograms(), 3);
  }

  void test_different_start_end_x() {
    // GIVEN
    auto constexpr PXD_VALUE_DIFX = 9.3256240143;
    auto constexpr PXD_ERROR_DIFX = 7.9249356146;
    auto const &[mtWs, depWs] = m_testWs;
    auto alg = createAlgorithm(mtWs, depWs);
    alg->setProperty("StartX", 1.5);
    alg->setProperty("EndX", 14.5);
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    ITableWorkspace_sptr const &outputWs = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr const &fitWs = alg->getProperty("OutputFitCurves");
    TS_ASSERT_DELTA(outputWs->getColumn("Value")->toDouble(0), PXD_VALUE_DIFX, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(outputWs->getColumn("Error")->toDouble(0), PXD_ERROR_DIFX, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_LESS_THAN(outputWs->getColumn("Value")->toDouble(1), COST_FUNC_MAX);
    TS_ASSERT_EQUALS(fitWs, nullptr)
  }

  void test_failed_fit() {
    // GIVEN
    auto const &[mtWs, depWs] = m_testWs;
    auto alg = createAlgorithm(mtWs, depWs);
    alg->setProperty("PxDStartingValue", 1e50);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Failed to fit to transmission workspace, : Changes in function value are too small");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_apparently_successful_fit() {
    // GIVEN
    m_parameters.updateNameAndFunc("__mt", "name=UserFunction, Formula=0*x");
    MatrixWorkspace_sptr const &mtWs = generateFunctionDefinedWorkspace(m_parameters);
    auto const &depWs = m_testWs.second;
    auto alg = createAlgorithm(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Failed to fit to transmission workspace, : Fit quality (chi-squared) is too poor "
                            "(0.000000. Should be 0 < x < 1). You may want to check that the correct spectrum "
                            "and starting fitting values were provided.");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_invalid_workspace_lengths() {
    // GIVEN
    m_parameters.updateSpectra(12, X_MIN, X_MAX, 0.1);
    MatrixWorkspace_sptr const &mtWs = generateFunctionDefinedWorkspace(m_parameters);
    m_parameters.updateSpectra(2, X_MIN, X_MAX, 0.1);
    MatrixWorkspace_sptr const &depWs = generateFunctionDefinedWorkspace(m_parameters);
    auto alg = createAlgorithm(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n DepolarizedWorkspace: DepolarizedWorkspace must "
                            "contain a single spectrum. Contains 2 spectra.\n EmptyCellWorkspace: EmptyCellWorkspace "
                            "must contain a single spectrum. Contains 12 spectra.");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_invalid_empty_cell_workspace_length_from_file() {
    // GIVEN
    m_parameters.updateSpectra(12, X_MIN, X_MAX, 0.1);
    MatrixWorkspace_sptr const &mtWs = generateFunctionDefinedWorkspace(m_parameters);
    auto const &depWs = m_testWs.second;
    auto alg = createAlgorithmUsingFilename(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n EmptyCellFilename: EmptyCellFilename "
                            "must contain a single spectrum. Contains 12 spectra.");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_non_matching_workspace_bins() {
    // GIVEN
    m_parameters.updateSpectra(1, X_MIN, X_MAX, 0.2);
    MatrixWorkspace_sptr const &mtWs = generateFunctionDefinedWorkspace(m_parameters);
    auto const &depWs = m_testWs.second;
    auto alg = createAlgorithm(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n DepolarizedWorkspace: The bins in the "
                            "DepolarizedWorkspace and EmptyCellWorkspace do not match.");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_error_if_neither_empty_cell_workspace_or_file_are_set() {
    // GIVEN
    auto const &depWs = m_testWs.first;

    auto alg = std::make_shared<DepolarizedAnalyserTransmission>();
    alg->setChild(true);
    alg->initialize();
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("DepolarizedWorkspace", depWs);
    alg->setPropertyValue("OutputWorkspace", "__unused_for_child");

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n EmptyCellWorkspace: Must set either EmptyCellWorkspace "
                            "or EmptyCellFilename.");
    TS_ASSERT(!alg->isExecuted());
  }

private:
  TestWorkspaceParameters m_parameters;
  std::pair<MatrixWorkspace_sptr, MatrixWorkspace_sptr> m_testWs;

  std::shared_ptr<DepolarizedAnalyserTransmission> createAlgorithm(MatrixWorkspace_sptr const &mtWs,
                                                                   MatrixWorkspace_sptr const &depWs) {
    auto alg = std::make_shared<DepolarizedAnalyserTransmission>();
    alg->setChild(true);
    alg->initialize();
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("DepolarizedWorkspace", depWs);
    alg->setProperty("EmptyCellWorkspace", mtWs);
    alg->setPropertyValue("OutputWorkspace", "__unused_for_child");
    return alg;
  }

  std::shared_ptr<DepolarizedAnalyserTransmission> createAlgorithmUsingFilename(MatrixWorkspace_sptr const &mtWs,
                                                                                MatrixWorkspace_sptr const &depWs) {

    std::filesystem::path filePath = std::filesystem::temp_directory_path() / Mantid::Kernel::Strings::randomString(8);

    auto saveAlg = Mantid::API::AlgorithmManager::Instance().create("SaveNexus");
    saveAlg->setChild(true);
    saveAlg->initialize();
    saveAlg->setProperty("Filename", filePath.string());
    saveAlg->setProperty("InputWorkspace", mtWs);
    saveAlg->execute();
    TS_ASSERT(std::filesystem::exists(filePath));

    auto alg = std::make_shared<DepolarizedAnalyserTransmission>();
    alg->setChild(true);
    alg->initialize();
    TS_ASSERT(alg->isInitialized());
    alg->setProperty("DepolarizedWorkspace", depWs);
    alg->setProperty("EmptyCellFilename", filePath.string());
    alg->setPropertyValue("OutputWorkspace", "__unused_for_child");
    return alg;
  }

  void validateOutputParameters(ITableWorkspace_sptr const &paramsWs) {
    TS_ASSERT_DELTA(paramsWs->getColumn("Value")->toDouble(0), PXD_VALUE, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(paramsWs->getColumn("Error")->toDouble(0), PXD_ERROR, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_LESS_THAN(paramsWs->getColumn("Value")->toDouble(1), COST_FUNC_MAX);
  }
};
