// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Batch/BatchPresenter.h"
#include "../../../ISISReflectometry/Reduction/RowExceptions.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../MainWindow/MockMainWindowPresenter.h"
#include "../Preview/MockPreviewPresenter.h"
#include "../Reduction/MockBatch.h"
#include "../ReflMockObjects.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/MockJobRunner.h"
#include "MockBatchView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::API::IConfiguredAlgorithm_sptr;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;
using testing::Throw;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

MATCHER_P(CheckRunNumbers, runNumbers, "") { return arg.runNumbers() == runNumbers; }

GNU_DIAG_ON_SUGGEST_OVERRIDE

class BatchPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchPresenterTest *createSuite() { return new BatchPresenterTest(); }
  static void destroySuite(BatchPresenterTest *suite) { delete suite; }

  BatchPresenterTest()
      : m_view(), m_jobManager(nullptr), m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"}, m_tolerance(0.1),
        m_experiment(makeEmptyExperiment()), m_instrument(makeEmptyInstrument()),
        m_runsTable(m_instruments, 0.1, ReductionJobs()), m_slicing(),
        m_mockAlgorithmsList{std::make_shared<MockBatchJobAlgorithm>()} {
    Mantid::API::FrameworkManager::Instance();
  }

  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void testInitInstrumentListUpdatesRunsPresenter() {
    auto presenter = makePresenter(makeModel());
    std::string const selectedInstrument = "INTER";
    EXPECT_CALL(*m_runsPresenter, initInstrumentList(selectedInstrument)).Times(1).WillOnce(Return(selectedInstrument));
    TS_ASSERT_EQUALS(presenter->initInstrumentList(selectedInstrument), selectedInstrument);
  }

  void testMainPresenterUpdatedWhenChangeInstrumentRequested() {
    auto presenter = makePresenter(makeModel());
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(instrument)).Times(1);
    presenter->notifyChangeInstrumentRequested(instrument);
  }

  void testChildPresentersAreUpdatedWhenInstrumentChanged() {
    auto presenter = makePresenter(makeModel());
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(*m_runsPresenter, notifyInstrumentChanged(instrument)).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyInstrumentChanged(instrument)).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyInstrumentChanged(instrument)).Times(1);
    presenter->notifyInstrumentChanged(instrument);
  }

  void testMainPresenterUpdatedWhenUpdateInstrumentRequested() {
    auto presenter = makePresenter(makeModel());
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(m_mainPresenter, notifyUpdateInstrumentRequested()).Times(1);
    presenter->notifyUpdateInstrumentRequested();
  }

  void testChildPresentersUpdatedWhenSettingsChanged() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, settingsChanged()).Times(1);
    presenter->notifySettingsChanged();
  }

  void testModelUpdatedWhenReductionResumed() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    presenter->notifyResumeReductionRequested();
  }

  void testBatchIsExecutedWhenReductionResumed() {
    auto presenter = makePresenter(makeModel());
    expectBatchIsExecuted();
    presenter->notifyResumeReductionRequested();
  }

  void testOtherPresentersUpdatedWhenReductionResumed() {
    auto presenter = makePresenter(makeModel());
    expectReductionResumed();
    presenter->notifyResumeReductionRequested();
  }

  void testJobManagerGetProcessAll() {
    auto presenter = makePresenter(makeModel());
    TS_ASSERT_EQUALS(m_jobManager->getProcessAll(), false);
    expectReductionResumed();
    presenter->notifyResumeReductionRequested();
  }

  void testJobManagerGetProcessPartial() {
    auto presenter = makePresenter(makeModel());
    TS_ASSERT_EQUALS(m_jobManager->getProcessPartial(), false);
    expectReductionResumed();
    presenter->notifyResumeReductionRequested();
  }

  void testWarnProcessAllWhenReductionResumedOptionChecked() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, getProcessAll()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessAllPrevented()).Times(1).WillOnce(Return(true));
    presenter->notifyResumeReductionRequested();
  }

  void testNoWarnProcessAllWhenReductionResumedOptionUnchecked() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, getProcessAll()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessAllPrevented()).Times(1).WillOnce(Return(false));
    presenter->notifyResumeReductionRequested();
  }

  void testWarnProcessPartialGroupWhenReductionResumedOptionChecked() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, getProcessPartial()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessPartialGroupPrevented()).Times(1).WillOnce(Return(true));
    presenter->notifyResumeReductionRequested();
  }

  void testNoWarnProcessPartialGroupWhenReductionResumedOptionUnchecked() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, getProcessPartial()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessPartialGroupPrevented()).Times(1).WillOnce(Return(false));
    presenter->notifyResumeReductionRequested();
  }

  void testChildPresentersUpdatedWhenAnyBatchReductionResumed() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchReductionResumed()).Times(1);
    presenter->notifyAnyBatchReductionResumed();
  }

  void testChildPresentersUpdatedWhenAnyBatchReductionPaused() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchReductionPaused()).Times(1);
    presenter->notifyAnyBatchReductionPaused();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAnyBatchAutoreductionResumed() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchAutoreductionResumed()).Times(1);
    presenter->notifyAnyBatchAutoreductionResumed();
  }

  void testChildPresentersUpdatedWhenAnyBatchAutoreductionPaused() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchAutoreductionPaused()).Times(1);
    presenter->notifyAnyBatchAutoreductionPaused();
  }

  void testMainPresenterQueriedWhenCheckingAnyBatchProcessing() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing()).Times(1).WillOnce(Return(true));
    auto result = presenter->isAnyBatchProcessing();
    TS_ASSERT_EQUALS(result, true);
  }

  void testMainPresenterQueriedWhenCheckingAnyBatchAutoreducing() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing()).Times(1).WillOnce(Return(true));
    auto result = presenter->isAnyBatchAutoreducing();
    TS_ASSERT_EQUALS(result, true);
  }

  void testAutoreductionCompletedWhenReductionResumedWithNoRemainingJobs() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, getAlgorithms()).Times(1).WillOnce(Return(std::deque<IConfiguredAlgorithm_sptr>()));
    EXPECT_CALL(*m_jobManager, isAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_runsPresenter, autoreductionCompleted()).Times(1);
    presenter->notifyResumeReductionRequested();
  }

  void testAutoreductionNotCompletedWhenReductionResumedWithRemainingJobs() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, autoreductionCompleted()).Times(0);
    presenter->notifyResumeReductionRequested();
  }

  void testBatchIsCancelledWhenReductionPaused() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobRunner, cancelAlgorithmQueue()).Times(1);
    presenter->notifyPauseReductionRequested();
  }

  void testModelUpdatedWhenBatchCancelled() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, notifyReductionPaused()).Times(1);
    presenter->notifyBatchCancelled();
  }

  void testChildPresentersUpdatedWhenBatchCancelled() {
    auto presenter = makePresenter(makeModel());
    expectReductionPaused();
    expectAutoreductionPaused();
    presenter->notifyBatchCancelled();
  }

  void testModelUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_jobManager, notifyAutoreductionPaused()).Times(0);
    presenter->notifyResumeAutoreductionRequested();
  }

  void testRunsPresenterCalledWhenAutoreductionResumed() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, resumeAutoreduction()).Times(1);
    presenter->notifyResumeAutoreductionRequested();
  }

  void testModelResetWhenAutoreductionCancelled() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, resumeAutoreduction()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_jobManager, notifyAutoreductionPaused()).Times(1);
    presenter->notifyResumeAutoreductionRequested();
  }

  void testOtherPresentersUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter(makeModel());
    expectAutoreductionResumed();
    presenter->notifyResumeAutoreductionRequested();
  }

  void testChildPresentersNotUpdatedWhenAutoreductionCanelled() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, resumeAutoreduction()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_savePresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_eventPresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_experimentPresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_instrumentPresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyAutoreductionResumed()).Times(0);
    presenter->notifyResumeAutoreductionRequested();
  }

  void testModelUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, notifyAutoreductionPaused()).Times(1);
    presenter->notifyPauseAutoreductionRequested();
  }

  void testBatchIsCancelledWhenAutoreductionPaused() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobRunner, cancelAlgorithmQueue()).Times(1);
    presenter->notifyPauseAutoreductionRequested();
  }

  void testOtherPresentersUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter(makeModel());
    expectAutoreductionPaused();
    presenter->notifyPauseAutoreductionRequested();
  }

  void testAutoreductionComplete() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, autoreductionCompleted()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter->notifyAutoreductionCompleted();
  }

  void testNextBatchIsStartedWhenBatchFinished() {
    auto presenter = makePresenter(makeModel());
    expectBatchIsExecuted();
    presenter->notifyBatchComplete(false);
  }

  void testChildPresentersUpdatedWhenBatchFinishedAndNothingLeftToProcess() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, getAlgorithms()).Times(1).WillOnce(Return(std::deque<IConfiguredAlgorithm_sptr>()));
    expectReductionPaused();
    presenter->notifyBatchComplete(false);
  }

  void testNotifyAlgorithmStarted() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    auto row = makeRow();
    auto optionalRow = boost::optional<Item &>(row);
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(optionalRow));
    EXPECT_CALL(*m_jobManager, algorithmStarted(algorithm)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(1);
    presenter->notifyAlgorithmStarted(algorithm);
  }

  void testNotifyAlgorithmComplete() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    auto row = makeRow();
    auto optionalRow = boost::optional<Item &>(row);
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(optionalRow));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(1);
    presenter->notifyAlgorithmComplete(algorithm);
  }

  void testNotifyAlgorithmStartedSkipsNonItems() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(boost::none));
    EXPECT_CALL(*m_jobManager, algorithmStarted(_)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(0);
    presenter->notifyAlgorithmStarted(algorithm);
  }

  void testNotifyAlgorithmCompleteSkipsNonItems() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(boost::none));
    EXPECT_CALL(*m_jobManager, algorithmComplete(_)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(0);
    presenter->notifyAlgorithmComplete(algorithm);
  }

  void testNotifyAlgorithmErrorSkipsNonItems() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(boost::none));
    EXPECT_CALL(*m_jobManager, algorithmError(_, _)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(0);
    presenter->notifyAlgorithmError(algorithm, "");
  }

  void testOutputWorkspacesSavedOnAlgorithmComplete() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_savePresenter, shouldAutosave()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_savePresenter, shouldAutosaveGroupRows()).Times(1).WillOnce(Return(false));
    auto const workspaces = std::vector<std::string>{"test1", "test2"};
    auto row = makeRow();
    auto optionalRow = boost::optional<Item &>(row);
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(optionalRow));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_jobManager, algorithmOutputWorkspacesToSave(algorithm, false)).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(*m_savePresenter, saveWorkspaces(workspaces, true)).Times(1);
    presenter->notifyAlgorithmComplete(algorithm);
  }

  void testOutputWorkspacesSavedOnAlgorithmCompleteWithAutosaveGroupRows() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_savePresenter, shouldAutosave()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_savePresenter, shouldAutosaveGroupRows()).Times(1).WillOnce(Return(true));
    auto const workspaces = std::vector<std::string>{"test1", "test2"};
    auto row = makeRow();
    auto optionalRow = boost::optional<Item &>(row);
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(optionalRow));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_jobManager, algorithmOutputWorkspacesToSave(algorithm, true)).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(*m_savePresenter, saveWorkspaces(workspaces, true)).Times(1);
    presenter->notifyAlgorithmComplete(algorithm);
  }

  void testOutputWorkspacesNotSavedIfAutosaveDisabled() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_savePresenter, shouldAutosave()).Times(1).WillOnce(Return(false));
    auto row = makeRow();
    auto optionalRow = boost::optional<Item &>(row);
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(optionalRow));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_jobManager, algorithmOutputWorkspacesToSave(_, _)).Times(0);
    EXPECT_CALL(*m_savePresenter, saveWorkspaces(_, true)).Times(0);
    presenter->notifyAlgorithmComplete(algorithm);
  }

  void testOutputWorkspacesNotSavedWithAutosaveIfNoWorkspacesToSave() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_savePresenter, shouldAutosave()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_savePresenter, shouldAutosaveGroupRows()).Times(1).WillOnce(Return(true));
    const std::vector<std::string> workspaces;
    auto row = makeRow();
    auto optionalRow = boost::optional<Item &>(row);
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(optionalRow));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_jobManager, algorithmOutputWorkspacesToSave(algorithm, true)).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(*m_savePresenter, saveWorkspaces(workspaces, true)).Times(0);
    presenter->notifyAlgorithmComplete(algorithm);
  }

  void testNotifyAlgorithmError() {
    auto presenter = makePresenter(makeModel());
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    auto const errorMessage = std::string("test error");
    auto row = makeRow();
    auto optionalRow = boost::optional<Item &>(row);
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(optionalRow));
    EXPECT_CALL(*m_jobManager, algorithmError(algorithm, errorMessage)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(1);
    presenter->notifyAlgorithmError(algorithm, errorMessage);
  }

  void testModelUpdatedWhenWorkspaceDeleted() {
    auto presenter = makePresenter(makeModel());
    auto name = std::string("test_workspace");
    EXPECT_CALL(*m_jobManager, notifyWorkspaceDeleted(name)).Times(1);
    presenter->postDeleteHandle(name);
  }

  void testRowStateUpdatedWhenWorkspaceDeleted() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(1);
    presenter->postDeleteHandle("");
  }

  void testModelUpdatedWhenWorkspaceRenamed() {
    auto presenter = makePresenter(makeModel());
    auto oldName = std::string("test_workspace1");
    auto newName = std::string("test_workspace2");
    EXPECT_CALL(*m_jobManager, notifyWorkspaceRenamed(oldName, newName)).Times(1);
    presenter->renameHandle(oldName, newName);
  }

  void testRowStateUpdatedWhenWorkspaceRenamed() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged(_)).Times(1);
    presenter->renameHandle("", "");
  }

  void testModelUpdatedWhenWorkspacesCleared() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_jobManager, notifyAllWorkspacesDeleted()).Times(1);
    presenter->clearADSHandle();
  }

  void testRowStateUpdatedWhenWorkspacesCleared() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged()).Times(1);
    presenter->clearADSHandle();
  }

  void testPercentCompleteIsRequestedFromJobManager() {
    auto presenter = makePresenter(makeModel());
    auto progress = 33;
    EXPECT_CALL(*m_jobManager, percentComplete()).Times(1).WillOnce(Return(progress));
    TS_ASSERT_EQUALS(presenter->percentComplete(), progress);
  }

  void testRunsPresenterNotifiesSetRoundPrecision() {
    auto presenter = makePresenter(makeModel());
    auto prec = 2;
    EXPECT_CALL(*m_runsPresenter, setRoundPrecision(prec));
    presenter->notifySetRoundPrecision(prec);
  }

  void testRunsPresenterNotifiesResetRoundPrecision() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, resetRoundPrecision());
    presenter->notifyResetRoundPrecision();
  }

  void testNotifyBatchLoaded() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_runsPresenter, notifyBatchLoaded());
    presenter->notifyBatchLoaded();
  }

  void testWarningShownOnResumeWhenExperimentSettingsInvalid() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_experimentPresenter, hasValidSettings()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_messageHandler, giveUserCritical(_, _)).Times(1);
    presenter->notifyResumeReductionRequested();
  }

  void testWarningShownOnAutoreduceWhenExperimentSettingsInvalid() {
    auto presenter = makePresenter(makeModel());
    EXPECT_CALL(*m_experimentPresenter, hasValidSettings()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_messageHandler, giveUserCritical(_, _)).Times(1);
    presenter->notifyResumeAutoreductionRequested();
  }

  void testAllIndexesUpdatedWhenSettingsChanged() {
    auto mock = makeMockModel();
    EXPECT_CALL(*mock, updateLookupIndexesOfTable()).Times(1);
    auto presenter = makePresenter(std::move(mock));
    presenter->notifySettingsChanged();
  }

  void testSingleRowUpdatedWhenRowContentChanged() {
    auto mock = makeMockModel();
    auto row = makeRow(0.7);
    EXPECT_CALL(*mock, updateLookupIndex(row)).Times(1);
    auto presenter = makePresenter(std::move(mock));
    presenter->notifyRowContentChanged(row);
  }

  void testModelInformedWhenGroupNameChanged() {
    auto mock = makeMockModel();
    auto group = makeGroupWithOneRow();
    EXPECT_CALL(*mock, updateLookupIndexesOfGroup(group)).Times(1);
    auto presenter = makePresenter(std::move(mock));
    presenter->notifyGroupNameChanged(group);
  }

  void testIndexesUpdatedWhenRowsTransferred() {
    auto mock = makeMockModel();
    EXPECT_CALL(*mock, updateLookupIndexesOfTable()).Times(1);
    auto presenter = makePresenter(std::move(mock));
    EXPECT_CALL(*m_runsPresenter, notifyRowModelChanged()).Times(1);
    presenter->notifyRunsTransferred();
  }

  void testNotifyPreviewApplyRequested() {
    auto presenter = makePresenter(makeModel());
    auto const previewRow = PreviewRow({"12345"});
    EXPECT_CALL(*m_previewPresenter, getPreviewRow()).Times(1).WillOnce(ReturnRef(previewRow));
    EXPECT_CALL(*m_experimentPresenter, notifyPreviewApplyRequested(CheckRunNumbers(previewRow.runNumbers()))).Times(1);
    presenter->notifyPreviewApplyRequested();
  }

  void testHasROIDetectorIDsForPreviewRow() {
    auto const lookupRow = makeLookupRow(boost::none);
    auto const maybeLookupRow = boost::optional<LookupRow>(lookupRow);
    runHasROIDetectorIDsForPreviewRowTest(maybeLookupRow, true);
  }

  void testHasROIDetectorIDsForPreviewRowNoDetectorIdsInLookupRow() {
    auto lookupRow = makeLookupRow(boost::none);
    lookupRow.setRoiDetectorIDs(boost::none);
    auto const maybeLookupRow = boost::optional<LookupRow>(lookupRow);
    runHasROIDetectorIDsForPreviewRowTest(maybeLookupRow, false);
  }

  void testHasROIDetectorIDsForPreviewRowNoLookupRowFound() {
    runHasROIDetectorIDsForPreviewRowTest(boost::none, false);
  }

  void testHasROIDetectorIDsForPreviewRowMultipleLookupRowsFound() {
    auto mockModel = makeMockModel();
    auto const previewRow = PreviewRow({"12345"});
    EXPECT_CALL(*mockModel, findLookupPreviewRowProxy(_)).WillOnce(Throw(MultipleRowsFoundException("")));
    auto presenter = makePresenter(std::move(mockModel));
    EXPECT_CALL(*m_previewPresenter, getPreviewRow()).Times(1).WillOnce(ReturnRef(previewRow));
    TS_ASSERT_EQUALS(presenter->hasROIDetectorIDsForPreviewRow(), false);
  }

