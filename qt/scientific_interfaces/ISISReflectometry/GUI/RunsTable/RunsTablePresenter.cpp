// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTablePresenter.h"
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "Reduction/Group.h"
#include "Reduction/RowLocation.h"
#include "Reduction/ValidateRow.h"
#include "RegexRowFilter.h"
#include <boost/range/algorithm/fill.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/regex.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace { // unnamed
void clearStateStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor(Colour::DEFAULT);
  cell.setToolTip("");
}

void applyInvalidStateStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor(Colour::INVALID);
  cell.setToolTip("Row will not be processed: it either contains invalid cell values, "
                  "or duplicates a reduction in another row");
}

void applyRunningStateStyling(MantidWidgets::Batch::Cell &cell) { cell.setBackgroundColor(Colour::RUNNING); }

void applyCompletedStateStyling(MantidWidgets::Batch::Cell &cell) { cell.setBackgroundColor(Colour::SUCCESS); }

void applyErrorStateStyling(MantidWidgets::Batch::Cell &cell, std::string const &errorMessage) {
  cell.setBackgroundColor(Colour::FAILURE);
  cell.setToolTip(errorMessage);
}

void applyWarningStateStyling(MantidWidgets::Batch::Cell &cell, std::string const &errorMessage) {
  cell.setBackgroundColor(Colour::WARNING);
  cell.setToolTip(errorMessage);
}

void applyChildSuccessStateStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setBackgroundColor(Colour::CHILDREN_SUCCESS);
}

/** Check if a group name exists in the given model.
 * @param groupName : the name to check
 * @param jobs : the model to compare against
 * @rootsToIgnore : rows to ignore in the check
 */
bool groupNameExists(std::string const &groupName, ReductionJobs const &jobs,
                     std::vector<MantidWidgets::Batch::RowLocation> const &rootsToIgnore) {

  // Check if the group name exists in the jobs
  auto maybeExistingGroupIndex = jobs.indexOfGroupWithName(groupName);
  if (!maybeExistingGroupIndex.has_value())
    return false;

  // If it exists but in one of the roots to ignore, return false
  auto existingGroupLocation = MantidWidgets::Batch::RowLocation({maybeExistingGroupIndex.value()});
  if (std::find(rootsToIgnore.cbegin(), rootsToIgnore.cend(), existingGroupLocation) != rootsToIgnore.cend())
    return false;

  return true;
}

/** Check if a group name exists in the clipboard.
 * @param groupName : the name to check
 * @param clipboard : the clipboard to compare against
 * @indexToIgnore : the index of the original item which should be ignored
 */
bool groupNameExists(std::string const &groupName, Clipboard const &clipboard, int indexToIgnore) {
  for (auto compareIndex = 0; compareIndex < clipboard.numberOfRoots(); ++compareIndex) {
    if (compareIndex != indexToIgnore && groupName == clipboard.groupName(compareIndex))
      return true;
  }
  return false;
}

void makePastedGroupNamesUnique(Clipboard &clipboard, int rootIndex,
                                std::vector<MantidWidgets::Batch::RowLocation> const &replacementRoots,
                                ReductionJobs const &jobs) {

  if (!clipboard.isGroupLocation(rootIndex))
    return;

  auto clipGroupName = clipboard.groupName(rootIndex);
  if (clipGroupName.empty())
    return;

  // Replace the group name until it is unique in the model and clipboard
  while (groupNameExists(clipGroupName, jobs, replacementRoots) || groupNameExists(clipGroupName, clipboard, rootIndex))
    clipGroupName.append(" (copy)");

  // Set the new name
  clipboard.setGroupName(rootIndex, clipGroupName);
}

void makePastedGroupNamesUnique(Clipboard &clipboard,
                                std::vector<MantidWidgets::Batch::RowLocation> const &replacementRoots,
                                ReductionJobs const &jobs) {
  for (auto rootIndex = 0; rootIndex < clipboard.numberOfRoots(); ++rootIndex) {
    makePastedGroupNamesUnique(clipboard, rootIndex, replacementRoots, jobs);
  }
}
} // namespace

