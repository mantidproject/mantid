// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTablePresenter.h"
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "Reduction/Group.h"
#include "Reduction/RowLocation.h"
#include "Reduction/ValidateRow.h"
#include "RegexRowFilter.h"
#include <boost/range/algorithm/fill.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/regex.hpp>

#include <iostream>

namespace MantidQt {
namespace CustomInterfaces {

RunsTablePresenter::RunsTablePresenter(
    IRunsTableView *view, std::vector<std::string> const &instruments,
    double thetaTolerance, ReductionJobs jobs)
    : m_view(view), m_model(instruments, thetaTolerance, jobs),
      m_jobViewUpdater(m_view->jobs()) {
  m_view->subscribe(this);
}

void RunsTablePresenter::acceptMainPresenter(IRunsPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

RunsTable const &RunsTablePresenter::runsTable() const { return m_model; }

RunsTable &RunsTablePresenter::mutableRunsTable() { return m_model; }

void RunsTablePresenter::mergeAdditionalJobs(
    ReductionJobs const &additionalJobs) {
  std::cout << "Before Transfer:" << std::endl;
  prettyPrintModel(m_model.reductionJobs());

  std::cout << "Transfering:" << std::endl;
  prettyPrintModel(additionalJobs);

  mergeJobsInto(m_model.mutableReductionJobs(), additionalJobs,
                m_model.thetaTolerance(), m_jobViewUpdater);

  std::cout << "After Transfer:" << std::endl;
  prettyPrintModel(m_model.reductionJobs());
}

void RunsTablePresenter::removeRowsFromModel(
    std::vector<MantidWidgets::Batch::RowLocation> rows) {
  std::sort(rows.begin(), rows.end());
  for (auto row = rows.crbegin(); row != rows.crend(); ++row) {
    auto const groupIndex = groupOf(*row);
    auto const rowIndex = rowOf(*row);
    removeRow(m_model.mutableReductionJobs(), groupIndex, rowIndex);
  }
}

void RunsTablePresenter::notifyDeleteRowRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (!selected.empty()) {
    if (!containsGroups(selected)) {
      removeRowsAndGroupsFromView(selected);
      removeRowsFromModel(selected);
    } else {
      m_view->mustNotSelectGroup();
    }
  } else {
    m_view->mustSelectRow();
  }
}

void RunsTablePresenter::notifyDeleteGroupRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groupIndicesOrderedLowToHigh = groupIndexesFromSelection(selected);
    removeGroupsFromModel(groupIndicesOrderedLowToHigh);
    removeGroupsFromView(groupIndicesOrderedLowToHigh);
  } else {
    m_view->mustSelectGroupOrRow();
  }
}

void RunsTablePresenter::removeGroupsFromView(
    std::vector<int> const &groupIndicesOrderedLowToHigh) {
  for (auto it = groupIndicesOrderedLowToHigh.crbegin();
       it < groupIndicesOrderedLowToHigh.crend(); ++it)
    m_view->jobs().removeRowAt(MantidWidgets::Batch::RowLocation({*it}));
}

void RunsTablePresenter::removeGroupsFromModel(
    std::vector<int> const &groupIndicesOrderedLowToHigh) {
  for (auto it = groupIndicesOrderedLowToHigh.crbegin();
       it < groupIndicesOrderedLowToHigh.crend(); ++it)
    removeGroup(m_model.mutableReductionJobs(), *it);
}

void RunsTablePresenter::notifyReductionResumed() {
  m_mainPresenter->notifyReductionResumed();
}

void RunsTablePresenter::notifyReductionPaused() {
  m_mainPresenter->notifyReductionPaused();
}

void RunsTablePresenter::notifyInsertRowRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groups = groupIndexesFromSelection(selected);
    appendRowsToGroupsInModel(groups);
    appendRowsToGroupsInView(groups);
  } else {
    m_view->mustSelectGroupOrRow();
  }
}

void RunsTablePresenter::notifyFilterChanged(std::string const &filterString) {
  try {
    auto regexFilter = filterFromRegexString(filterString, m_view->jobs(),
                                             m_model.reductionJobs());
    m_view->jobs().filterRowsBy(std::move(regexFilter));
  } catch (boost::regex_error &) {
  }
}

void RunsTablePresenter::notifyInstrumentChanged() {
  auto const instrumentName = m_view->getInstrumentName();
  if (m_mainPresenter)
    m_mainPresenter->notifyInstrumentChanged(instrumentName);
}

void RunsTablePresenter::notifyFilterReset() { m_view->resetFilterBox(); }

