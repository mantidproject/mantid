#ifndef MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflMainWindowPresenter.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainWindowPresenterTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMainWindowPresenterTest *createSuite() {
    return new ReflMainWindowPresenterTest();
  }
  static void destroySuite(ReflMainWindowPresenterTest *suite) { delete suite; }

  ReflMainWindowPresenterTest() {}

  void testGetTransmissionOptions() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockSettingsPresenter, getTransmissionOptions(0))
        .Times(Exactly(1));
    presenter.getTransmissionOptions(0);

    EXPECT_CALL(mockSettingsPresenter, getTransmissionOptions(1))
        .Times(Exactly(1));
    presenter.getTransmissionOptions(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSettingsPresenter));
  }

  void testGetReductionOptions() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockSettingsPresenter, getReductionOptions(0))
        .Times(Exactly(1));
    presenter.getReductionOptions(0);

    EXPECT_CALL(mockSettingsPresenter, getReductionOptions(1))
        .Times(Exactly(1));
    presenter.getReductionOptions(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSettingsPresenter));
  }

  void testStitchOptions() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockSettingsPresenter, getStitchOptions(0)).Times(Exactly(1));
    presenter.getStitchOptions(0);

    EXPECT_CALL(mockSettingsPresenter, getStitchOptions(1)).Times(Exactly(1));
    presenter.getStitchOptions(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSettingsPresenter));
  }

  void testAskUserString() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockView, askUserString("Prompt", "Title", "Value"))
        .Times(Exactly(1));
    presenter.askUserString("Prompt", "Title", "Value");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testAskUserYesNo() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockView, askUserYesNo("Prompt", "Title")).Times(Exactly(1));
    presenter.askUserYesNo("Prompt", "Title");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGiveUserWarning() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockView, giveUserWarning("Prompt", "Title")).Times(Exactly(1));
    presenter.giveUserWarning("Prompt", "Title");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGiveUserCritical() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockView, giveUserCritical("Prompt", "Title"))
        .Times(Exactly(1));
    presenter.giveUserCritical("Prompt", "Title");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGiveUserInfo() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockView, giveUserInfo("Prompt", "Title")).Times(Exactly(1));
    presenter.giveUserInfo("Prompt", "Title");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testUserPythonCode() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    MockSaveTabPresenter mockSaveTabPresenter;
    ReflMainWindowPresenter presenter(&mockView, &mockRunsPresenter,
                                      &mockSettingsPresenter,
                                      &mockSaveTabPresenter);

    EXPECT_CALL(mockView, runPythonAlgorithm("Python code to run"))
        .Times(Exactly(1));
    presenter.runPythonAlgorithm("Python code to run");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTERTEST_H */
