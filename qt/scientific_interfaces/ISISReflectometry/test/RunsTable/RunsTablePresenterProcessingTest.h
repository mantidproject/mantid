// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using MantidQt::MantidWidgets::Batch::Cell;
using MantidQt::MantidWidgets::Batch::RowLocation;
using MantidQt::MantidWidgets::Batch::RowPath;
using testing::AllOf;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

namespace {
MATCHER_P(AreAllColour, colour, "Colour checker for vectors of Cells") {
  auto const col = colour;
  return std::all_of(arg.cbegin(), arg.cend(),
                     [col](auto const &cell) { return strcmp(cell.backgroundColor().c_str(), col) == 0; });
}
} // namespace

class RunsTablePresenterProcessingTest : public CxxTest::TestSuite, RunsTablePresenterTest {
public:
  static RunsTablePresenterProcessingTest *createSuite() { return new RunsTablePresenterProcessingTest(); }

  static void destroySuite(RunsTablePresenterProcessingTest *suite) { delete suite; }

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
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    presenter.notifyAnyBatchReductionResumed();
    verifyAndClearExpectations();
  }

  void testChangingInstrumentIsEnabledWhenNoBatchesReducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing()).Times(1).WillOnce(Return(false));
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
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    presenter.notifyAnyBatchAutoreductionResumed();
    verifyAndClearExpectations();
  }

  void testChangingInstrumentIsEnabledWhenNoBatchesAutoreducing() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(true)).Times(1);
    presenter.notifyAnyBatchAutoreductionPaused();
    verifyAndClearExpectations();
  }

  void testNotifyChangeInstrumentRequested() {
    auto presenter = makePresenter(m_view, ReductionJobs());
    auto const instrument = std::string("test_instrument");
    EXPECT_CALL(m_view, getInstrumentName()).Times(1).WillOnce(Return(instrument));
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(instrument)).Times(1);
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
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), rowCells(Colour::DEFAULT))).Times(1);
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0}), rowCells(Colour::DEFAULT))).Times(1);
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), rowCellsWithSomeValues())).Times(1);
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
    expectRowState(Colour::RUNNING);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForCompleteRow() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setSuccess();
    expectGroupState(Colour::CHILDREN_SUCCESS);
    expectRowState(Colour::SUCCESS);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowParentStateChangedForAllRowsInGroupComplete() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setSuccess();
    expectRowState(Colour::SUCCESS);
    expectGroupState(Colour::CHILDREN_SUCCESS);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForErrorRow() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setError("error message");
    expectGroupStateCleared();
    expectRowState(Colour::FAILURE);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForCompleteGroup() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setSuccess();
    getGroup(presenter, 0).setSuccess();
    expectGroupState(Colour::SUCCESS);
    expectRowState(Colour::SUCCESS);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testRowStateChangedForErrorGroup() {
    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    getRow(presenter, 0, 0)->setSuccess();
    getGroup(presenter, 0).setError("error message");
    expectGroupState(Colour::FAILURE);
    expectRowState(Colour::SUCCESS);
    presenter.notifyRowStateChanged();
    verifyAndClearExpectations();
  }

  void testNotifyRowModelChangedForInputQRange() {
    auto presenter = makePresenter(m_view, oneGroupWithARowWithInputQRangeModel());
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), rowCellsWithValues(Colour::DEFAULT))).Times(1);
    presenter.notifyRowModelChanged();
    verifyAndClearExpectations();
  }

  void testNotifyRowModelChangedForOutputQRange() {
    auto presenter = makePresenter(m_view, oneGroupWithARowWithOutputQRangeModel());
    auto cells = rowCellsWithValues(Colour::DEFAULT);
    cells[4].setOutput();
    cells[5].setOutput();
    cells[6].setOutput();
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), cells)).Times(1);
    presenter.notifyRowModelChanged();
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
  std::vector<Cell> rowCells(const char *colour) {
    auto cells = std::vector<Cell>{Cell(""), Cell(""), Cell(""), Cell(""), Cell(""),
                                   Cell(""), Cell(""), Cell(""), Cell(""), Cell("")};
    for (auto &cell : cells)
      cell.setBackgroundColor(colour);
    return cells;
  }

  std::vector<Cell> rowCellsWithValues(const char *colour) {
    auto cells =
        std::vector<Cell>{Cell("12345"),    Cell("0.500000"), Cell("Trans A"), Cell("Trans B"), Cell("0.500000"),
                          Cell("0.900000"), Cell("0.010000"), Cell(""),        Cell(""),        Cell("")};
    for (auto &cell : cells)
      cell.setBackgroundColor(colour);
    return cells;
  }

  std::vector<Cell> rowCellsWithSomeValues() {
    return std::vector<Cell>{Cell("12345"), Cell("0.500000"), Cell("Trans A"), Cell("Trans B"), Cell(""),
                             Cell(""),      Cell(""),         Cell(""),        Cell(""),        Cell("")};
  }

  void expectGroupStateCleared() {
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0}), AllOf(rowCells(Colour::DEFAULT), AreAllColour(Colour::DEFAULT))))
        .Times(AtLeast(1));
  }

  void expectRowStateCleared() {
    EXPECT_CALL(m_jobs,
                setCellsAt(RowLocation({0, 0}), AllOf(rowCells(Colour::DEFAULT), AreAllColour(Colour::DEFAULT))))
        .Times(AtLeast(1));
  }

  void expectRowStateInvalid() {
    auto cells = rowCells(Colour::INVALID);
    for (auto &cell : cells)
      cell.setToolTip("Row will not be processed: it either contains invalid cell values, "
                      "or duplicates a reduction in another row");

    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), AllOf(cells, AreAllColour(Colour::INVALID)))).Times(1);
  }

  void expectGroupState(const char *colour) {
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0}), AllOf(rowCells(colour), AreAllColour(colour)))).Times(1);
  }

  void expectRowState(const char *colour) {
    EXPECT_CALL(m_jobs, setCellsAt(RowLocation({0, 0}), AllOf(rowCells(colour), AreAllColour(colour)))).Times(1);
  }

  void expectUpdateProgressBar() {
    auto progress = 33;
    EXPECT_CALL(m_mainPresenter, percentComplete()).Times(1).WillOnce(Return(progress));
    EXPECT_CALL(m_view, setProgress(progress)).Times(1);
  }

  void expectTableEditingEnabled(bool enabled) {
    EXPECT_CALL(m_view, setJobsTableEnabled(enabled)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::InsertRow, enabled)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::InsertGroup, enabled)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::DeleteRow, enabled)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::DeleteGroup, enabled)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Copy, enabled)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Paste, enabled)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Cut, enabled)).Times(1);
  }

  void expectIsProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Process, false)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, true)).Times(1);
    expectTableEditingEnabled(false);
  }

  void expectIsNotProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing()).Times(AtLeast(1)).WillRepeatedly(Return(false));
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing()).Times(AtLeast(1)).WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Process, true)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, false)).Times(1);
    expectTableEditingEnabled(true);
  }

  void expectIsAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(false)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Process, false)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, false)).Times(1);
    expectTableEditingEnabled(false);
  }

  void expectIsNotAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(false));
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(false));
    EXPECT_CALL(m_view, setInstrumentSelectorEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setProcessButtonEnabled(true)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Process, true)).Times(1);
    EXPECT_CALL(m_view, setActionEnabled(IRunsTableView::Action::Pause, false)).Times(1);
    expectTableEditingEnabled(true);
  }
};
