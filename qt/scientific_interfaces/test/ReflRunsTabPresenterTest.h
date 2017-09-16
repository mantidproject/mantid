#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidKernel/ConfigService.h"
#include "../ISISReflectometry/ReflRunsTabPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProgressableViewMockObject.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {
ACTION(ICATRuntimeException) { throw std::runtime_error(""); }
}

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflRunsTabPresenterTest : public CxxTest::TestSuite {

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
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);

    // Expect that the transfer methods get initialized on the view
    EXPECT_CALL(mockRunsTabView, setTransferMethods(_)).Times(Exactly(1));
    // Expect that the list of instruments gets initialized on the view
    EXPECT_CALL(mockRunsTabView, setInstrumentList(_, _)).Times(Exactly(1));

    // Constructor
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter));
  }

  void test_table_presenters_accept_this_presenter() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    MockDataProcessorPresenter mockTablePresenter_1;
    MockDataProcessorPresenter mockTablePresenter_2;
    MockDataProcessorPresenter mockTablePresenter_3;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter_1);
    tablePresenterVec.push_back(&mockTablePresenter_2);
    tablePresenterVec.push_back(&mockTablePresenter_3);

    // Expect that the table presenters accept this presenter as a workspace
    // receiver
    EXPECT_CALL(mockTablePresenter_1, accept(_)).Times(Exactly(1));
    EXPECT_CALL(mockTablePresenter_2, accept(_)).Times(Exactly(1));
    EXPECT_CALL(mockTablePresenter_3, accept(_)).Times(Exactly(1));

    // Constructor
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_2));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_3));
  }

  void test_presenter_sets_commands_when_ADS_changed() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);

    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);

    // Expect that the view clears the list of commands
    EXPECT_CALL(mockRunsTabView, clearCommands()).Times(Exactly(1));
    // Expect that the view is populated with the list of table commands
    EXPECT_CALL(mockRunsTabView, setTableCommandsProxy()).Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(mockRunsTabView, setRowCommandsProxy()).Times(Exactly(1));
    // The presenter is notified that something changed in the ADS
    presenter.notifyADSChanged(QSet<QString>());

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_preprocessingOptions() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    int group = 199;
    EXPECT_CALL(mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(group));
    EXPECT_CALL(mockMainPresenter, getTransmissionRuns(group)).Times(1);
    presenter.getPreprocessingOptionsAsString();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_processingOptions() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    int group = 199;
    EXPECT_CALL(mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(group));
    EXPECT_CALL(mockMainPresenter, getReductionOptions(group)).Times(1);
    presenter.getProcessingOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_postprocessingOptions() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    int group = 199;
    EXPECT_CALL(mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(group));
    EXPECT_CALL(mockMainPresenter, getStitchOptions(group)).Times(1);
    presenter.getPostprocessingOptions();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_when_group_changes_commands_are_updated() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter_0;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter_1;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter_2;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter_0);
    tablePresenterVec.push_back(&mockTablePresenter_1);
    tablePresenterVec.push_back(&mockTablePresenter_2);

    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    EXPECT_CALL(mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(1));
    // Commands should be updated with presenter of selected group
    EXPECT_CALL(mockTablePresenter_0, publishCommandsMocked()).Times(0);
    EXPECT_CALL(mockTablePresenter_1, publishCommandsMocked()).Times(1);
    EXPECT_CALL(mockTablePresenter_2, publishCommandsMocked()).Times(0);
    presenter.notify(IReflRunsTabPresenter::GroupChangedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_2));
  }

  void test_instrumentChanged() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    MockMainWindowPresenter mockMainPresenter;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);

    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    std::vector<std::string> instruments = {"INTER", "POLREF", "OFFSPEC",
                                            "SURF", "CRISP"};
    for (const auto &instrument : instruments) {
      EXPECT_CALL(mockRunsTabView, getSearchInstrument())
          .Times(Exactly(1))
          .WillOnce(Return(instrument));
      EXPECT_CALL(mockMainPresenter, setInstrumentName(instrument))
          .Times(Exactly(1));
      presenter.notify(IReflRunsTabPresenter::InstrumentChangedFlag);
      TS_ASSERT_EQUALS(Mantid::Kernel::ConfigService::Instance().getString(
                           "default.instrument"),
                       instrument);
    }
  }

  void test_invalid_ICAT_login_credentials_gives_user_critical() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    std::stringstream pythonSrc;
    pythonSrc << "try:\n";
    pythonSrc << "  algm = CatalogLoginDialog()\n";
    pythonSrc << "except:\n";
    pythonSrc << "  pass\n";

    EXPECT_CALL(mockRunsTabView, getSearchString())
        .Times(Exactly(1))
        .WillOnce(Return("12345"));
    EXPECT_CALL(mockMainPresenter, runPythonAlgorithm(pythonSrc.str()))
        .Times(Exactly(1))
        .WillRepeatedly(ICATRuntimeException());
    EXPECT_CALL(mockMainPresenter, giveUserCritical("Error Logging in:\n",
                                                    "login failed")).Times(1);
    EXPECT_CALL(
        mockMainPresenter,
        giveUserInfo("Error Logging in: Please press 'Search' to try again.",
                     "Login Failed")).Times(1);
    presenter.notify(IReflRunsTabPresenter::SearchFlag);
  }

  void test_pause() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);

    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    // Expect that the view enables the 'process' button and disables the
    // 'pause' button
    EXPECT_CALL(mockRunsTabView, setRowActionEnabled(0, true))
        .Times(Exactly(1));
    EXPECT_CALL(mockRunsTabView, setRowActionEnabled(1, false))
        .Times(Exactly(1));
    EXPECT_CALL(mockRunsTabView, setAutoreduceButtonEnabled(true))
        .Times(Exactly(1));
    // Pause presenter
    presenter.pause();

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_resume() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);

    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    // Expect that the view enables the 'process' button and disables the
    // 'pause' button
    EXPECT_CALL(mockRunsTabView, setRowActionEnabled(0, false))
        .Times(Exactly(1));
    EXPECT_CALL(mockRunsTabView, setRowActionEnabled(1, true))
        .Times(Exactly(1));
    EXPECT_CALL(mockRunsTabView, setAutoreduceButtonEnabled(false))
        .Times(Exactly(1));
    // Resume presenter
    presenter.resume();

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_confirmReductionPaused() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    // Expect that the main presenter is notified that data reduction is paused
    EXPECT_CALL(
        mockMainPresenter,
        notify(IReflMainWindowPresenter::Flag::ConfirmReductionPausedFlag))
        .Times(Exactly(1));

    presenter.confirmReductionPaused();

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }

  void test_confirmReductionResumed() {
    NiceMock<MockRunsTabView> mockRunsTabView;
    MockProgressableView mockProgress;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter;
    MockMainWindowPresenter mockMainPresenter;
    std::vector<DataProcessorPresenter *> tablePresenterVec;
    tablePresenterVec.push_back(&mockTablePresenter);
    ReflRunsTabPresenter presenter(&mockRunsTabView, &mockProgress,
                                   tablePresenterVec);
    presenter.acceptMainPresenter(&mockMainPresenter);

    // Expect that the main presenter is notified that data reduction is resumed
    EXPECT_CALL(
        mockMainPresenter,
        notify(IReflMainWindowPresenter::Flag::ConfirmReductionResumedFlag))
        .Times(Exactly(1));

    presenter.confirmReductionResumed();

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockRunsTabView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H */
