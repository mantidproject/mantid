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

  void testLoadWorkspaceFromDock() {
    auto mockView = boost::make_shared<NiceMock<MockWorkspaceDockView>>();
    mockView->init();

    auto presenter = mockView->getPresenterSharedPtr();

    EXPECT_CALL(*mockView.get(), showLoadDialog()).Times(1);

    presenter->notifyFromView(ViewNotifiable::Flag::LoadWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testLoadWorkspaceExternal() {
    auto mockView = boost::make_shared<NiceMock<MockWorkspaceDockView>>();
    mockView->init();

    auto presenter = mockView->getPresenterSharedPtr();

    auto wksp = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    EXPECT_CALL(*mockView.get(), updateTree(_)).Times(AtLeast(1));

    AnalysisDataService::Instance().add("wksp", wksp);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    AnalysisDataService::Instance().remove("wksp");
  }

  void testSomething() { TS_FAIL("Create tests for WorkspacePresenter"); }
};