#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidKernel/ConfigService.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflRunsTabPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ProgressableViewMockObject.h"
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

  ReflRunsTabPresenterTest()
      : m_tablePresenterVec({&m_mockTablePresenter}),
        m_presenter(&m_mockRunsTabView, &m_mockProgress, m_tablePresenterVec) {}

  void setUpPresenter(std::vector<DataProcessorPresenter *> &tablePresenters) {
    m_presenter = ReflRunsTabPresenter(&m_mockRunsTabView, &m_mockProgress,
                                       tablePresenters);
    m_presenter.acceptMainPresenter(&m_mockMainPresenter);
  }

  void setUpPresenter() { setUpPresenter(m_tablePresenterVec); }

  void setUp() override { setUpPresenter(); }

  void test_constructor_sets_possible_transfer_methods() {
    // Expect that the transfer methods get initialized on the view
    EXPECT_CALL(m_mockRunsTabView, setTransferMethods(_)).Times(Exactly(1));
    // Expect that the list of instruments gets initialized on the view
    EXPECT_CALL(m_mockRunsTabView, setInstrumentList(_, _)).Times(Exactly(1));

    setUpPresenter();
  }

  void test_table_presenters_accept_this_presenter() {
    MockDataProcessorPresenter mockTablePresenter_1;
    MockDataProcessorPresenter mockTablePresenter_2;
    MockDataProcessorPresenter mockTablePresenter_3;
    std::vector<DataProcessorPresenter *> tablePresenterVec{
        {&mockTablePresenter_1, &mockTablePresenter_2, &mockTablePresenter_3}};

    // Expect that the table presenters accept this presenter as a workspace
    // receiver
    EXPECT_CALL(mockTablePresenter_1, accept(_)).Times(Exactly(1));
    EXPECT_CALL(mockTablePresenter_2, accept(_)).Times(Exactly(1));
    EXPECT_CALL(mockTablePresenter_3, accept(_)).Times(Exactly(1));

    setUpPresenter(tablePresenterVec);
  }

  void test_presenter_sets_commands_when_ADS_changed() {
    EXPECT_CALL(m_mockRunsTabView, clearCommands()).Times(Exactly(1));
    // Expect that the view is populated with the list of table commands
    EXPECT_CALL(m_mockRunsTabView, setTableCommandsProxy()).Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(m_mockRunsTabView, setRowCommandsProxy()).Times(Exactly(1));
    // The presenter is notified that something changed in the ADS
    m_presenter.notifyADSChanged(QSet<QString>());
  }

  void test_preprocessingOptions() {
    const auto group = 199;
    EXPECT_CALL(m_mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(group));
    EXPECT_CALL(m_mockMainPresenter, getTransmissionRuns(group)).Times(1);

    m_presenter.getPreprocessingOptionsAsString();
  }

  void test_processingOptions() {
    const auto group = 199;
    EXPECT_CALL(m_mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(group));
    EXPECT_CALL(m_mockMainPresenter, getReductionOptions(group)).Times(1);

    m_presenter.getProcessingOptions();
  }

  void test_postprocessingOptions() {
    const auto group = 199;
    EXPECT_CALL(m_mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(group));
    EXPECT_CALL(m_mockMainPresenter, getStitchOptions(group)).Times(1);

    m_presenter.getPostprocessingOptions();
  }

  void test_when_group_changes_commands_are_updated() {
    NiceMock<MockDataProcessorPresenter> mockTablePresenter_0;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter_1;
    NiceMock<MockDataProcessorPresenter> mockTablePresenter_2;
    std::vector<DataProcessorPresenter *> tablePresenterVec{
        {&mockTablePresenter_0, &mockTablePresenter_1, &mockTablePresenter_2}};

    setUpPresenter(tablePresenterVec);

    EXPECT_CALL(m_mockRunsTabView, getSelectedGroup())
        .Times(Exactly(1))
        .WillOnce(Return(1));

    // Commands should be updated with presenter of selected group
    EXPECT_CALL(mockTablePresenter_0, publishCommandsMocked()).Times(0);
    EXPECT_CALL(mockTablePresenter_1, publishCommandsMocked()).Times(1);
    EXPECT_CALL(mockTablePresenter_2, publishCommandsMocked()).Times(0);

    m_presenter.notify(IReflRunsTabPresenter::GroupChangedFlag);
  }

  void test_instrumentChanged() {
    std::vector<std::string> instruments = {"INTER", "POLREF", "OFFSPEC",
                                            "SURF", "CRISP"};
    for (const auto &instrument : instruments) {
      EXPECT_CALL(m_mockRunsTabView, getSearchInstrument())
          .Times(Exactly(1))
          .WillOnce(Return(instrument));
      EXPECT_CALL(m_mockMainPresenter, setInstrumentName(instrument))
          .Times(Exactly(1));
      m_presenter.notify(IReflRunsTabPresenter::InstrumentChangedFlag);
      TS_ASSERT_EQUALS(Mantid::Kernel::ConfigService::Instance().getString(
                           "default.instrument"),
                       instrument);
    }
  }

  void test_invalid_ICAT_login_credentials_gives_user_critical() {
    std::stringstream pythonSrc;
    pythonSrc << "try:\n";
    pythonSrc << "  algm = CatalogLoginDialog()\n";
    pythonSrc << "except:\n";
    pythonSrc << "  pass\n";

    EXPECT_CALL(m_mockRunsTabView, getSearchString())
        .Times(Exactly(1))
        .WillOnce(Return("12345"));
    EXPECT_CALL(m_mockMainPresenter, runPythonAlgorithm(pythonSrc.str()))
        .Times(Exactly(1))
        .WillRepeatedly(ICATRuntimeException());
    EXPECT_CALL(m_mockMainPresenter, giveUserCritical("Error Logging in:\n",
                                                      "login failed")).Times(1);
    EXPECT_CALL(
        m_mockMainPresenter,
        giveUserInfo("Error Logging in: Please press 'Search' to try again.",
                     "Login Failed")).Times(1);
    m_presenter.notify(IReflRunsTabPresenter::SearchFlag);
  }

  void test_pause_disables_pause_when_pause_requested() {
    // Expect view disables the 'pause' button only
    EXPECT_CALL(m_mockRunsTabView, disableAction(DataProcessorAction::PAUSE))
        .Times(Exactly(1));

    m_presenter.pause();
  }

  void test_notifies_main_presenter_on_resume() {
    EXPECT_CALL(
        m_mockMainPresenter,
        notify(IReflMainWindowPresenter::Flag::ConfirmReductionResumedFlag))
        .Times(Exactly(1));

    m_presenter.resume();
  }

  void expectPreventsTableModificationThroughReflectometryMenu() {
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(ReflectometryAction::OPEN_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(ReflectometryAction::NEW_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(ReflectometryAction::SAVE_TABLE_AS));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(ReflectometryAction::SAVE_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(ReflectometryAction::IMPORT_TBL));
  }

  void expectPreventsTableModificationThroughDataProcessor() {
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::INSERT_ROW_AFTER))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::INSERT_GROUP_AFTER))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::GROUP_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::COPY_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::CUT_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::PASTE_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::CLEAR_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::DELETE_ROW))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                disableAction(DataProcessorAction::DELETE_GROUP))
        .Times(Exactly(1));
  }

  void expectPreventsTableModification() {
    expectPreventsTableModificationThroughReflectometryMenu();
    expectPreventsTableModificationThroughDataProcessor();
  }

  void test_prevents_table_modification_on_resume() {
    expectPreventsTableModification();
    m_presenter.resume();
  }

  void test_disables_processing_on_resume() {
    EXPECT_CALL(m_mockRunsTabView, disableAction(DataProcessorAction::PROCESS))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView, enableAction(DataProcessorAction::PAUSE))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView, disableAutoreduceButton()).Times(Exactly(1));

    m_presenter.resume();
  }

  void test_re_enable_pause_on_resume() {
    EXPECT_CALL(m_mockRunsTabView, enableAction(DataProcessorAction::PAUSE))
        .Times(Exactly(1));
    m_presenter.resume();
  }

  void test_notifies_main_presenter_on_pause_confirmation() {
    // Expect main presenter is notified that data reduction is paused
    // Expect view enables the 'process' button
    EXPECT_CALL(
        m_mockMainPresenter,
        notify(IReflMainWindowPresenter::Flag::ConfirmReductionPausedFlag))
        .Times(Exactly(1));
    m_presenter.confirmReductionPaused();
  }

  void expectAllowsTableModificationThroughReflectometryMenu() {
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(ReflectometryAction::OPEN_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(ReflectometryAction::NEW_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(ReflectometryAction::SAVE_TABLE_AS));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(ReflectometryAction::SAVE_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(ReflectometryAction::IMPORT_TBL));
  }

  void expectAllowsTableModificationThroughDataProcessor() {
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::INSERT_ROW_AFTER))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::INSERT_GROUP_AFTER))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::GROUP_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::COPY_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::CUT_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::PASTE_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::CLEAR_SELECTED))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::DELETE_ROW))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView,
                enableAction(DataProcessorAction::DELETE_GROUP))
        .Times(Exactly(1));
  }

  void expectAllowsTableModification() {
    expectAllowsTableModificationThroughReflectometryMenu();
    expectAllowsTableModificationThroughDataProcessor();
  }

  void test_modification_re_enabled_on_pause_confirmation() {
    expectAllowsTableModification();

    m_presenter.confirmReductionPaused();
  }

  void test_processing_re_enabled_on_pause_confirmation() {
    EXPECT_CALL(m_mockRunsTabView, enableAction(DataProcessorAction::PROCESS))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView, enableAutoreduceButton()).Times(Exactly(1));

    m_presenter.confirmReductionPaused();
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mockRunsTabView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mockProgress));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mockTablePresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mockMainPresenter));
  }

protected:
  MockMainWindowPresenter m_mockMainPresenter;
  NiceMock<MockDataProcessorPresenter> m_mockTablePresenter;
  NiceMock<MockRunsTabView> m_mockRunsTabView;
  MockProgressableView m_mockProgress;
  std::vector<DataProcessorPresenter *> m_tablePresenterVec;
  ReflRunsTabPresenter m_presenter;
};

#endif /* MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H */
