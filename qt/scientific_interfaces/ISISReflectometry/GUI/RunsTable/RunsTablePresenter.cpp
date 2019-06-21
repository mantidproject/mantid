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

namespace MantidQt {
namespace CustomInterfaces {
namespace Colour {
constexpr const char *DEFAULT = "#ffffff"; // white
constexpr const char *INVALID = "#dddddd"; // very pale grey
constexpr const char *RUNNING = "#f0e442"; // pale yellow
constexpr const char *SUCCESS = "#d0f4d0"; // pale green
constexpr const char *WARNING = "#e69f00"; // pale orange
constexpr const char *FAILURE = "#accbff"; // pale blue
} // namespace Colour

namespace { // unnamed
void clearStateStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor(Colour::DEFAULT);
  cell.setToolTip("");
}

void applyInvalidStateStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor(Colour::INVALID);
  cell.setToolTip(
      "Row will not be processed: it either contains invalid cell values, "
      "or duplicates a reduction in another row");
}

void applyRunningStateStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor(Colour::RUNNING);
}

void applyCompletedStateStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor(Colour::SUCCESS);
}

void applyErrorStateStyling(MantidWidgets::Batch::Cell &cell,
                            std::string const &errorMessage) {
  cell.setBackgroundColor(Colour::FAILURE);
  cell.setToolTip(errorMessage);
}

void applyWarningStateStyling(MantidWidgets::Batch::Cell &cell,
                              std::string const &errorMessage) {
  cell.setBackgroundColor(Colour::WARNING);
  cell.setToolTip(errorMessage);
}
} // namespace

RunsTablePresenter::RunsTablePresenter(
    IRunsTableView *view, std::vector<std::string> const &instruments,
    double thetaTolerance, ReductionJobs jobs, const IPlotter &plotter)
    : m_view(view), m_model(instruments, thetaTolerance, std::move(jobs)),
      m_jobViewUpdater(m_view->jobs()), m_plotter(plotter) {
  m_view->subscribe(this);
}

void RunsTablePresenter::acceptMainPresenter(IRunsPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

RunsTable const &RunsTablePresenter::runsTable() const { return m_model; }

RunsTable &RunsTablePresenter::mutableRunsTable() { return m_model; }

void RunsTablePresenter::mergeAdditionalJobs(
    ReductionJobs const &additionalJobs) {
  mergeJobsInto(m_model.mutableReductionJobs(), additionalJobs,
                m_model.thetaTolerance(), m_jobViewUpdater);
  updateProgressBar();
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
  if (isProcessing() || isAutoreducing())
    return;

  auto selected = m_view->jobs().selectedRowLocations();
  if (!selected.empty()) {
    if (!containsGroups(selected)) {
      removeRowsAndGroupsFromView(selected);
      removeRowsFromModel(selected);
      ensureAtLeastOneGroupExists();
      notifyRowStateChanged();
      notifySelectionChanged();
    } else {
      m_view->mustNotSelectGroup();
    }
  } else {
    m_view->mustSelectRow();
  }
}

void RunsTablePresenter::notifyDeleteGroupRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groupIndicesOrderedLowToHigh = groupIndexesFromSelection(selected);
    removeGroupsFromModel(groupIndicesOrderedLowToHigh);
    removeGroupsFromView(groupIndicesOrderedLowToHigh);
    ensureAtLeastOneGroupExists();
    notifyRowStateChanged();
    notifySelectionChanged();
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
  if (isProcessing() || isAutoreducing())
    return;

  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groups = groupIndexesFromSelection(selected);
    appendRowsToGroupsInModel(groups);
    appendRowsToGroupsInView(groups);
  } else {
    m_view->mustSelectGroupOrRow();
  }
  notifyRowStateChanged();
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
  m_view->setActionEnabled(IRunsTableView::Action::Pause,
                           processing && !autoreducing);
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
  m_model.resetState();
  notifyRowStateChanged();
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
  if (isProcessing() || isAutoreducing())
    return;

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
  notifyRowStateChanged();
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

