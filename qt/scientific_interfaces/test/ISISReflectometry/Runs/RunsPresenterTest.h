// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Runs/RunsPresenter.h"
#include "../../../ISISReflectometry/Reduction/RunsTable.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../ReflMockObjects.h"
#include "../RunsTable/MockRunsTablePresenter.h"
#include "../RunsTable/MockRunsTableView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"
#include "MantidQtWidgets/Common/MockAlgorithmRunner.h"
#include "MantidQtWidgets/Common/MockProgressableView.h"
#include "MockRunsView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::
    ModelCreationHelper;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

//=====================================================================================
// Functional tests
//=====================================================================================
class RunsPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunsPresenterTest *createSuite() { return new RunsPresenterTest(); }
  static void destroySuite(RunsPresenterTest *suite) { delete suite; }

  RunsPresenterTest()
      : m_thetaTolerance(0.01), m_instruments{"INTER", "SURF", "CRISP",
                                              "POLREF", "OFFSPEC"},
        m_runsTable(m_instruments, m_thetaTolerance, ReductionJobs()),
        m_searchString("test search string"), m_searchResult("", ""),
        m_instrument("INTER"), m_cycle("19_4"), m_searcher(nullptr),
        m_runNotifier(nullptr) {
    Mantid::API::FrameworkManager::Instance();
    ON_CALL(m_view, table()).WillByDefault(Return(&m_runsTableView));
    ON_CALL(m_view, getSearchInstrument()).WillByDefault(Return(m_instrument));
    ON_CALL(m_view, getSearchCycle()).WillByDefault(Return(m_cycle));
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

  void testInitInstrumentListUpdatesView() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, setInstrumentList(m_instruments)).Times(1);
    presenter.initInstrumentList();
    verifyAndClear();
  }

  void testCreatePresenterUpdatesView() {
    expectUpdateViewWhenMonitorStopped();
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testSettingsChanged() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsTablePresenter, settingsChanged()).Times(1);
    presenter.settingsChanged();
    verifyAndClear();
  }

  void testStartingSearchDoesNotClearPreviousResults() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_searcher, reset()).Times(0);
    presenter.notifySearch();
    verifyAndClear();
  }

  void testStartingSearchClearsPreviousResultsIfSettingsChanged() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    EXPECT_CALL(*m_searcher, reset()).Times(AtLeast(1));
    presenter.notifySearch();
    verifyAndClear();
  }

  void testStartingSearchDoesNotClearPreviousResultsIfOverwritePrevented() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    expectOverwriteSearchResultsPrevented();
    EXPECT_CALL(*m_searcher, reset()).Times(0);
    presenter.notifySearch();
    verifyAndClear();
  }

  void testStartingSearchDisablesSearchInputs() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_searcher, searchInProgress())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setSearchButtonEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setSearchResultsEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false)).Times(1);
    presenter.notifySearch();
    verifyAndClear();
  }

  void testNotifySearchResultsEnablesSearchInputs() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_searcher, searchInProgress())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setSearchButtonEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setSearchResultsEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(true)).Times(1);
    presenter.notifySearchComplete();
    verifyAndClear();
  }

  void testSearchUsesCorrectSearchProperties() {
    auto presenter = makePresenter();
    auto searchString = std::string("test search string");
    auto instrument = std::string("test instrument");
    auto cycle = std::string("test cycle");
    expectSearchString(searchString);
    expectSearchInstrument(instrument);
    expectSearchCycle(cycle);
    EXPECT_CALL(*m_searcher, startSearchAsync(SearchCriteria{instrument, cycle,
                                                             searchString}))
        .Times(1);
    presenter.notifySearch();
    verifyAndClear();
  }

  void testSearchWithEmptyStringDoesNotStartSearch() {
    auto presenter = makePresenter();
    auto searchString = std::string("");
    expectSearchString(searchString);
    EXPECT_CALL(*m_searcher, startSearchAsync(_)).Times(0);
    presenter.notifySearch();
    verifyAndClear();
  }

  void testStartingSearchFails() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    EXPECT_CALL(*m_searcher, startSearchAsync(SearchCriteria{
                                 m_instrument, m_cycle, m_searchString}))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(m_messageHandler,
                giveUserCritical("Error starting search", "Error"))
        .Times(1);
    presenter.notifySearch();
    verifyAndClear();
  }

  void testStartingSearchSucceeds() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    EXPECT_CALL(*m_searcher, startSearchAsync(SearchCriteria{
                                 m_instrument, m_cycle, m_searchString}))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(m_messageHandler, giveUserCritical(_, _)).Times(0);

    presenter.notifySearch();
    verifyAndClear();
  }

  void testNotifyReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyResumeReductionRequested())
        .Times(AtLeast(1));
    presenter.notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testNotifyReductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyPauseReductionRequested());
    presenter.notifyPauseReductionRequested();
    verifyAndClear();
  }

  void testNotifyAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyResumeAutoreductionRequested());
    presenter.notifyResumeAutoreductionRequested();
    verifyAndClear();
  }

  void testNotifyAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifyPauseAutoreductionRequested());
    presenter.notifyPauseAutoreductionRequested();
    verifyAndClear();
  }

  void testNoCheckOnOverwritingBatchOnAutoreductionResumed() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    EXPECT_CALL(m_mainPresenter, isOverwriteBatchPrevented()).Times(0);
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testNoCheckOnDiscardChangesOnAutoreductionResumed() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    EXPECT_CALL(m_mainPresenter, discardChanges(_)).Times(0);
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testCheckDiscardChangesOnAutoreductionResumedIfUnsavedSearchResults() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    expectUnsavedSearchResults();
    EXPECT_CALL(
        m_mainPresenter,
        discardChanges("This will cause unsaved changes in the search results "
                       "to be lost. Continue?"))
        .Times(AtLeast(1));
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testCheckDiscardChangesOnAutoreductionResumedIfUnsavedTable() {
    auto presenter = makePresenter();
    presenter.notifyTableChanged();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    EXPECT_CALL(m_mainPresenter,
                discardChanges("This will cause unsaved changes in the table "
                               "to be lost. Continue?"))
        .Times(AtLeast(1));
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void
  testCheckDiscardChangesOnAutoreductionResumedIfUnsavedTableAndSearchResults() {
    auto presenter = makePresenter();
    presenter.notifyTableChanged();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    expectUnsavedSearchResults();
    EXPECT_CALL(
        m_mainPresenter,
        discardChanges("This will cause unsaved changes in the search results "
                       "and main table to be lost. Continue?"))
        .Times(AtLeast(1));
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void
  testDoNotStartAutoreductionWhenOverwritePreventedOnResumeAutoreductionWithNewSettings() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    expectOverwriteSearchResultsPrevented();
    expectDoNotStartAutoreduction();
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testTableClearedWhenStartAutoreductionForFirstTime() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    expectClearExistingTable();
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testTableNotClearedWhenRestartAutoreduction() {
    auto presenter = makePresenter();
    // Set up first search and run autoreduction
    expectSearchString(m_searchString);
    presenter.resumeAutoreduction();
    verifyAndClear();
    // Resume autoreduction with same settings
    expectSearchString(m_searchString);
    expectSearchSettingsDefault();
    expectDoNotClearExistingTable();
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testTableClearedWhenResumeAutoreductionWithNewSettings() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    expectClearExistingTable();
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testTableNotClearedWhenOverwritePreventedOnResumeAutoreduction() {
    auto presenter = makePresenter();
    expectSearchString(m_searchString);
    expectSearchSettingsChanged();
    expectOverwriteSearchResultsPrevented();
    expectDoNotClearExistingTable();
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testResumeAutoreductionCancelledIfSearchStringIsEmpty() {
    auto presenter = makePresenter();
    expectSearchString("");
    expectDoNotStartAutoreduction();
    presenter.resumeAutoreduction();
    verifyAndClear();
  }

  void testAutoreductionResumed() {
    auto presenter = makePresenter();
    expectWidgetsEnabledForAutoreducing();
    EXPECT_CALL(*m_runsTablePresenter, notifyAutoreductionResumed()).Times(1);
    presenter.notifyAutoreductionResumed();
    verifyAndClear();
  }

  void testAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runNotifier, stopPolling()).Times(1);
    EXPECT_CALL(*m_runsTablePresenter, notifyAutoreductionPaused()).Times(1);
    expectWidgetsEnabledForPaused();
    presenter.notifyAutoreductionPaused();
    verifyAndClear();
  }

  void testAutoreductionCompleted() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runNotifier, startPolling()).Times(1);
    expectWidgetsEnabledForAutoreducing();
    presenter.autoreductionCompleted();
    verifyAndClear();
  }

  void testChildPresentersAreUpdatedWhenAnyBatchReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsTablePresenter, notifyAnyBatchReductionResumed())
        .Times(1);
    presenter.notifyAnyBatchReductionResumed();
    verifyAndClear();
  }

  void testChildPresentersAreUpdatedWhenAnyBatchReductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsTablePresenter, notifyAnyBatchReductionPaused())
        .Times(1);
    presenter.notifyAnyBatchReductionPaused();
    verifyAndClear();
  }

  void testChildPresentersAreUpdatedWhenAnyBatchAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsTablePresenter, notifyAnyBatchAutoreductionResumed())
        .Times(1);
    presenter.notifyAnyBatchAutoreductionResumed();
    verifyAndClear();
  }

  void testChildPresentersAreUpdatedWhenAnyBatchAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsTablePresenter, notifyAnyBatchAutoreductionPaused())
        .Times(1);
    presenter.notifyAnyBatchAutoreductionPaused();
    verifyAndClear();
  }

  void testChangingInstrumentIsDisabledWhenAnotherBatchReducing() {
    auto presenter = makePresenter();
    expectInstrumentComboIsDisabledWhenAnotherBatchReducing();
    presenter.notifyAnyBatchReductionResumed();
    verifyAndClear();
  }

  void testChangingInstrumentIsEnabledWhenNoBatchesAreReducing() {
    auto presenter = makePresenter();
    expectInstrumentComboIsEnabledWhenNoBatchesAreReducing();
    presenter.notifyAnyBatchReductionPaused();
    verifyAndClear();
  }

  void testChangingInstrumentIsDisabledWhenAnotherBatchAutoreducing() {
    auto presenter = makePresenter();
    expectInstrumentComboIsDisabledWhenAnotherBatchAutoreducing();
    presenter.notifyAnyBatchAutoreductionResumed();
    verifyAndClear();
  }

  void testChangingInstrumentIsEnabledWhenNoBatchesAreAutoreducing() {
    auto presenter = makePresenter();
    expectInstrumentComboIsEnabledWhenNoBatchesAreAutoreducing();
    presenter.notifyAnyBatchAutoreductionPaused();
    verifyAndClear();
  }

  void testAutoreductionDisabledWhenAnotherBatchAutoreducing() {
    auto presenter = makePresenter();
    expectAutoreduceButtonDisabledWhenAnotherBatchAutoreducing();
    presenter.notifyAnyBatchAutoreductionResumed();
    verifyAndClear();
  }

  void testAutoreductionEnabledWhenAnotherBatchNotAutoreducing() {
    auto presenter = makePresenter();
    expectAutoreduceButtonEnabledWhenNoBatchesAreAutoreducing();
    presenter.notifyAnyBatchAutoreductionPaused();
    verifyAndClear();
  }

  void testNotifyCheckForNewRuns() {
    auto presenter = makePresenter();
    expectCheckForNewRuns();
    presenter.notifyCheckForNewRuns();
    verifyAndClear();
  }

  void testNotifySearchResultsResizesColumnsWhenNotAutoreducing() {
    auto presenter = makePresenter();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_view, resizeSearchResultsColumnsToContents()).Times(1);
    presenter.notifySearchComplete();
    verifyAndClear();
  }

  void testNotifySearchResultsDoesNotResizeColumnsWhenAutoreducing() {
    auto presenter = makePresenter();
    expectIsAutoreducing();
    EXPECT_CALL(m_view, resizeSearchResultsColumnsToContents()).Times(0);
    presenter.notifySearchComplete();
    verifyAndClear();
  }

  void testNotifySearchResultsResumesReductionWhenAutoreducing() {
    auto presenter = makePresenter();
    expectIsAutoreducing();
    EXPECT_CALL(m_mainPresenter, notifyResumeReductionRequested())
        .Times(AtLeast(1));
    presenter.notifySearchComplete();
    verifyAndClear();
  }

  void testNotifySearchResultsTransfersRowsWhenAutoreducing() {
    auto presenter = makePresenter();
    expectIsAutoreducing();
    // Transfer some valid rows
    auto rowsToTransfer = std::set<int>{0, 1, 2};
    EXPECT_CALL(m_view, getAllSearchRows())
        .Times(1)
        .WillOnce(Return(rowsToTransfer));
    auto searchResult = SearchResult("12345", "Test run th=0.5");
    for (auto rowIndex : rowsToTransfer)
      EXPECT_CALL(*m_searcher, getSearchResult(rowIndex))
          .Times(1)
          .WillOnce(ReturnRef(searchResult));
    EXPECT_CALL(m_messageHandler, giveUserCritical(_, _)).Times(0);
    presenter.notifySearchComplete();
    verifyAndClear();
  }

  void testTransferWithNoRowsSelected() {
    auto presenter = makePresenter();
    auto const selectedRows = std::set<int>({});
    EXPECT_CALL(m_view, getSelectedSearchRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(m_messageHandler,
                giveUserCritical("Please select at least one run to transfer.",
                                 "No runs selected"))
        .Times(1);
    presenter.notifyTransfer();
    verifyAndClear();
  }

  void testTransferWithAutoreductionRunning() {
    auto presenter = makePresenter();
    expectGetValidSearchRowSelection();
    expectIsAutoreducing();
    expectCreateEndlessProgressIndicator();
    presenter.notifyTransfer();
    verifyAndClear();
  }

  void testTransferWithAutoreductionStopped() {
    auto presenter = makePresenter();
    expectGetValidSearchRowSelection();
    expectIsNotAutoreducing();
    expectCreatePercentageProgressIndicator();
    presenter.notifyTransfer();
    verifyAndClear();
  }

  void testTransferUpdatesTablePresenter() {
    auto presenter = makePresenter();
    auto expectedJobs = expectGetValidSearchResult();
    EXPECT_CALL(*m_runsTablePresenter, mergeAdditionalJobs(expectedJobs))
        .Times(1);
    presenter.notifyTransfer();
    verifyAndClear();
  }

  void testChangeInstrumentOnViewNotifiesMainPresenter() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    expectSearchInstrument(instrument);
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(instrument))
        .Times(AtLeast(1));
    presenter.notifyChangeInstrumentRequested();
    verifyAndClear();
  }

  void testChangeInstrumentOnViewPromptsToDiscardChangesIfUnsaved() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    expectSearchInstrument(instrument);
    expectUnsavedSearchResults();
    EXPECT_CALL(
        m_mainPresenter,
        discardChanges(
            "This will cause unsaved changes in the search results to be "
            "lost. Continue?"))
        .Times(1);
    presenter.notifyChangeInstrumentRequested();
    verifyAndClear();
  }

  void testChangeInstrumentOnViewDoesNotPromptToDiscardChangesIfSaved() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    expectSearchInstrument(instrument);
    expectNoUnsavedSearchResults();
    EXPECT_CALL(m_mainPresenter, discardChanges(_)).Times(0);
    presenter.notifyChangeInstrumentRequested();
    verifyAndClear();
  }

  void testChangeInstrumentOnViewDoesNotNotifyMainPresenterIfPrevented() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    expectSearchInstrument(instrument);
    expectChangeInstrumentPrevented();
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(_)).Times(0);
    presenter.notifyChangeInstrumentRequested();
    verifyAndClear();
  }

  void testChangeInstrumentOnViewRevertsChangeIfPrevented() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    expectSearchInstrument(instrument);
    expectChangeInstrumentPrevented();
    EXPECT_CALL(m_view, setSearchInstrument("INTER")).Times(1);
    presenter.notifyChangeInstrumentRequested();
    verifyAndClear();
  }

  void testChangeInstrumentOnChildNotifiesMainPresenter() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(instrument))
        .Times(AtLeast(1));
    presenter.notifyChangeInstrumentRequested(instrument);
    verifyAndClear();
  }

  void testChangeInstrumentOnChildDoesNotNotifyMainPresenterIfPrevented() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    expectChangeInstrumentPrevented();
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(_)).Times(0);
    presenter.notifyChangeInstrumentRequested(instrument);
    verifyAndClear();
  }

  void testChangeInstrumentOnChildReturnsTrueIfSuccess() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    auto success = presenter.notifyChangeInstrumentRequested(instrument);
    TS_ASSERT_EQUALS(success, true);
    verifyAndClear();
  }

  void testChangeInstrumentOnChildReturnsFalseIfPrevented() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    expectPreviousInstrument("INTER");
    expectChangeInstrumentPrevented();
    auto success = presenter.notifyChangeInstrumentRequested(instrument);
    TS_ASSERT_EQUALS(success, false);
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesView() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    EXPECT_CALL(m_view, setSearchInstrument(instrument)).Times(1);
    presenter.notifyInstrumentChanged(instrument);
    verifyAndClear();
  }

  void testInstrumentChangedUpdatesChildPresenter() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    EXPECT_CALL(*m_runsTablePresenter, notifyInstrumentChanged(instrument))
        .Times(1);
    presenter.notifyInstrumentChanged(instrument);
    verifyAndClear();
  }

  void testInstrumentChangedClearsPreviousSearchResultsModel() {
    auto presenter = makePresenter();
    auto const instrument = std::string("TEST-instrumnet");
    EXPECT_CALL(*m_searcher, reset()).Times(1);
    presenter.notifyInstrumentChanged(instrument);
    verifyAndClear();
  }

  void testNotifyRowStateChanged() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsTablePresenter, notifyRowStateChanged()).Times(1);
    presenter.notifyRowStateChanged();
    verifyAndClear();
  }

  void testNotifyRowStateChangedItem() {
    auto presenter = makePresenter();
    auto row = makeRow();
    EXPECT_CALL(*m_runsTablePresenter, notifyRowStateChanged(_)).Times(1);
    presenter.notifyRowStateChanged(row);
    verifyAndClear();
  }

  void testNotifyRowOutputsChanged() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsTablePresenter, notifyRowOutputsChanged()).Times(1);
    presenter.notifyRowOutputsChanged();
    verifyAndClear();
  }

  void testNotifyRowOutputsChangedItem() {
    auto presenter = makePresenter();
    auto row = makeRow();
    EXPECT_CALL(*m_runsTablePresenter, notifyRowOutputsChanged(_)).Times(1);
    presenter.notifyRowOutputsChanged(row);
    verifyAndClear();
  }

  void testPercentCompleteIsRequestedFromMainPresenter() {
    auto presenter = makePresenter();
    auto progress = 33;
    EXPECT_CALL(m_mainPresenter, percentComplete())
        .Times(1)
        .WillOnce(Return(progress));
    TS_ASSERT_EQUALS(presenter.percentComplete(), progress);
    verifyAndClear();
  }

  void testStartMonitorStartsAlgorithmRunner() {
    auto presenter = makePresenter();
    expectStartingLiveDataSucceeds();
    auto algRunner = expectGetAlgorithmRunner();
    EXPECT_CALL(*algRunner, startAlgorithmImpl(_)).Times(1);
    presenter.notifyStartMonitor();
    verifyAndClear();
  }

  void testStartMonitorUpdatesView() {
    auto presenter = makePresenter();
    expectStartingLiveDataSucceeds();
    expectUpdateViewWhenMonitorStarting();
    presenter.notifyStartMonitor();
    verifyAndClear();
  }

  void testStartMonitorSetsAlgorithmProperties() {
    auto presenter = makePresenter();
    auto instrument = std::string("INTER");
    auto updateInterval = 20;
    expectGetLiveDataOptions(instrument, updateInterval);
    auto algRunner = expectGetAlgorithmRunner();
    presenter.notifyStartMonitor();
    auto expected =
        defaultLiveMonitorAlgorithmOptions(instrument, updateInterval);
    assertAlgorithmPropertiesContainOptions(expected, algRunner);
    verifyAndClear();
  }

  void testStartMonitorSetsDefaultPostProcessingProperties() {
    auto presenter = makePresenter();
    auto options = defaultLiveMonitorReductionOptions();
    expectGetLiveDataOptions(options);
    auto algRunner = expectGetAlgorithmRunner();
    presenter.notifyStartMonitor();
    assertPostProcessingPropertiesContainOptions(options, algRunner);
    verifyAndClear();
  }

  void testStartMonitorSetsUserSpecifiedPostProcessingProperties() {
    auto presenter = makePresenter();
    auto options = AlgorithmRuntimeProps{{"Prop1", "val1"}, {"Prop2", "val2"}};
    expectGetLiveDataOptions(options);
    auto algRunner = expectGetAlgorithmRunner();
    presenter.notifyStartMonitor();
    assertPostProcessingPropertiesContainOptions(options, algRunner);
    verifyAndClear();
  }

  void testStopMonitorUpdatesView() {
    auto presenter = makePresenter();
    presenter.m_monitorAlg =
        AlgorithmManager::Instance().createUnmanaged("MonitorLiveData");
    expectUpdateViewWhenMonitorStopped();
    presenter.notifyStopMonitor();
    TS_ASSERT_EQUALS(presenter.m_monitorAlg, nullptr);
    verifyAndClear();
  }

  void testMonitorNotRunningAfterStartMonitorFails() {
    auto presenter = makePresenter();
    auto algRunner = expectGetAlgorithmRunner();

    // Ideally we should have a mock algorithm but for now just create the real
    // one but don't run it so that it will fail to find the results
    auto startMonitorAlg =
        AlgorithmManager::Instance().createUnmanaged("StartLiveData");
    startMonitorAlg->initialize();
    EXPECT_CALL(*algRunner, getAlgorithm())
        .Times(1)
        .WillOnce(Return(startMonitorAlg));
    expectUpdateViewWhenMonitorStopped();
    presenter.notifyStartMonitorComplete();
    verifyAndClear();
  }

  void testNotifyTableChangedSetsUnsavedFlag() {
    auto presenter = makePresenter();
    presenter.notifyTableChanged();
    TS_ASSERT_EQUALS(presenter.hasUnsavedChanges(), true);
    verifyAndClear();
  }

  void testNotifyChangsSavedClearsUnsavedFlag() {
    auto presenter = makePresenter();
    presenter.notifyTableChanged();
    presenter.notifyChangesSaved();
    TS_ASSERT_EQUALS(presenter.hasUnsavedChanges(), false);
    verifyAndClear();
  }

  void testNotifyChangsSavedUpdatesSearcher() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_searcher, setSaved()).Times(1);
    presenter.notifyChangesSaved();
    verifyAndClear();
  }

