// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceDockMockObjects.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspacePresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

using namespace testing;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

class WorkspacePresenterTest : public CxxTest::TestSuite {
public:
  static WorkspacePresenterTest *createSuite() {
    return new WorkspacePresenterTest();
  }
  static void destroySuite(WorkspacePresenterTest *suite) { delete suite; }

  WorkspacePresenterTest() { FrameworkManager::Instance(); }

  void setUp() override {
    mockView.reset();
    mockView = boost::make_shared<NiceMock<MockWorkspaceDockView>>();
    presenter = mockView->getPresenterSharedPtr();
  }

  void testLoadWorkspaceFromDock() {
    EXPECT_CALL(*mockView.get(), showLoadDialog()).Times(1);

    presenter->notifyFromView(ViewNotifiable::Flag::LoadWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testLoadLiveData() {
    EXPECT_CALL(*mockView.get(), showLiveDataDialog()).Times(1);

    presenter->notifyFromView(ViewNotifiable::Flag::LoadLiveDataWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testLoadWorkspaceExternal() {
    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().add("wksp", wksp);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().remove("wksp");
  }

  void testDeleteWorkspacesFromDockWithPrompt() {
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);

    ::testing::DefaultValue<StringList>::Set(
        StringList(StringList{"ws1", "ws2"}));
    ON_CALL(*mockView.get(), deleteConfirmation()).WillByDefault(Return(true));
    ON_CALL(*mockView.get(), isPromptDelete()).WillByDefault(Return(true));

    EXPECT_CALL(*mockView.get(), isPromptDelete()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), deleteConfirmation()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), deleteWorkspaces(StringList{"ws1", "ws2"}))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void testDeleteWorkspacesFromDockWithPromptUserDecline() {
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);

    ::testing::DefaultValue<StringList>::Set(
        StringList(StringList{"ws1", "ws2"}));
    ON_CALL(*mockView.get(), deleteConfirmation()).WillByDefault(Return(false));
    ON_CALL(*mockView.get(), isPromptDelete()).WillByDefault(Return(true));

    EXPECT_CALL(*mockView.get(), isPromptDelete()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), deleteConfirmation()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void testDeleteWorkspacesFromDockWithoutPrompt() {
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);

    ::testing::DefaultValue<StringList>::Set(
        StringList(StringList{"ws1", "ws2"}));
    ON_CALL(*mockView.get(), isPromptDelete()).WillByDefault(Return(false));

    EXPECT_CALL(*mockView.get(), isPromptDelete()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), deleteWorkspaces(StringList{"ws1", "ws2"}))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void testDeleteWorkspacesInvalidInput() {
    ::testing::DefaultValue<StringList>::Set(
        StringList(StringList{"ws1", "ws2"}));

    EXPECT_CALL(*mockView.get(), showCriticalUserMessage(_, _))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testDeleteWorkspacesExternal() {
    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp", wksp);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(Exactly(1));

    AnalysisDataService::Instance().remove("wksp");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testADSCleared() {
    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp", wksp);

    EXPECT_CALL(*mockView.get(), clearView()).Times(Exactly(1));

    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testRenameWorkspaceFromDock() {
    // Instruct gmock to return empty StringList
    ::testing::DefaultValue<StringList>::Set(StringList(StringList()));

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), showRenameDialog(_)).Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::RenameWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testRenameWorkspaceExternal() {
    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp", wksp);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().rename("wksp", "myWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().remove("myWorkspace");
  }

  void testWorkspacesGrouped() {
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);
    ::testing::DefaultValue<StringList>::Set(StringList{"ws1", "ws2"});

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);

    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("NewGroup"));

    TS_ASSERT(group != nullptr);

    if (group) {
      auto names = group->getNames();
      TS_ASSERT_EQUALS(names.size(), 2);
      TS_ASSERT_EQUALS(names[0], "ws1");
      TS_ASSERT_EQUALS(names[1], "ws2");
    }

    AnalysisDataService::Instance().deepRemoveGroup("NewGroup");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testInvalidGroupFails() {
    ::testing::DefaultValue<StringList>::Set(StringList());

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), showCriticalUserMessage(_, _))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGroupAlreadyExistsUserConfirm() {
    createGroup("NewGroup");
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);