RunsTablePresenter::RunsTablePresenter(IRunsTableView *view, std::vector<std::string> const &instruments,
                                       double thetaTolerance, ReductionJobs jobs, const IPlotter &plotter)
    : m_view(view), m_model(instruments, thetaTolerance, std::move(jobs)), m_clipboard(),
      m_jobViewUpdater(m_view->jobs()), m_plotter(plotter) {
  m_view->subscribe(this);

  // Add Group to view and model, add row to this group in view and model.
  ensureAtLeastOneGroupExists();
}

void RunsTablePresenter::appendRowAndGroup() {
  // Calculate
  std::vector<int> localGroupIndices;
  localGroupIndices.emplace_back(static_cast<int>(m_model.reductionJobs().groups().size()));
  appendEmptyGroupInModel();
  appendEmptyGroupInView();
  appendRowsToGroupsInView(localGroupIndices);
  appendRowsToGroupsInModel(localGroupIndices);
}

void RunsTablePresenter::acceptMainPresenter(IRunsPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

RunsTable const &RunsTablePresenter::runsTable() const { return m_model; }

RunsTable &RunsTablePresenter::mutableRunsTable() { return m_model; }

void RunsTablePresenter::mergeAdditionalJobs(ReductionJobs const &additionalJobs) {
  mergeJobsInto(m_model.mutableReductionJobs(), additionalJobs, m_model.thetaTolerance(), m_jobViewUpdater);
  updateProgressBar();
}

void RunsTablePresenter::removeRowsFromModel(std::vector<MantidWidgets::Batch::RowLocation> rows) {
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
      notifyTableChanged();
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
    notifyTableChanged();
    notifyRowStateChanged();
    notifySelectionChanged();
  } else {
    m_view->mustSelectGroupOrRow();
  }
}

void RunsTablePresenter::removeGroupsFromView(std::vector<int> const &groupIndicesOrderedLowToHigh) {
  for (auto it = groupIndicesOrderedLowToHigh.crbegin(); it < groupIndicesOrderedLowToHigh.crend(); ++it)
    m_view->jobs().removeRowAt(MantidWidgets::Batch::RowLocation({*it}));
}

void RunsTablePresenter::removeGroupsFromModel(std::vector<int> const &groupIndicesOrderedLowToHigh) {
  for (auto it = groupIndicesOrderedLowToHigh.crbegin(); it < groupIndicesOrderedLowToHigh.crend(); ++it)
    removeGroup(m_model.mutableReductionJobs(), *it);
}

void RunsTablePresenter::notifyResumeReductionRequested() { m_mainPresenter->notifyResumeReductionRequested(); }

void RunsTablePresenter::notifyPauseReductionRequested() { m_mainPresenter->notifyPauseReductionRequested(); }

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
  notifyTableChanged();
  notifyRowStateChanged();
}

void RunsTablePresenter::notifyFilterChanged(std::string const &filterString) {
  try {
    auto regexFilter = filterFromRegexString(filterString, m_view->jobs(), m_model.reductionJobs());
    m_view->jobs().filterRowsBy(std::move(regexFilter));
  } catch (boost::regex_error &) {
  }
}

void RunsTablePresenter::notifyChangeInstrumentRequested() {
  auto const instrumentName = m_view->getInstrumentName();
  // If the instrument cannot be changed, revert to the original
  if (!m_mainPresenter->notifyChangeInstrumentRequested(instrumentName))
    m_view->setInstrumentName(m_mainPresenter->instrumentName());
}

void RunsTablePresenter::notifyFilterReset() { m_view->resetFilterBox(); }

void RunsTablePresenter::updateWidgetEnabledState() {
  auto const processing = isProcessing();
  auto const autoreducing = isAutoreducing();

  m_view->setJobsTableEnabled(!processing && !autoreducing);
  m_view->setInstrumentSelectorEnabled(!isAnyBatchProcessing() && !isAnyBatchAutoreducing());
  m_view->setProcessButtonEnabled(!processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Process, !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Pause, processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::InsertRow, !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::InsertGroup, !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::DeleteRow, !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::DeleteGroup, !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Copy, !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Paste, !processing && !autoreducing);
  m_view->setActionEnabled(IRunsTableView::Action::Cut, !processing && !autoreducing);
}

