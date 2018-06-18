#include "BatchPresenter.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "Reduction/Group.h"
#include "Reduction/Slicing.h"
#include "ValidateRow.h"

#include <boost/regex.hpp>
#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

BatchPresenter::BatchPresenter(IBatchView *view,
                               std::vector<std::string> const &instruments,
                               Jobs jobs)
    : m_view(view), m_instruments(instruments), m_model(std::move(jobs)),
      m_jobViewUpdater(m_view->jobs()), m_workspaceNameFactory(Slicing()) {
  m_view->subscribe(this);
}

Jobs const &BatchPresenter::reductionJobs() const { return m_model; }

void BatchPresenter::mergeAdditionalJobs(Jobs const &additionalJobs) {
  std::cout << "Before Transfer:" << std::endl;
  prettyPrintModel(m_model);

  std::cout << "Transfering:" << std::endl;
  prettyPrintModel(additionalJobs);

  mergeJobsInto(m_model, additionalJobs, 0.0001, m_workspaceNameFactory,
                m_jobViewUpdater);

  std::cout << "After Transfer:" << std::endl;
  prettyPrintModel(m_model);
}

void BatchPresenter::removeRowsFromModel(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> rows) {
  std::sort(rows.begin(), rows.end());
  for (auto row = rows.crbegin(); row != rows.crend(); ++row) {
    auto const groupIndex = groupOf(*row);
    auto const rowIndex = rowOf(*row);
    removeRow(m_model, groupIndex, rowIndex);
  }
}

void BatchPresenter::notifyDeleteRowRequested() {
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

void BatchPresenter::notifyDeleteGroupRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groupIndicesOrderedLowToHigh = groupIndexesFromSelection(selected);
    removeGroupsFromModel(groupIndicesOrderedLowToHigh);
    removeGroupsFromView(groupIndicesOrderedLowToHigh);
  } else {
    m_view->mustSelectGroupOrRow();
  }
}

void BatchPresenter::removeGroupsFromView(
    std::vector<int> const &groupIndicesOrderedLowToHigh) {
  for (auto it = groupIndicesOrderedLowToHigh.crbegin();
       it < groupIndicesOrderedLowToHigh.crend(); ++it)
    m_view->jobs().removeRowAt(
        MantidQt::MantidWidgets::Batch::RowLocation({*it}));
}

void BatchPresenter::removeGroupsFromModel(
    std::vector<int> const &groupIndicesOrderedLowToHigh) {
  for (auto it = groupIndicesOrderedLowToHigh.crbegin();
       it < groupIndicesOrderedLowToHigh.crend(); ++it)
    removeGroup(m_model, *it);
}

void BatchPresenter::notifyPauseRequested() {}

void BatchPresenter::notifyProcessRequested() { prettyPrintModel(m_model); }

template <typename T>
void sortAndRemoveDuplicatesInplace(std::vector<T> &items) {
  std::sort(items.begin(), items.end());
  auto eraseBegin = std::unique(items.begin(), items.end());
  items.erase(eraseBegin, items.end());
}

std::vector<int> BatchPresenter::mapToContainingGroups(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
        mustNotContainRoot) const {
  auto groups = std::vector<int>();
  std::transform(mustNotContainRoot.cbegin(), mustNotContainRoot.cend(),
                 std::back_inserter(groups),
                 [this](MantidWidgets::Batch::RowLocation const &location)
                     -> int { return groupOf(location); });
  return groups;
}

std::vector<int> BatchPresenter::groupIndexesFromSelection(
    std::vector<MantidWidgets::Batch::RowLocation> const &selected) const {
  auto groups = mapToContainingGroups(selected);
  sortAndRemoveDuplicatesInplace(groups);
  return groups;
}

void BatchPresenter::notifyInsertRowRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groups = groupIndexesFromSelection(selected);
    appendRowsToGroupsInModel(groups);
    appendRowsToGroupsInView(groups);
  } else {
    m_view->mustSelectGroup();
  }
}

void BatchPresenter::notifyFilterChanged(std::string const &filterValue) {
  try {
    auto regexFilter = boost::regex(filterValue);
    m_view->jobs().filterRowsBy(
        MantidQt::MantidWidgets::Batch::makeFilterFromLambda(
            [this, filterValue, regexFilter](
                MantidQt::MantidWidgets::Batch::RowLocation const &location)
                -> bool {
                  if (location.isRoot()) {
                    return true;
                  } else if (isGroupLocation(location)) {
                    auto cellText =
                        m_view->jobs().cellAt(location, 0).contentText();
                    return boost::regex_search(cellText, regexFilter);
                  } else {
                    assert(isRowLocation(location));
                    auto cellText =
                        m_view->jobs().cellAt(location, 0).contentText();
                    auto groupText = groupName(m_model, groupOf(location));
                    return boost::regex_search(cellText, regexFilter) ||
                           boost::regex_search(groupText, regexFilter);
                  }
                }));
  } catch (boost::regex_error &) {
  }
}

void BatchPresenter::notifyFilterReset() { m_view->resetFilterBox(); }

void BatchPresenter::appendRowsToGroupsInView(
    std::vector<int> const &groupIndices) {
  for (auto &&groupIndex : groupIndices)
    m_view->jobs().appendChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}));
}

void BatchPresenter::appendRowsToGroupsInModel(
    std::vector<int> const &groupIndices) {
  for (auto &&groupIndex : groupIndices)
    appendEmptyRow(m_model, groupIndex);
}

