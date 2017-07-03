
#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEPRESENTERTEST_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QDir>
#include <QFileInfo>

#include "MantidQtMantidWidgets/ProjectSavePresenter.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "ProjectSaveMockObjects.h"

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================

class ProjectSavePresenterTest : public CxxTest::TestSuite {

private:
  NiceMock<MockProjectSaveView> m_view;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProjectSavePresenterTest *createSuite() {
    return new ProjectSavePresenterTest();
  }

  static void destroySuite(ProjectSavePresenterTest *suite) { delete suite; }

  ProjectSavePresenterTest() {}

  // Tests
  // ---------------------------------------------------

  void testConstructWithNoWorkspacesAndNoWindows() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    std::vector<WindowInfo> winInfo;
    std::vector<WorkspaceInfo> wsInfo;

    // View should be passed what workspaces exist and what windows
    // are currently included.
    // As the ADS is empty at this point all lists should be empty
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(wsInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(winInfo)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void testConstructWithSingleWorkspaceAndNoWindows() {
    std::vector<WindowInfo> winInfo;
    auto workspaces = setUpWorkspaces({"ws1"});
    std::vector<MantidQt::API::IProjectSerialisable *> windows;

    // View should be passed what workspaces exist and what windows
    // are currently included names.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(winInfo)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithTwoWorkspacesAndNoWindows() {
    auto workspaces = setUpWorkspaces({"ws1", "ws2"});
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    std::vector<WindowInfo> winInfo;

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillRepeatedly(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithOneWorkspaceAndOneWindow() {
    auto workspaces = setUpWorkspaces({"ws1"});

    WindowInfo info;
    info.name = "WindowName1Workspace";
    WindowStub window(info.name, {"ws1"});

    std::vector<MantidQt::API::IProjectSerialisable *> windows = {&window};
    std::vector<WindowInfo> winInfo = {info};

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithOneWorkspaceAndTwoWindows() {
    std::vector<std::string> wsNames = {"ws1"};
    auto workspaces = setUpWorkspaces(wsNames);

    WindowInfo win1Info, win2Info;
    win1Info.name = "WindowName1Workspace";
    win2Info.name = "WindowName2Workspace";

    WindowStub window1(win1Info.name, wsNames);
    WindowStub window2(win2Info.name, wsNames);

    std::vector<MantidQt::API::IProjectSerialisable *> windows = {&window1,
                                                                  &window2};
    std::vector<WindowInfo> winInfo = {win1Info, win2Info};

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithTwoWorkspacesAndOneWindow() {
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    auto workspaces = setUpWorkspaces(wsNames);

    WindowInfo info;
    info.name = "Windowname2Workspaces";
    WindowStub window(info.name, wsNames);

    std::vector<MantidQt::API::IProjectSerialisable *> windows = {&window};
    std::vector<WindowInfo> winInfo = {info};

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithTwoWorkspacesAndTwoWindows() {
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    auto workspaces = setUpWorkspaces(wsNames);

    WindowInfo win1Info, win2Info;
    win1Info.name = "WindowName1Workspace";
    win2Info.name = "WindowName2Workspace";

    WindowStub window1(win1Info.name, {wsNames[0]});
    WindowStub window2(win2Info.name, {wsNames[1]});

    std::vector<MantidQt::API::IProjectSerialisable *> windows = {&window1,
                                                                  &window2};
    std::vector<WindowInfo> winInfo = {win1Info, win2Info};

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testDeselectWorkspaceWithAWindow() {
    std::vector<std::string> wsNames = {"ws1"};
    auto workspaces = setUpWorkspaces(wsNames);

    WindowInfo info;
    info.name = "WindowName1Workspaces";
    WindowStub window(info.name, wsNames);

    std::vector<MantidQt::API::IProjectSerialisable *> windows = {&window};
    std::vector<std::string> windowNames = {info.name};
    std::vector<WindowInfo> winInfo = {info};

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    ON_CALL(m_view, getUncheckedWorkspaceNames())
        .WillByDefault(Return(wsNames));

    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, getUncheckedWorkspaceNames()).WillOnce(Return(wsNames));
    EXPECT_CALL(m_view, updateExcludedWindowsList(winInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, removeFromIncludedWindowsList(windowNames))
        .Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);
    presenter.notify(ProjectSavePresenter::Notification::UncheckWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testReselectWorkspaceWithAWindow() {
    std::vector<std::string> wsNames = {"ws1"};
    auto workspaces = setUpWorkspaces(wsNames);

    WindowInfo info;
    info.name = "WindowName1Workspaces";
    WindowStub window(info.name, wsNames);

    std::vector<MantidQt::API::IProjectSerialisable *> windows = {&window};
    std::vector<std::string> windowNames = {info.name};
    std::vector<WindowInfo> winInfo = {info};

    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    ON_CALL(m_view, getUncheckedWorkspaceNames())
        .WillByDefault(Return(wsNames));
    ON_CALL(m_view, getCheckedWorkspaceNames()).WillByDefault(Return(wsNames));

    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(2));
    EXPECT_CALL(m_view, getUncheckedWorkspaceNames()).WillOnce(Return(wsNames));
    EXPECT_CALL(m_view, updateExcludedWindowsList(winInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, getCheckedWorkspaceNames()).WillOnce(Return(wsNames));
    EXPECT_CALL(m_view, removeFromIncludedWindowsList(windowNames))
        .Times(Exactly(1));
    EXPECT_CALL(m_view, removeFromExcludedWindowsList(windowNames))
        .Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);
    presenter.notify(ProjectSavePresenter::Notification::UncheckWorkspace);
    presenter.notify(ProjectSavePresenter::Notification::CheckWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testPrepareProjectFolder_withFile() {
    std::vector<WindowInfo> winInfo;
    std::vector<WorkspaceInfo> wsInfo;
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    QFileInfo fi(".");
    QString filePath =
        fi.absolutePath() + "/mantidprojecttest/mantidprojecttest.mantid";

    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    ON_CALL(m_view, getProjectPath()).WillByDefault(Return(filePath));

    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(wsInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));

    EXPECT_CALL(m_view, getProjectPath()).Times(Exactly(1));
    EXPECT_CALL(m_view, setProjectPath(filePath)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);
    presenter.notify(ProjectSavePresenter::Notification::PrepareProjectFolder);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void testPrepareProjectFolder_withFolder() {
    std::vector<WindowInfo> winInfo;
    std::vector<WorkspaceInfo> wsInfo;
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    QFileInfo fi(".");
    QString filePath = fi.absolutePath() + "/mantidprojecttest";

    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    ON_CALL(m_view, getProjectPath()).WillByDefault(Return(filePath));

    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(wsInfo)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(winInfo)).Times(Exactly(1));

    EXPECT_CALL(m_view, getProjectPath()).Times(Exactly(1));
    EXPECT_CALL(m_view, setProjectPath(filePath + "/mantidprojecttest.mantid"))
        .Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);
    presenter.notify(ProjectSavePresenter::Notification::PrepareProjectFolder);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));

    // clean up
    fi.absoluteDir().rmdir(filePath);
  }

  //============================================================================
  // Test Helper Methods
  //============================================================================

  /**
 * Create some workspaces and add them to the ADS
 * @param workspaces :: List of workspace names
 * @return a vector of workspace info structs
 */
  std::vector<WorkspaceInfo>
  setUpWorkspaces(const std::vector<std::string> &workspaces) {
    std::vector<WorkspaceInfo> wsInfo;

    for (auto &name : workspaces) {
      auto ws = WorkspaceCreationHelper::create1DWorkspaceRand(10, true);
      WorkspaceCreationHelper::storeWS(name, ws);
      WorkspaceInfo info;
      info.name = name;
      wsInfo.push_back(info);
    }

    return wsInfo;
  }

  /**
 * Remove a list of workspaces from the ADS
 * @param workspaces :: List of workspace names
 */
  void tearDownWorkspaces(const std::vector<WorkspaceInfo> &workspaces) {
    for (auto &info : workspaces) {
      WorkspaceCreationHelper::removeWS(info.name);
    }
  }
};

#endif