void RunsTablePresenter::notifyReductionResumed() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyReductionPaused() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyAutoreductionResumed() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyAutoreductionPaused() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyAnyBatchReductionResumed() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyAnyBatchReductionPaused() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyAnyBatchAutoreductionResumed() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyAnyBatchAutoreductionPaused() { updateWidgetEnabledState(); }

void RunsTablePresenter::notifyInstrumentChanged(std::string const &instrumentName) {
  m_view->setInstrumentName(instrumentName);
}

void RunsTablePresenter::setTablePrecision(int &precision) { m_jobViewUpdater.setPrecision(precision); }

void RunsTablePresenter::resetTablePrecision() { m_jobViewUpdater.resetPrecision(); }

void RunsTablePresenter::settingsChanged() {
  m_model.resetState();
  notifyRowStateChanged();
  notifyRowModelChanged();
}

void RunsTablePresenter::appendRowsToGroupsInView(std::vector<int> const &groupIndices) {
  for (auto const &groupIndex : groupIndices)
    m_view->jobs().appendChildRowOf(MantidWidgets::Batch::RowLocation({groupIndex}));
}

void RunsTablePresenter::appendRowsToGroupsInModel(std::vector<int> const &groupIndices) {
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
  notifyTableChanged();
  notifyRowStateChanged();
}

void RunsTablePresenter::appendEmptyGroupInModel() { appendEmptyGroup(m_model.mutableReductionJobs()); }

void RunsTablePresenter::appendEmptyGroupInView() {
  auto location = m_view->jobs().appendChildRowOf(MantidWidgets::Batch::RowLocation());
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
  auto location = m_view->jobs().insertChildRowOf(MantidWidgets::Batch::RowLocation(), beforeGroup);
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

  if (m_model.reductionJobs().groups().size() == 0) {
    appendRowAndGroup();
    notifyExpandAllRequested();
    return;
  }

  auto const &group = m_model.reductionJobs().groups()[0];
  if (group.rows().size() > 0)
    return;

  auto location = MantidWidgets::Batch::RowLocation({0});
  auto cells = m_view->jobs().cellsAt(location);

  // Groups should only have one cell
  if (cells.size() == 1)
    return;

  // The model is fine, we just need to update the view. Add a new, proper,
  // group first, then delete the original one
  appendEmptyGroupInView();
  removeRowsAndGroupsFromView({location});

  // Insert a new group (and include an expanded row, for usability) and then
  // delete the original "bad" group
  appendRowAndGroup();
  notifyExpandAllRequested();

  // After repairing the view and adding the Row and Group
  removeGroupsFromModel({0});
  removeGroupsFromView({0});
}

void RunsTablePresenter::notifyExpandAllRequested() { m_view->jobs().expandAll(); }

void RunsTablePresenter::notifyCollapseAllRequested() { m_view->jobs().collapseAll(); }

namespace {
bool isGroup(MantidWidgets::Batch::RowLocation const &location) { return isGroupLocation(location); }
} // namespace

void RunsTablePresenter::notifyFillDown() {
  auto selected = m_view->jobs().selectedRowLocations();
  selected.erase(std::remove_if(selected.begin(), selected.end(), isGroup), selected.end());
  std::sort(selected.begin(), selected.end());

  if (selected.size() < 1)
    return;

  auto const topRow = selected[0];
  auto const column = m_view->jobs().currentColumn();
  auto const cellContent = m_view->jobs().cellAt(topRow, column).contentText();

  for (auto const &rowLocation : selected) {
    auto cell = m_view->jobs().cellAt(rowLocation, column);
    auto const oldCellContent = cell.contentText();
    cell.setContentText(cellContent);
    m_view->jobs().setCellAt(rowLocation, column, cell);
    notifyCellTextChanged(rowLocation, column, oldCellContent, cellContent);
  }
}

std::vector<std::string>
RunsTablePresenter::cellTextFromViewAt(MantidWidgets::Batch::RowLocation const &location) const {
  return map(m_view->jobs().cellsAt(location), [](MantidWidgets::Batch::Cell const &cell) -> std::string {
    if (cell.isOutput())
      return std::string();
    else
      return cell.contentText();
  });
}

void RunsTablePresenter::clearInvalidCellStyling(std::vector<MantidWidgets::Batch::Cell> &cells) {
  for (auto &cell : cells)
    clearInvalidCellStyling(cell);
}