    ::testing::DefaultValue<StringList>::Set(StringList{"ws1", "ws2"});
    ON_CALL(*mockView.get(), askUserYesNo(_, _)).WillByDefault(Return(true));

    EXPECT_CALL(*mockView.get(), askUserYesNo(_, _)).Times(1);
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);

    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve("NewGroup"));
    auto names = AnalysisDataService::Instance().getObjectNames();

    // The old "NewGroup" would have been ungrouped in order to create
    // the another "NewGroup" so check to make sure previously grouped
    // workspaces still exist
    TS_ASSERT(
        std::any_of(names.cbegin(), names.cend(), [](const std::string name) {
          return name.compare("wksp1") == 0;
        }));
    TS_ASSERT(
        std::any_of(names.cbegin(), names.cend(), [](const std::string name) {
          return name.compare("wksp2") == 0;
        }));

    TS_ASSERT(group != nullptr);

    if (group) {
      auto names = group->getNames();
      TS_ASSERT_EQUALS(names.size(), 2);
      TS_ASSERT_EQUALS(names[0], "ws1");
      TS_ASSERT_EQUALS(names[1], "ws2");
    }

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    // Remove group and left over workspaces
    removeGroup("NewGroup");
    AnalysisDataService::Instance().remove("wksp1");
    AnalysisDataService::Instance().remove("wksp2");
  }

  void testGroupAlreadyExistsUserDenies() {
    createGroup("NewGroup");

    ::testing::DefaultValue<StringList>::Set(StringList{"ws1", "ws2"});
    ON_CALL(*mockView.get(), askUserYesNo(_, _)).WillByDefault(Return(false));

    EXPECT_CALL(*mockView.get(), askUserYesNo(_, _)).Times(1);
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    removeGroup("NewGroup");
  }

  void testWorkspacesUngrouped() {
    createGroup("group");
    ::testing::DefaultValue<StringList>::Set(StringList(StringList{"group"}));

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::UngroupWorkspaces);

    auto names = AnalysisDataService::Instance().getObjectNames();

    TS_ASSERT(std::none_of(names.cbegin(), names.cend(), [](std::string name) {
      return name.compare("group") == 0;
    }));
    TS_ASSERT(std::any_of(names.cbegin(), names.cend(), [](std::string name) {
      return name.compare("wksp1") == 0;
    }));
    TS_ASSERT(std::any_of(names.cbegin(), names.cend(), [](std::string name) {
      return name.compare("wksp2") == 0;
    }));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().clear();
  }

  void testInvalidGroupForUngrouping() {
    ::testing::DefaultValue<StringList>::Set(StringList(StringList()));

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), showCriticalUserMessage(_, _))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::UngroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testWorkspacesGroupedExternal() {
    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().notificationCenter.postNotification(
        new WorkspacesGroupedNotification(std::vector<std::string>()));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testWorkspacesUnGroupedExternal() {
    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().notificationCenter.postNotification(
        new Mantid::API::WorkspaceUnGroupingNotification("", nullptr));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testWorkspaceGroupUpdated() {
    std::string groupName = "group";
    createGroup(groupName);

    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("wksp", wksp);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().addToGroup(groupName, "wksp");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    removeGroup(groupName);
  }

  void testSortWorkspacesByNameAscending() {
    using SortCriteria = IWorkspaceDockView::SortCriteria;
    using SortDir = IWorkspaceDockView::SortDirection;
    ::testing::DefaultValue<SortCriteria>::Set(SortCriteria::ByName);
    ::testing::DefaultValue<SortDir>::Set(SortDir::Ascending);

    EXPECT_CALL(*mockView.get(), getSortCriteria()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSortDirection()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(),
                sortWorkspaces(SortCriteria::ByName, SortDir::Ascending))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSortWorkspacesByNameDescending() {
    using SortCriteria = IWorkspaceDockView::SortCriteria;
    using SortDir = IWorkspaceDockView::SortDirection;
    ::testing::DefaultValue<SortCriteria>::Set(SortCriteria::ByName);
    ::testing::DefaultValue<SortDir>::Set(SortDir::Descending);

    EXPECT_CALL(*mockView.get(), getSortCriteria()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSortDirection()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(),
                sortWorkspaces(SortCriteria::ByName, SortDir::Descending))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSortWorkspacesByLastModifiedAscending() {
    using SortCriteria = IWorkspaceDockView::SortCriteria;
    using SortDir = IWorkspaceDockView::SortDirection;
    ::testing::DefaultValue<SortCriteria>::Set(SortCriteria::ByLastModified);
    ::testing::DefaultValue<SortDir>::Set(SortDir::Ascending);

    EXPECT_CALL(*mockView.get(), getSortCriteria()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSortDirection()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), sortWorkspaces(SortCriteria::ByLastModified,
                                                SortDir::Ascending))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSortWorkspacesByLastModifiedDescending() {
    using SortCriteria = IWorkspaceDockView::SortCriteria;
    using SortDir = IWorkspaceDockView::SortDirection;
    ::testing::DefaultValue<SortCriteria>::Set(SortCriteria::ByLastModified);
    ::testing::DefaultValue<SortDir>::Set(SortDir::Descending);

    EXPECT_CALL(*mockView.get(), getSortCriteria()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSortDirection()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), sortWorkspaces(SortCriteria::ByLastModified,
                                                SortDir::Descending))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SortWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveSingleWorkspaceNexus() {
    using SaveFileType = IWorkspaceDockView::SaveFileType;
    ::testing::DefaultValue<SaveFileType>::Set(SaveFileType::Nexus);

    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    EXPECT_CALL(*mockView.get(), getSelectedWorkspace()).WillOnce(Return(ws1));
    EXPECT_CALL(*mockView.get(), getSaveFileType()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(),
                saveWorkspace(ws1->getName(), SaveFileType::Nexus))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveSingleWorkspaceASCIIv1() {
    using SaveFileType = IWorkspaceDockView::SaveFileType;
    ::testing::DefaultValue<SaveFileType>::Set(SaveFileType::ASCIIv1);
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    EXPECT_CALL(*mockView.get(), getSelectedWorkspace()).WillOnce(Return(ws1));
    EXPECT_CALL(*mockView.get(), getSaveFileType()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(),
                saveWorkspace(ws1->getName(), SaveFileType::ASCIIv1))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveSingleWorkspaceASCII() {
    using SaveFileType = IWorkspaceDockView::SaveFileType;
    ::testing::DefaultValue<SaveFileType>::Set(SaveFileType::ASCII);

    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    EXPECT_CALL(*mockView.get(), getSelectedWorkspace()).WillOnce(Return(ws1));
    EXPECT_CALL(*mockView.get(), getSaveFileType()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(),
                saveWorkspace(ws1->getName(), SaveFileType::ASCII))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveWorkspaceCollection() {
    ::testing::DefaultValue<StringList>::Set(StringList{"ws1", "ws2"});
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), saveWorkspaces(StringList{"ws1", "ws2"}))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SaveWorkspaceCollection);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testFilterWorkspaces() {
    ::testing::DefaultValue<std::string>::Set(std::string());
    EXPECT_CALL(*mockView.get(), getFilterText()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), filterWorkspaces(std::string()))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::FilterWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testRefreshWorkspaces() {
    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::RefreshWorkspaces);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  // Popup Context Menu Tests
  void testShowPopupMenu() {
    EXPECT_CALL(*mockView.get(), popupContextMenu()).Times(Exactly(1));
    presenter->notifyFromView(
        ViewNotifiable::Flag::PopulateAndShowWorkspaceContextMenu);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowWorkspaceData() {
    EXPECT_CALL(*mockView.get(), showWorkspaceData()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowWorkspaceData);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowInstrumentView() {
    EXPECT_CALL(*mockView.get(), showInstrumentView()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowInstrumentView);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveToProgram() {
    EXPECT_CALL(*mockView.get(), saveToProgram()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::SaveToProgram);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testPlotSpectrum() {
    EXPECT_CALL(*mockView.get(), plotSpectrum("Simple")).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrum);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testPlotSpectrumWithErrors() {
    EXPECT_CALL(*mockView.get(), plotSpectrum("Errors")).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrumWithErrors);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testPlotSpectrumAdvanced() {
    EXPECT_CALL(*mockView.get(), plotSpectrum("Advanced")).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::PlotSpectrumAdvanced);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowColourFillPlot() {
    EXPECT_CALL(*mockView.get(), showColourFillPlot()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowColourFillPlot);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowDetectorsTable() {
    EXPECT_CALL(*mockView.get(), showDetectorsTable()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowDetectorsTable);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowBoxDataTable() {
    EXPECT_CALL(*mockView.get(), showBoxDataTable()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowBoxDataTable);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowVatesGUI() {
    EXPECT_CALL(*mockView.get(), showVatesGUI()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowVatesGUI);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowMDPlot() {
    EXPECT_CALL(*mockView.get(), showMDPlot()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowMDPlot);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowListData() {
    EXPECT_CALL(*mockView.get(), showListData()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowListData);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowSpectrumViewer() {
    EXPECT_CALL(*mockView.get(), showSpectrumViewer()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowSpectrumViewer);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowSliceViewer() {
    EXPECT_CALL(*mockView.get(), showSliceViewer()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowSliceViewer);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowLogs() {
    EXPECT_CALL(*mockView.get(), showLogs()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowLogs);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowSampleMaterialWindow() {
    EXPECT_CALL(*mockView.get(), showSampleMaterialWindow()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowSampleMaterialWindow);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowAlgorithmHistory() {
    EXPECT_CALL(*mockView.get(), showAlgorithmHistory()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowAlgorithmHistory);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testShowTransposed() {
    EXPECT_CALL(*mockView.get(), showTransposed()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ShowTransposed);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testConvertToMatrixWorkspace() {
    EXPECT_CALL(*mockView.get(), convertToMatrixWorkspace()).Times(Exactly(1));
    presenter->notifyFromView(ViewNotifiable::Flag::ConvertToMatrixWorkspace);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testConvertMDHistoToMatrixWorkspace() {
    EXPECT_CALL(*mockView.get(), convertMDHistoToMatrixWorkspace())
        .Times(Exactly(1));
    presenter->notifyFromView(
        ViewNotifiable::Flag::ConvertMDHistoToMatrixWorkspace);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testClearUBMatrix() {
    ::testing::DefaultValue<StringList>::Set(StringList{"ws1"});
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("ws1", ws1);

    // Setup a UB matrix before attempting to remove it
    auto setUB = Mantid::API::AlgorithmManager::Instance().create("SetUB");
    setUB->initialize();
    setUB->setProperty("Workspace", "ws1");
    setUB->execute();

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), executeAlgorithmAsync(_, _)).Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::ClearUBMatrix);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("ws1");
  }

private:
  boost::shared_ptr<NiceMock<MockWorkspaceDockView>> mockView;
  WorkspacePresenterVN_sptr presenter;

  void createGroup(std::string groupName) {
    auto group =
        WorkspaceCreationHelper::createWorkspaceGroup(0, 10, 10, groupName);
    auto wksp1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto wksp2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp1", wksp1);
    AnalysisDataService::Instance().add("wksp2", wksp2);
    AnalysisDataService::Instance().addToGroup(groupName, "wksp1");
    AnalysisDataService::Instance().addToGroup(groupName, "wksp2");
  }

  void removeGroup(std::string groupName) {
    AnalysisDataService::Instance().deepRemoveGroup(groupName);
  }
};
