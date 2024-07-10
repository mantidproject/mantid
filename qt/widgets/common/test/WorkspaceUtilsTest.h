// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

#include <QPair>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace Mantid::IndirectFitDataCreationHelper;

class WorkspaceUtilsTest : public CxxTest::TestSuite {
public:
  static WorkspaceUtilsTest *createSuite() { return new WorkspaceUtilsTest(); }

  static void destroySuite(WorkspaceUtilsTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_getXRangeFromWorkspace_returns_valid_range_within_default_precision() {
    auto const testWorkspace = createWorkspace(1, 5);
    auto const xRange = testWorkspace->x(0);
    TS_ASSERT_DELTA(getXRangeFromWorkspace(testWorkspace).first, xRange.front(), 1e-5);
    TS_ASSERT_DELTA(getXRangeFromWorkspace(testWorkspace).second, xRange.back(), 1e-5);
  }

  void test_getWorkspaceSuffix_returns_proper_suffix() {
    auto const suffix1 = "test_wkspace_file_1__red";
    auto const suffix2 = "test_wkspace_file_2_results_sqw";
    auto const noSuffix = "plainOldWorkspace.nsx";

    TS_ASSERT_EQUALS(getWorkspaceSuffix(suffix1), "red");
    TS_ASSERT_EQUALS(getWorkspaceSuffix(suffix2), "sqw");
    TS_ASSERT_EQUALS(getWorkspaceSuffix(noSuffix), std::string());
  }
  void test_getWorkspaceBasename_returns_proper_basename() {
    auto const workspaceName = "irs26176_graphite002_red";
    auto const noSuffix = "plainOldWorkspace";

    TS_ASSERT_EQUALS(getWorkspaceBasename(workspaceName), "irs26176_graphite002");
    TS_ASSERT_EQUALS(getWorkspaceBasename(noSuffix), "plainOldWorkspace");
  }

  void test_getResolutionFromWs_returns_false_for_no_instrument_workspace() {
    auto res = QPair<double, double>(0, 0);
    auto const testWorkspace = createWorkspace(1, 5);

    TS_ASSERT(!getResolutionRangeFromWs(testWorkspace, res));
  }
  void test_getResolutionFromWs_returns_res_for_instrument_workspace() {
    auto res = QPair<double, double>(0, 0);
    auto const testWorkspace = createWorkspaceWithIndirectInstrumentAndParameters();
    getResolutionRangeFromWs(testWorkspace, res);

    TS_ASSERT_DELTA(res.first, -0.0175, 1e-5);
    TS_ASSERT_DELTA(res.second, 0.0175, 1e-5);
  }

  void test_getEmode_defaults_to_indirect_with_InelasticWorkspace() {
    auto const testWorkspace = createWorkspaceWithInelasticInstrument(2);

    TS_ASSERT_EQUALS(getEMode(testWorkspace), "Indirect");
  }

  void test_getEFixed_returns_std_nullopt_for_no_instrument() {
    auto const testWorkspace = createWorkspace(5);
    TS_ASSERT(!getEFixed(testWorkspace));
  }

  void test_getEFixed_returns_std_nullopt_for_instrument_but_no_efixed_parameter() {
    auto const testWorkspace = createWorkspaceWithInelasticInstrument(2);
    TS_ASSERT(!getEFixed(testWorkspace));
  }

  void test_getEFixed_returns_an_efixed_for_a_workspace_with_parameters() {
    auto const testWorkspace = createWorkspaceWithIndirectInstrumentAndParameters();

    auto const efixed = getEFixed(testWorkspace);

    TS_ASSERT(efixed);
    TS_ASSERT_EQUALS(1.845, *efixed);
  }

  void test_getEFixed_returns_an_efixed_for_fmica_analyser() {
    auto const testWorkspace = createWorkspaceWithIndirectInstrumentAndParameters("fmica");

    auto const efixed = getEFixed(testWorkspace);

    TS_ASSERT(efixed);
    TS_ASSERT_DELTA(0.2067, *efixed, 0.00001);
  }

  void test_extractAxisLabels_gives_labels() {
    std::vector<std::string> const labels = {"A.a", "B.b", "C.c"};
    auto const textWorkspace = createWorkspaceWithTextAxis(3, labels);

    TS_ASSERT_EQUALS(extractAxisLabels(textWorkspace, 1).size(), 3);
  }

  void test_extractAxisLabels_gives_empty_labels_for_no_TextAxis() {
    auto const testWorkspace = createWorkspace(3);

    TS_ASSERT_EQUALS(extractAxisLabels(testWorkspace, 1).size(), 0);
  }

  void test_parseRunNumber_calls_with_different_inputs() {
    std::vector<std::string> workspaces_with_run_numbers{"irs123_test", "irs280_test", "irs60"};
    std::vector<std::string> workspaces_without_run_numbers{"irs_test", "irs213_test"};
    std::vector<std::string> individual_workspace{"irs123_test"};

    TS_ASSERT_EQUALS(parseRunNumbers(workspaces_with_run_numbers), "irs60-280_test");
    TS_ASSERT_EQUALS(parseRunNumbers(workspaces_without_run_numbers), "irs_test");
    TS_ASSERT_EQUALS(parseRunNumbers(individual_workspace), "irs123_test");
  }

  void test_parseRunNumber_call_with_empty_array_returns_empty_string() {
    std::vector<std::string> empty_workspace{};
    TS_ASSERT_EQUALS(parseRunNumbers(empty_workspace), "");
  }

  void test_MaximumIndex_returns_properIndex() {
    auto const testWorkspace = createWorkspace(3);

    TS_ASSERT_EQUALS(*maximumIndex(testWorkspace), 2);
  }

  void test_getIndexStrings_return_formatted_index_range() {
    auto const testWorkspace = createWorkspace(5);
    AnalysisDataService::Instance().add("testWs", testWorkspace);

    TS_ASSERT_EQUALS(getIndexString("testWs"), "0-4");
    AnalysisDataService::Instance().clear();
  }
};
