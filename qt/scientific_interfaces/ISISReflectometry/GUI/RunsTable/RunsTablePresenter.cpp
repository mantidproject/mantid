#include "RunsTablePresenter.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "Map.h"
#include "Reduction/Group.h"
#include "Reduction/Slicing.h"
#include "Reduction/ValidateRow.h"
#include "RegexRowFilter.h"
#include "RowLocation.h"
#include <boost/range/algorithm/fill.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/regex.hpp>

#include <iostream>

namespace MantidQt {
namespace CustomInterfaces {

RunsTablePresenter::RunsTablePresenter(IRunsTableView *view,
                               std::vector<std::string> const &instruments,
                               double thetaTolerance,
                               WorkspaceNamesFactory workspaceNamesFactory,
                               Jobs jobs)
    : m_view(view), m_instruments(instruments), m_model(std::move(jobs)),
      m_thetaTolerance(thetaTolerance), m_jobViewUpdater(m_view->jobs()),
      m_workspaceNameFactory(std::move(workspaceNamesFactory)) {
  m_view->subscribe(this);
}

Jobs const &RunsTablePresenter::reductionJobs() const { return m_model; }

void RunsTablePresenter::mergeAdditionalJobs(Jobs const &additionalJobs) {
  std::cout << "Before Transfer:" << std::endl;
  prettyPrintModel(m_model);

  std::cout << "Transfering:" << std::endl;
  prettyPrintModel(additionalJobs);

  mergeJobsInto(m_model, additionalJobs, m_thetaTolerance,
                m_workspaceNameFactory, m_jobViewUpdater);

  std::cout << "After Transfer:" << std::endl;
  prettyPrintModel(m_model);
}

void RunsTablePresenter::removeRowsFromModel(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> rows) {
  std::sort(rows.begin(), rows.end());
  for (auto row = rows.crbegin(); row != rows.crend(); ++row) {
    auto const groupIndex = groupOf(*row);
    auto const rowIndex = rowOf(*row);
    removeRow(m_model, groupIndex, rowIndex);
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
    m_view->jobs().removeRowAt(
        MantidQt::MantidWidgets::Batch::RowLocation({*it}));
}

void RunsTablePresenter::removeGroupsFromModel(
    std::vector<int> const &groupIndicesOrderedLowToHigh) {
  for (auto it = groupIndicesOrderedLowToHigh.crbegin();
       it < groupIndicesOrderedLowToHigh.crend(); ++it)
    removeGroup(m_model, *it);
}

void RunsTablePresenter::notifyPauseRequested() {}

void RunsTablePresenter::notifyProcessRequested() { prettyPrintModel(m_model); }

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
    auto regexFilter =
        filterFromRegexString(filterString, m_view->jobs(), m_model);
    m_view->jobs().filterRowsBy(std::move(regexFilter));
  } catch (boost::regex_error &) {
  }
}

void RunsTablePresenter::notifyFilterReset() { m_view->resetFilterBox(); }

void RunsTablePresenter::appendRowsToGroupsInView(
    std::vector<int> const &groupIndices) {
  for (auto const &groupIndex : groupIndices)
    m_view->jobs().appendChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}));
}

void RunsTablePresenter::appendRowsToGroupsInModel(
    std::vector<int> const &groupIndices) {
  for (auto const &groupIndex : groupIndices)
    appendEmptyRow(m_model, groupIndex);
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

void RunsTablePresenter::appendEmptyGroupInModel() { appendEmptyGroup(m_model); }

void RunsTablePresenter::appendEmptyGroupInView() {
  auto location = m_view->jobs().appendChildRowOf(
      MantidQt::MantidWidgets::Batch::RowLocation());
  applyGroupStylingToRow(location);
  // TODO: Consider using the other version of appendChildRowOf
}

void RunsTablePresenter::insertEmptyGroupInModel(int beforeGroup) {
  insertEmptyGroup(m_model, beforeGroup);
}

void RunsTablePresenter::insertEmptyRowInModel(int groupIndex, int beforeRow) {
  insertEmptyRow(m_model, groupIndex, beforeRow);
}

void RunsTablePresenter::insertEmptyGroupInView(int beforeGroup) {
  auto location = m_view->jobs().insertChildRowOf(
      MantidQt::MantidWidgets::Batch::RowLocation(), beforeGroup);
  applyGroupStylingToRow(location);
}

void RunsTablePresenter::notifyExpandAllRequested() { m_view->jobs().expandAll(); }

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

void RunsTablePresenter::clearInvalidCellStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setIconFilePath("");
  cell.setBorderColor("darkGrey");
}

