#include <cxxtest\TestSuite.h>

class WorkspacePresenterTest : public CxxTest::TestSuite {
  static WorkspacePresenterTest *createSuite() {
    return new WorkspacePresenterTest();
  }
  static void destroySuite(WorkspacePresenterTest *suite) { delete suite; }

  void testSomething() {
	  TS_FAIL("Create tests for WorkspacePresenter");
  }
};