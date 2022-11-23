// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "PreviewModel.h"
#include "ROIType.h"
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

  void test_load_workspace_from_ads_throws_if_wrong_type() {
    auto mockJobManager = MockJobManager();
    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(0);

    PreviewModel model;
    auto workspaceName = std::string("test workspace");
    // We don't currently support workspace groups so it should throw with this type
    AnalysisDataService::Instance().addOrReplace(workspaceName, std::make_shared<WorkspaceGroup>());

    TS_ASSERT_THROWS(model.loadWorkspaceFromAds(workspaceName), std::runtime_error const &);
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
    const ProcessingInstructions inputRoi{"56,57,58,59"};
    model.setSelectedBanks(std::move(inputRoi));
    TS_ASSERT_EQUALS(inputRoi, *model.getSelectedBanks())
  }

  void test_set_selected_signal_region_converts_to_processing_instructions_string() {
    PreviewModel model;
    const IPreviewModel::Selection inputRoi{3.6, 11.4};
    model.setSelectedRegion(ROIType::Signal, inputRoi);
    // Start and end are rounded to nearest integer and converted to a string
    TS_ASSERT_EQUALS(ProcessingInstructions{"4-11"}, model.getProcessingInstructions(ROIType::Signal))
  }

  void test_set_selected_background_region_converts_to_processing_instructions_string() {
    PreviewModel model;
    const IPreviewModel::Selection inputRoi{3.6, 11.4};
    model.setSelectedRegion(ROIType::Background, inputRoi);
    // Start and end are rounded to nearest integer and converted to a string
    TS_ASSERT_EQUALS(ProcessingInstructions{"4-11"}, model.getProcessingInstructions(ROIType::Background))
  }

  void test_set_selected_transmission_region_converts_to_processing_instructions_string() {
    PreviewModel model;
    const IPreviewModel::Selection inputRoi{3.6, 11.4};
    model.setSelectedRegion(ROIType::Transmission, inputRoi);
    // Start and end are rounded to nearest integer and converted to a string
    TS_ASSERT_EQUALS(ProcessingInstructions{"4-11"}, model.getProcessingInstructions(ROIType::Transmission))
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

  void test_reduce() {
    auto mockJobManager = MockJobManager();
    auto expectedWs = createWorkspace();
    auto wsReductionEffect = [&expectedWs](PreviewRow &row) { row.setReducedWs(expectedWs); };
    EXPECT_CALL(mockJobManager, startReduction(_)).Times(1).WillOnce(Invoke(wsReductionEffect));

    PreviewModel model;
    model.reduceAsync(mockJobManager);

    auto workspace = model.getReducedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace, expectedWs);
  }

  void test_export_summed_ws_to_ads() {
    PreviewModel model;
    auto mockJobManager = MockJobManager();
    auto ws = generateSummedWs(mockJobManager, model);

    model.exportSummedWsToAds();
    auto &ads = AnalysisDataService::Instance();
    const std::string expectedName = "preview_summed_ws";

    TS_ASSERT(ads.doesExist(expectedName));
    TS_ASSERT_EQUALS(ws, ads.retrieveWS<MatrixWorkspace>(expectedName));
    ads.remove(expectedName);
  }

  void test_export_summed_ws_with_no_ws_set_does_not_throw() {
    PreviewModel model;
    // This should emit an error, but we cannot observe this from our test
    model.exportSummedWsToAds();
  }

  void test_export_reduced_ws_to_ads() {
    PreviewModel model;
    auto mockJobManager = MockJobManager();
    auto ws = generateReducedWs(mockJobManager, model);

    model.exportReducedWsToAds();
    auto &ads = AnalysisDataService::Instance();
    const std::string expectedName = "preview_reduced_ws";

    TS_ASSERT(ads.doesExist(expectedName));
    TS_ASSERT_EQUALS(ws, ads.retrieveWS<MatrixWorkspace>(expectedName));
    ads.remove(expectedName);
  }

  void test_export_reduced_ws_with_no_ws_set_does_not_throw() {
    PreviewModel model;
    // This should emit an error, but we cannot observe this from our test
    model.exportReducedWsToAds();
  }

  void test_get_set_loaded_workspace() {
    PreviewModel model;
    auto ws = createWorkspace();
    model.setLoadedWs(ws);

    TS_ASSERT_EQUALS(model.getLoadedWs(), ws);
  }

  void test_get_theta_from_workspace() {
    PreviewModel model;
    auto theta = 2.3;
    auto ws = createWorkspace();
    ws->mutableRun().addProperty("Theta", theta, true);
    model.setLoadedWs(ws);

    TS_ASSERT(model.getDefaultTheta());
    TS_ASSERT_DELTA(*model.getDefaultTheta(), theta, 1e-6);
  }

  void test_get_theta_from_workspace_not_found() {
    PreviewModel model;
    auto ws = createWorkspace();
    model.setLoadedWs(ws);

    TS_ASSERT(!model.getDefaultTheta());
  }

  void test_get_theta_from_workspace_is_invalid() {
    PreviewModel model;
    auto thetas = std::vector<double>{0.0, -1.2, 0.00000000008};
    for (auto theta : thetas) {
      auto ws = createWorkspace();
      ws->mutableRun().addProperty("Theta", theta, true);
      model.setLoadedWs(ws);

      TS_ASSERT(!model.getDefaultTheta());
    }
  }

  void test_get_preview_row() {
    PreviewModel model;
    auto ws = createWorkspace();
    model.setLoadedWs(ws);

    PreviewRow const &previewRow = model.getPreviewRow();
    TS_ASSERT_EQUALS(ws, previewRow.getLoadedWs())
  }

private:
  MatrixWorkspace_sptr generateSummedWs(MockJobManager &mockJobManager, PreviewModel &model) {
    auto expectedWs = createWorkspace();
    auto wsSumBanksEffect = [&expectedWs](PreviewRow &row) { row.setSummedWs(expectedWs); };
    ON_CALL(mockJobManager, startSumBanks(_)).WillByDefault(Invoke(wsSumBanksEffect));
    model.sumBanksAsync(mockJobManager);
    return expectedWs;
  }

  MatrixWorkspace_sptr generateReducedWs(MockJobManager &mockJobManager, PreviewModel &model) {
    auto expectedWs = createWorkspace();
    auto wsReduceEffect = [&expectedWs](PreviewRow &row) { row.setReducedWs(expectedWs); };
    ON_CALL(mockJobManager, startReduction(_)).WillByDefault(Invoke(wsReduceEffect));
    model.reduceAsync(mockJobManager);
    return expectedWs;
  }

  MatrixWorkspace_sptr createWorkspace() { return WorkspaceCreationHelper::create2DWorkspace(1, 1); }
};
