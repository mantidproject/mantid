// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/PolarizationCorrections/DepolarizedAnalyserTransmission.h"

namespace {
constexpr double PXD_VALUE = 9.32564;
constexpr double PXD_ERROR = 7.92860;
constexpr double COST_FUNC_MAX = 3.3e-5;

constexpr double FIT_DELTA = 1e-6;
} // namespace

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::DepolarizedAnalyserTransmission;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;

class DepolarizedAnalyserTransmissionTest : public CxxTest::TestSuite {
public:
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
    auto const &[mtWs, depWs] = createDefaultWorkspaces();

    auto alg = createAlgorithm(mtWs, depWs);
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
    auto const &[mtWs, depWs] = createDefaultWorkspaces();
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
    auto const &[mtWs, depWs] = createDefaultWorkspaces();
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
    auto const &[mtWs, depWs] = createDefaultWorkspaces();
    auto alg = createAlgorithm(mtWs, depWs);
    alg->setProperty("PxDStartingValue", 1e50);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Failed to fit to transmission workspace, : Changes in function value are too small");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_apparently_successful_fit() {
    // GIVEN
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "name=UserFunction, Formula=0*x");
    auto const &depWs = createTestingWorkspace("__dep", "name=ExpDecay, Height=0.1239, Lifetime=1.338");
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
    MatrixWorkspace_sptr const &mtWs =
        createTestingWorkspace("__mt", "name=LinearBackground, A0=0.112, A1=-0.004397", 12);
    auto const &depWs = createTestingWorkspace("__dep", "name=ExpDecay, Height=0.1239, Lifetime=1.338", 2);
    auto alg = createAlgorithm(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n DepolarizedWorkspace: DepolarizedWorkspace must "
                            "contain a single spectrum. Contains 2 spectra.\n EmptyCellWorkspace: EmptyCellWorkspace "
                            "must contain a single spectrum. Contains 12 spectra.");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_non_matching_workspace_bins() {
    // GIVEN
    MatrixWorkspace_sptr const &mtWs =
        createTestingWorkspace("__mt", "name=LinearBackground, A0=0.112, A1=-0.004397", 1, true, 0.2);
    auto const &depWs = createTestingWorkspace("__dep", "name=ExpDecay, Height=0.1239, Lifetime=1.338", 1);
    auto alg = createAlgorithm(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n DepolarizedWorkspace: The bins in the "
                            "DepolarizedWorkspace and EmptyCellWorkspace do not match.");
    TS_ASSERT(!alg->isExecuted());
  }

private:
  MatrixWorkspace_sptr createTestingWorkspace(std::string const &outName, std::string const &function,
                                              int const numSpectra = 1, bool const isMonitor = true,
                                              double const binWidth = 0.1) {
    CreateSampleWorkspace makeWsAlg;
    makeWsAlg.initialize();
    makeWsAlg.setChild(true);
    makeWsAlg.setPropertyValue("OutputWorkspace", outName);
    makeWsAlg.setPropertyValue("Function", "User Defined");
    makeWsAlg.setPropertyValue("UserDefinedFunction", function);
    makeWsAlg.setPropertyValue("XUnit", "wavelength");
    if (isMonitor) {
      makeWsAlg.setProperty("NumBanks", 0);
      makeWsAlg.setProperty("NumMonitors", numSpectra);
    } else {
      makeWsAlg.setProperty("NumBanks", numSpectra);
    }
    makeWsAlg.setProperty("BankPixelWidth", 1);
    makeWsAlg.setProperty("XMin", 3.5);
    makeWsAlg.setProperty("XMax", 16.5);
    makeWsAlg.setProperty("BinWidth", binWidth);
    makeWsAlg.execute();
    return makeWsAlg.getProperty("OutputWorkspace");
  }

  std::pair<MatrixWorkspace_sptr, MatrixWorkspace_sptr> createDefaultWorkspaces() {
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "name=LinearBackground, A0=0.112, A1=-0.004397");
    auto const &depWs = createTestingWorkspace("__dep", "name=ExpDecay, Height=0.1239, Lifetime=1.338");
    return std::pair{mtWs, depWs};
  }

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

  void validateOutputParameters(ITableWorkspace_sptr const &paramsWs) {
    TS_ASSERT_DELTA(paramsWs->getColumn("Value")->toDouble(0), PXD_VALUE, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(paramsWs->getColumn("Error")->toDouble(0), PXD_ERROR, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_LESS_THAN(paramsWs->getColumn("Value")->toDouble(1), COST_FUNC_MAX);
  }
};
