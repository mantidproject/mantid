// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_RUNSPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_RUNSPRESENTERTEST_H

#include "../../../ISISReflectometry/GUI/Runs/RunsPresenter.h"
#include "../ReflMockObjects.h"
#include "../RunsTable/MockRunsTablePresenterFactory.h"
#include "../RunsTable/MockRunsTableView.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"
#include "MantidQtWidgets/Common/MockProgressableView.h"
#include "MockRunsView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

//=====================================================================================
// Functional tests
//=====================================================================================
class RunsPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunsPresenterTest *createSuite() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    IMainWindowView *mainWindowMock = new MockMainWindowView();
    Plotter plotter(mainWindowMock);
#else
    Plotter plotter;
#endif
    return new RunsPresenterTest(plotter);
  }
  static void destroySuite(RunsPresenterTest *suite) { delete suite; }

  RunsPresenterTest(Plotter plotter)
      : m_thetaTolerance(0.01), m_instruments{"INTER", "SURF", "CRISP",
                                              "POLREF", "OFFSPEC"},
        m_view(), m_runsTableView(), m_progressView(), m_messageHandler(),
        m_autoreduction(new MockAutoreduction), m_searcher(new MockSearcher),
        m_runsTablePresenterFactory(m_instruments, m_thetaTolerance, plotter) {
    ON_CALL(m_view, table()).WillByDefault(Return(&m_runsTableView));
    ON_CALL(m_runsTableView, jobs()).WillByDefault(ReturnRef(m_jobs));
  }

  void testCreatePresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testCreatePresenterGetsRunsTableView() {
    EXPECT_CALL(m_view, table()).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testCreatePresenterSetsInstrumentList() {
    auto const defaultInstrumentIndex = 0;
    EXPECT_CALL(m_view,
                setInstrumentList(m_instruments, defaultInstrumentIndex))
        .Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testCreatePresenterUpdatesView() {
    expectUpdateViewWhenMonitorStopped();
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testSettingsChanged() {
    // TODO
  }

  void testSearchWithEmptyString() {
    auto presenter = makePresenter();
    auto searchString = std::string("");
    EXPECT_CALL(m_view, getSearchString())
        .Times(1)
        .WillOnce(Return(searchString));
    expectSearchFailed();
    presenter.notifySearch();
    verifyAndClear();
  }

  void testSearchCatalogLoginFails() {
    auto presenter = makePresenter();
    auto searchString = std::string("test string");
    EXPECT_CALL(m_view, getSearchString())
        .Times(1)
        .WillOnce(Return(searchString));
    // TODO: add expected call to python runner when implemented
    EXPECT_CALL(m_view, noActiveICatSessions()).Times(1);
    expectSearchFailed();
    presenter.notifySearch();
    verifyAndClear();
  }

  void testSearchSucceeds() {
    // TODO: add this test when python runner implemented
  }

  void testNotifyReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyReductionResumed());
    presenter.notifyReductionResumed();
    verifyAndClear();
  }

  void testNotifyReductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyReductionPaused());
    presenter.notifyReductionPaused();
    verifyAndClear();
  }

  void testReductionResumed() {}

  void testReductionPaused() {}

  void testNotifyAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyAutoreductionResumed());
    presenter.notifyAutoreductionResumed();
    verifyAndClear();
  }

  void testNotifyAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyAutoreductionPaused());
    presenter.notifyAutoreductionPaused();
    verifyAndClear();
  }

  void testAutoreductionResumedWithNewSettings() {
    auto settingsChanged = true;
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getSearchString()).Times(2);
    EXPECT_CALL(*m_autoreduction, searchStringChanged(_))
        .WillOnce(Return(settingsChanged));
    EXPECT_CALL(*m_autoreduction, setupNewAutoreduction(_))
        .WillOnce(Return(true));
    expectCheckForNewRuns();
    expectWidgetsEnabledForAutoreducing();
    presenter.autoreductionResumed();
    verifyAndClear();
  }

  void testAutoreductionResumedWithSameSettings() {
    auto settingsChanged = false;
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getSearchString()).Times(2);
    EXPECT_CALL(*m_autoreduction, searchStringChanged(_))
        .WillOnce(Return(settingsChanged));
    EXPECT_CALL(*m_autoreduction, setupNewAutoreduction(_))
        .WillOnce(Return(true));
    expectCheckForNewRuns();
    expectWidgetsEnabledForAutoreducing();
    presenter.autoreductionResumed();
    verifyAndClear();
  }

  void testAutoreductionResumedWarnsUserIfTableChanged() {
    // TODO
  }

  void testPauseAutoreduction() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, stopTimer()).Times(1);
    EXPECT_CALL(*m_autoreduction, stop()).Times(1);
    expectWidgetsEnabledForPaused();
    presenter.autoreductionPaused();
    verifyAndClear();
  }

  void testAutoreductionPollsForNewRunsOnTimerEvent() {
    auto presenter = makePresenter();
    expectCheckForNewRuns();
    presenter.notifyTimerEvent();
    verifyAndClear();
  }

  void testICATSearchComplete() {
    // TODO
    // auto presenter = makePresenter();
    // presenter.notifyICATSearchComplete();
    // verifyAndClear();
  }

  void testTransferWithNoRowsSelected() {
    auto presenter = makePresenter();
    auto const selectedRows = std::set<int>({});
    EXPECT_CALL(m_view, getSelectedSearchRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(m_view, missingRunsToTransfer()).Times(1);
    presenter.notifyTransfer();
    verifyAndClear();
  }

  void testTransferWithAutoreductionRunning() {
    auto presenter = makePresenter();
    expectGetValidSearchRowSelection(presenter);
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(1)
        .WillOnce(Return(true));
    expectCreateEndlessProgressIndicator();
    presenter.notifyTransfer();
    verifyAndClear();
  }

  void testTransferWithAutoreductionStopped() {
    auto presenter = makePresenter();
    expectGetValidSearchRowSelection(presenter);
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(1)
        .WillOnce(Return(false));
    expectCreatePercentageProgressIndicator();
    presenter.notifyTransfer();
    verifyAndClear();
  }

  void testInstrumentChanged() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    EXPECT_CALL(m_view, getSearchInstrument())
        .Times(1)
        .WillOnce(Return(instrument));
    EXPECT_CALL(m_mainPresenter, notifyInstrumentChanged(instrument)).Times(1);
    presenter.notifyInstrumentChanged();
    verifyAndClear();
  }

  // TODO
  //  void testStartMonitor() {
  //    auto presenter = makePresenter();
  //    EXPECT_CALL(m_view, getMonitorAlgorithmRunner()).Times(1);
  //    EXPECT_CALL(m_view, getSearchInstrument()).Times(1);
  //    expectUpdateViewWhenMonitorStarting();
  //    presenter.notifyStartMonitor();
  //    verifyAndClear();
  //  }
  //
  //  void testStopMonitor() {
  //    auto presenter = makePresenter();
  //    expectUpdateViewWhenMonitorStopped();
  //    presenter.notifyStopMonitor();
  //    verifyAndClear();
  //  }
  //
  //  void testStartMonitorComplete() {
  //    auto presenter = makePresenter();
  //    expectUpdateViewWhenMonitorStarted();
  //    presenter.notifyStartMonitorComplete();
  //    verifyAndClear();
  //  }