void RunsTablePresenter::clearInvalidCellStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setIconFilePath("");
  cell.setBorderColor("darkGrey");
}

void RunsTablePresenter::showAllCellsOnRowAsValid(MantidWidgets::Batch::RowLocation const &itemIndex) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  clearInvalidCellStyling(cells);
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::applyInvalidCellStyling(MantidWidgets::Batch::Cell &cell) {
  cell.setIconFilePath(":/invalid.png");
  cell.setBorderColor("darkRed");
}

void RunsTablePresenter::showCellsAsInvalidInView(MantidWidgets::Batch::RowLocation const &itemIndex,
                                                  std::vector<int> const &invalidColumns) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  clearInvalidCellStyling(cells);
  for (auto &column : invalidColumns)
    applyInvalidCellStyling(cells[column]);
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void RunsTablePresenter::updateGroupName(MantidWidgets::Batch::RowLocation const &itemIndex, int column,
                                         std::string const &oldValue, std::string const &newValue) {
  assertOrThrow(column == 0, "Changed value of cell which should be uneditable");
  auto const groupIndex = groupOf(itemIndex);
  if (!setGroupName(m_model.mutableReductionJobs(), groupIndex, newValue)) {
    auto cell = m_view->jobs().cellAt(itemIndex, column);
    cell.setContentText(oldValue);
    m_view->jobs().setCellAt(itemIndex, column, cell);
  }
  m_mainPresenter->notifyGroupNameChanged(m_model.mutableReductionJobs().mutableGroups()[groupIndex]);
  notifyRowModelChanged();
}

void RunsTablePresenter::updateRowField(MantidWidgets::Batch::RowLocation const &itemIndex, int column,
                                        std::string const &, std::string const &) {
  // User has edited the text so reset the output-value flag as it now contains
  // an input
  auto cell = m_view->jobs().cellAt(itemIndex, column);
  cell.setInput();
  m_view->jobs().setCellAt(itemIndex, column, cell);

  auto const groupIndex = groupOf(itemIndex);
  auto const rowIndex = rowOf(itemIndex);
  auto rowValidationResult = validateRow(cellTextFromViewAt(itemIndex));
  updateRow(m_model.mutableReductionJobs(), groupIndex, rowIndex, rowValidationResult.validElseNone());
  if (rowValidationResult.isValid()) {
    showAllCellsOnRowAsValid(itemIndex);
    m_mainPresenter->notifyRowContentChanged(
        m_model.mutableReductionJobs().mutableGroups()[groupIndex].mutableRows()[rowIndex].get());
    notifyRowModelChanged(m_model.reductionJobs().groups()[groupIndex].rows()[rowIndex].get());
  } else {
    showCellsAsInvalidInView(itemIndex, rowValidationResult.assertError());
  }
}

void RunsTablePresenter::notifyCellTextChanged(MantidWidgets::Batch::RowLocation const &itemIndex, int column,
                                               std::string const &oldValue, std::string const &newValue) {
  if (isGroupLocation(itemIndex))
    updateGroupName(itemIndex, column, oldValue, newValue);
  else
    updateRowField(itemIndex, column, oldValue, newValue);
  notifyTableChanged();
  notifyRowStateChanged();
}

void RunsTablePresenter::notifyBatchLoaded() {
  notifySelectionChanged();
  m_model.mutableReductionJobs().setAllRowParents();
}

void RunsTablePresenter::notifySelectionChanged() {
  m_model.setSelectedRowLocations(m_view->jobs().selectedRowLocations());
}

void RunsTablePresenter::applyGroupStylingToRow(MantidWidgets::Batch::RowLocation const &location) {
  auto cells = m_view->jobs().cellsAt(location);
  if (cells.size() > 0) {
    boost::fill(boost::make_iterator_range(cells.begin() + 1, cells.end()), m_view->jobs().deadCell());
    m_view->jobs().setCellsAt(location, cells);
  }
}

void RunsTablePresenter::applyStylingToParent(Row const &row) {
  auto *parent = row.getParent();
  if (parent) {
    auto const parentLocation = m_model.reductionJobs().getLocation(*parent);
    setRowStylingForItem(parentLocation, *parent);
  }
}

