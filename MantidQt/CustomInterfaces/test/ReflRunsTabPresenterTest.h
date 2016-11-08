#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflRunsTabPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ProgressableViewMockObject.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
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

  ReflRunsTabPresenterTest() {}

  void test_constructor_sets_possible_transfer_methods() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    MockDataProcessorPresenter mockTablePresenter;

    // Expect that the table presenter accepts this presenter as a workspace
    // receiver
    EXPECT_CALL(mockTablePresenter, accept(_)).Times(Exactly(1));

    // Expect that the transfer methods get initialized on the view
    EXPECT_CALL(mockRunsTabView, setTransferMethods(_)).Times(Exactly(1));
    // Expect that the list of instruments gets initialized on the view
    EXPECT_CALL(mockRunsTabView, setInstrumentList(_, _)).Times(Exactly(1));

    // Constructor
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter));
  }

  void test_presenter_sets_commands_when_notified() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;

    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);

    // Expect that the view clears the list of commands
    EXPECT_CALL(mockRunsTabView, clearCommands()).Times(Exactly(1));
    // Expect that the view is populated with the list of table commands
    EXPECT_CALL(mockRunsTabView, setTableCommandsProxy()).Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(mockRunsTabView, setRowCommandsProxy()).Times(Exactly(1));
    // The presenter is notified that something changed in the ADS
    presenter.notify(DataProcessorMainPresenter::ADSChangedFlag);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_askUserString() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter,
                askUserString("Prompt", "Title", "DefaultValue")).Times(1);
    presenter.askUserString("Prompt", "Title", "DefaultValue");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_askUserYesNo() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, askUserYesNo("Prompt", "Title")).Times(1);
    presenter.askUserYesNo("Prompt", "Title");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_giveUserWarning() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, giveUserWarning("Prompt", "Warning Message"))
        .Times(1);
    presenter.giveUserWarning("Prompt", "Warning Message");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_giveUserCritical() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter,
                giveUserCritical("Prompt", "Critical Message")).Times(1);
    presenter.giveUserCritical("Prompt", "Critical Message");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_runPythonCode() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, runPythonAlgorithm("Python code to run"))
        .Times(1);
    presenter.runPythonAlgorithm("Python code to run");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_preprocessingOptions() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, getTransmissionOptions()).Times(1);
    presenter.getPreprocessingOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_processingOptions() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, getReductionOptions()).Times(1);
    presenter.getProcessingOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void test_postprocessingOptions() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   &mockTablePresenter);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockMainPresenter, getStitchOptions()).Times(1);
    presenter.getPostprocessingOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H */