void RunsTablePresenter::ensureAtLeastOneGroupExists() {
  // The JobTreeView leaves an empty item in the table if all rows are
  // deleted. However, it will be a row rather than a group. I'm not sure
  // how to get around this so the workaround here is to check if there
  // is only one item in the table and it's a row, then we replace it with
  // a group.

  // If we have more than one group/row then it's ok
  if (m_model.reductionJobs().groups().size() > 1)
    return;
  auto const &group = m_model.reductionJobs().groups()[0];
  if (group.rows().size() > 0)
    return;

  auto location =
      MantidWidgets::Batch::RowLocation(MantidWidgets::Batch::RowPath{0});
  auto cells = m_view->jobs().cellsAt(location);

  // Groups should only have one cell
  if (cells.size() == 1)
    return;

  // The model is fine, we just need to update the view. Add a new, proper,
  // group first, then delete the original one
  appendEmptyGroupInView();
  removeRowsAndGroupsFromView({location});
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
               if (cell.isOutput())
                 return std::string();
               else
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
    MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &, std::string const &) {
  // User has edited the text so reset the output-value flag as it now contains
  // an input
  auto cell = m_view->jobs().cellAt(itemIndex, column);
  cell.setInput();
  m_view->jobs().setCellAt(itemIndex, column, cell);

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
}

void RunsTablePresenter::notifyCellTextChanged(
    MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {
  if (isGroupLocation(itemIndex))
    updateGroupName(itemIndex, column, oldValue, newValue);
  else
    updateRowField(itemIndex, column, oldValue, newValue);
  notifyRowStateChanged();
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
  notifyRowStateChanged();
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

void RunsTablePresenter::notifyAppendAndEditAtChildRowRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  m_view->jobs().appendAndEditAtChildRow();
}

void RunsTablePresenter::notifyAppendAndEditAtRowBelowRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  m_view->jobs().appendAndEditAtRowBelow();
}

void RunsTablePresenter::notifyEditAtRowAboveRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  m_view->jobs().editAtRowAbove();
}

void RunsTablePresenter::notifyRemoveRowsRequested(
    std::vector<MantidWidgets::Batch::RowLocation> const
        &locationsOfRowsToRemove) {
  if (isProcessing() || isAutoreducing())
    return;

  removeRowsAndGroupsFromModel(locationsOfRowsToRemove);
  removeRowsAndGroupsFromView(locationsOfRowsToRemove);
  ensureAtLeastOneGroupExists();
  notifySelectionChanged();
}

void RunsTablePresenter::notifyRemoveAllRowsAndGroupsRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  removeAllRowsAndGroupsFromModel();
  removeAllRowsAndGroupsFromView();
  ensureAtLeastOneGroupExists();
  notifySelectionChanged();
}

void RunsTablePresenter::notifyCopyRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized())
    m_view->jobs().clearSelection();
  else
    m_view->invalidSelectionForCopy();
}

void RunsTablePresenter::notifyCutRowsRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  m_clipboard = m_view->jobs().selectedSubtrees();
  auto selected = m_view->jobs().selectedRowLocations();
  if (m_clipboard.is_initialized()) {
    removeRowsAndGroupsFromView(selected);
    removeRowsFromModel(selected);
    m_view->jobs().clearSelection();
    ensureAtLeastOneGroupExists();
    notifySelectionChanged();
  } else {
    m_view->invalidSelectionForCut();
  }
}

void RunsTablePresenter::notifyPasteRowsRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  auto maybeReplacementRoots = m_view->jobs().selectedSubtreeRoots();
  if (maybeReplacementRoots.is_initialized() && m_clipboard.is_initialized()) {
    auto &replacementRoots = maybeReplacementRoots.get();
    if (!replacementRoots.empty())
      pasteRowsAtRoots(replacementRoots);
    else
      pasteRowsAtEnd();
    notifyRowStateChanged();
    notifySelectionChanged();
  }
}

void RunsTablePresenter::pasteRowsAtRoots(
    std::vector<MantidWidgets::Batch::RowLocation> &replacementRoots) {
  // Paste onto given locations. Only possible if the depth is the same.
  if (containsGroups(replacementRoots) == containsGroups(m_clipboard.get()))
    m_view->jobs().replaceRows(replacementRoots, m_clipboard.get());
  else
    m_view->invalidSelectionForPaste();
}

void RunsTablePresenter::pasteRowsAtEnd() {
  // Paste rows into a group location at the end of the table. Only possible if
  // the clipboard contains groups
  if (containsGroups(m_clipboard.get()))
    m_view->jobs().appendSubtreesAt(MantidWidgets::Batch::RowLocation(),
                                    m_clipboard.get());
  else
    m_view->invalidSelectionForPaste();
}

