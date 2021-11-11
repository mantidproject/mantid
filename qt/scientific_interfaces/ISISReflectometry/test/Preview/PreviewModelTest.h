// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "PreviewModel.h"
#include "test/ReflMockObjects.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace Mantid::API;
using ::testing::_;
using ::testing::Invoke;

class PreviewModelTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_run_details_created_by_default() {
    PreviewModel model;
    // This will throw if the underlying RunDetails is null
    TS_ASSERT_THROWS_NOTHING(model.getSelectedBanks())
  }

  void test_load_workspace_from_ads() {
    auto mockJobManager = MockJobManager();
    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(0);

    PreviewModel model;
    auto workspaceName = std::string("test workspace");
    AnalysisDataService::Instance().addOrReplace(workspaceName, createWorkspace());

    TS_ASSERT(model.loadWorkspaceFromAds(workspaceName));
    auto workspace = model.getLoadedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getName(), workspaceName);
  }

  void test_load_workspace_from_file() {
    auto mockJobManager = MockJobManager();

    auto expectedWs = createWorkspace();
    auto wsLoadEffect = [&expectedWs](PreviewRow &row) { row.setLoadedWs(expectedWs); };

    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(1).WillOnce(Invoke(wsLoadEffect));

    PreviewModel model;
    auto workspaceName = std::string("not there");

    model.loadAndPreprocessWorkspaceAsync(workspaceName, mockJobManager);
    auto workspace = model.getLoadedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace, expectedWs);
  }

  void test_set_and_get_selected_banks() {
    PreviewModel model;
    const std::vector<Mantid::detid_t> inputRoi{56, 57, 58, 59};
    model.setSelectedBanks(inputRoi);
    TS_ASSERT_EQUALS(inputRoi, model.getSelectedBanks())
  }

  void test_sum_banks() {
    auto mockJobManager = MockJobManager();
    auto expectedWs = createWorkspace();
    auto wsSumBanksEffect = [&expectedWs](PreviewRow &row) { row.setSummedWs(expectedWs); };
    EXPECT_CALL(mockJobManager, startSumBanks(_)).Times(1).WillOnce(Invoke(wsSumBanksEffect));

    PreviewModel model;
    model.sumBanksAsync(mockJobManager);

    auto workspace = model.getSummedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace, expectedWs);
  }

  void test_convert_detIDs_to_string() {
    PreviewModel model;
    auto const indices = std::vector<Mantid::detid_t>{99, 4, 5};
    auto result = model.detIDsToString(indices);
    TS_ASSERT_EQUALS(result, "99,4,5");
  }

  void test_convert_empty_detIDs_to_string() {
    PreviewModel model;
    auto const indices = std::vector<Mantid::detid_t>{};
    auto result = model.detIDsToString(indices);
    TS_ASSERT_EQUALS("", result);
  }

  void test_export_summed_ws_to_ads() {
    PreviewModel model;
    auto mockJobManager = MockJobManager();

    // TODO refactor this block
    auto expectedWs = createWorkspace();
    auto wsSumBanksEffect = [&expectedWs](PreviewRow &row) { row.setSummedWs(expectedWs); };
    ON_CALL(mockJobManager, startSumBanks(_)).WillByDefault(Invoke(wsSumBanksEffect));
    model.sumBanksAsync(mockJobManager);

    model.exportSummedWsToAds();
    TS_ASSERT(AnalysisDataService::Instance().doesExist("preview_summed_ws"));
  }

  void test_export_summed_ws_with_no_ws_set_does_not_throw() {
    PreviewModel model;
    // This should emit an error, but we cannot observe this from our test
    model.exportSummedWsToAds();
  }

private:
  MatrixWorkspace_sptr createWorkspace() { return WorkspaceCreationHelper::create2DWorkspace(1, 1); }
};