void RunsTablePresenter::notifyRowInserted(MantidWidgets::Batch::RowLocation const &newRowLocation) {
  if (newRowLocation.depth() > DEPTH_LIMIT) {
    m_view->jobs().removeRowAt(newRowLocation);
  } else if (isGroupLocation(newRowLocation)) {
    insertEmptyGroupInModel(groupOf(newRowLocation));
    applyGroupStylingToRow(newRowLocation);
  } else if (isRowLocation(newRowLocation)) {
    insertEmptyRowInModel(groupOf(newRowLocation), rowOf(newRowLocation));
  }
  notifyTableChanged();
  notifyRowStateChanged();
}

void RunsTablePresenter::removeRowsAndGroupsFromModel(
    std::vector<MantidWidgets::Batch::RowLocation> locationsOfRowsToRemove) {
  std::sort(locationsOfRowsToRemove.begin(), locationsOfRowsToRemove.end());
  for (auto location = locationsOfRowsToRemove.crbegin(); location != locationsOfRowsToRemove.crend(); ++location) {
    auto const groupIndex = groupOf(*location);
    if (isRowLocation(*location)) {
      auto const rowIndex = rowOf(*location);
      removeRow(m_model.mutableReductionJobs(), groupIndex, rowIndex);
    } else if (isGroupLocation(*location)) {
      removeGroup(m_model.mutableReductionJobs(), groupIndex);
    }
  }
}

void RunsTablePresenter::removeAllRowsAndGroupsFromModel() { removeAllRowsAndGroups(m_model.mutableReductionJobs()); }

void RunsTablePresenter::removeRowsAndGroupsFromView(
    std::vector<MantidWidgets::Batch::RowLocation> const &locationsOfRowsToRemove) {
  m_view->jobs().removeRows(locationsOfRowsToRemove);
}

void RunsTablePresenter::removeAllRowsAndGroupsFromView() { m_view->jobs().removeAllRows(); }

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
    std::vector<MantidWidgets::Batch::RowLocation> const &locationsOfRowsToRemove) {
  if (isProcessing() || isAutoreducing())
    return;

  removeRowsAndGroupsFromModel(locationsOfRowsToRemove);
  removeRowsAndGroupsFromView(locationsOfRowsToRemove);
  ensureAtLeastOneGroupExists();
  notifySelectionChanged();
}

void RunsTablePresenter::notifyRemoveAllRowsAndGroupsRequested() {
  removeAllRowsAndGroupsFromModel();
  removeAllRowsAndGroupsFromView();
  ensureAtLeastOneGroupExists();
  notifySelectionChanged();
}

void RunsTablePresenter::notifyCopyRowsRequested() {
  m_clipboard = Clipboard(m_view->jobs().selectedSubtrees(), m_view->jobs().selectedSubtreeRoots());
  if (m_clipboard.isInitialized())
    m_view->jobs().clearSelection();
  else
    m_view->invalidSelectionForCopy();
}

void RunsTablePresenter::notifyCutRowsRequested() {
  if (isProcessing() || isAutoreducing())
    return;

  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() < 1)
    return;

  m_clipboard = Clipboard(m_view->jobs().selectedSubtrees(), m_view->jobs().selectedSubtreeRoots());
  if (m_clipboard.isInitialized()) {
    removeRowsAndGroupsFromView(selected);
    removeRowsAndGroupsFromModel(selected);
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
  if (maybeReplacementRoots.is_initialized() && m_clipboard.isInitialized()) {
    auto &replacementRoots = maybeReplacementRoots.get();
    makePastedGroupNamesUnique(m_clipboard, replacementRoots, m_model.reductionJobs());
    if (replacementRoots.empty())
      pasteGroupsAtEnd();
    else if (containsGroups(m_clipboard) && containsGroups(replacementRoots))
      pasteGroupsOntoGroups(replacementRoots);
    else if (containsGroups(replacementRoots))
      pasteRowsOntoGroup(replacementRoots);
    else
      pasteRowsOntoRows(replacementRoots);
    notifyTableChanged();
    notifyRowStateChanged();
    notifySelectionChanged();
  }
}