void RunsTablePresenter::showAllCellsOnRowAsValid(
    MantidWidgets::Batch::RowLocation const &itemIndex) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  clearInvalidCellStyling(cells);
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::applyInvalidCellStyling(MantidWidgets::Batch::Cell &cell) {
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
    MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {
  assertOrThrow(column == 0,
                "Changed value of cell which should be uneditable");
  auto const groupIndex = groupOf(itemIndex);
  if (!setGroupName(m_model, groupIndex, newValue)) {
    auto cell = m_view->jobs().cellAt(itemIndex, column);
    cell.setContentText(oldValue);
    m_view->jobs().setCellAt(itemIndex, column, cell);
  }
}

void RunsTablePresenter::updateRowField(
    MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int,
    std::string const &, std::string const &) {
  auto const groupIndex = groupOf(itemIndex);
  auto const rowIndex = rowOf(itemIndex);
  auto rowValidationResult = validateRow(m_model, m_workspaceNameFactory,
                                         cellTextFromViewAt(itemIndex));
  updateRow(m_model, groupIndex, rowIndex,
            rowValidationResult.validElseNone());
  if (rowValidationResult.isValid()) {
    showAllCellsOnRowAsValid(itemIndex);
  } else {
    showCellsAsInvalidInView(itemIndex, rowValidationResult.assertError());
  }
}

void RunsTablePresenter::notifyCellTextChanged(
    MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {
  if (isGroupLocation(itemIndex))
    updateGroupName(itemIndex, column, oldValue, newValue);
  else
    updateRowField(itemIndex, column, oldValue, newValue);
}

void RunsTablePresenter::applyGroupStylingToRow(
    MantidWidgets::Batch::RowLocation const &location) {
  auto cells = m_view->jobs().cellsAt(location);
  boost::fill(boost::make_iterator_range(cells.begin() + 1, cells.end()),
              m_view->jobs().deadCell());
  m_view->jobs().setCellsAt(location, cells);
}

void RunsTablePresenter::notifyRowInserted(
    MantidQt::MantidWidgets::Batch::RowLocation const &newRowLocation) {
  if (newRowLocation.depth() > DEPTH_LIMIT) {
    m_view->jobs().removeRowAt(newRowLocation);
  } else if (isGroupLocation(newRowLocation)) {
    insertEmptyGroupInModel(groupOf(newRowLocation));
    applyGroupStylingToRow(newRowLocation);
  } else if (isRowLocation(newRowLocation)) {
    insertEmptyRowInModel(groupOf(newRowLocation), rowOf(newRowLocation));
  }
}

void BatchPresenter::removeRowsAndGroupsFromModel(std::vector<
    MantidQt::MantidWidgets::Batch::RowLocation> locationsOfRowsToRemove) {
  std::sort(locationsOfRowsToRemove.begin(), locationsOfRowsToRemove.end());
  for (auto location = locationsOfRowsToRemove.crbegin();
       location != locationsOfRowsToRemove.crend(); ++location) {
    auto const groupIndex = groupOf(*location);
    if (isRowLocation(*location)) {
      auto const rowIndex = rowOf(*location);
      removeRow(m_model, groupIndex, rowIndex);
    } else if (isGroupLocation(*location)) {
      removeGroup(m_model, groupIndex);
    }
  }
}

void BatchPresenter::removeRowsAndGroupsFromView(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
        locationsOfRowsToRemove) {
  m_view->jobs().removeRows(locationsOfRowsToRemove);
}

void BatchPresenter::notifyRemoveRowsRequested(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
        locationsOfRowsToRemove) {
  removeRowsAndGroupsFromModel(locationsOfRowsToRemove);
  removeRowsAndGroupsFromView(locationsOfRowsToRemove);
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
      m_view->jobs().appendSubtreesAt(
          MantidQt::MantidWidgets::Batch::RowLocation(), m_clipboard.get());
  } else {
    m_view->invalidSelectionForPaste();
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt
