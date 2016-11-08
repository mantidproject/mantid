
#ifndef MANTID_CUSTOMINTERFACES_PROJECTSAVEPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_PROJECTSAVEPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QList>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtCustomInterfaces/ProjectSavePresenter.h"
#include "ProjectSaveMockObjects.h"

using namespace MantidQt::API;
using namespace MantidQt::CustomInterfaces;
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
    std::vector<std::string> empty;
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    // View should be passed what workspaces exist and what windows
    // are currently included.
    // As the ADS is empty at this point all lists should be empty
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(empty)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(empty)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void testConstructWithSingleWorkspaceAndNoWindows() {
    std::vector<std::string> empty;
    std::vector<std::string> workspaces = {"ws1"};

    setUpWorkspaces(workspaces);
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(empty)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithTwoWorkspacesAndNoWindows() {
    std::vector<std::string> empty;
    std::vector<std::string> workspaces = {"ws1", "ws2"};

    setUpWorkspaces(workspaces);
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillRepeatedly(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(empty)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithOneWorkspaceAndOneWindow() {
    std::vector<std::string> empty;
    std::vector<std::string> workspaces = {"ws1"};

    std::vector<MantidQt::API::IProjectSerialisable*> windows;
    std::vector<std::string> windowNames = {"WindowName1Workspace"};
    WindowStub window(windowNames[0], workspaces);
    windows.push_back(&window);

    setUpWorkspaces(workspaces);

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(windowNames)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithOneWorkspaceAndTwoWindows() {
    std::vector<std::string> empty;
    std::vector<std::string> workspaces = {"ws1"};

    std::vector<MantidQt::API::IProjectSerialisable*> windows;
    std::vector<std::string> windowNames = {"WindowName1Workspace",
                                            "WindowName2Workspace"};
    WindowStub window1(windowNames[0], workspaces);
    WindowStub window2(windowNames[1], workspaces);
    windows.push_back(&window1);
    windows.push_back(&window2);

    setUpWorkspaces(workspaces);

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(windowNames)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }


  void testConstructWithTwoWorkspacesAndOneWindow() {
    std::vector<std::string> empty;
    std::vector<std::string> workspaces = {"ws1", "ws2"};

    std::vector<MantidQt::API::IProjectSerialisable*> windows;
    std::vector<std::string> windowNames = {"WindowName2Workspaces"};
    WindowStub window(windowNames[0], workspaces);
    windows.push_back(&window);


    setUpWorkspaces(workspaces);

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(windowNames)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testConstructWithTwoWorkspacesAndTwoWindows() {
    std::vector<std::string> empty;
    std::vector<std::string> workspaces = {"ws1", "ws2"};

    std::vector<MantidQt::API::IProjectSerialisable*> windows;
    std::vector<std::string> windowNames = {"WindowName1Workspace",
                                            "WindowName2Workspace"};
    WindowStub window1(windowNames[0], {workspaces[0]});
    WindowStub window2(windowNames[1], {workspaces[1]});
    windows.push_back(&window1);
    windows.push_back(&window2);

    setUpWorkspaces(workspaces);

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(windowNames)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(0));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testDeselectWorkspaceWithAWindow() {
    std::vector<std::string> workspaces = {"ws1"};
    std::vector<MantidQt::API::IProjectSerialisable*> windows;
    std::vector<std::string> windowNames = {"WindowName1Workspaces"};
    WindowStub window(windowNames[0], workspaces);
    windows.push_back(&window);

    setUpWorkspaces(workspaces);

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    ON_CALL(m_view, getUncheckedWorkspaceNames())
        .WillByDefault(Return(workspaces));

    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(windowNames))
        .Times(Exactly(1));
    EXPECT_CALL(m_view, getUncheckedWorkspaceNames())
        .WillOnce(Return(workspaces));
    EXPECT_CALL(m_view, updateExcludedWindowsList(windowNames))
        .Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);
    presenter.notify(ProjectSavePresenter::Notification::UncheckWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  void testReselectWorkspaceWithAWindow() {
    std::vector<std::string> workspaces = {"ws1"};
    std::vector<MantidQt::API::IProjectSerialisable*> windows;
    std::vector<std::string> windowNames = {"WindowName1Workspaces"};
    WindowStub window(windowNames[0], workspaces);
    windows.push_back(&window);

    setUpWorkspaces(workspaces);

    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    ON_CALL(m_view, getUncheckedWorkspaceNames())
        .WillByDefault(Return(workspaces));
    ON_CALL(m_view, getCheckedWorkspaceNames())
        .WillByDefault(Return(workspaces));

    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(windowNames))
        .Times(Exactly(2));
    EXPECT_CALL(m_view, getUncheckedWorkspaceNames())
        .WillOnce(Return(workspaces));
    EXPECT_CALL(m_view, updateExcludedWindowsList(windowNames))
        .Times(Exactly(1));
    EXPECT_CALL(m_view, getCheckedWorkspaceNames())
        .WillOnce(Return(workspaces));

    ProjectSavePresenter presenter(&m_view);
    presenter.notify(ProjectSavePresenter::Notification::UncheckWorkspace);
    presenter.notify(ProjectSavePresenter::Notification::CheckWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    tearDownWorkspaces(workspaces);
  }

  //============================================================================
  // Test Helper Methods
  //============================================================================

  /**
 * Create some workspaces and add them to the ADS
 * @param workspaces :: List of workspace names
 */
  void setUpWorkspaces(const std::vector<std::string> &workspaces) {
    for (auto & name : workspaces) {
      auto ws = WorkspaceCreationHelper::Create1DWorkspaceRand(10);
      WorkspaceCreationHelper::storeWS(name, ws);
    }
  }

  /**
 * Remove a list of workspaces from the ADS
 * @param workspaces :: List of workspace names
 */
  void tearDownWorkspaces(const std::vector<std::string> &workspaces) {
    for (auto &name : workspaces) {
      WorkspaceCreationHelper::removeWS(name);
    }
  }

};

#endif