void BatchPresenter::notifyInsertGroupRequested() {
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

void BatchPresenter::appendEmptyGroupInModel() { appendEmptyGroup(m_model); }

void BatchPresenter::appendEmptyGroupInView() {
  auto location = m_view->jobs().appendChildRowOf(
      MantidQt::MantidWidgets::Batch::RowLocation());
  applyGroupStyling(location);
  // TODO: Consider using the other version of appendChildRowOf
}

void BatchPresenter::insertEmptyGroupInModel(int beforeGroup) {
  insertEmptyGroup(m_model, beforeGroup);
}

void BatchPresenter::insertEmptyRowInModel(int groupIndex, int beforeRow) {
  insertEmptyRow(m_model, groupIndex, beforeRow);
}

void BatchPresenter::insertEmptyGroupInView(int beforeGroup) {
  auto location = m_view->jobs().insertChildRowOf(
      MantidQt::MantidWidgets::Batch::RowLocation(), beforeGroup);
  applyGroupStyling(location);
}

void BatchPresenter::notifyExpandAllRequested() { m_view->jobs().expandAll(); }

void BatchPresenter::notifyCollapseAllRequested() {
  m_view->jobs().collapseAll();
}

std::vector<std::string> BatchPresenter::mapToContentText(
    std::vector<MantidWidgets::Batch::Cell> const &cells) const {
  std::vector<std::string> cellText;
  cellText.reserve(cells.size());
  std::transform(cells.cbegin(), cells.cend(), std::back_inserter(cellText),
                 [](MantidWidgets::Batch::Cell const &cell)
                     -> std::string { return cell.contentText(); });
  return cellText;
}

std::vector<std::string> BatchPresenter::cellTextFromViewAt(
    MantidWidgets::Batch::RowLocation const &location) const {
  return mapToContentText(m_view->jobs().cellsAt(location));
}

void BatchPresenter::showAllCellsOnRowAsValid(
    MantidWidgets::Batch::RowLocation const &itemIndex) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  for (auto &&cell : cells) {
    cell.setIconFilePath("");
    cell.setBorderColor("darkGrey");
  }
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void BatchPresenter::showCellsAsInvalidInView(
    MantidWidgets::Batch::RowLocation const &itemIndex,
    std::vector<int> const &invalidColumns) {
  auto cells = m_view->jobs().cellsAt(itemIndex);
  for (auto &&cell : cells) {
    cell.setIconFilePath("");
    cell.setBorderColor("darkGrey");
  }
  for (auto &&column : invalidColumns) {
    cells[column].setIconFilePath(":/invalid.png");
    cells[column].setBorderColor("darkRed");
  }
  m_view->jobs().setCellsAt(itemIndex, cells);
}

void BatchPresenter::notifyCellTextChanged(
    MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {
  if (isGroupLocation(itemIndex)) {
    auto const groupIndex = groupOf(itemIndex);
    if (!setGroupName(m_model, groupIndex, newValue)) {
      auto cell = m_view->jobs().cellAt(itemIndex, column);
      cell.setContentText(oldValue);
      m_view->jobs().setCellAt(itemIndex, column, cell);
    }
  } else {
    auto const groupIndex = groupOf(itemIndex);
    auto const rowIndex = rowOf(itemIndex);
    auto slicing = Slicing(boost::blank());
    auto rowValidationResult =
        validateRow(m_model, slicing, cellTextFromViewAt(itemIndex));
    updateRow(m_model, groupIndex, rowIndex,
              rowValidationResult.validRowElseNone());
    if (rowValidationResult.isValid()) {
      showAllCellsOnRowAsValid(itemIndex);
    } else {
      showCellsAsInvalidInView(itemIndex, rowValidationResult.invalidColumns());
    }
  }
}

bool BatchPresenter::isGroupLocation(
    MantidQt::MantidWidgets::Batch::RowLocation const &location) const {
  return location.depth() == 1;
}

bool BatchPresenter::isRowLocation(
    MantidWidgets::Batch::RowLocation const &location) const {
  return location.depth() == 2;
}

int BatchPresenter::groupOf(
    MantidWidgets::Batch::RowLocation const &location) const {
  return location.path()[0];
}

int BatchPresenter::rowOf(
    MantidWidgets::Batch::RowLocation const &location) const {
  return location.path()[1];
}

bool BatchPresenter::containsGroups(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations)
    const {
  return std::count_if(
             locations.cbegin(), locations.cend(),
             [this](MantidQt::MantidWidgets::Batch::RowLocation const &location)
                 -> bool { return isGroupLocation(location); }) > 0;
}

void BatchPresenter::applyGroupStyling(
    MantidWidgets::Batch::RowLocation const &location) {
  auto cells = m_view->jobs().cellsAt(location);
  for (auto i = 1u; i < cells.size(); ++i) {
    cells[i] = m_view->jobs().deadCell();
  }
  m_view->jobs().setCellsAt(location, cells);
}

void BatchPresenter::notifyRowInserted(
    MantidQt::MantidWidgets::Batch::RowLocation const &newRowLocation) {
  if (newRowLocation.depth() > DEPTH_LIMIT) {
    m_view->jobs().removeRowAt(newRowLocation);
  } else if (isGroupLocation(newRowLocation)) {
    insertEmptyGroupInModel(groupOf(newRowLocation));
    applyGroupStyling(newRowLocation);
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

void BatchPresenter::notifyCopyRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized())
    m_view->jobs().clearSelection();
  else
    m_view->invalidSelectionForCopy();
}

void BatchPresenter::notifyCutRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized()) {
    m_view->jobs().removeRows(m_view->jobs().selectedRowLocations());
    m_view->jobs().clearSelection();
  } else {
    m_view->invalidSelectionForCut();
  }
}

void BatchPresenter::notifyPasteRowsRequested() {
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
}
}
