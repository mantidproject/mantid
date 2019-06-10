// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflAutoreduction.h"
#include "../ISISReflectometry/ReflRunsTabPresenter.h"
#include "MantidKernel/ConfigService.h"

#include "MantidQtWidgets/Common/DataProcessorUI/MockObjects.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProgressableViewMockObject.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {
ACTION(ICATRuntimeException) { throw std::runtime_error(""); }
} // namespace

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
    createMocks(1);

    // Expect that the transfer methods get initialized on the view
    EXPECT_CALL(*m_mockRunsTabView, setTransferMethods(_)).Times(Exactly(1));
    // Expect that the list of instruments gets initialized on the view
    EXPECT_CALL(*m_mockRunsTabView, setInstrumentList(_, _)).Times(Exactly(1));

    createPresenter();
    verifyAndClearExpectations();
  }

  void test_table_presenters_accept_this_presenter() {
    createMocks(3);

    // Expect that the table presenters accept this presenter as a workspace
    // receiver
    EXPECT_CALL(*mockTablePresenter(0), accept(_)).Times(Exactly(1));
    EXPECT_CALL(*mockTablePresenter(1), accept(_)).Times(Exactly(1));
    EXPECT_CALL(*mockTablePresenter(2), accept(_)).Times(Exactly(1));

    createPresenter();
    verifyAndClearExpectations();
  }

  void test_presenter_sets_commands_when_ADS_changed() {
    auto presenter = createMocksAndPresenter(1);

    constexpr int GROUP_NUMBER = 0;
    // Expect that the view clears the list of commands
    EXPECT_CALL(*m_mockRunsTabView, clearCommands()).Times(Exactly(1));
    // Expect that the view is populated with the list of table commands
    EXPECT_CALL(*m_mockRunsTabView, setTableCommandsProxy()).Times(Exactly(1));
    // Expect that the view is populated with the list of row commands
    EXPECT_CALL(*m_mockRunsTabView, setRowCommandsProxy()).Times(Exactly(1));
    // The presenter is notified that something changed in the ADS
    presenter.notifyADSChanged(QSet<QString>(), GROUP_NUMBER);

    verifyAndClearExpectations();
  }

  void test_presenter_sets_commands_on_correct_group_when_ADS_changed() {
    auto presenter = createMocksAndPresenter(3);

    constexpr int GROUP_NUMBER = 1;
    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup())
        .Times(Exactly(3))
        .WillRepeatedly(Return(GROUP_NUMBER));
    // Commands should be updated with presenter of selected group
    EXPECT_CALL(*mockTablePresenter(0), publishCommandsMocked()).Times(0);
    EXPECT_CALL(*mockTablePresenter(1), publishCommandsMocked()).Times(1);
    EXPECT_CALL(*mockTablePresenter(2), publishCommandsMocked()).Times(0);
    presenter.notifyADSChanged(QSet<QString>(), 0);
    presenter.notifyADSChanged(QSet<QString>(), 1);
    presenter.notifyADSChanged(QSet<QString>(), 2);

    verifyAndClearExpectations();
  }

  void test_preprocessingOptions() {
    auto presenter = createMocksAndPresenter(1);

    int group = 199;
    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup()).Times(Exactly(0));
    EXPECT_CALL(*m_mockMainPresenter, getTransmissionOptions(group))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    presenter.getPreprocessingOptions(group);

    verifyAndClearExpectations();
  }

  void test_processingOptions() {
    auto presenter = createMocksAndPresenter(1);

    int group = 199;
    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup()).Times(Exactly(0));
    EXPECT_CALL(*m_mockMainPresenter, getReductionOptions(group))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    presenter.getProcessingOptions(group);

    verifyAndClearExpectations();
  }

  void test_postprocessingOptions() {
    auto presenter = createMocksAndPresenter(1);

    int group = 199;
    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup()).Times(Exactly(0));
    EXPECT_CALL(*m_mockMainPresenter, getStitchOptions(group)).Times(1);
    presenter.getPostprocessingOptionsAsString(group);

    verifyAndClearExpectations();
  }

  void test_when_group_changes_commands_are_updated() {
    auto presenter = createMocksAndPresenter(3);

    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup())
        .Times(Exactly(2))
        .WillRepeatedly(Return(1));
    // Commands should be updated with presenter of selected group
    EXPECT_CALL(*mockTablePresenter(0), publishCommandsMocked()).Times(0);
    EXPECT_CALL(*mockTablePresenter(1), publishCommandsMocked()).Times(1);
    EXPECT_CALL(*mockTablePresenter(2), publishCommandsMocked()).Times(0);
    presenter.notify(IReflRunsTabPresenter::GroupChangedFlag);

    verifyAndClearExpectations();
  }

  void test_when_group_changes_widget_states_are_updated() {
    auto presenter = createMocksAndPresenter(1);

    expectSetWidgetEnabledState(false, false);
    expectSelectedGroup(0, 2);
    presenter.notify(IReflRunsTabPresenter::GroupChangedFlag);

    verifyAndClearExpectations();
  }

  void test_startNewAutoreduction() {
    auto presenter = createMocksAndPresenter(2);
    constexpr int GROUP_NUMBER = 1;
    expectSelectedGroup(GROUP_NUMBER);
    EXPECT_CALL(*m_mockRunsTabView, getSearchString()).Times(Exactly(2));
    expectStartAutoreduction();

    presenter.notify(IReflRunsTabPresenter::StartAutoreductionFlag);
    verifyAndClearExpectations();
    TS_ASSERT_EQUALS(presenter.m_autoreduction.running(), true);
    TS_ASSERT_EQUALS(presenter.m_autoreduction.group(), GROUP_NUMBER);
  }

  void
  test_starting_autoreduction_does_not_clear_tables_if_settings_not_changed() {
    auto presenter = createMocksAndPresenter(1);
    EXPECT_CALL(*mockTablePresenter(0), setPromptUser(false)).Times(Exactly(0));
    EXPECT_CALL(*mockTablePresenter(0),
                notify(DataProcessorPresenter::DeleteAllFlag))
        .Times(Exactly(0));

    presenter.notify(IReflRunsTabPresenter::StartAutoreductionFlag);
    verifyAndClearExpectations();
  }

  void
  test_start_new_autoreduction_clears_selected_table_if_settings_changed() {
    auto presenter = createMocksAndPresenter(2);

    // Change the instrument to force a new autoreduction to start
    EXPECT_CALL(*m_mockMainPresenter, setInstrumentName(_));
    presenter.notify(IReflRunsTabPresenter::InstrumentChangedFlag);
    // Check that all existing rows are deleted from the selected group only
    constexpr int GROUP_NUMBER = 1;
    expectSelectedGroup(GROUP_NUMBER);

    EXPECT_CALL(*mockTablePresenter(GROUP_NUMBER), setPromptUser(false))
        .Times(Exactly(1));
    EXPECT_CALL(*mockTablePresenter(GROUP_NUMBER),
                notify(DataProcessorPresenter::DeleteAllFlag))
        .Times(Exactly(1));
    // Check the other table is not cleared
    EXPECT_CALL(*mockTablePresenter(0),
                notify(DataProcessorPresenter::DeleteAllFlag))
        .Times(Exactly(0));
    // Check that the icat search is initiated
    EXPECT_CALL(*m_mockRunsTabView, startIcatSearch());

    presenter.notify(IReflRunsTabPresenter::StartAutoreductionFlag);
    verifyAndClearExpectations();
  }

  void test_pauseAutoreduction_when_autoreduction_not_running() {
    auto presenter = createMocksAndPresenter(1);

    EXPECT_CALL(*mockTablePresenter(0),
                notify(DataProcessorPresenter::PauseFlag))
        .Times(Exactly(0));

    presenter.notify(IReflRunsTabPresenter::PauseAutoreductionFlag);
    verifyAndClearExpectations();
    // Autoreduction was not running so still shouldn't be
    TS_ASSERT_EQUALS(presenter.m_autoreduction.running(), false);
  }

  void test_pauseAutoreduction_when_autoreduction_is_running() {
    auto presenter = createMocksAndPresenter(2);
    // Start autoreduction on the selected group
    constexpr int GROUP_NUMBER = 1;
    expectSelectedGroup(GROUP_NUMBER);
    presenter.startNewAutoreduction();
    verifyAndClearExpectations();

    // We shouldn't re-check the active group
    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup()).Times(Exactly(0));
    // Notify the cached autoreduction group
    EXPECT_CALL(*mockTablePresenter(GROUP_NUMBER),
                notify(DataProcessorPresenter::PauseFlag))
        .Times(Exactly(1));
    // Check the other table is not affected
    EXPECT_CALL(*mockTablePresenter(0),
                notify(DataProcessorPresenter::PauseFlag))
        .Times(Exactly(0));

    presenter.notify(IReflRunsTabPresenter::PauseAutoreductionFlag);
    verifyAndClearExpectations();
    // Autoreduction continues until we get confirmation paused
    TS_ASSERT_EQUALS(presenter.m_autoreduction.running(), true);
  }

  void test_pause_when_autoreduction_is_running_in_different_group() {
    auto presenter = createMocksAndPresenter(2);

    // Start autoreduction on one of the groups
    constexpr int GROUP_TO_PAUSE = 0;
    constexpr int AUTOREDUCTION_GROUP = 1;
    presenter.m_autoreduction.setupNewAutoreduction(AUTOREDUCTION_GROUP,
                                                    "dummy");

    EXPECT_CALL(*m_mockMainPresenter, notifyReductionPaused(GROUP_TO_PAUSE))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockMainPresenter,
                notifyReductionPaused(AUTOREDUCTION_GROUP))
        .Times(Exactly(0));
    expectSetWidgetEnabledState(false, true);

    presenter.pause(GROUP_TO_PAUSE);
    verifyAndClearExpectations();
    // Autoreduction is still running in its original group
    TS_ASSERT_EQUALS(presenter.m_autoreduction.running(), true);
  }

  void test_pause_when_autoreduction_is_paused_in_different_group() {
    auto presenter = createMocksAndPresenter(2);

    // Start and stop autoreduction on one of the groups
    constexpr int GROUP_TO_PAUSE = 0;
    constexpr int AUTOREDUCTION_GROUP = 1;
    EXPECT_CALL(*m_mockProgress, setProgressRange(0, 100)).Times(Exactly(1));
    presenter.m_autoreduction.setupNewAutoreduction(AUTOREDUCTION_GROUP,
                                                    "dummy");
    presenter.m_autoreduction.pause(AUTOREDUCTION_GROUP);
    verifyAndClearExpectations();

    // When autoreduction is not running its group should be ignored, so pause
    // should act on the requested group
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionPaused(GROUP_TO_PAUSE))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockMainPresenter,
                notifyReductionPaused(AUTOREDUCTION_GROUP))
        .Times(Exactly(0));

    presenter.pause(GROUP_TO_PAUSE);
    verifyAndClearExpectations();
    // Autoreduction was not running so still shouldn't be
    TS_ASSERT_EQUALS(presenter.m_autoreduction.running(), false);
  }

  void test_timer_event_starts_autoreduction() {
    auto presenter = createMocksAndPresenter(1);
    expectStartAutoreduction();
    presenter.notify(IReflRunsTabPresenter::TimerEventFlag);
    verifyAndClearExpectations();
  }

  void test_transfer_selected_rows() {
    auto presenter = createMocksAndPresenter(2);

    // Transfer should be done to the currently selected table
    constexpr int GROUP_NUMBER = 1;
    expectSelectedGroup(GROUP_NUMBER);
    // Select a couple of rows with random indices
    auto rows = std::set<int>{3, 5};
    EXPECT_CALL(*m_mockRunsTabView, getSelectedSearchRows())
        .Times(Exactly(1))
        .WillOnce(Return(rows));
    expectTransferDataForTwoRows(presenter);
    // Check that only the selected table is affecffed
    EXPECT_CALL(*mockTablePresenter(GROUP_NUMBER), transfer(_))
        .Times(Exactly(1));
    EXPECT_CALL(*mockTablePresenter(0), transfer(_)).Times(Exactly(0));

    presenter.notify(IReflRunsTabPresenter::TransferFlag);
    verifyAndClearExpectations();
  }

  void test_instrumentChanged() {
    auto presenter = createMocksAndPresenter(1);

    std::vector<std::string> instruments = {"INTER", "POLREF", "OFFSPEC",
                                            "SURF", "CRISP"};
    for (const auto &instrument : instruments) {
      EXPECT_CALL(*m_mockRunsTabView, getSearchInstrument())
          .Times(Exactly(1))
          .WillOnce(Return(instrument));
      EXPECT_CALL(*m_mockMainPresenter, setInstrumentName(instrument))
          .Times(Exactly(1));
      presenter.notify(IReflRunsTabPresenter::InstrumentChangedFlag);
      TS_ASSERT_EQUALS(Mantid::Kernel::ConfigService::Instance().getString(
                           "default.instrument"),
                       instrument);
    }

    verifyAndClearExpectations();
  }

  void test_invalid_ICAT_login_credentials_gives_user_critical() {
    auto presenter = createMocksAndPresenter(1);

    std::stringstream pythonSrc;
    pythonSrc << "try:\n";
    pythonSrc << "  algm = CatalogLoginDialog()\n";
    pythonSrc << "except:\n";
    pythonSrc << "  pass\n";

    EXPECT_CALL(*m_mockRunsTabView, getSearchString())
        .Times(Exactly(1))
        .WillOnce(Return("12345"));
    EXPECT_CALL(*m_mockMainPresenter, runPythonAlgorithm(pythonSrc.str()))
        .Times(Exactly(1))
        .WillRepeatedly(ICATRuntimeException());
    EXPECT_CALL(*m_mockMainPresenter,
                giveUserCritical("Error Logging in:\n", "login failed"))
        .Times(1);
    presenter.notify(IReflRunsTabPresenter::SearchFlag);

    verifyAndClearExpectations();
  }

  void test_pause() {
    auto presenter = createMocksAndPresenter(1);

    constexpr int GROUP_NUMBER = 0;
    expectSetWidgetEnabledState(false, false);
    EXPECT_CALL(*m_mockRunsTabView, stopTimer()).Times(Exactly(1));
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionPaused(GROUP_NUMBER))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockProgress, setProgressRange(0, 100)).Times(Exactly(1));

    presenter.pause(GROUP_NUMBER);

    verifyAndClearExpectations();
  }

  void test_confirmReductionCompleted() {
    auto presenter = createMocksAndPresenter(1);

    constexpr int GROUP_NUMBER = 0;
    EXPECT_CALL(*m_mockRunsTabView, startTimer(_)).Times(Exactly(1));

    presenter.confirmReductionCompleted(GROUP_NUMBER);
    verifyAndClearExpectations();
  }

  void test_confirmReductionPaused() {
    auto presenter = createMocksAndPresenter(1);

    constexpr int GROUP_NUMBER = 0;
    expectSetWidgetEnabledState(false, false);
    expectTablePresenterIsProcessing(GROUP_NUMBER, false, 2);
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionPaused(GROUP_NUMBER))
        .Times(Exactly(1));

    presenter.confirmReductionPaused(GROUP_NUMBER);
    verifyAndClearExpectations();
  }

  void test_confirmReductionResumed() {
    auto presenter = createMocksAndPresenter(1);

    auto GROUP_NUMBER = 0;
    expectTablePresenterIsProcessing(GROUP_NUMBER, true, 2);
    expectSetWidgetEnabledState(true, false);
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionResumed(GROUP_NUMBER))
        .Times(Exactly(1));

    presenter.confirmReductionResumed(GROUP_NUMBER);
    verifyAndClearExpectations();
  }

  void test_startMonitor() {
    auto presenter = createMocksAndPresenter(2);

    // Should get settings from default group even if another is selected
    auto DEFAULT_GROUP = 0;
    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup()).Times(0);
    EXPECT_CALL(*m_mockMainPresenter, getReductionOptions(DEFAULT_GROUP))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(*m_mockRunsTabView, getMonitorAlgorithmRunner())
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setStartMonitorButtonEnabled(false))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setStopMonitorButtonEnabled(false))
        .Times(Exactly(1));

    presenter.notify(IReflRunsTabPresenter::StartMonitorFlag);
    verifyAndClearExpectations();
  }

  void test_startMonitorComplete() {
    auto presenter = createMocksAndPresenter(2);

    EXPECT_CALL(*m_mockRunsTabView, getMonitorAlgorithmRunner())
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setStartMonitorButtonEnabled(false))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setStopMonitorButtonEnabled(true))
        .Times(Exactly(1));

    presenter.notify(IReflRunsTabPresenter::StartMonitorFlag);
    verifyAndClearExpectations();
  }

  void test_stopMonitor() {
    auto presenter = createMocksAndPresenter(2);

    EXPECT_CALL(*m_mockRunsTabView, setStartMonitorButtonEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setStopMonitorButtonEnabled(false))
        .Times(Exactly(1));

    presenter.notify(IReflRunsTabPresenter::StartMonitorFlag);
    verifyAndClearExpectations();
  }