void RunsTablePresenter::updateWidgetEnabledState() {
  auto const processing = isProcessing();
  auto const autoreducing = isAutoreducing();

  m_view->setJobsTableEnabled(!processing && !autoreducing);
  m_view->setInstrumentSelectorEnabled(!processing && !autoreducing);
  m_view->setProcessButtonEnabled(!processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Process,
                           !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Pause, processing);
  m_view->setActionEnabled(IRunsTableView::Action::InsertRow,
                           !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::InsertGroup,
                           !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::DeleteRow,
                           !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::DeleteGroup,
                           !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Copy,
                           !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Paste,
                           !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Cut,
                           !processing && !autoreducing);
}

void RunsTablePresenter::reductionResumed() { updateWidgetEnabledState(); }

void RunsTablePresenter::reductionPaused() { updateWidgetEnabledState(); }

void RunsTablePresenter::autoreductionResumed() { reductionResumed(); }

void RunsTablePresenter::autoreductionPaused() { reductionPaused(); }

void RunsTablePresenter::instrumentChanged(std::string const &instrumentName) {
  m_view->setInstrumentName(instrumentName);
}

void RunsTablePresenter::settingsChanged() {
  // TODO: reset state in reduction jobs
}

void RunsTablePresenter::appendRowsToGroupsInView(
    std::vector<int> const &groupIndices) {
  for (auto const &groupIndex : groupIndices)
    m_view->jobs().appendChildRowOf(
        MantidWidgets::Batch::RowLocation({groupIndex}));
}

void RunsTablePresenter::appendRowsToGroupsInModel(
    std::vector<int> const &groupIndices) {
  for (auto const &groupIndex : groupIndices)
    appendEmptyRow(m_model.mutableReductionJobs(), groupIndex);
}

void RunsTablePresenter::notifyInsertGroupRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto selectedGroupIndexes = groupIndexesFromSelection(selected);
    auto beforeGroup = selectedGroupIndexes.back() + 1;
    insertEmptyGroupInView(beforeGroup);
    insertEmptyGroupInModel(beforeGroup);
  } else {
    appendEmptyGroupInView();
    appendEmptyGroupInModel();
  }
}

void RunsTablePresenter::appendEmptyGroupInModel() {
  appendEmptyGroup(m_model.mutableReductionJobs());
}

void RunsTablePresenter::appendEmptyGroupInView() {
  auto location =
      m_view->jobs().appendChildRowOf(MantidWidgets::Batch::RowLocation());
  applyGroupStylingToRow(location);
  // TODO: Consider using the other version of appendChildRowOf
}

void RunsTablePresenter::insertEmptyGroupInModel(int beforeGroup) {
  insertEmptyGroup(m_model.mutableReductionJobs(), beforeGroup);
}

void RunsTablePresenter::insertEmptyRowInModel(int groupIndex, int beforeRow) {
  insertEmptyRow(m_model.mutableReductionJobs(), groupIndex, beforeRow);
}

void RunsTablePresenter::insertEmptyGroupInView(int beforeGroup) {
  auto location = m_view->jobs().insertChildRowOf(
      MantidWidgets::Batch::RowLocation(), beforeGroup);
  applyGroupStylingToRow(location);
}

void RunsTablePresenter::notifyExpandAllRequested() {
  m_view->jobs().expandAll();
}

void RunsTablePresenter::notifyCollapseAllRequested() {
  m_view->jobs().collapseAll();
}

std::vector<std::string> RunsTablePresenter::cellTextFromViewAt(
    MantidWidgets::Batch::RowLocation const &location) const {
  return map(m_view->jobs().cellsAt(location),
             [](MantidWidgets::Batch::Cell const &cell) -> std::string {
               return cell.contentText();
             });
}

void RunsTablePresenter::clearInvalidCellStyling(
    std::vector<MantidWidgets::Batch::Cell> &cells) {
  for (auto &cell : cells)
    clearInvalidCellStyling(cell);
}

void RunsTablePresenter::clearInvalidCellStyling(
    MantidWidgets::Batch::Cell &cell) {
  cell.setIconFilePath("");
  cell.setBorderColor("darkGrey");
}

void RunsTablePresenter::showAllCellsOnRowAsValid(
    MantidWidgets::Batch::RowLocation const &itemIndex) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  clearInvalidCellStyling(cells);
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::applyInvalidCellStyling(
    MantidWidgets::Batch::Cell &cell) {
  cell.setIconFilePath(":/invalid.png");
  cell.setBorderColor("darkRed");
}

