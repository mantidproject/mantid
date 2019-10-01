// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROCESSINGTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROCESSINGTEST_H_

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry::
    ModelCreationHelper;
using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using MantidQt::MantidWidgets::Batch::Cell;
using MantidQt::MantidWidgets::Batch::RowLocation;
using MantidQt::MantidWidgets::Batch::RowPath;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterProcessingTest : public CxxTest::TestSuite,
                                         RunsTablePresenterTest {
public:
  static RunsTablePresenterProcessingTest *createSuite() {
    return new RunsTablePresenterProcessingTest();
  }

  static void destroySuite(RunsTablePresenterProcessingTest *suite) {
    delete suite;
  }

  void testResumeReductionNotifiesParent() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, notifyResumeReductionRequested()).Times(1);
    presenter.notifyResumeReductionRequested();
    verifyAndClearExpectations();
  }

  void testPauseReductionNotifiesParent() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, notifyPauseReductionRequested()).Times(1);
    presenter.notifyPauseReductionRequested();
    verifyAndClearExpectations();
  }

  void testViewUpdatedWhenReductionResumed() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectIsProcessing();
    presenter.notifyReductionResumed();
    verifyAndClearExpectations();
  }

  void testChangingInstrumentIsDisabledWhenAnyBatchReducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    presenter.notifyAnyBatchReductionResumed();
    verifyAndClearExpectations();
  }

  void testChangingInstrumentIsEnabledWhenNoBatchesReducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(true)).Times(1);
    presenter.notifyAnyBatchReductionPaused();
    verifyAndClearExpectations();
  }

  void testViewUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectIsAutoreducing();
    presenter.notifyAutoreductionResumed();
    verifyAndClearExpectations();
  }

  void testViewUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectIsNotAutoreducing();
    presenter.notifyAutoreductionResumed();
    verifyAndClearExpectations();
  }

  void testChangingInstrumentIsDisabledWhenAnyBatchAutoreducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    presenter.notifyAnyBatchAutoreductionResumed();
    verifyAndClearExpectations();
  }

  void testChangingInstrumentIsEnabledWhenNoBatchesAutoreducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(true)).Times(1);
    presenter.notifyAnyBatchAutoreductionPaused();
    verifyAndClearExpectations();
  }

  void testNotifyChangeInstrumentRequested() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    auto const instrument = std::string("test_instrument");
    EXPECT_CALL(m_view, getInstrumentName())
        .Times(1)
        .WillOnce(Return(instrument));
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(instrument))
        .Times(1);
    presenter.notifyChangeInstrumentRequested();
    verifyAndClearExpectations();
  }

  void testNotifyInstrumentChanged() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    auto const instrument = std::string("test_instrument");
    EXPECT_CALL(m_view, setInstrumentName(instrument)).Times(1);
    presenter.notifyInstrumentChanged(instrument);
    verifyAndClearExpectations();
  }

  void testSettingsChangedResetsStateInModel() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    // Set success=true
    getGroup(presenter, 0).setSuccess();
    getRow(presenter, 0, 0)->setSuccess();
    presenter.settingsChanged();
    // Check success state is reset
    TS_ASSERT_EQUALS(getGroup(presenter, 0).success(), false);
    TS_ASSERT_EQUALS(getRow(presenter, 0, 0)->success(), false);
  }

  void testSettingsChangedResetsStateInView() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    expectGroupStateCleared();
    expectRowStateCleared();
    presenter.settingsChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForDefaultRowAndGroup() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    expectGroupStateCleared();
    expectRowStateCleared();
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForInvalidRow() {
    auto presenter = makePresenter(m_view, oneGroupWithAnInvalidRowModel());
    expectGroupStateCleared();
    expectRowStateInvalid();
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForStartingRow() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setStarting();
    expectGroupStateCleared();
    expectRowStateCleared();
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForRunningRow() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setRunning();
    expectGroupStateCleared();
    expectRowState(RUNNING);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForCompleteRow() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setSuccess();
    expectGroupStateCleared();
    expectRowState(SUCCESS);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForErrorRow() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setError("error message");
    expectGroupStateCleared();
    expectRowState(FAILURE);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForCompleteGroup() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getGroup(presenter, 0).setSuccess();
    getRow(presenter, 0, 0)->setSuccess();
    expectGroupState(SUCCESS);
    expectRowState(SUCCESS);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForErrorGroup() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getGroup(presenter, 0).setError("error message");
    getRow(presenter, 0, 0)->setSuccess();
    expectGroupState(FAILURE);
    expectRowState(SUCCESS);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testNotifyRowOutputsChangedForInputQRange() {
    auto presenter =
        makePresenter(m_view, oneGroupWithARowWithInputQRangeModel());
    EXPECT_CALL(m_jobs,
                setCellsAt(RowLocation({0, 0}), rowCellsWithValues(DEFAULT)))
        .Times(1);
    presenter.notifyRowOutputsChanged();
    verifyAndClearExpectations();
  }

  void testNotifyRowOutputsChangedForOutputQRange() {
    auto presenter =
        makePresenter(m_view, oneGroupWithARowWithOutputQRangeModel());
    auto cells = rowCellsWithValues(DEFAULT);
    cells[4].setOutput();
    cells[5].setOutput();
    cells[6].setOutput();
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), cells)).Times(1);
    presenter.notifyRowOutputsChanged();
    verifyAndClearExpectations();
  }

  void testMergeJobsUpdatesProgressBar() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectUpdateProgressBar();
    presenter.mergeAdditionalJobs(ReductionJobs());
    verifyAndClearExpectations();
  }

  void testRowStateChangedUpdatesProgressBar() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    expectUpdateProgressBar();
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

