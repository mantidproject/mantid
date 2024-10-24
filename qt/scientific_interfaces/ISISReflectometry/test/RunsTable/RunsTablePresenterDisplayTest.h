// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "RunsTablePresenterTest.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::MantidWidgets::Batch::Cell;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class RunsTablePresenterDisplayTest : public CxxTest::TestSuite, RunsTablePresenterTest {
public:
  static RunsTablePresenterDisplayTest *createSuite() { return new RunsTablePresenterDisplayTest(); }

  static void destroySuite(RunsTablePresenterDisplayTest *suite) { delete suite; }

  void testExpandsAllGroupsWhenRequested() {
    EXPECT_CALL(m_jobs, expandAll()).Times(2);

    auto presenter = makePresenter(m_view);
    presenter.notifyExpandAllRequested();

    verifyAndClearExpectations();
  }

  void testCollapsesAllGroupsWhenRequested() {
    EXPECT_CALL(m_jobs, collapseAll());

    auto presenter = makePresenter(m_view);
    presenter.notifyCollapseAllRequested();

    verifyAndClearExpectations();
  }

  void testFilterChanged() {
    EXPECT_CALL(m_jobs, filterRowsBy(_)).Times(1);

    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.notifyFilterChanged("test filter");

    verifyAndClearExpectations();
  }

  void testFilterReset() {
    EXPECT_CALL(m_view, resetFilterBox()).Times(1);

    auto presenter = makePresenter(m_view, oneGroupWithARowModel());
    presenter.notifyFilterReset();

    verifyAndClearExpectations();
  }

  void testPlotSelected() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoRowsWithOutputNamesModel());

    // Set the second row as selected and complete
    selectedRowLocationsAre(m_jobs, {location(0, 1)});
    presenter.notifySelectionChanged();
    getRow(presenter, 0, 1)->setSuccess();

    auto const expected = std::vector<std::string>{"IvsQ_binned_2"};
    EXPECT_CALL(m_plotter, reflectometryPlot(expected)).Times(1);

    presenter.notifyPlotSelectedPressed();

    verifyAndClearExpectations();
  }

  void testPlotSelectedStitchedOutputs() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoRowsWithOutputNamesModel());

    // Set the group as selected and complete
    selectedRowLocationsAre(m_jobs, {location(0)});
    presenter.notifySelectionChanged();
    getGroup(presenter, 0).setSuccess();
    getGroup(presenter, 0).setOutputNames(std::vector<std::string>{"stitched_group"});

    auto const expected = std::vector<std::string>{"stitched_group"};
    EXPECT_CALL(m_plotter, reflectometryPlot(expected)).Times(1);

    presenter.notifyPlotSelectedStitchedOutputPressed();

    verifyAndClearExpectations();
  }

  void testNotifyFillDownRuns() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoSimpleRowsModel());

    auto const column = 0;
    selectedColumnIs(m_jobs, column);
    auto const src = location(0, 0);
    auto const dest = location(0, 1);
    selectedRowLocationsAre(m_jobs, {src, dest});

    auto const srcStr = std::string("12345");
    expectCellThenDefault(src, column, Cell(srcStr), Cell(srcStr));
    expectCellThenDefault(dest, column, Cell("12346"), Cell(srcStr));
    updatedCellsAre(src, cellsArray(srcStr, "0.5"));
    updatedCellsAre(dest, cellsArray(srcStr, "0.8"));

    presenter.notifyFillDown();

    TS_ASSERT_EQUALS(*getRow(presenter, src), makeSimpleRow(srcStr, 0.5));
    TS_ASSERT_EQUALS(*getRow(presenter, dest), makeSimpleRow(srcStr, 0.8));

    verifyAndClearExpectations();
  }

  void testNotifyFillDownTheta() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoSimpleRowsModel());

    auto const column = 1;
    selectedColumnIs(m_jobs, column);
    auto const src = location(0, 0);
    auto const dest = location(0, 1);
    selectedRowLocationsAre(m_jobs, {src, dest});

    auto const srcValue = 0.5;
    auto const srcStr = std::to_string(srcValue);
    expectCellThenDefault(src, column, Cell(srcStr), Cell(srcStr));
    expectCellThenDefault(dest, column, Cell("0.8"), Cell(srcStr));
    updatedCellsAre(src, cellsArray("12345", srcStr));
    updatedCellsAre(dest, cellsArray("12346", srcStr));

    presenter.notifyFillDown();

    TS_ASSERT_EQUALS(*getRow(presenter, src), makeSimpleRow("12345", srcValue));
    TS_ASSERT_EQUALS(*getRow(presenter, dest), makeSimpleRow("12346", srcValue));

    verifyAndClearExpectations();
  }

  void testNotifyFillDownFirstTransmissionRun() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoRowsWithSrcAndDestTransRuns());

    auto const column = 2;
    selectedColumnIs(m_jobs, column);
    auto const src = location(0, 0);
    auto const dest = location(0, 1);
    selectedRowLocationsAre(m_jobs, {src, dest});

    auto const srcStr = std::string("src trans A");
    expectCellThenDefault(src, column, Cell(srcStr), Cell(srcStr));
    expectCellThenDefault(dest, column, Cell("dest trans A"), Cell(srcStr));
    updatedCellsAre(src, cellsArray("12345", "0.5", srcStr, "src trans B"));
    updatedCellsAre(dest, cellsArray("12346", "0.8", srcStr, "dest trans B"));

    presenter.notifyFillDown();

    TS_ASSERT_EQUALS(*getRow(presenter, src), makeRow("12345", 0.5, srcStr, "src trans B"));
    TS_ASSERT_EQUALS(*getRow(presenter, dest), makeRow("12346", 0.8, srcStr, "dest trans B"));

    verifyAndClearExpectations();
  }

  void testNotifyFillDownSecondTransmissionRun() {
    auto presenter = makePresenter(m_view, oneGroupWithTwoRowsWithSrcAndDestTransRuns());

    auto const column = 3;
    selectedColumnIs(m_jobs, column);
    auto const src = location(0, 0);
    auto const dest = location(0, 1);
    selectedRowLocationsAre(m_jobs, {src, dest});

    auto const srcStr = std::string("src trans B");
    expectCellThenDefault(src, column, Cell(srcStr), Cell(srcStr));
    expectCellThenDefault(dest, column, Cell("dest trans A"), Cell(srcStr));
    updatedCellsAre(src, cellsArray("12345", "0.5", "src trans A", srcStr));
    updatedCellsAre(dest, cellsArray("12346", "0.8", "dest trans A", srcStr));

    presenter.notifyFillDown();

    TS_ASSERT_EQUALS(*getRow(presenter, src), makeRow("12345", 0.5, "src trans A", srcStr));
    TS_ASSERT_EQUALS(*getRow(presenter, dest), makeRow("12346", 0.8, "dest trans A", srcStr));

    verifyAndClearExpectations();
  }

  void testNotifyFillDownAcrossTwoGroupsWithMixedRows() {
    auto presenter = makePresenter(m_view, twoGroupsWithMixedRowsModel());

    auto const column = 1;
    selectedColumnIs(m_jobs, column);
    selectedRowLocationsAre(m_jobs, {location(0, 0), location(0, 1), location(0, 2), location(1, 1)});

    auto const srcValue = 0.5;
    auto const srcStr = std::to_string(srcValue);
    auto srcCell = Cell(srcStr);
    expectCellThenDefault(location(0, 0), column, Cell(srcStr), Cell(srcStr));
    expectCellThenDefault(location(0, 1), column, Cell(""), Cell(srcStr));
    expectCellThenDefault(location(0, 2), column, Cell("0.8"), Cell(srcStr));
    expectCellThenDefault(location(1, 1), column, Cell("0.9"), Cell(srcStr));

    updatedCellsAre(location(0, 0), cellsArray("12345", srcStr));
    updatedCellsAre(location(0, 1), cellsArray("", srcStr));
    updatedCellsAre(location(0, 2), cellsArray("12346", srcStr));
    updatedCellsAre(location(1, 1), cellsArray("22346", srcStr));

    presenter.notifyFillDown();

    // Check valid rows have been updated
    TS_ASSERT_EQUALS(getRow(presenter, 0, 0)->theta(), srcValue);
    TS_ASSERT_EQUALS(getRow(presenter, 0, 2)->theta(), srcValue);
    TS_ASSERT_EQUALS(getRow(presenter, 1, 1)->theta(), srcValue);
    // Check that the uninitialized row is still uninitialized
    TS_ASSERT_EQUALS(presenter.runsTable().reductionJobs()[0][1].is_initialized(), false);

    verifyAndClearExpectations();
  }

  void testNotifyRowModelChangedRounding() {
    auto presenter = makePresenter(m_view, oneGroupWithARowWithInputQRangeModelMixedPrecision());
    auto precision = 2;
    presenter.setTablePrecision(precision);
    auto rowLocation = location(0, 0);
    auto reductionOptions = ReductionOptionsMap();
    std::vector<MantidQt::MantidWidgets::Batch::Cell> roundedCells = std::vector<MantidQt::MantidWidgets::Batch::Cell>(
        {MantidQt::MantidWidgets::Batch::Cell("12345"), MantidQt::MantidWidgets::Batch::Cell("0.56"),
         MantidQt::MantidWidgets::Batch::Cell("Trans A"), MantidQt::MantidWidgets::Batch::Cell("Trans B"),
         MantidQt::MantidWidgets::Batch::Cell("0.56"), MantidQt::MantidWidgets::Batch::Cell("0.90"),
         MantidQt::MantidWidgets::Batch::Cell("0.01"), MantidQt::MantidWidgets::Batch::Cell(""),
         MantidQt::MantidWidgets::Batch::Cell(MantidQt::MantidWidgets::optionsToString(ReductionOptionsMap())),
         MantidQt::MantidWidgets::Batch::Cell("")});
    EXPECT_CALL(m_jobs, setCellsAt(rowLocation, roundedCells)).Times(1);
    presenter.notifyRowModelChanged();

    verifyAndClearExpectations();
  }

  void testNotifyBatchLoaded() {
    auto presenter = makePresenter(m_view, twoGroupsWithMixedRowsModel());
    EXPECT_CALL(m_view, jobs()).Times(1);
    presenter.notifyBatchLoaded();
    verifyAndClearExpectations();
  }

  void testNotifyBatchRowCellChanged() {
    auto constexpr groupIndex = 1;
    auto constexpr rowIndex = 1;
    auto constexpr cellIndex = 1;
    auto reductionJobs = twoGroupsWithMixedRowsModel();
    auto presenter = makePresenter(m_view, reductionJobs);
    auto const rowLocation = location(groupIndex, rowIndex);
    ON_CALL(m_jobs, cellAt(rowLocation, cellIndex)).WillByDefault(Return(Cell("")));
    // This extra call is needed to sort out some row states that get changed by calls to Update Row. That's not what
    // we're testing here, so just get the state in line before checking notify is called correctly.
    presenter.notifyCellTextChanged(rowLocation, cellIndex, "", "");
    EXPECT_CALL(m_mainPresenter, notifyRowContentChanged(presenter.mutableRunsTable()
                                                             .mutableReductionJobs()
                                                             .mutableGroups()[groupIndex]
                                                             .mutableRows()[rowIndex]
                                                             .get()))
        .Times(1);
    presenter.notifyCellTextChanged(rowLocation, cellIndex, "", "");
    verifyAndClearExpectations();
  }

  void testMainPresenterBatchThatGroupNameChanged() {
    auto reductionJobs = twoGroupsWithMixedRowsModel();
    auto presenter = makePresenter(m_view, reductionJobs);
    auto const groupLocation = location(0);
    ON_CALL(m_jobs, cellAt(groupLocation, 0)).WillByDefault(Return(Cell("")));
    EXPECT_CALL(m_mainPresenter, notifyGroupNameChanged(_)).Times(1);
    presenter.notifyCellTextChanged(groupLocation, 0, "old", "new");
    verifyAndClearExpectations();
  }

private:
  ReductionJobs oneGroupWithTwoRowsWithSrcAndDestTransRuns() {
    auto reductionJobs = ReductionJobs();
    auto group = Group("Test group 1");
    group.appendRow(makeRow("12345", 0.5, "src trans A", "src trans B"));
    group.appendRow(makeRow("12346", 0.8, "dest trans A", "dest trans B"));
    reductionJobs.appendGroup(std::move(group));
    return reductionJobs;
  }

  // This sets up calls to return the given "updated" cell values, which are
  // used after a fill-down operation has been completed
  void updatedCellsAre(MantidQt::MantidWidgets::Batch::RowLocation const &location, std::vector<Cell> const &cells) {
    ON_CALL(m_jobs, cellsAt(location)).WillByDefault(Return(cells));
  }

  // Expect the first cell value to be returned first for a particular location
  // and subsequent calls to return the second cell value by default
  void expectCellThenDefault(MantidQt::MantidWidgets::Batch::RowLocation const &location, int column,
                             Cell const &firstCell, Cell const &defaultCell) {
    EXPECT_CALL(m_jobs, cellAt(location, column))
        .Times(AtLeast(1))
        .WillOnce(Return(firstCell))
        .WillRepeatedly(Return(defaultCell));
  }
};
