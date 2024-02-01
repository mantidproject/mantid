// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/WorkspaceUtils.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

#include <QPair>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::WorkspaceUtils;
using namespace Mantid::IndirectFitDataCreationHelper;
namespace {

MatrixWorkspace_sptr createWorkspaceWithInstrumentAndParameters() {

  auto testWorkspace = createWorkspace(1, 5);
  std::string idfdirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
  // IRIS instrument with graphite detector.
  auto const ipfFilename = idfdirectory + "IRIS" + "_" + "graphite" + "_" + "002" + "_Parameters.xml";

  auto loadInst = AlgorithmManager::Instance().create("LoadInstrument");
  loadInst->setLogging(true);
  loadInst->initialize();
  loadInst->setProperty("Workspace", testWorkspace);
  loadInst->setProperty("InstrumentName", "IRIS");
  loadInst->setProperty("RewriteSpectraMap", "False");
  loadInst->execute();

  auto loadParams = AlgorithmManager::Instance().create("LoadParameterFile");
  loadParams->setChild(true);
  loadParams->setLogging(true);
  loadParams->initialize();
  loadParams->setProperty("Workspace", testWorkspace);
  loadParams->setProperty("Filename", ipfFilename);
  loadParams->execute();

  return testWorkspace;
}
} // namespace

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
    auto &res = QPair<double, double>(0, 0);
    auto const testWorkspace = createWorkspace(1, 5);

    TS_ASSERT(!getResolutionRangeFromWs(testWorkspace, res));
  }
  void test_getResolutionFromWs_returns_res_for_instrument_workspace() {
    auto &res = QPair<double, double>(0, 0);
    auto const testWorkspace = createWorkspaceWithInstrumentAndParameters();
    getResolutionRangeFromWs(testWorkspace, res);

    TS_ASSERT_DELTA(res.first, -0.0175, 1e-5);
    TS_ASSERT_DELTA(res.second, 0.0175, 1e-5);
  }

  void test_getEmode_defaults_to_indirect_with_InelasticWorkspace() {
    auto const testWorkspace = createWorkspaceWithInelasticInstrument(2);

    TS_ASSERT_EQUALS(getEMode(testWorkspace), "Indirect");
  }

  void test_getEFixed_throws_for_no_instrument() {
    auto const testWorkspace = createWorkspace(5);
    TS_ASSERT_THROWS(getEFixed(testWorkspace), std::runtime_error);
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
};