private:
  class RunsPresenterFriend : public RunsPresenter {
    friend class RunsPresenterTest;

  public:
    RunsPresenterFriend(IRunsView *mainView, ProgressableView *progressView,
                        const RunsTablePresenterFactory &makeRunsTablePresenter,
                        double thetaTolerance,
                        std::vector<std::string> const &instruments,
                        IMessageHandler *messageHandler)
        : RunsPresenter(mainView, progressView, makeRunsTablePresenter,
                        thetaTolerance, instruments, messageHandler) {}
  };

  RunsPresenterFriend makePresenter() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    IPythonRunner *pythonRunner = new MockPythonRunner();
    Plotter plotter(pythonRunner);
#else
    Plotter plotter;
#endif
    auto makeRunsTablePresenter = RunsTablePresenterFactory(
        m_instruments, m_thetaTolerance, std::move(plotter));
    auto presenter =
        RunsPresenterFriend(&m_view, &m_progressView, makeRunsTablePresenter,
                            m_thetaTolerance, m_instruments, &m_messageHandler);

    presenter.acceptMainPresenter(&m_mainPresenter);
    presenter.m_tablePresenter.reset(new NiceMock<MockRunsTablePresenter>());
    m_runsTablePresenter = dynamic_cast<NiceMock<MockRunsTablePresenter> *>(
        presenter.m_tablePresenter.get());
    presenter.m_runNotifier.reset(new NiceMock<MockRunNotifier>());
    m_runNotifier = dynamic_cast<NiceMock<MockRunNotifier> *>(
        presenter.m_runNotifier.get());
    presenter.m_searcher.reset(new NiceMock<MockSearcher>());
    m_searcher =
        dynamic_cast<NiceMock<MockSearcher> *>(presenter.m_searcher.get());

    // Return an empty table by default
    ON_CALL(*m_runsTablePresenter, runsTable())
        .WillByDefault(ReturnRef(m_runsTable));

    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_runsTableView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_runsTablePresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_progressView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_messageHandler));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_searcher));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_pythonRunner));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_runNotifier));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobs));
  }

  AlgorithmRuntimeProps defaultLiveMonitorAlgorithmOptions(
      const std::string &instrument = std::string("OFFSPEC"),
      const int &updateInterval = 15) {
    const std::string updateIntervalString = std::to_string(updateInterval);
    return AlgorithmRuntimeProps{
        {"Instrument", instrument},
        {"OutputWorkspace", "IvsQ_binned_live"},
        {"AccumulationWorkspace", "TOF_live"},
        {"AccumulationMethod", "Replace"},
        {"UpdateEvery", updateIntervalString},
        {"PostProcessingAlgorithm", "ReflectometryReductionOneLiveData"},
        {"RunTransitionBehavior", "Restart"},
    };
  }

  AlgorithmRuntimeProps defaultLiveMonitorReductionOptions(
      const std::string &instrument = std::string("OFFSPEC")) {
    return AlgorithmRuntimeProps{
        {"GetLiveValueAlgorithm", "GetLiveInstrumentValue"},
        {"InputWorkspace", "TOF_live"},
        {"Instrument", instrument}};
  }

  void expectRunsTableWithContent(RunsTable &runsTable) {
    EXPECT_CALL(*m_runsTablePresenter, runsTable())
        .Times(1)
        .WillOnce(ReturnRef(runsTable));
  }

  void expectUpdateViewWhenMonitorStarting() {
    EXPECT_CALL(m_view, setStartMonitorButtonEnabled(false));
    EXPECT_CALL(m_view, setStopMonitorButtonEnabled(false));
    EXPECT_CALL(m_view, setUpdateIntervalSpinBoxEnabled(false));
  }

  void expectUpdateViewWhenMonitorStarted() {
    EXPECT_CALL(m_view, setStartMonitorButtonEnabled(false));
    EXPECT_CALL(m_view, setStopMonitorButtonEnabled(true));
    EXPECT_CALL(m_view, setUpdateIntervalSpinBoxEnabled(false));
  }

  void expectUpdateViewWhenMonitorStopped() {
    EXPECT_CALL(m_view, setStartMonitorButtonEnabled(true));
    EXPECT_CALL(m_view, setStopMonitorButtonEnabled(false));
    EXPECT_CALL(m_view, setUpdateIntervalSpinBoxEnabled(true));
  }

  void expectStopAutoreduction() {
    EXPECT_CALL(*m_runNotifier, stopPolling()).Times(1);
  }

  void expectSearchSettingsChanged() {
    auto const newCriteria =
        SearchCriteria{"new_instrument", "new cycle", "new search string"};
    EXPECT_CALL(*m_searcher, searchCriteria())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(newCriteria));
  }

  void expectSearchSettingsDefault() {
    auto const criteria = SearchCriteria{m_instrument, m_cycle, m_searchString};
    EXPECT_CALL(*m_searcher, searchCriteria())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(criteria));
  }

  void expectClearExistingTable() {
    EXPECT_CALL(*m_searcher, reset()).Times(1);
    EXPECT_CALL(*m_runsTablePresenter, notifyRemoveAllRowsAndGroupsRequested())
        .Times(1);
  }

  void expectDoNotClearExistingTable() {
    EXPECT_CALL(*m_searcher, reset()).Times(0);
    EXPECT_CALL(*m_runsTablePresenter, notifyRemoveAllRowsAndGroupsRequested())
        .Times(0);
  }

  void expectCheckForNewRuns() {
    EXPECT_CALL(*m_runNotifier, stopPolling()).Times(1);
    expectSearchInstrument(m_instrument);
    expectSearchString(m_searchString);
    expectSearchCycle(m_cycle);
    EXPECT_CALL(*m_searcher, startSearchAsync(SearchCriteria{
                                 m_instrument, m_cycle, m_searchString}))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(m_messageHandler, giveUserCritical(_, _)).Times(0);
  }

  void expectDoNotStartAutoreduction() {
    EXPECT_CALL(*m_runNotifier, stopPolling()).Times(0);
    EXPECT_CALL(*m_searcher, startSearchAsync(_)).Times(0);
  }

  void expectGetValidSearchRowSelection() {
    // Select a couple of rows with random indices
    auto row1Index = 3;
    auto row2Index = 5;
    auto const selectedRows = std::set<int>({row1Index, row2Index});
    EXPECT_CALL(m_view, getSelectedSearchRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    m_searchResult = SearchResult("", "");
    for (auto rowIndex : selectedRows)
      EXPECT_CALL(*m_searcher, getSearchResult(rowIndex))
          .Times(1)
          .WillOnce(ReturnRef(m_searchResult));
  }

  // Set up a valid search result with content and return the corresponding
  // model
  ReductionJobs
  expectGetValidSearchResult(std::string const &run = "13245",
                             std::string const &groupName = "Test group 1",
                             double theta = 0.5) {
    // Set a selected row in the search results table
    auto const rowIndex = 0;
    auto const selectedRows = std::set<int>({rowIndex});
    EXPECT_CALL(m_view, getSelectedSearchRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    // Set the expected result from the search results model
    auto const title = groupName + std::string("th=") + std::to_string(theta);
    m_searchResult = SearchResult(run, title);
    EXPECT_CALL(*m_searcher, getSearchResult(rowIndex))
        .Times(1)
        .WillOnce(ReturnRef(m_searchResult));
    // Construct the corresponding model expected in the main table
    auto jobs = ReductionJobs();
    auto group = Group(groupName);
    group.appendRow(Row({run}, theta, TransmissionRunPair(), RangeInQ(),
                        boost::none, ReductionOptionsMap(),
                        ReductionWorkspaces({run}, TransmissionRunPair())));
    jobs.appendGroup(group);
    return jobs;
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
    expectIsNotProcessing();
    expectIsAutoreducing();
    EXPECT_CALL(m_view, updateMenuEnabledState(false));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(false));
    EXPECT_CALL(m_view, setSearchButtonEnabled(false));
    EXPECT_CALL(m_view, setSearchResultsEnabled(false));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(true));
    EXPECT_CALL(m_view, setTransferButtonEnabled(false));
  }

  void expectWidgetsEnabledForProcessing() {
    expectIsProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_view, updateMenuEnabledState(true));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(false));
    EXPECT_CALL(m_view, setSearchButtonEnabled(false));
    EXPECT_CALL(m_view, setSearchResultsEnabled(false));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(false));
    EXPECT_CALL(m_view, setTransferButtonEnabled(false));
  }

  void expectWidgetsEnabledForProcessingAndAutoreducing() {
    expectIsProcessing();
    expectIsAutoreducing();
    EXPECT_CALL(m_view, updateMenuEnabledState(true));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(false));
    EXPECT_CALL(m_view, setSearchButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(true));
    EXPECT_CALL(m_view, setTransferButtonEnabled(false));
  }

  void expectWidgetsEnabledForPaused() {
    expectIsNotProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_view, updateMenuEnabledState(false));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(true));
    EXPECT_CALL(m_view, setSearchTextEntryEnabled(true));
    EXPECT_CALL(m_view, setSearchButtonEnabled(true));
    EXPECT_CALL(m_view, setSearchResultsEnabled(true));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(true));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(false));
    EXPECT_CALL(m_view, setTransferButtonEnabled(true));
  }

  void expectInstrumentComboIsDisabledWhenAnotherBatchReducing() {
    expectIsNotProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
  }

  void expectInstrumentComboIsEnabledWhenNoBatchesAreReducing() {
    expectIsNotProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(true));
  }

  void expectInstrumentComboIsDisabledWhenAnotherBatchAutoreducing() {
    expectIsNotProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(false));
  }

  void expectInstrumentComboIsEnabledWhenNoBatchesAreAutoreducing() {
    expectIsNotProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setInstrumentComboEnabled(true));
  }

  void expectAutoreduceButtonDisabledWhenAnotherBatchAutoreducing() {
    expectIsNotProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(false));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(false));
  }

  void expectAutoreduceButtonEnabledWhenNoBatchesAreAutoreducing() {
    expectIsNotProcessing();
    expectIsNotAutoreducing();
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setAutoreduceButtonEnabled(true));
    EXPECT_CALL(m_view, setAutoreducePauseButtonEnabled(false));
  }

  void expectIsProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    ON_CALL(m_mainPresenter, isAnyBatchProcessing())
        .WillByDefault(Return(true));
  }

  void expectIsNotProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    ON_CALL(m_mainPresenter, isAnyBatchProcessing())
        .WillByDefault(Return(false));
  }

  void expectIsAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    ON_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .WillByDefault(Return(true));
  }

  void expectIsNotAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    ON_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .WillByDefault(Return(false));
  }

  // current search instrument on the view
  void expectSearchInstrument(std::string const &instrument) {
    EXPECT_CALL(m_view, getSearchInstrument())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(instrument));
  }

  // previously saved instrument
  void expectPreviousInstrument(std::string const &instrument) {
    EXPECT_CALL(m_mainPresenter, instrumentName())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(instrument));
  }

  void expectUnsavedSearchResults() {
    EXPECT_CALL(*m_searcher, hasUnsavedChanges())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
  }

  void expectNoUnsavedSearchResults() {
    EXPECT_CALL(*m_searcher, hasUnsavedChanges())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
  }

  void expectChangeInstrumentPrevented() {
    expectUnsavedSearchResults();
    EXPECT_CALL(m_mainPresenter, discardChanges(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
  }

  void expectSearchString(std::string const &searchString) {
    EXPECT_CALL(m_view, getSearchString())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(searchString));
  }

  void expectSearchCycle(std::string const &cycle) {
    EXPECT_CALL(m_view, getSearchCycle())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(cycle));
  }

  void expectGetUpdateInterval(int const &updateInterval) {
    EXPECT_CALL(m_view, getLiveDataUpdateInterval())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(updateInterval));
  }

  void expectGetLiveDataOptions(
      AlgorithmRuntimeProps options = AlgorithmRuntimeProps(),
      std::string const &instrument = std::string("OFFSPEC"),
      int const &updateInterval = 15) {
    expectSearchInstrument(instrument);
    expectGetUpdateInterval(updateInterval);
    EXPECT_CALL(m_mainPresenter, rowProcessingProperties())
        .Times(1)
        .WillOnce(Return(std::move(options)));
  }

  void expectGetLiveDataOptions(std::string const &instrument,
                                const int &updateInterval) {
    expectGetLiveDataOptions(AlgorithmRuntimeProps(), instrument,
                             updateInterval);
  }

  std::shared_ptr<NiceMock<MockAlgorithmRunner>> expectGetAlgorithmRunner() {
    // Get the algorithm runner
    auto algRunner = std::make_shared<NiceMock<MockAlgorithmRunner>>();
    ON_CALL(m_view, getMonitorAlgorithmRunner())
        .WillByDefault(Return(algRunner));
    return algRunner;
  }

  void expectStartingLiveDataSucceeds() {
    // The view must return valid reduction options and algorithm runner for
    // the presenter to be able to run live data
    expectGetLiveDataOptions();
    expectGetAlgorithmRunner();
  }

  void expectOverwriteSearchResultsPrevented() {
    expectUnsavedSearchResults();
    EXPECT_CALL(m_mainPresenter, discardChanges(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
  }

  void assertAlgorithmPropertiesContainOptions(
      AlgorithmRuntimeProps const &expected,
      std::shared_ptr<NiceMock<MockAlgorithmRunner>> &algRunner) {
    auto alg = algRunner->algorithm();
    for (auto const &kvp : expected) {
      TS_ASSERT_EQUALS(alg->getPropertyValue(kvp.first), kvp.second);
    }
  }

  void assertPostProcessingPropertiesContainOptions(
      AlgorithmRuntimeProps &expected,
      std::shared_ptr<NiceMock<MockAlgorithmRunner>> &algRunner) {
    auto alg = algRunner->algorithm();
    auto resultString = alg->getPropertyValue("PostProcessingProperties");
    auto result = parseKeyValueString(resultString, ";");
    for (auto const &kvp : expected) {
      TS_ASSERT(result.find(kvp.first) != result.end());
      TS_ASSERT_EQUALS(result[kvp.first], expected[kvp.first]);
    }
  }

  double m_thetaTolerance;
  std::vector<std::string> m_instruments;
  RunsTable m_runsTable;
  std::string m_searchString;
  SearchResult m_searchResult;
  std::string m_instrument;
  std::string m_cycle;

  NiceMock<MockRunsView> m_view;
  NiceMock<MockRunsTableView> m_runsTableView;
  NiceMock<MockRunsTablePresenter> *m_runsTablePresenter;
  NiceMock<MockBatchPresenter> m_mainPresenter;
  NiceMock<MockProgressableView> m_progressView;
  NiceMock<MockMessageHandler> m_messageHandler;
  NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> m_jobs;
  NiceMock<MockSearcher> *m_searcher;
  MockPythonRunner *m_pythonRunner;
  MockRunNotifier *m_runNotifier;
};