void RunsTablePresenter::forAllCellsAt(
    MantidWidgets::Batch::RowLocation const &location,
    UpdateCellFunc updateFunc) {
  auto cells = m_view->jobs().cellsAt(location);
  std::for_each(cells.begin(), cells.end(), updateFunc);
  m_view->jobs().setCellsAt(location, cells);
}

void RunsTablePresenter::forAllCellsAt(
    MantidWidgets::Batch::RowLocation const &location,
    UpdateCellWithTooltipFunc updateFunc, std::string const &tooltip) {
  auto cells = m_view->jobs().cellsAt(location);
  std::for_each(
      cells.begin(), cells.end(),
      [&](MantidWidgets::Batch::Cell &cell) { updateFunc(cell, tooltip); });
  m_view->jobs().setCellsAt(location, cells);
}

void RunsTablePresenter::setRowStylingForItem(
    MantidWidgets::Batch::RowPath const &rowPath, Item const &item) {

  switch (item.state()) {
  case State::ITEM_NOT_STARTED: // fall through
  case State::ITEM_STARTING:
    forAllCellsAt(rowPath, clearStateStyling);
    break;
  case State::ITEM_RUNNING:
    forAllCellsAt(rowPath, applyRunningStateStyling);
    break;
  case State::ITEM_COMPLETE:
    forAllCellsAt(rowPath, applyCompletedStateStyling);
    break;
  case State::ITEM_ERROR:
    forAllCellsAt(rowPath, applyErrorStateStyling, item.message());
    break;
  case State::ITEM_WARNING:
    forAllCellsAt(rowPath, applyWarningStateStyling, item.message());
    break;
  };
}

void RunsTablePresenter::updateProgressBar() {
  m_view->setProgress(m_mainPresenter->percentComplete());
}

void RunsTablePresenter::notifyRowStateChanged() {
  updateProgressBar();

  int groupIndex = 0;
  for (auto &group : m_model.reductionJobs().groups()) {
    auto groupPath = MantidWidgets::Batch::RowPath{groupIndex};
    setRowStylingForItem(groupPath, group);

    int rowIndex = 0;
    for (auto &row : group.rows()) {
      auto rowPath = MantidWidgets::Batch::RowPath{groupIndex, rowIndex};

      if (!row)
        forAllCellsAt(rowPath, applyInvalidStateStyling);
      else {
        setRowStylingForItem(rowPath, *row);
      }

      ++rowIndex;
    }
    ++groupIndex;
  }
}

void RunsTablePresenter::notifyRowOutputsChanged() {
  int groupIndex = 0;
  for (auto &group : m_model.reductionJobs().groups()) {
    auto groupPath = MantidWidgets::Batch::RowPath{groupIndex};
    int rowIndex = 0;
    for (auto &row : group.rows()) {
      auto rowPath = MantidWidgets::Batch::RowPath{groupIndex, rowIndex};
      m_jobViewUpdater.rowModified(groupOf(rowPath), rowOf(rowPath), *row);
      ++rowIndex;
    }
    ++groupIndex;
  }
}

bool RunsTablePresenter::isProcessing() const {
  return m_mainPresenter->isProcessing();
}

bool RunsTablePresenter::isAutoreducing() const {
  return m_mainPresenter->isAutoreducing();
}

void RunsTablePresenter::notifyPlotSelectedPressed() {
  std::vector<std::string> workspaces;
  const auto rows = m_model.selectedRows();

  for (const auto &row : rows) {
    if (row.state() == State::ITEM_COMPLETE)
      workspaces.emplace_back(row.reducedWorkspaceNames().iVsQBinned());
  }

  if (workspaces.empty())
    return;

  m_plotter.reflectometryPlot(workspaces);
}

void RunsTablePresenter::notifyPlotSelectedStitchedOutputPressed() {
  std::vector<std::string> workspaces;
  const auto groups = m_model.selectedGroups();

  for (const auto &group : groups) {
    if (group.state() == State::ITEM_COMPLETE)
      workspaces.emplace_back(group.postprocessedWorkspaceName());
  }

  if (workspaces.empty())
    return;

  m_plotter.reflectometryPlot(workspaces);
}
} // namespace CustomInterfaces
} // namespace MantidQt