void RunsTablePresenter::pasteRowsOntoRows(std::vector<MantidWidgets::Batch::RowLocation> &replacementRoots) {
  // Paste rows onto given locations. Only possible if the depth is the same.
  // Also limit source and destination sizes to be equal for now for
  // simplicity.
  if (!containsGroups(replacementRoots) && !containsGroups(m_clipboard) &&
      m_clipboard.numberOfRoots() == static_cast<int>(replacementRoots.size())) {
    // Update view
    m_view->jobs().replaceRows(replacementRoots, m_clipboard.subtrees());
    // Update model
    auto &groups = m_model.mutableReductionJobs().mutableGroups();
    auto replacementRows = m_clipboard.createRowsForAllRoots();
    auto replacementRow = replacementRows.begin();
    auto replacementPoint = replacementRoots.cbegin();
    for (; replacementRow < replacementRows.end() && replacementPoint < replacementRoots.cend();
         ++replacementRow, ++replacementPoint) {
      auto &group = groups[groupOf(*replacementPoint)];
      auto &rows = group.mutableRows();
      rows[rowOf(*replacementPoint)] = *replacementRow;
    }
  } else {
    m_view->invalidSelectionForPaste();
  }
}

void RunsTablePresenter::pasteRowsOntoGroup(std::vector<MantidWidgets::Batch::RowLocation> &) {
  // Not currently supported
  m_view->invalidSelectionForPaste();
}

void RunsTablePresenter::pasteGroupsOntoGroups(std::vector<MantidWidgets::Batch::RowLocation> &replacementRoots) {
  // Paste onto given locations. Only possible if the depth is the same.
  if (containsGroups(replacementRoots) && containsGroups(m_clipboard) &&
      static_cast<int>(replacementRoots.size()) == m_clipboard.numberOfRoots()) {
    // Update view
    m_view->jobs().replaceRows(replacementRoots, m_clipboard.subtrees());
    // Update model
    auto &groups = m_model.mutableReductionJobs().mutableGroups();
    int clipboardIndex = 0;
    for (auto const &replacementPoint : replacementRoots) {
      groups[groupOf(replacementPoint)] = m_clipboard.createGroupForRoot(clipboardIndex);
      ++clipboardIndex;
    }
  } else {
    m_view->invalidSelectionForPaste();
  }
}

void RunsTablePresenter::pasteGroupsAtEnd() {
  // Paste rows into a group location at the end of the table. Only possible if
  // the clipboard contains groups
  if (containsGroups(m_clipboard)) {
    // Update view
    m_view->jobs().appendSubtreesAt(MantidWidgets::Batch::RowLocation(), m_clipboard.subtrees());
    // Update model
    for (int rootIndex = 0; rootIndex < m_clipboard.numberOfRoots(); ++rootIndex) {
      m_model.mutableReductionJobs().appendGroup(m_clipboard.createGroupForRoot(rootIndex));
    }
  } else {
    m_view->invalidSelectionForPaste();
  }
}

void RunsTablePresenter::forAllCellsAt(MantidWidgets::Batch::RowLocation const &location, UpdateCellFunc updateFunc) {
  auto cells = m_view->jobs().cellsAt(location);
  std::for_each(cells.begin(), cells.end(), updateFunc);
  m_view->jobs().setCellsAt(location, cells);
}

void RunsTablePresenter::forAllCellsAt(MantidWidgets::Batch::RowLocation const &location,
                                       UpdateCellWithTooltipFunc updateFunc, std::string const &tooltip) {
  auto cells = m_view->jobs().cellsAt(location);
  std::for_each(cells.begin(), cells.end(), [&](MantidWidgets::Batch::Cell &cell) { updateFunc(cell, tooltip); });
  m_view->jobs().setCellsAt(location, cells);
}

void RunsTablePresenter::setRowStylingForItem(MantidWidgets::Batch::RowLocation const &rowLocation, Item const &item) {
  switch (item.state()) {
  case State::ITEM_NOT_STARTED: // fall through
  case State::ITEM_STARTING:
    forAllCellsAt(rowLocation, clearStateStyling);
    break;
  case State::ITEM_RUNNING:
    forAllCellsAt(rowLocation, applyRunningStateStyling);
    break;
  case State::ITEM_SUCCESS:
    forAllCellsAt(rowLocation, applyCompletedStateStyling);
    break;
  case State::ITEM_ERROR:
    forAllCellsAt(rowLocation, applyErrorStateStyling, item.message());
    break;
  case State::ITEM_WARNING:
    forAllCellsAt(rowLocation, applyWarningStateStyling, item.message());
    break;
  case State::ITEM_CHILDREN_SUCCESS:
    forAllCellsAt(rowLocation, applyChildSuccessStateStyling);
    break;
  };
}

