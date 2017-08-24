#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflRunsTabPresenter.h"
#include "MantidKernel/ConfigService.h"
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
  
  static auto constexpr OPEN_TABLE = 0;
  static auto constexpr NEW_TABLE = 1;
  static auto constexpr SAVE_TABLE = 2;
  static auto constexpr SAVE_TABLE_AS = 3;
  static auto constexpr IMPORT_TBL_FILE = 5;
  
  static auto constexpr PROCESS = 0;
  static auto constexpr PAUSE = 1;
  static auto constexpr INSERT_ROW_AFTER = 10;
  static auto constexpr INSERT_GROUP_AFTER = 11;
  static auto constexpr GROUP_SELECTED = 13;
  static auto constexpr COPY_SELECTED = 14;
  static auto constexpr CUT_SELECTED = 15;
  static auto constexpr PASTE_SELECTED = 16;
  static auto constexpr CLEAR_SELECTED = 17;
  static auto constexpr DELETE_ROW = 19;
  static auto constexpr DELETE_GROUP = 20;

  ReflRunsTabPresenterTest()
      : m_tablePresenterVec({&m_mockTablePresenter}),
        m_presenter(&m_mockRunsTabView, &m_mockProgress, m_tablePresenterVec) {
    ON_CALL(m_mockTablePresenter, indexOfCommand(TableAction::OPEN_TABLE))
        .WillByDefault(Return(OPEN_TABLE));
    ON_CALL(m_mockTablePresenter, indexOfCommand(TableAction::NEW_TABLE))
        .WillByDefault(Return(NEW_TABLE));
    ON_CALL(m_mockTablePresenter, indexOfCommand(TableAction::SAVE_TABLE))
        .WillByDefault(Return(SAVE_TABLE));
    ON_CALL(m_mockTablePresenter, indexOfCommand(TableAction::SAVE_TABLE_AS))
        .WillByDefault(Return(SAVE_TABLE_AS));
    ON_CALL(m_mockTablePresenter, indexOfCommand(TableAction::IMPORT_TBL_FILE))
        .WillByDefault(Return(IMPORT_TBL_FILE));
    
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::PROCESS))
        .WillByDefault(Return(PROCESS));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::PAUSE))
        .WillByDefault(Return(PAUSE));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::INSERT_ROW_AFTER))
        .WillByDefault(Return(INSERT_ROW_AFTER));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::INSERT_GROUP_AFTER))
        .WillByDefault(Return(INSERT_GROUP_AFTER));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::GROUP_SELECTED))
        .WillByDefault(Return(GROUP_SELECTED));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::COPY_SELECTED))
        .WillByDefault(Return(COPY_SELECTED));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::CUT_SELECTED))
        .WillByDefault(Return(CUT_SELECTED));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::PASTE_SELECTED))
        .WillByDefault(Return(PASTE_SELECTED));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::CLEAR_SELECTED))
        .WillByDefault(Return(CLEAR_SELECTED));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::DELETE_ROW))
        .WillByDefault(Return(DELETE_ROW));
    ON_CALL(m_mockTablePresenter, indexOfCommand(EditAction::DELETE_GROUP))
        .WillByDefault(Return(DELETE_GROUP));
  }

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
    EXPECT_CALL(m_mockRunsTabView, setEditMenuCommandsProxy())
        .Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(m_mockRunsTabView, setReflectometryMenuCommandsProxy())
        .Times(Exactly(1));
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
    EXPECT_CALL(mockTablePresenter_0, getTableCommandsMocked()).Times(0);
    EXPECT_CALL(mockTablePresenter_1, getTableCommandsMocked()).Times(1);
    EXPECT_CALL(mockTablePresenter_2, getTableCommandsMocked()).Times(0);

    EXPECT_CALL(mockTablePresenter_0, getEditCommandsMocked()).Times(0);
    EXPECT_CALL(mockTablePresenter_1, getEditCommandsMocked()).Times(1);
    EXPECT_CALL(mockTablePresenter_2, getEditCommandsMocked()).Times(0);

    m_presenter.notify(IReflRunsTabPresenter::GroupChangedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockTablePresenter_2));
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
    EXPECT_CALL(m_mockMainPresenter,
                giveUserCritical("Error Logging in:\n", "login failed"))
        .Times(1);
    EXPECT_CALL(
        m_mockMainPresenter,
        giveUserInfo("Error Logging in: Please press 'Search' to try again.",
                     "Login Failed"))
        .Times(1);
    m_presenter.notify(IReflRunsTabPresenter::SearchFlag);
  }

  void test_pause_disables_pause_when_pause_requested() {
    // Expect view disables the 'pause' button only
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(PAUSE));

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
    EXPECT_CALL(m_mockRunsTabView, disableReflectometryMenuAction(OPEN_TABLE));
    EXPECT_CALL(m_mockRunsTabView, disableReflectometryMenuAction(NEW_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                disableReflectometryMenuAction(SAVE_TABLE_AS));
    EXPECT_CALL(m_mockRunsTabView, disableReflectometryMenuAction(SAVE_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                disableReflectometryMenuAction(IMPORT_TBL_FILE));
  }

  void expectPreventsTableModificationThroughDataProcessor() {
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(INSERT_ROW_AFTER));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(INSERT_GROUP_AFTER));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(GROUP_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(COPY_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(CUT_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(PASTE_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(CLEAR_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(DELETE_ROW));
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(DELETE_GROUP));
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
    EXPECT_CALL(m_mockRunsTabView, disableEditMenuAction(PROCESS))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(PAUSE))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView, disableAutoreduce()).Times(Exactly(1));

    m_presenter.resume();
  }

  void test_re_enable_pause_on_resume() {
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(PAUSE))
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
    EXPECT_CALL(m_mockRunsTabView, enableReflectometryMenuAction(OPEN_TABLE));
    EXPECT_CALL(m_mockRunsTabView, enableReflectometryMenuAction(NEW_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                enableReflectometryMenuAction(SAVE_TABLE_AS));
    EXPECT_CALL(m_mockRunsTabView, enableReflectometryMenuAction(SAVE_TABLE));
    EXPECT_CALL(m_mockRunsTabView,
                enableReflectometryMenuAction(IMPORT_TBL_FILE));
  }

  void expectAllowsTableModificationThroughDataProcessor() {
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(INSERT_ROW_AFTER));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(INSERT_GROUP_AFTER));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(GROUP_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(COPY_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(CUT_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(PASTE_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(CLEAR_SELECTED));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(DELETE_ROW));
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(DELETE_GROUP));
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
    EXPECT_CALL(m_mockRunsTabView, enableEditMenuAction(PROCESS))
        .Times(Exactly(1));
    EXPECT_CALL(m_mockRunsTabView, enableAutoreduce()).Times(Exactly(1));

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