private:
  NiceMock<MockBatchView> m_view;
  NiceMock<MockBatchJobManager> *m_jobManager;
  NiceMock<MockJobRunner> *m_jobRunner;
  NiceMock<MockMainWindowPresenter> m_mainPresenter;
  NiceMock<MockRunsPresenter> *m_runsPresenter;
  NiceMock<MockEventPresenter> *m_eventPresenter;
  NiceMock<MockExperimentPresenter> *m_experimentPresenter;
  NiceMock<MockInstrumentPresenter> *m_instrumentPresenter;
  NiceMock<MockSavePresenter> *m_savePresenter;
  NiceMock<MockPreviewPresenter> *m_previewPresenter;
  StrictMock<MockMessageHandler> m_messageHandler;
  std::vector<std::string> m_instruments;
  double m_tolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
  std::deque<IConfiguredAlgorithm_sptr> m_mockAlgorithmsList;

  void runHasROIDetectorIDsForPreviewRowTest(boost::optional<LookupRow> lookupRow, bool expectedResult) {
    auto mockModel = makeMockModel();
    auto const previewRow = PreviewRow({"12345"});
    EXPECT_CALL(*mockModel, findLookupPreviewRowProxy(_)).Times(1).WillOnce(Return(lookupRow));
    auto presenter = makePresenter(std::move(mockModel));
    EXPECT_CALL(*m_previewPresenter, getPreviewRow()).Times(1).WillOnce(ReturnRef(previewRow));
    TS_ASSERT_EQUALS(presenter->hasROIDetectorIDsForPreviewRow(), expectedResult);
  }

  class BatchPresenterFriend : public BatchPresenter {
    friend class BatchPresenterTest;

  public:
    BatchPresenterFriend(IBatchView *view, std::unique_ptr<IBatch> model,
                         std::unique_ptr<MantidQt::API::IJobRunner> jobRunner,
                         std::unique_ptr<IRunsPresenter> runsPresenter, std::unique_ptr<IEventPresenter> eventPresenter,
                         std::unique_ptr<IExperimentPresenter> experimentPresenter,
                         std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
                         std::unique_ptr<ISavePresenter> savePresenter,
                         std::unique_ptr<IPreviewPresenter> previewPresenter,
                         MantidQt::MantidWidgets::IMessageHandler *messageHandler)
        : BatchPresenter(view, std::move(model), std::move(jobRunner), std::move(runsPresenter),
                         std::move(eventPresenter), std::move(experimentPresenter), std::move(instrumentPresenter),
                         std::move(savePresenter), std::move(previewPresenter), messageHandler) {}
  };

  RunsTable makeRunsTable() { return RunsTable(m_instruments, m_tolerance, ReductionJobs()); }

  std::unique_ptr<IBatch> makeModel() {
    return std::make_unique<Batch>(m_experiment, m_instrument, m_runsTable, m_slicing);
  }

  std::unique_ptr<MockBatch> makeMockModel() { return std::make_unique<MockBatch>(); }

  std::unique_ptr<BatchPresenterFriend> makePresenter(std::unique_ptr<IBatch> batchModel) {
    // Create pointers to the child presenters and pass them into the batch
    auto runsPresenter = std::make_unique<NiceMock<MockRunsPresenter>>();
    auto eventPresenter = std::make_unique<NiceMock<MockEventPresenter>>();
    auto experimentPresenter = std::make_unique<NiceMock<MockExperimentPresenter>>();
    auto instrumentPresenter = std::make_unique<NiceMock<MockInstrumentPresenter>>();
    auto savePresenter = std::make_unique<NiceMock<MockSavePresenter>>();
    auto previewPresenter = std::make_unique<NiceMock<MockPreviewPresenter>>();
    auto jobRunner = std::make_unique<NiceMock<MockJobRunner>>();
    m_runsPresenter = runsPresenter.get();
    m_eventPresenter = eventPresenter.get();
    m_experimentPresenter = experimentPresenter.get();
    m_instrumentPresenter = instrumentPresenter.get();
    m_savePresenter = savePresenter.get();
    m_previewPresenter = previewPresenter.get();
    m_jobRunner = jobRunner.get();
    EXPECT_CALL(*m_jobRunner, subscribe(_)).Times(1);
    // Create the batch presenter
    auto presenter = std::make_unique<BatchPresenterFriend>(
        &m_view, std::move(batchModel), std::move(jobRunner), std::move(runsPresenter), std::move(eventPresenter),
        std::move(experimentPresenter), std::move(instrumentPresenter), std::move(savePresenter),
        std::move(previewPresenter), &m_messageHandler);
    presenter->acceptMainPresenter(&m_mainPresenter);
    // Replace the constructed job runner with a mock
    m_jobManager = new NiceMock<MockBatchJobManager>();
    presenter->m_jobManager.reset(m_jobManager);
    // The mock job runner should by default return our default algorithms list
    ON_CALL(*m_jobManager, getAlgorithms()).WillByDefault(Return(m_mockAlgorithmsList));
    ON_CALL(*m_jobManager, getProcessAll()).WillByDefault(Return(false));
    ON_CALL(*m_jobManager, getProcessPartial()).WillByDefault(Return(false));
    // The mock runs presenter should by default return true when autoreduction
    // is resumed
    ON_CALL(*m_runsPresenter, resumeAutoreduction()).WillByDefault(Return(true));
    ON_CALL(*m_experimentPresenter, hasValidSettings()).WillByDefault(Return(true));
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_runsPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_jobRunner));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_eventPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_experimentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_instrumentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_savePresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_previewPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_jobManager));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
  }

  void expectReductionResumed() {
    EXPECT_CALL(*m_previewPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_savePresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, notifyAnyBatchReductionResumed()).Times(1);
  }

  void expectReductionPaused() {
    EXPECT_CALL(*m_previewPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_savePresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyReductionPaused()).Times(1);
  }

  void expectAutoreductionResumed() {
    EXPECT_CALL(*m_previewPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_savePresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    EXPECT_CALL(m_mainPresenter, notifyAnyBatchAutoreductionResumed()).Times(1);
  }

  void expectAutoreductionPaused() {
    EXPECT_CALL(*m_previewPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_savePresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(m_mainPresenter, notifyAnyBatchAutoreductionPaused()).Times(1);
  }

  void expectBatchIsExecuted() {
    EXPECT_CALL(*m_jobManager, getAlgorithms()).Times(1);
    EXPECT_CALL(*m_jobRunner, clearAlgorithmQueue()).Times(1);
    EXPECT_CALL(*m_jobRunner, setAlgorithmQueue(m_mockAlgorithmsList)).Times(1);
    EXPECT_CALL(*m_jobRunner, executeAlgorithmQueue()).Times(1);
  }
};