void RunsTablePresenter::showCellsAsInvalidInView(
    MantidWidgets::Batch::RowLocation const &itemIndex,
    std::vector<int> const &invalidColumns) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  clearInvalidCellStyling(cells);
  for (auto &column : invalidColumns)
    applyInvalidCellStyling(cells[column]);
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::updateGroupName(
    MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {
  assertOrThrow(column == 0,
                "Changed value of cell which should be uneditable");
  auto const groupIndex = groupOf(itemIndex);
  if (!setGroupName(m_model.mutableReductionJobs(), groupIndex, newValue)) {
    auto cell = m_view->jobs().cellAt(itemIndex, column);
    cell.setContentText(oldValue);
    m_view->jobs().setCellAt(itemIndex, column, cell);
  }
}

void RunsTablePresenter::updateRowField(
    MantidWidgets::Batch::RowLocation const &itemIndex, int,
    std::string const &, std::string const &) {
  auto const groupIndex = groupOf(itemIndex);
  auto const rowIndex = rowOf(itemIndex);
  auto rowValidationResult =
      validateRow(m_model.reductionJobs(), cellTextFromViewAt(itemIndex));
  updateRow(m_model.mutableReductionJobs(), groupIndex, rowIndex,
            rowValidationResult.validElseNone());
  if (rowValidationResult.isValid()) {
    showAllCellsOnRowAsValid(itemIndex);
  } else {
    showCellsAsInvalidInView(itemIndex, rowValidationResult.assertError());
  }
  notifyRowStateChanged();
}

void RunsTablePresenter::notifyCellTextChanged(
    MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {
  if (isGroupLocation(itemIndex))
    updateGroupName(itemIndex, column, oldValue, newValue);
  else
    updateRowField(itemIndex, column, oldValue, newValue);
}

void RunsTablePresenter::notifySelectionChanged() {
  m_model.setSelectedRowLocations(m_view->jobs().selectedRowLocations());
}

void RunsTablePresenter::applyGroupStylingToRow(
    MantidWidgets::Batch::RowLocation const &location) {
  auto cells = m_view->jobs().cellsAt(location);
  boost::fill(boost::make_iterator_range(cells.begin() + 1, cells.end()),
              m_view->jobs().deadCell());
  m_view->jobs().setCellsAt(location, cells);
}

void RunsTablePresenter::notifyRowInserted(
    MantidWidgets::Batch::RowLocation const &newRowLocation) {
  if (newRowLocation.depth() > DEPTH_LIMIT) {
    m_view->jobs().removeRowAt(newRowLocation);
  } else if (isGroupLocation(newRowLocation)) {
    insertEmptyGroupInModel(groupOf(newRowLocation));
    applyGroupStylingToRow(newRowLocation);
  } else if (isRowLocation(newRowLocation)) {
    insertEmptyRowInModel(groupOf(newRowLocation), rowOf(newRowLocation));
  }
}

void RunsTablePresenter::removeRowsAndGroupsFromModel(
    std::vector<MantidWidgets::Batch::RowLocation> locationsOfRowsToRemove) {
  std::sort(locationsOfRowsToRemove.begin(), locationsOfRowsToRemove.end());
  for (auto location = locationsOfRowsToRemove.crbegin();
       location != locationsOfRowsToRemove.crend(); ++location) {
    auto const groupIndex = groupOf(*location);
    if (isRowLocation(*location)) {
      auto const rowIndex = rowOf(*location);
      removeRow(m_model.mutableReductionJobs(), groupIndex, rowIndex);
    } else if (isGroupLocation(*location)) {
      removeGroup(m_model.mutableReductionJobs(), groupIndex);
    }
  }
}

void RunsTablePresenter::removeAllRowsAndGroupsFromModel() {
  removeAllRowsAndGroups(m_model.mutableReductionJobs());
}

void RunsTablePresenter::removeRowsAndGroupsFromView(
    std::vector<MantidWidgets::Batch::RowLocation> const
        &locationsOfRowsToRemove) {
  m_view->jobs().removeRows(locationsOfRowsToRemove);
}

void RunsTablePresenter::removeAllRowsAndGroupsFromView() {
  m_view->jobs().removeAllRows();
}

void RunsTablePresenter::notifyRemoveRowsRequested(
    std::vector<MantidWidgets::Batch::RowLocation> const
        &locationsOfRowsToRemove) {
  removeRowsAndGroupsFromModel(locationsOfRowsToRemove);
  removeRowsAndGroupsFromView(locationsOfRowsToRemove);
}

void RunsTablePresenter::notifyRemoveAllRowsAndGroupsRequested() {
  removeAllRowsAndGroupsFromModel();
  removeAllRowsAndGroupsFromView();
}

void RunsTablePresenter::notifyCopyRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized())
    m_view->jobs().clearSelection();
  else
    m_view->invalidSelectionForCopy();
}

