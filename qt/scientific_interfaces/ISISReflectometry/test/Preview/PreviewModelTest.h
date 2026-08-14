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
    TS_ASSERT_EQUALS(model.getNumberOfGroupMembers(), 0)
  }

  void test_load_workspace_from_ads() {
    auto mockJobManager = MockJobManager();
    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(0);

    PreviewModel model;
    auto workspaceName = std::string("test workspace");
    AnalysisDataService::Instance().addOrReplace(workspaceName, createWorkspace());

    TS_ASSERT(model.loadWorkspaceFromAds(workspaceName));
    auto workspace = model.getSelectedLoadedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getName(), workspaceName);
  }

  void test_load_workspace_group_from_ads() {
    PreviewModel model;
    auto const first = createWorkspace();
    auto const second = createWorkspace();
    auto const group = createWorkspaceGroup({first, second});
    auto const workspaceName = std::string("test group");
    AnalysisDataService::Instance().addOrReplace(workspaceName, group);

    TS_ASSERT(model.loadWorkspaceFromAds(workspaceName));
    TS_ASSERT(model.isWorkspaceGroup());
    TS_ASSERT_EQUALS(model.getNumberOfGroupMembers(), 2);
    TS_ASSERT_EQUALS(model.getSelectedGroupMember(), 0);
    TS_ASSERT_EQUALS(model.getLoadedWs(), group);
    TS_ASSERT_EQUALS(model.getSelectedLoadedWs(), first);

    model.setSelectedGroupMember(1);

    TS_ASSERT_EQUALS(model.getSelectedLoadedWs(), second);
  }

  void test_group_member_names_are_returned_in_group_order() {
    PreviewModel model;
    auto const first = createWorkspace();
    auto const second = createWorkspace();
    AnalysisDataService::Instance().addOrReplace("first", first);
    AnalysisDataService::Instance().addOrReplace("second", second);
    auto const group = createWorkspaceGroup({first, second});
    AnalysisDataService::Instance().addOrReplace("group", group);

    model.loadWorkspaceFromAds("group");

    TS_ASSERT_EQUALS(model.getGroupMemberDisplayNames(), std::vector<std::string>({"first", "second"}));
  }

  void test_group_member_names_are_empty_for_matrix_workspace() {
    PreviewModel model;
    AnalysisDataService::Instance().addOrReplace("workspace", createWorkspace());
    model.loadWorkspaceFromAds("workspace");

    TS_ASSERT(model.getGroupMemberDisplayNames().empty());
  }

  void test_load_workspace_from_ads_throws_if_group_is_empty() {
    PreviewModel model;
    auto workspaceName = std::string("test workspace");
    AnalysisDataService::Instance().addOrReplace(workspaceName, std::make_shared<WorkspaceGroup>());

    TS_ASSERT_THROWS_EQUALS(model.loadWorkspaceFromAds(workspaceName), std::runtime_error const &e,
                            std::string(e.what()),
                            "Unsupported workspace type; expected MatrixWorkspace or WorkspaceGroup of "
                            "MatrixWorkspaces");
  }

  void test_setting_group_member_outside_loaded_group_is_rejected() {
    PreviewModel model;
    auto const group = createWorkspaceGroup({createWorkspace(), createWorkspace()});
    AnalysisDataService::Instance().addOrReplace("group", group);
    model.loadWorkspaceFromAds("group");

    TS_ASSERT_THROWS_EQUALS(model.setSelectedGroupMember(2), std::out_of_range const &e, std::string(e.what()),
                            "Workspace group member index is out of range");
    TS_ASSERT_EQUALS(model.getSelectedGroupMember(), 0);
  }

  void test_load_workspace_from_file() {
    auto mockJobManager = MockJobManager();

    auto expectedWs = createWorkspace();
    auto wsLoadEffect = [&expectedWs](PreviewRow &row) { row.setLoadedWs(expectedWs); };

    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(1).WillOnce(Invoke(wsLoadEffect));

    PreviewModel model;
    auto workspaceName = std::string("not there");

    model.loadAndPreprocessWorkspaceAsync(workspaceName, mockJobManager);
    auto workspace = model.getSelectedLoadedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace, expectedWs);
  }

  void test_load_workspace_group_from_file() {
    auto mockJobManager = MockJobManager();
    auto const first = createWorkspace();
    auto const second = createWorkspace();
    auto const expectedGroup = createWorkspaceGroup({first, second});
    auto wsLoadEffect = [&expectedGroup](PreviewRow &row) { row.setLoadedWs(expectedGroup); };
    EXPECT_CALL(mockJobManager, startPreprocessing(_)).Times(1).WillOnce(Invoke(wsLoadEffect));

    PreviewModel model;
    model.loadAndPreprocessWorkspaceAsync("/data/POLREF00004699.nxs", mockJobManager);

    TS_ASSERT(model.isWorkspaceGroup());
    TS_ASSERT_EQUALS(model.getLoadedWs(), expectedGroup);
    TS_ASSERT_EQUALS(model.getSelectedLoadedWs(), first);
    TS_ASSERT_EQUALS(model.getGroupMemberDisplayNames(),
                     std::vector<std::string>({"POLREF00004699_1", "POLREF00004699_2"}));
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

    auto workspace = model.getSelectedSummedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace, expectedWs);
  }

  void test_sum_banks_stores_group_and_exposes_selected_member() {
    auto mockJobManager = MockJobManager();
    auto const first = createWorkspace();
    auto const second = createWorkspace();
    auto const expectedGroup = createWorkspaceGroup({first, second});
    auto wsSumBanksEffect = [&expectedGroup](PreviewRow &row) { row.setSummedWs(expectedGroup); };
    EXPECT_CALL(mockJobManager, startSumBanks(_)).Times(1).WillOnce(Invoke(wsSumBanksEffect));

    PreviewModel model;
    model.setLoadedWs(expectedGroup);
    model.sumBanksAsync(mockJobManager);
    model.setSelectedGroupMember(1);

    TS_ASSERT_EQUALS(model.getSummedWs(), expectedGroup);
    TS_ASSERT_EQUALS(model.getSelectedSummedWs(), second);
  }

  void test_reduce() {
    auto mockJobManager = MockJobManager();
    auto expectedWs = createWorkspace();
    auto wsReductionEffect = [&expectedWs](PreviewRow &row) { row.setReducedWs(expectedWs); };
    EXPECT_CALL(mockJobManager, startReduction(_)).Times(1).WillOnce(Invoke(wsReductionEffect));

    PreviewModel model;
    model.reduceAsync(mockJobManager);

    auto workspace = model.getSelectedReducedWs();
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace, expectedWs);
  }

  void test_reduce_stores_group_and_exposes_selected_member() {
    auto mockJobManager = MockJobManager();
    auto const inputGroup = createWorkspaceGroup({createWorkspace(), createWorkspace()});
    auto const firstOutput = createWorkspace();
    auto const secondOutput = createWorkspace();
    auto const expectedGroup = createWorkspaceGroup({firstOutput, secondOutput});
    auto const groupName = std::string("input_group");
    AnalysisDataService::Instance().addOrReplace(groupName, inputGroup);
    auto wsReductionEffect = [&expectedGroup, &inputGroup, &groupName](PreviewRow &row) {
      TS_ASSERT_EQUALS(row.getLoadedWs(), inputGroup);
      TS_ASSERT_EQUALS(row.runNumbers().size(), 1);
      TS_ASSERT_EQUALS(row.runNumbers()[0], groupName);
      row.setReducedWs(expectedGroup);
    };
    EXPECT_CALL(mockJobManager, startReduction(_)).Times(1).WillOnce(Invoke(wsReductionEffect));

    PreviewModel model;
    model.loadWorkspaceFromAds(groupName);
    model.reduceAsync(mockJobManager);
    model.setSelectedGroupMember(1);

    TS_ASSERT_EQUALS(model.getReducedWs(), expectedGroup);
    TS_ASSERT_EQUALS(model.getSelectedReducedWs(), secondOutput);
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

  void test_export_summed_workspace_group_to_ads() {
    PreviewModel model;
    auto const first = createWorkspace();
    auto const second = createWorkspace();
    auto const group = createWorkspaceGroup({first, second});
    model.setSummedWs(group);

    model.exportSummedWsToAds();

    auto const exported = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("preview_summed_ws");
    TS_ASSERT_EQUALS(exported, group);
    TS_ASSERT_EQUALS(exported->getItem(0), first);
    TS_ASSERT_EQUALS(exported->getItem(1), second);
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

  void test_export_reduced_workspace_group_to_ads() {
    PreviewModel model;
    auto mockJobManager = MockJobManager();
    auto const first = createWorkspace();
    auto const second = createWorkspace();
    auto const group = createWorkspaceGroup({first, second});
    auto wsReductionEffect = [&group](PreviewRow &row) { row.setReducedWs(group); };
    EXPECT_CALL(mockJobManager, startReduction(_)).WillOnce(Invoke(wsReductionEffect));
    model.reduceAsync(mockJobManager);

    model.exportReducedWsToAds();

    auto const exported = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("preview_reduced_ws");
    TS_ASSERT_EQUALS(exported, group);
    TS_ASSERT_EQUALS(exported->getItem(0), first);
    TS_ASSERT_EQUALS(exported->getItem(1), second);
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

    TS_ASSERT_EQUALS(model.getSelectedLoadedWs(), ws);
  }

  void test_loading_new_workspace_resets_selected_group_member() {
    PreviewModel model;
    auto const firstGroup = createWorkspaceGroup({createWorkspace(), createWorkspace()});
    AnalysisDataService::Instance().addOrReplace("first", firstGroup);
    AnalysisDataService::Instance().addOrReplace("second", createWorkspace());
    model.loadWorkspaceFromAds("first");
    model.setSelectedGroupMember(1);

    model.loadWorkspaceFromAds("second");

    TS_ASSERT_EQUALS(model.getSelectedGroupMember(), 0);
    TS_ASSERT(!model.isWorkspaceGroup());
    TS_ASSERT_EQUALS(model.getNumberOfGroupMembers(), 1);
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

  WorkspaceGroup_sptr createWorkspaceGroup(std::initializer_list<MatrixWorkspace_sptr> members) {
    auto group = std::make_shared<WorkspaceGroup>();
    for (auto const &member : members)
      group->addWorkspace(member);
    return group;
  }
};
