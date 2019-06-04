// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceDockMockObjects.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/ADSAdapter.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace testing;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

using NotifyFlag = WorkspaceProviderNotifiable::Flag;

class ADSAdapterTest : public CxxTest::TestSuite {
public:
  static ADSAdapterTest *createSuite() { return new ADSAdapterTest(); }

  static void destroySuite(ADSAdapterTest *suite) { delete suite; }

  void setUp() override {
    mockPresenter.reset();
    mockPresenter =
        boost::make_shared<NiceMock<MockWorkspaceProviderNotifiable>>();
    adapter.registerPresenter(mockPresenter);
    AnalysisDataService::Instance().clear();
  }

  void testLoadWorkspaceIntoADS() {
    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    EXPECT_CALL(*mockPresenter.get(),
                notifyFromWorkspaceProvider(NotifyFlag::WorkspaceLoaded))
        .Times(Exactly(1));

    AnalysisDataService::Instance().add("wksp", wksp);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void testRemoveWorkspaceFromADS() {
    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp", wksp);

    EXPECT_CALL(*mockPresenter.get(),
                notifyFromWorkspaceProvider(NotifyFlag::WorkspaceDeleted))
        .Times(Exactly(1));

    AnalysisDataService::Instance().remove("wksp");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void testClearADS() {
    auto wksp1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto wksp2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    AnalysisDataService::Instance().add("wksp1", wksp1);
    AnalysisDataService::Instance().add("wksp2", wksp2);

    EXPECT_CALL(*mockPresenter.get(),
                notifyFromWorkspaceProvider(NotifyFlag::WorkspacesCleared))
        .Times(Exactly(1));

    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void testRenameWorkspace() {
    auto wksp = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("wksp", wksp);
    EXPECT_CALL(*mockPresenter.get(),
                notifyFromWorkspaceProvider(NotifyFlag::WorkspaceRenamed))
        .Times(Exactly(1));

    AnalysisDataService::Instance().rename("wksp", "myWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void testGroupWorkspaces() {
    EXPECT_CALL(*mockPresenter.get(),
                notifyFromWorkspaceProvider(NotifyFlag::WorkspacesGrouped))
        .Times(Exactly(1));

    AnalysisDataService::Instance().notificationCenter.postNotification(
        new WorkspacesGroupedNotification(std::vector<std::string>()));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void testUngroupWorkspaces() {
    EXPECT_CALL(*mockPresenter.get(),
                notifyFromWorkspaceProvider(NotifyFlag::WorkspacesUngrouped))
        .Times(Exactly(1));

    AnalysisDataService::Instance().notificationCenter.postNotification(
        new Mantid::API::WorkspaceUnGroupingNotification("", nullptr));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void testWorkspaceGroupUpdated() {
    auto wksp1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto wksp2 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    auto wksp3 = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    auto group =
        WorkspaceCreationHelper::createWorkspaceGroup(0, 10, 10, "group");

    AnalysisDataService::Instance().add("wksp1", wksp1);
    AnalysisDataService::Instance().add("wksp2", wksp2);
    AnalysisDataService::Instance().add("wksp3", wksp3);
    AnalysisDataService::Instance().addToGroup("group", "wksp1");
    AnalysisDataService::Instance().addToGroup("group", "wksp2");

    EXPECT_CALL(*mockPresenter.get(),
                notifyFromWorkspaceProvider(NotifyFlag::WorkspaceGroupUpdated))
        .Times(Exactly(1));

    AnalysisDataService::Instance().addToGroup("group", "wksp3");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

private:
  boost::shared_ptr<NiceMock<MockWorkspaceProviderNotifiable>> mockPresenter;
  ADSAdapter adapter;
};