private:
  static constexpr const char *DEFAULT = "#ffffff"; // white
  static constexpr const char *INVALID = "#dddddd"; // very pale grey
  static constexpr const char *RUNNING = "#f0e442"; // pale yellow
  static constexpr const char *SUCCESS = "#d0f4d0"; // pale green
  static constexpr const char *WARNING = "#e69f00"; // pale orange
  static constexpr const char *FAILURE = "#accbff"; // pale blue

  std::vector<Cell> rowCells(const char *colour) {
    auto cells =
        std::vector<Cell>{Cell(""), Cell(""), Cell(""), Cell(""), Cell(""),
                          Cell(""), Cell(""), Cell(""), Cell("")};
    for (auto &cell : cells)
      cell.setBackgroundColor(colour);
    return cells;
  }

  std::vector<Cell> rowCellsWithValues(const char *colour) {
    auto cells =
        std::vector<Cell>{Cell("12345"),    Cell("0.500000"), Cell("Trans A"),
                          Cell("Trans B"),  Cell("0.500000"), Cell("0.900000"),
                          Cell("0.010000"), Cell(""),         Cell("")};
    for (auto &cell : cells)
      cell.setBackgroundColor(colour);
    return cells;
  }

  void expectGroupStateCleared() {
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0}), rowCells(DEFAULT)))
        .Times(1);
  }

  void expectRowStateCleared() {
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), rowCells(DEFAULT)))
        .Times(1);
  }

  void expectRowStateInvalid() {
    auto cells = rowCells(INVALID);
    for (auto &cell : cells)
      cell.setToolTip(
          "Row will not be processed: it either contains invalid cell values, "
          "or duplicates a reduction in another row");

    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), cells)).Times(1);
  }

  void expectGroupState(const char *colour) {
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0}), rowCells(colour)))
        .Times(1);
  }

  void expectRowState(const char *colour) {
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), rowCells(colour)))
        .Times(1);
  }

  void expectUpdateProgressBar() {
    auto progress = 33;
    EXPECT_CALL(m_mainPresenter, percentComplete())
        .Times(1)
        .WillOnce(Return(progress));
    EXPECT_CALL(m_view, setProgress(progress)).Times(1);
  }

  void expectTableEditingEnabled(bool enabled) {
    EXPECT_CALL(m_view, setJobsTableEnabled(enabled)).Times(1);
    EXPECT_CALL(m_view,
                setActionEnabled(IRunsTableView::Action::InsertRow, enabled))
        .Times(1);
    EXPECT_CALL(m_view,
                setActionEnabled(IRunsTableView::Action::InsertGroup, enabled))
        .Times(1);
    EXPECT_CALL(m_view,
                setActionEnabled(IRunsTableView::Action::DeleteRow, enabled))
        .Times(1);
    EXPECT_CALL(m_view,
                setActionEnabled(IRunsTableView::Action::DeleteGroup, enabled))
        .Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Copy, enabled))
        .Times(1);
    EXPECT_CALL(m_view,
                setActionEnabled(IRunsTableView::Action::Paste, enabled))
        .Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Cut, enabled))
        .Times(1);
  }

  void expectIsProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(false)).Times(1);
    EXPECT_CALL(m_view,
                setActionEnabled(IRunsTableView::Action::Process, false))
        .Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, true))
        .Times(1);
    expectTableEditingEnabled(false);
  }

  void expectIsNotProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Process, true))
        .Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, false))
        .Times(1);
    expectTableEditingEnabled(true);
  }

  void expectIsAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(false)).Times(1);
    EXPECT_CALL(m_view,
                setActionEnabled(IRunsTableView::Action::Process, false))
        .Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, false))
        .Times(1);
    expectTableEditingEnabled(false);
  }

  void expectIsNotAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Process, true))
        .Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, false))
        .Times(1);
    expectTableEditingEnabled(true);
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLRUNSTABLEPRESENTERPROCESSINGTEST_H_