void RunsTablePresenter::updateProgressBar() { m_view->setProgress(m_mainPresenter->percentComplete()); }

void RunsTablePresenter::notifyTableChanged() { m_mainPresenter->notifyTableChanged(); }

void RunsTablePresenter::notifyRowStateChanged() {
  updateProgressBar();

  int groupIndex = 0;
  for (auto &group : m_model.mutableReductionJobs().mutableGroups()) {
    auto groupLocation = MantidWidgets::Batch::RowLocation({groupIndex});
    setRowStylingForItem(groupLocation, group);

    int rowIndex = 0;
    for (auto &row : group.rows()) {
      auto rowLocation = MantidWidgets::Batch::RowLocation({groupIndex, rowIndex});

      if (!row)
        forAllCellsAt(rowLocation, applyInvalidStateStyling);
      else {
        setRowStylingForItem(rowLocation, *row);
      }

      ++rowIndex;
    }
    ++groupIndex;
  }
}

void RunsTablePresenter::notifyRowStateChanged(boost::optional<Item const &> item) {
  if (!item)
    return;

  updateProgressBar();
  auto const location = m_model.reductionJobs().getLocation(item.get());
  setRowStylingForItem(location, item.get());
  if (item->isGroup()) {
    return;
  }
  auto const &row = dynamic_cast<const Row &>(item.get());
  applyStylingToParent(row);
}

void RunsTablePresenter::notifyRowModelChanged() {
  int groupIndex = 0;
  for (auto &group : m_model.reductionJobs().groups()) {
    auto groupLocation = MantidWidgets::Batch::RowLocation({groupIndex});
    int rowIndex = 0;
    for (auto &row : group.rows()) {
      if (row) {
        auto rowLocation = MantidWidgets::Batch::RowLocation({groupIndex, rowIndex});
        m_jobViewUpdater.rowModified(groupOf(rowLocation), rowOf(rowLocation), *row);
      }
      ++rowIndex;
    }
    ++groupIndex;
  }
}

void RunsTablePresenter::notifyRowModelChanged(boost::optional<Item const &> item) {
  if (!item.is_initialized() || item->isGroup())
    return;

  auto const &row = dynamic_cast<Row const &>(item.get());
  auto const location = m_model.reductionJobs().getLocation(row);
  m_jobViewUpdater.rowModified(groupOf(location), rowOf(location), row);
}

bool RunsTablePresenter::isProcessing() const { return m_mainPresenter->isProcessing(); }

bool RunsTablePresenter::isAutoreducing() const { return m_mainPresenter->isAutoreducing(); }

bool RunsTablePresenter::isAnyBatchProcessing() const { return m_mainPresenter->isAnyBatchProcessing(); }

bool RunsTablePresenter::isAnyBatchAutoreducing() const { return m_mainPresenter->isAnyBatchAutoreducing(); }

void RunsTablePresenter::notifyPlotSelectedPressed() {
  std::vector<std::string> workspaces;
  const auto rows = m_model.selectedRows();

  for (const auto &row : rows) {
    if (row.state() == State::ITEM_SUCCESS)
      workspaces.emplace_back(row.reducedWorkspaceNames().iVsQBinned());
  }

  if (workspaces.empty())
    return;

  m_plotter.reflectometryPlot(workspaces);
}

void RunsTablePresenter::notifyPlotSelectedStitchedOutputPressed() {
  std::vector<std::string> workspaces;
  const auto groups = m_model.selectedGroups();

  for (const auto *group : groups) {
    if (group->state() == State::ITEM_SUCCESS)
      workspaces.emplace_back(group->postprocessedWorkspaceName());
  }

  if (workspaces.empty())
    return;

  m_plotter.reflectometryPlot(workspaces);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