void RunsTablePresenter::notifyCutRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized()) {
    m_view->jobs().removeRows(m_view->jobs().selectedRowLocations());
    m_view->jobs().clearSelection();
  } else {
    m_view->invalidSelectionForCut();
  }
}

void RunsTablePresenter::notifyPasteRowsRequested() {
  auto maybeReplacementRoots = m_view->jobs().selectedSubtreeRoots();
  if (maybeReplacementRoots.is_initialized() && m_clipboard.is_initialized()) {
    auto &replacementRoots = maybeReplacementRoots.get();
    if (!replacementRoots.empty())
      m_view->jobs().replaceRows(replacementRoots, m_clipboard.get());
    else
      m_view->jobs().appendSubtreesAt(MantidWidgets::Batch::RowLocation(),
                                      m_clipboard.get());
  } else {
    m_view->invalidSelectionForPaste();
  }
}

void RunsTablePresenter::clearStateCellStyling(
    MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor("white");
  cell.setToolTip("");
}

void RunsTablePresenter::applyRunningStateCellStyling(
    MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor("yellow");
}

void RunsTablePresenter::applyCompletedStateCellStyling(
    MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor("green");
}

void RunsTablePresenter::applyErrorStateCellStyling(
    MantidWidgets::Batch::Cell &cell, std::string const &errorMessage) {
  cell.setBackgroundColor("red");
  cell.setToolTip(errorMessage);
}

void RunsTablePresenter::applyWarningStateCellStyling(
    MantidWidgets::Batch::Cell &cell, std::string const &errorMessage) {
  cell.setBackgroundColor("orange");
  cell.setToolTip(errorMessage);
}

void RunsTablePresenter::clearStateCellStyling(
    MantidWidgets::Batch::RowLocation const &itemIndex) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  for (size_t column = 0; column < cells.size(); ++column) {
    clearStateCellStyling(cells[column]);
  }
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::showCellsAsRunningStateInView(
    MantidWidgets::Batch::RowLocation const &itemIndex) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  for (size_t column = 0; column < cells.size(); ++column) {
    clearStateCellStyling(cells[column]);
    applyRunningStateCellStyling(cells[column]);
  }
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::showCellsAsCompletedStateInView(
    MantidWidgets::Batch::RowLocation const &itemIndex) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  for (size_t column = 0; column < cells.size(); ++column) {
    clearStateCellStyling(cells[column]);
    applyCompletedStateCellStyling(cells[column]);
  }
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::showCellsAsErrorStateInView(
    MantidWidgets::Batch::RowLocation const &itemIndex,
    std::string const &errorMessage) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  for (size_t column = 0; column < cells.size(); ++column) {
    clearStateCellStyling(cells[column]);
    applyErrorStateCellStyling(cells[column], errorMessage);
  }
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::showCellsAsWarningStateInView(
    MantidWidgets::Batch::RowLocation const &itemIndex,
    std::string const &errorMessage) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  for (size_t column = 0; column < cells.size(); ++column) {
    clearStateCellStyling(cells[column]);
    applyWarningStateCellStyling(cells[column], errorMessage);
  }
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::notifyRowStateChanged() {
  int groupIndex = 0;
  for (auto &group : m_model.reductionJobs().groups()) {
    auto groupPath = MantidWidgets::Batch::RowPath{groupIndex};
    clearStateCellStyling(groupPath);

    int rowIndex = 0;
    for (auto &row : group.rows()) {
      if (!row)
        continue;

      auto rowPath = MantidWidgets::Batch::RowPath{groupIndex, rowIndex};

      switch (row->state()) {
      case State::NOT_STARTED:
      case State::STARTING: // fall through
        clearStateCellStyling(rowPath);
        break;
      case State::RUNNING:
        showCellsAsRunningStateInView(rowPath);
        break;
      case State::SUCCESS:
        showCellsAsCompletedStateInView(rowPath);
        break;
      case State::ERROR:
        showCellsAsErrorStateInView(rowPath, row->message());
        break;
      case State::WARNING:
        showCellsAsWarningStateInView(rowPath, row->message());
        break;
      };

      ++rowIndex;
    }
    ++groupIndex;
  }
  if (m_model.reductionJobs().groups().size() > 1) {
    auto itemIndex = MantidWidgets::Batch::RowLocation({1});
    showCellsAsErrorStateInView(itemIndex, "this is a test error message");
  }
}

bool RunsTablePresenter::isProcessing() const {
  return m_mainPresenter->isProcessing();
}

bool RunsTablePresenter::isAutoreducing() const {
  return m_mainPresenter->isAutoreducing();
}
} // namespace CustomInterfaces
} // namespace MantidQt
