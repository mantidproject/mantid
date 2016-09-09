#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceDockViewMockObjects.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"

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

  void testLoadWorkspaceExternal() {
    auto wksp = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().add("wksp", wksp);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().remove("wksp");
  }

  void testDeleteWorkspacesFromDock() {
    EXPECT_CALL(*mockView.get(), deleteConfirmation()).Times(Exactly(1));
    EXPECT_CALL(*mockView.get(), deleteWorkspaces()).Times(AtLeast(0));

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

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(Exactly(1));

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

    AnalysisDataService::Instance().clear();
  }

private:
  boost::shared_ptr<NiceMock<MockWorkspaceDockView>> mockView;
  boost::shared_ptr<WorkspacePresenter> presenter;
};