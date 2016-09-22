#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceDockViewMockObjects.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"

#include <MantidAPI/AlgorithmManager.h>
#include <MantidAPI/AnalysisDataService.h>
#include <MantidTestHelpers/WorkspaceCreationHelper.h>

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

  void setUp() {
    mockView.reset();
    mockView = boost::make_shared<NiceMock<MockWorkspaceDockView>>();
    mockView->init();

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
    auto wksp = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().add("wksp", wksp);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().remove("wksp");
  }

  void testDeleteWorkspacesFromDockWithPrompt() {
    auto ws1 = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    auto ws2 = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);

    ::testing::DefaultValue<StringList>::Set(
        StringList(StringList{"ws1", "ws2"}));
    ON_CALL(*mockView.get(), deleteConfirmation()).WillByDefault(Return(true));
    ON_CALL(*mockView.get(), isPromptDelete()).WillByDefault(Return(true));

    EXPECT_CALL(*mockView.get(), isPromptDelete()).Times(Exactly(2));
    EXPECT_CALL(*mockView.get(), deleteConfirmation()).Times(Exactly(2));
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(2));
    EXPECT_CALL(*mockView.get(), deleteWorkspaces(StringList{"ws1", "ws2"}))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);

    ON_CALL(*mockView.get(), deleteConfirmation()).WillByDefault(Return(false));

    presenter->notifyFromView(ViewNotifiable::Flag::DeleteWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void testDeleteWorkspacesFromDockWithoutPrompt() {
    auto ws1 = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    auto ws2 = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
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
    auto wksp = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp", wksp);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(Exactly(1));

    AnalysisDataService::Instance().remove("wksp");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testADSCleared() {
    auto wksp = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

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
    auto wksp = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp", wksp);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().rename("wksp", "myWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().remove("wksp");
  }

  void testWorkspacesGrouped() {
    ::testing::DefaultValue<StringList>::Set(StringList{"ws1", "ws2"});

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(),
                groupWorkspaces(StringList{"ws1", "ws2"}, "NewGroup"))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);

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

    ::testing::DefaultValue<StringList>::Set(StringList{"ws1", "ws2"});
    ON_CALL(*mockView.get(), askUserYesNo(_, _)).WillByDefault(Return(true));

    EXPECT_CALL(*mockView.get(), askUserYesNo(_, _)).Times(1);
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(),
                groupWorkspaces(StringList{"ws1", "ws2"}, "NewGroup"))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    removeGroup("NewGroup");
  }

  void testGroupAlreadyExistsUserDenies() {
    createGroup("NewGroup");

    ::testing::DefaultValue<StringList>::Set(StringList{"ws1", "ws2"});
    ON_CALL(*mockView.get(), askUserYesNo(_, _)).WillByDefault(Return(false));

    EXPECT_CALL(*mockView.get(), askUserYesNo(_, _)).Times(1);
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), groupWorkspaces(_, _)).Times(Exactly(0));

    presenter->notifyFromView(ViewNotifiable::Flag::GroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    removeGroup("NewGroup");
  }

  void testWorkspacesUngrouped() {
    ::testing::DefaultValue<StringList>::Set(StringList(StringList{"group"}));

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), ungroupWorkspaces(StringList{"group"}))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::UngroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testInvalidGroupForUngrouping() {
    ::testing::DefaultValue<StringList>::Set(StringList(StringList()));

    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), showCriticalUserMessage(_, _))
        .Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), ungroupWorkspaces(_)).Times(Exactly(0));

    presenter->notifyFromView(ViewNotifiable::Flag::UngroupWorkspaces);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testWorkspacesGroupedExternal() {
    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    createGroup("group");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    removeGroup("group");
  }

  void testWorkspacesUnGroupedExternal() {
    createGroup("group");

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().remove("group");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().clear();
  }

  void testWorkspaceGroupUpdated() {
    std::string groupName = "group";
    createGroup(groupName);

    auto wksp = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
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
    ::testing::DefaultValue<StringList>::Set(StringList{"ws"});
    ::testing::DefaultValue<SaveFileType>::Set(SaveFileType::Nexus);

    EXPECT_CALL(*mockView.get(), getSaveFileType()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), saveWorkspace("ws", SaveFileType::Nexus))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveSingleWorkspaceASCIIv1() {
    using SaveFileType = IWorkspaceDockView::SaveFileType;
    ::testing::DefaultValue<StringList>::Set(StringList{"ws"});
    ::testing::DefaultValue<SaveFileType>::Set(SaveFileType::ASCIIv1);

    EXPECT_CALL(*mockView.get(), getSaveFileType()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), saveWorkspace("ws", SaveFileType::ASCIIv1))
        .Times(Exactly(1));

    presenter->notifyFromView(ViewNotifiable::Flag::SaveSingleWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testSaveSingleWorkspaceASCII() {
    using SaveFileType = IWorkspaceDockView::SaveFileType;
    ::testing::DefaultValue<SaveFileType>::Set(SaveFileType::ASCII);
    ::testing::DefaultValue<StringList>::Set(StringList{"ws"});

    EXPECT_CALL(*mockView.get(), getSaveFileType()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), getSelectedWorkspaceNames()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), saveWorkspace("ws", SaveFileType::ASCII))
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

private:
  boost::shared_ptr<NiceMock<MockWorkspaceDockView>> mockView;
  WorkspacePresenterVN_sptr presenter;

  void createGroup(std::string groupName) {
    auto group =
        WorkspaceCreationHelper::CreateWorkspaceGroup(0, 10, 10, groupName);
    auto wksp1 = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    auto wksp2 = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp1", wksp1);
    AnalysisDataService::Instance().add("wksp2", wksp2);
    AnalysisDataService::Instance().addToGroup(groupName, "wksp1");
    AnalysisDataService::Instance().addToGroup(groupName, "wksp2");
  }

  void removeGroup(std::string groupName) {
    AnalysisDataService::Instance().deepRemoveGroup(groupName);
  }
};