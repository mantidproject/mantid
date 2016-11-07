
#ifndef MANTID_CUSTOMINTERFACES_PROJECTSAVEPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_PROJECTSAVEPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QList>

#include "MantidQtCustomInterfaces/ProjectSavePresenter.h"
#include "ProjectSaveMockObjects.h"

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
    std::set<std::string> empty;
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    // View should be passed what workspaces exist and what windows
    // are currently included.
    // As the ADS is empty at this point all lists should be empty
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(empty)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(empty)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void testConstructWithSingleWorkspaceAndNoWindows() {
    std::set<std::string> empty;
    std::set<std::string> workspaces = {"ws1"};
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    // View should be passed what workspaces exist and what windows
    // are currently included.
    ON_CALL(m_view, getWindows()).WillByDefault(Return(windows));
    EXPECT_CALL(m_view, getWindows()).WillOnce(Return(windows));
    EXPECT_CALL(m_view, updateWorkspacesList(workspaces)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateIncludedWindowsList(empty)).Times(Exactly(1));
    EXPECT_CALL(m_view, updateExcludedWindowsList(empty)).Times(Exactly(1));

    ProjectSavePresenter presenter(&m_view);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void testSaveAllEmptyADS() {
    ProjectSavePresenter presenter(&m_view);
    TS_FAIL("Test is unimplemented");
  }

  void testSavePartialEmptyADS() {
    ProjectSavePresenter presenter(&m_view);
    TS_FAIL("Test is unimplemented");
  }

  void testCancel() {
    ProjectSavePresenter presenter(&m_view);
    TS_FAIL("Test is unimplemented");
  }

  void testDeselectWorkspace() {
    ProjectSavePresenter presenter(&m_view);
    TS_FAIL("Test is unimplemented");

  }

  void testReselectWorkspace() {
    ProjectSavePresenter presenter(&m_view);
    TS_FAIL("Test is unimplemented");

  }

  void testDeselectAllWorkspaces() {
    ProjectSavePresenter presenter(&m_view);
    TS_FAIL("Test is unimplemented");
  }

  void testReselectAllWorkspaces() {
    ProjectSavePresenter presenter(&m_view);
    TS_FAIL("Test is unimplemented");
  }
};

#endif
