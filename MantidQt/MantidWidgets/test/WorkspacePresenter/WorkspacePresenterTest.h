#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceDockViewMockObjects.h"

using namespace testing;

class WorkspacePresenterTest : public CxxTest::TestSuite {
  static WorkspacePresenterTest *createSuite() {
    return new WorkspacePresenterTest();
  }
  static void destroySuite(WorkspacePresenterTest *suite) { delete suite; }


  void testLoadWorkspaceFromDock() {
  }

  void testSomething() {
	  TS_FAIL("Create tests for WorkspacePresenter");
  }
};