private:
  class RunsPresenterFriend : public RunsPresenter {
    friend class RunsPresenterTest;

  public:
    RunsPresenterFriend(
        IRunsView *mainView, ProgressableView *progressView,
        const RunsTablePresenterFactory &makeRunsTablePresenter,
        double thetaTolerance, std::vector<std::string> const &instruments,
        int defaultInstrumentIndex, IMessageHandler *messageHandler,
        boost::shared_ptr<IAutoreduction> autoreduction =
            boost::shared_ptr<IAutoreduction>(),
        boost::shared_ptr<ISearcher> searcher = boost::shared_ptr<ISearcher>())
        : RunsPresenter(mainView, progressView, makeRunsTablePresenter,
                        thetaTolerance, instruments, defaultInstrumentIndex,
                        messageHandler, autoreduction, searcher) {}
  };

  RunsPresenterFriend makePresenter() {
    auto const defaultInstrumentIndex = 0;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    IMainWindowView *mainWindowMock = new MockMainWindowView();
    Plotter plotter(mainWindowMock);
#else
    Plotter plotter;
#endif
    m_runsTablePresenterFactory =
        MockRunsTablePresenterFactory(m_instruments, m_thetaTolerance, plotter);
    auto presenter = RunsPresenterFriend(
        &m_view, &m_progressView, m_runsTablePresenterFactory, m_thetaTolerance,
        m_instruments, defaultInstrumentIndex, &m_messageHandler,
        m_autoreduction, m_searcher);

    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_runsTableView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_progressView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_messageHandler));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_autoreduction));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_searcher));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobs));
  }

  void expectUpdateViewWhenMonitorStarting() {
    EXPECT_CALL(m_view, setStartMonitorButtonEnabled(false));
    EXPECT_CALL(m_view, setStopMonitorButtonEnabled(false));
  }

  void expectUpdateViewWhenMonitorStarted() {
    EXPECT_CALL(m_view, setStartMonitorButtonEnabled(false));
    EXPECT_CALL(m_view, setStopMonitorButtonEnabled(true));
  }

  void expectUpdateViewWhenMonitorStopped() {
    EXPECT_CALL(m_view, setStartMonitorButtonEnabled(true));
    EXPECT_CALL(m_view, setStopMonitorButtonEnabled(false));
  }

  void expectStopAutoreduction() {
    EXPECT_CALL(m_view, stopTimer()).Times(1);
    EXPECT_CALL(*m_autoreduction, stop()).Times(1);
  }

  void expectSearchFailed() {
    EXPECT_CALL(m_view, getAlgorithmRunner()).Times(0);
    expectStopAutoreduction();
  }

  void expectCheckForNewRuns() {
    EXPECT_CALL(m_view, stopTimer()).Times(1);
    EXPECT_CALL(m_view, startIcatSearch()).Times(1);
  }

  void expectGetValidSearchRowSelection(RunsPresenterFriend &presenter) {
    // Select a couple of rows with random indices
    auto row1Index = 3;
    auto row2Index = 5;
    auto const selectedRows = std::set<int>({row1Index, row2Index});
    EXPECT_CALL(m_view, getSelectedSearchRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    // Set up a mock search model in the presenter to return something
    // sensible for getRowData
    auto searchModel = boost::make_shared<MockSearchModel>(
        "13460", "my title th=0.5", "my location");
    presenter.m_searchModel = searchModel;
  }

  void expectCreateEndlessProgressIndicator() {
    EXPECT_CALL(m_progressView, clearProgress()).Times(1);
    EXPECT_CALL(m_progressView, setProgressRange(_, _)).Times(2);
  }

  void expectCreatePercentageProgressIndicator() {
    EXPECT_CALL(m_progressView, clearProgress()).Times(1);
    EXPECT_CALL(m_progressView, setProgressRange(_, _)).Times(2);
  }

  void expectWidgetsEnabledForAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, updateMenuEnabledState(false));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(false));
    EXPECT_CALL(m_view, setSearchButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(true));
    EXPECT_CALL(m_view, setTransferButtonEnabled(false));
  }

  void expectWidgetsEnabledForProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, updateMenuEnabledState(true));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(false));
    EXPECT_CALL(m_view, setSearchButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(false));
    EXPECT_CALL(m_view, setTransferButtonEnabled(false));
  }

  void expectWidgetsEnabledForProcessingAndAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, updateMenuEnabledState(true));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(false));
    EXPECT_CALL(m_view, setSearchButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(true));
    EXPECT_CALL(m_view, setTransferButtonEnabled(false));
  }

  void expectWidgetsEnabledForPaused() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, updateMenuEnabledState(false));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(true));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(true));
    EXPECT_CALL(m_view, setSearchButtonEnabled(true));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(true));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(false));
    EXPECT_CALL(m_view, setTransferButtonEnabled(true));
  }

  double m_thetaTolerance;
  std::vector<std::string> m_instruments;
  NiceMock<MockRunsView> m_view;
  NiceMock<MockRunsTableView> m_runsTableView;
  NiceMock<MockBatchPresenter> m_mainPresenter;
  NiceMock<MockProgressableView> m_progressView;
  NiceMock<MockMessageHandler> m_messageHandler;
  boost::shared_ptr<MockAutoreduction> m_autoreduction;
  boost::shared_ptr<MockSearcher> m_searcher;
  NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> m_jobs;
  MockRunsTablePresenterFactory m_runsTablePresenterFactory;
};

#endif /* MANTID_CUSTOMINTERFACES_RUNSPRESENTERTEST_H */
