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
constexpr double T_E_VALUE = 82593.9;
constexpr double PXD_VALUE = 14.9860;
constexpr double T_E_ERROR = 26088049.0;
constexpr double PXD_ERROR = 467.994241;
constexpr double T_E_COV = 680586304785150.26;
constexpr double PXD_COV = 219018.61;
constexpr double OFF_DIAG_COV = 11828693410.21;
constexpr double COST_FUNC_MAX = 5e-15;

constexpr double FIT_DELTA = 1e-6;
constexpr double COV_DELTA = 1e-10;
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
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "1.465e-07*exp(0.0733*4.76*x)");
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)");

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
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "1.465e-07*exp(0.0733*4.76*x)");
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)");
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

  void test_covariance_matrix_is_output_when_optional_prop_set() {
    // GIVEN
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "1.465e-07*exp(0.0733*4.76*x)");
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)");
    auto alg = createAlgorithm(mtWs, depWs);
    alg->setPropertyValue("OutputCovarianceMatrix", "__unused_for_child");
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    ITableWorkspace_sptr const &outputWs = alg->getProperty("OutputWorkspace");
    ITableWorkspace_sptr const &covWs = alg->getProperty("OutputCovarianceMatrix");
    validateOutputParameters(outputWs);
    TS_ASSERT_DELTA(covWs->getColumn("T_E")->toDouble(0), T_E_COV, T_E_COV * COV_DELTA);
    TS_ASSERT_DELTA(covWs->getColumn("T_E")->toDouble(1), OFF_DIAG_COV, OFF_DIAG_COV * COV_DELTA);
    TS_ASSERT_DELTA(covWs->getColumn("pxd")->toDouble(0), OFF_DIAG_COV, OFF_DIAG_COV * COV_DELTA);
    TS_ASSERT_DELTA(covWs->getColumn("pxd")->toDouble(1), PXD_COV, PXD_COV * COV_DELTA);
  }

  void test_different_start_end_x() {
    // GIVEN
    auto constexpr T_E_ERROR_DIFX = 25693283.9;
    auto constexpr PXD_ERROR_DIFX = 458.447482;
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "1.465e-07*exp(0.0733*4.76*x)");
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)");

    auto alg = createAlgorithm(mtWs, depWs);
    alg->setProperty("StartX", 1.5);
    alg->setProperty("EndX", 14.5);
    alg->execute();

    // THEN
    TS_ASSERT(alg->isExecuted());
    ITableWorkspace_sptr const &outputWs = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr const &fitWs = alg->getProperty("OutputFitCurves");
    TS_ASSERT_DELTA(outputWs->getColumn("Value")->toDouble(0), T_E_VALUE, T_E_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(outputWs->getColumn("Value")->toDouble(1), PXD_VALUE, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(outputWs->getColumn("Error")->toDouble(0), T_E_ERROR_DIFX, T_E_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(outputWs->getColumn("Error")->toDouble(1), PXD_ERROR_DIFX, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_LESS_THAN(outputWs->getColumn("Value")->toDouble(2), COST_FUNC_MAX);
    TS_ASSERT_EQUALS(fitWs, nullptr)
  }

  void test_failed_fit() {
    // GIVEN
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "1.465e-07*exp(0.0733*4.76*x)");
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)");
    auto alg = createAlgorithm(mtWs, depWs);
    alg->setProperty("TEStartingValue", 1e50);
    alg->setProperty("PxDStartingValue", 1e50);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Failed to fit to transmission workspace, : Changes in function value are too small");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_apparently_successful_fit() {
    // GIVEN
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "0*x");
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)");
    auto alg = createAlgorithm(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Failed to fit to transmission workspace, : Fit quality (chi-squared) is too poor "
                            "(0.000000. Should be 0 < x < 1). You may want to check that the corrects spectrum "
                            "and starting fitting values were provided.");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_invalid_workspace_lengths() {
    // GIVEN
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "1.465e-07*exp(0.0733*4.76*x)", 12);
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)", 2);
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
    MatrixWorkspace_sptr const &mtWs = createTestingWorkspace("__mt", "1.465e-07*exp(0.0733*4.76*x)", 1, true, 0.2);
    auto const &depWs = createTestingWorkspace("__dep", "0.0121*exp(-0.0733*10.226*x)", 1);
    auto alg = createAlgorithm(mtWs, depWs);

    // THEN
    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n DepolarizedWorkspace: The bins in the "
                            "DepolarizedWorkspace and EmptyCellWorkspace do not match.");
    TS_ASSERT(!alg->isExecuted());
  }

private:
  MatrixWorkspace_sptr createTestingWorkspace(std::string const &outName, std::string const &formula,
                                              int const numSpectra = 1, bool const isMonitor = true,
                                              double const binWidth = 0.1) {
    CreateSampleWorkspace makeWsAlg;
    makeWsAlg.initialize();
    makeWsAlg.setChild(true);
    makeWsAlg.setPropertyValue("OutputWorkspace", outName);
    makeWsAlg.setPropertyValue("Function", "User Defined");
    makeWsAlg.setPropertyValue("UserDefinedFunction", "name=UserFunction, Formula=" + formula);
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
    TS_ASSERT_DELTA(paramsWs->getColumn("Value")->toDouble(0), T_E_VALUE, T_E_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(paramsWs->getColumn("Value")->toDouble(1), PXD_VALUE, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(paramsWs->getColumn("Error")->toDouble(0), T_E_ERROR, T_E_VALUE * FIT_DELTA);
    TS_ASSERT_DELTA(paramsWs->getColumn("Error")->toDouble(1), PXD_ERROR, PXD_VALUE * FIT_DELTA);
    TS_ASSERT_LESS_THAN(paramsWs->getColumn("Value")->toDouble(2), COST_FUNC_MAX);
  }
};
