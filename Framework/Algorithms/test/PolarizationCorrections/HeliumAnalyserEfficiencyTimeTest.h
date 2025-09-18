// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiencyTime.h"

#include "PolarizationCorrectionsTestUtils.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace PolCorrTestUtils;

class HeliumAnalyserEfficiencyTimeTest : public CxxTest::TestSuite {
public:
  void setUp() override { xy = createXYFromParams(); }
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_name() {
    HeliumAnalyserEfficiencyTime alg;
    TS_ASSERT_EQUALS(alg.name(), "HeliumAnalyserEfficiencyTime");
  }

  void test_init() {
    HeliumAnalyserEfficiencyTime alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }
  void test_algorithm_throws_for_non_wavelength_workspace() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second, "TOF");
    const auto heAlgorithm = AlgorithmManager::Instance().create(HE_ANALYZER_TIME_ALG);
    heAlgorithm->initialize();

    TS_ASSERT_THROWS_EQUALS(heAlgorithm->setProperty("InputWorkspace", ws), std::invalid_argument const &e,
                            std::string(e.what()), "Workspace must have time logs and Wavelength units");
  }

  void test_algorithm_throws_for_workspace_without_time_logs() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second);
    auto &run = ws->mutableRun();
    run.removeProperty("start_time");
    run.removeProperty("run_start");
    run.removeProperty("end_time");
    run.removeProperty("run_end");

    const auto heAlgorithm = AlgorithmManager::Instance().create(HE_ANALYZER_TIME_ALG);
    heAlgorithm->initialize();

    TS_ASSERT_THROWS_EQUALS(heAlgorithm->setProperty("InputWorkspace", ws), std::invalid_argument const &e,
                            std::string(e.what()), "Workspace must have time logs and Wavelength units");
  }

  void test_algorithm_throws_when_no_timestamp_is_provided_in_any_way() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second);
    const auto alg = prepareHeTimeAlgorithm(ws);

    TS_ASSERT_THROWS_EQUALS(alg->execute(), std::runtime_error const &e, std::string(e.what()),
                            "Some invalid Properties found: \n ReferenceWorkspace: Both ReferenceWorkspace and "
                            "ReferenceTimeStamp properties are empty, "
                            "at least one of the two has to be supplied to execute the Algorithm");
  }

  void test_algorithm_executes_for_default_timestamp() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second);
    const auto alg = prepareHeTimeAlgorithm(ws, REF_TIMESTAMP);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
  }

  void test_algorithm_can_accept_groups_as_input() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second);
    const auto group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(ws);
    AnalysisDataService::Instance().addOrReplace(GROUP_NAME, group);
    const auto alg = prepareHeTimeAlgorithm(group, REF_TIMESTAMP);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
  }

  void test_algorithm_output_with_string_time_stamp_input() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second, X_UNIT, 1, 0, "2025-07-01T09:00:00");
    const auto alg = prepareHeTimeAlgorithm(ws, REF_TIMESTAMP);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUTPUT_NAME);
    const auto y = generateOutputFunc(xy.first, createFunctionArgument());
    TS_ASSERT_DELTA(out->dataY(0), y, DELTA);
  }

  void test_algorithm_unpolarized_transmission_output_with_string_time_stamp_input() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second, X_UNIT, 1, 0, "2025-07-01T09:00:00");
    const auto alg = prepareHeTimeAlgorithm(ws, REF_TIMESTAMP);
    const std::string outUnpolarized = OUTPUT_NAME + "unpol";
    alg->setProperty("UnpolarizedTransmission", outUnpolarized);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outUnpolarized);
    const auto y =
        generateOutputFunc(xy.first, createFunctionArgument(), LAMBDA_CONVERSION_FACTOR * DEFAULT_PXD, false);
    TS_ASSERT_DELTA(out->dataY(0), y, DELTA);
  }

  void test_algorithm_output_with_reference_workspace_input() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second, X_UNIT, 1, 0, "2025-07-01T09:00:00");
    const auto ref = generateWorkspace(REFERENCE_NAME, xy.first, xy.second, X_UNIT, 1, 0, REF_TIMESTAMP);

    const auto alg = prepareHeTimeAlgorithm(ws, "", ref);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUTPUT_NAME);
    const auto y = generateOutputFunc(xy.first, createFunctionArgument());
    TS_ASSERT_DELTA(out->dataY(0), y, DELTA);
  }

  void test_reference_workspace_takes_precedence_over_timestamp_if_both_are_provided() {
    const auto ws = generateWorkspace(INPUT_NAME, xy.first, xy.second, X_UNIT, 1, 0, "2025-07-01T09:00:00");
    const auto ref = generateWorkspace(REFERENCE_NAME, xy.first, xy.second, X_UNIT, 1, 0, REF_TIMESTAMP);

    const auto alg = prepareHeTimeAlgorithm(ws, "2011-11-11T11:11:11", ref);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUTPUT_NAME);
    const auto y = generateOutputFunc(xy.first, createFunctionArgument());
    TS_ASSERT_DELTA(out->dataY(0), y, DELTA)
  }

private:
  std::pair<std::vector<double>, std::vector<double>> xy;
};