private:
  class ReflRunsTabPresenterFriend : public ReflRunsTabPresenter {
    friend class ReflRunsTabPresenterTest;

  public:
    ReflRunsTabPresenterFriend(
        IReflRunsTabView *mainView, ProgressableView *progressView,
        std::vector<DataProcessorPresenter *> tablePresenter,
        boost::shared_ptr<IReflSearcher> searcher =
            boost::shared_ptr<IReflSearcher>())
        : ReflRunsTabPresenter(mainView, progressView, tablePresenter,
                               searcher) {}
  };

  using MockRunsTabView_uptr = std::unique_ptr<NiceMock<MockRunsTabView>>;
  using MockMainWindowPresenter_uptr = std::unique_ptr<MockMainWindowPresenter>;
  using MockProgressableView_uptr = std::unique_ptr<MockProgressableView>;
  using MockDataProcessorPresenter_uptr =
      std::unique_ptr<NiceMock<MockDataProcessorPresenter>>;
  using TablePresenterList = std::vector<MockDataProcessorPresenter_uptr>;

  MockRunsTabView_uptr m_mockRunsTabView;
  MockMainWindowPresenter_uptr m_mockMainPresenter;
  MockProgressableView_uptr m_mockProgress;
  TablePresenterList m_tablePresenters;

  // Create the mock objects. The number of groups defines the number of table
  // presenters
  void createMocks(int numGroups) {
    m_mockRunsTabView = std::make_unique<NiceMock<MockRunsTabView>>();
    m_mockMainPresenter = std::make_unique<MockMainWindowPresenter>();
    m_mockProgress = std::make_unique<MockProgressableView>();

    for (int i = 0; i < numGroups; ++i) {
      // The runs tab presenter requires a vector of raw pointers
      m_tablePresenters.emplace_back(
          std::make_unique<NiceMock<MockDataProcessorPresenter>>());
    }
  }

  // Create the runs tab presenter. You must call createMocks() first.
  ReflRunsTabPresenterFriend createPresenter() {
    TS_ASSERT(m_mockRunsTabView && m_mockMainPresenter && m_mockProgress);
    // The presenter requires the table presenters as a vector of raw pointers
    std::vector<DataProcessorPresenter *> tablePresenters;
    for (auto &tablePresenter : m_tablePresenters)
      tablePresenters.push_back(tablePresenter.get());
    // Create the presenter
    ReflRunsTabPresenterFriend presenter(m_mockRunsTabView.get(),
                                         m_mockProgress.get(), tablePresenters);
    presenter.acceptMainPresenter(m_mockMainPresenter.get());
    return presenter;
  }

  // Shortcut to create both mocks and presenter
  ReflRunsTabPresenterFriend createMocksAndPresenter(int numGroups) {
    createMocks(numGroups);
    return createPresenter();
  }

  // Return the table presenter for the given group
  NiceMock<MockDataProcessorPresenter> *mockTablePresenter(int group) {
    TS_ASSERT(group < static_cast<int>(m_tablePresenters.size()));
    return m_tablePresenters[group].get();
  }

  void verifyAndClearExpectations() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mockRunsTabView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mockMainPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mockProgress));
    for (auto &tablePresenter : m_tablePresenters)
      TS_ASSERT(Mock::VerifyAndClearExpectations(tablePresenter.get()));
  }

  void expectStartAutoreduction() {
    EXPECT_CALL(*m_mockRunsTabView, stopTimer()).Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, startIcatSearch()).Times(Exactly(1));
  }

  void expectTransferDataForTwoRows(ReflRunsTabPresenterFriend &presenter) {
    constexpr int NUMBER_ROWS = 2;
    // Set up a transfer method
    presenter.m_currentTransferMethod = "Description";
    EXPECT_CALL(*m_mockRunsTabView, getTransferMethod())
        .Times(Exactly(1))
        .WillOnce(Return(presenter.m_currentTransferMethod));
    // Set up some search results for our two fake rows
    auto searchModel = boost::make_shared<MockReflSearchModel>();
    presenter.m_searchModel = searchModel;
    EXPECT_CALL(*searchModel, data(_, _))
        .Times(Exactly(4 * NUMBER_ROWS)) // 4 values for each row
        .WillOnce(Return("run1"))
        .WillOnce(Return("description1"))
        .WillOnce(Return("location1"))
        .WillOnce(Return("run2"))
        .WillOnce(Return("description2"))
        .WillOnce(Return("location2"))
        .WillOnce(Return("error1"))
        .WillOnce(Return("")); // no error
    // Setting up progress bar clears progress then sets range then re-sets
    // range due to update as percentage indicator
    EXPECT_CALL(*m_mockProgress, clearProgress()).Times(Exactly(1));
    EXPECT_CALL(*m_mockProgress, setProgressRange(_, _)).Times(Exactly(2));
    // Each row is a step in the progress bar
    EXPECT_CALL(*m_mockProgress, setProgress(_)).Times(Exactly(NUMBER_ROWS));
  }

  void expectSelectedGroup(int group, int numTimes = 1) {
    EXPECT_CALL(*m_mockRunsTabView, getSelectedGroup())
        .Times(Exactly(numTimes))
        .WillRepeatedly(Return(group));
  }

  void expectTablePresenterIsProcessing(int group, bool processing,
                                        int numTimes = 1) {
    EXPECT_CALL(*mockTablePresenter(group), isProcessing())
        .Times(Exactly(numTimes))
        .WillRepeatedly(Return(processing));
  }

  void expectSetWidgetEnabledState(bool isProcessing, bool isAutoreducing) {
    EXPECT_CALL(*m_mockRunsTabView, updateMenuEnabledState(isProcessing))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setTransferButtonEnabled(!isProcessing))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setInstrumentComboEnabled(!isProcessing))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView,
                setTransferMethodComboEnabled(!isAutoreducing))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setSearchTextEntryEnabled(!isAutoreducing))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setSearchButtonEnabled(!isAutoreducing))
        .Times(Exactly(1));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H */
