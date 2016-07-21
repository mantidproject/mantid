#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflRunsTabPresenter.h"

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ProgressableViewMockObject.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflRunsTabPresenterTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflRunsTabPresenterTest *createSuite() {
    return new ReflRunsTabPresenterTest();
  }
  static void destroySuite(ReflRunsTabPresenterTest *suite) { delete suite; }

  ReflRunsTabPresenterTest() { FrameworkManager::Instance(); }

  void test_constructor_sets_possible_transfer_methods() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    MockDataProcessorPresenter mockTablePresenter;

    // Expect that the table presenter accepts this presenter as a workspace
    // receiver
    EXPECT_CALL(mockTablePresenter, accept(_)).Times(Exactly(1));

    // Expect that the transfer methods get initialized on the view
    EXPECT_CALL(mockView, setTransferMethods(_)).Times(Exactly(1));
    // Expect that the list of instruments gets initialized on the view
    EXPECT_CALL(mockView, setInstrumentList(_, _)).Times(Exactly(1));

    // Constructor
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter));
  }

  void test_presenter_sets_commands_when_notified() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;

    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);

    // Expect that the view clears the list of commands
    EXPECT_CALL(mockView, clearCommands()).Times(Exactly(1));
    // Expect that the view is populated with the list of table commands
    EXPECT_CALL(mockView, setTableCommandsProxy()).Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(mockView, setRowCommandsProxy()).Times(Exactly(1));
    // The presenter is notified that something changed in the ADS
    presenter.notify(DataProcessorMainPresenter::ADSChangedFlag);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_askUserString() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter,
                askUserString("Prompt", "Title", "DefaultValue"))
        .Times(1);
    presenter.askUserString("Prompt", "Title", "DefaultValue");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_askUserYesNo() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, askUserYesNo("Prompt", "Title")).Times(1);
    presenter.askUserYesNo("Prompt", "Title");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_giveUserWarning() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, giveUserWarning("Prompt", "Warning Message"))
        .Times(1);
    presenter.giveUserWarning("Prompt", "Warning Message");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_giveUserCritical() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter,
                giveUserCritical("Prompt", "Critical Message"))
        .Times(1);
    presenter.giveUserCritical("Prompt", "Critical Message");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_runPythonCode() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, runPythonAlgorithm("Python code to run"))
        .Times(1);
    presenter.runPythonAlgorithm("Python code to run");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_processingOptions() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, getProcessingOptions()).Times(1);
    presenter.getProcessingOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_postprocessingOptions() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions()).Times(1);
    presenter.getPostprocessingOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H */
