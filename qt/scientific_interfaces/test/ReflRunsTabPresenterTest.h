#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflRunsTabPresenter.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/MockObjects.h"
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
        .Times(Exactly(1))
        .WillOnce(Return(1));
    // Commands should be updated with presenter of selected group
    EXPECT_CALL(*mockTablePresenter(0), publishCommandsMocked()).Times(0);
    EXPECT_CALL(*mockTablePresenter(1), publishCommandsMocked()).Times(1);
    EXPECT_CALL(*mockTablePresenter(2), publishCommandsMocked()).Times(0);
    presenter.notify(IReflRunsTabPresenter::GroupChangedFlag);

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
    // Expect that the view updates the menu with isProcessing=false
    // and enables the 'autoreduce', 'transfer' and 'instrument' buttons
    EXPECT_CALL(*m_mockRunsTabView, updateMenuEnabledState(false))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setAutoreduceButtonEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setAutoreducePauseButtonEnabled(false))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setTransferButtonEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setInstrumentComboEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setTransferMethodComboEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setSearchTextEntryEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setSearchButtonEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionPaused(GROUP_NUMBER))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockProgress, setProgressRange(0, 100)).Times(Exactly(1));

    presenter.pause(GROUP_NUMBER);

    verifyAndClearExpectations();
  }

  void test_resume() {
    auto presenter = createMocksAndPresenter(1);

    // Expect that the view updates the menu with isProcessing=true
    // and disables the 'autoreduce', 'transfer' and 'instrument' buttons
    EXPECT_CALL(*m_mockRunsTabView, updateMenuEnabledState(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setAutoreduceButtonEnabled(false))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setAutoreducePauseButtonEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setTransferButtonEnabled(false))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setInstrumentComboEnabled(false))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setTransferMethodComboEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setSearchTextEntryEnabled(true))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockRunsTabView, setSearchButtonEnabled(true))
        .Times(Exactly(1));
    // Resume presenter
    constexpr int GROUP_NUMBER = 0;
    presenter.resume(GROUP_NUMBER);

    verifyAndClearExpectations();
  }

  void test_confirmReductionFinished() {
    auto presenter = createMocksAndPresenter(1);

    constexpr int GROUP_NUMBER = 0;
    // Expect that the main presenter is notified that data reduction is
    // finished
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionFinished(GROUP_NUMBER))
        .Times(Exactly(1));

    presenter.confirmReductionFinished(GROUP_NUMBER);

    verifyAndClearExpectations();
  }

  void test_confirmReductionPaused() {
    auto presenter = createMocksAndPresenter(1);

    constexpr int GROUP_NUMBER = 0;
    // Expect that the main presenter is notified that data reduction is paused
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionPaused(GROUP_NUMBER))
        .Times(Exactly(1));

    presenter.confirmReductionPaused(GROUP_NUMBER);

    verifyAndClearExpectations();
  }

  void test_confirmReductionResumed() {
    auto presenter = createMocksAndPresenter(1);

    auto GROUP_NUMBER = 0;
    // Expect that the main presenter is notified that data reduction is resumed
    EXPECT_CALL(*m_mockMainPresenter, notifyReductionResumed(GROUP_NUMBER))
        .Times(Exactly(1));

    presenter.confirmReductionResumed(GROUP_NUMBER);

    verifyAndClearExpectations();
  }

private:
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
    m_mockRunsTabView =
        Mantid::Kernel::make_unique<NiceMock<MockRunsTabView>>();
    m_mockMainPresenter =
        Mantid::Kernel::make_unique<MockMainWindowPresenter>();
    m_mockProgress = Mantid::Kernel::make_unique<MockProgressableView>();

    for (int i = 0; i < numGroups; ++i) {
      // The runs tab presenter requires a vector of raw pointers
      m_tablePresenters.emplace_back(
          Mantid::Kernel::make_unique<NiceMock<MockDataProcessorPresenter>>());
    }
  }

  // Create the runs tab presenter. You must call createMocks() first.
  ReflRunsTabPresenter createPresenter() {
    TS_ASSERT(m_mockRunsTabView && m_mockMainPresenter && m_mockProgress);
    // The presenter requires the table presenters as a vector of raw pointers
    std::vector<DataProcessorPresenter *> tablePresenters;
    for (auto &tablePresenter : m_tablePresenters)
      tablePresenters.push_back(tablePresenter.get());
    // Create the presenter
    ReflRunsTabPresenter presenter(m_mockRunsTabView.get(),
                                   m_mockProgress.get(), tablePresenters);
    presenter.acceptMainPresenter(m_mockMainPresenter.get());
    return presenter;
  }

  // Shortcut to create both mocks and presenter
  ReflRunsTabPresenter createMocksAndPresenter(int numGroups) {
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
};

#endif /* MANTID_CUSTOMINTERFACES_REFLRUNSTABPRESENTERTEST_H */
