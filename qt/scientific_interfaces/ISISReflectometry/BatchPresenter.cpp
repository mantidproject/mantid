#include "BatchPresenter.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "Reduction/Group.h"
#include "ValidateRow.h"

#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

BatchPresenterFactory::BatchPresenterFactory(
    std::vector<std::string> const &instruments)
    : m_instruments(instruments) {}

std::unique_ptr<BatchPresenter> BatchPresenterFactory::
operator()(IBatchView *view) const {
  return std::make_unique<BatchPresenter>(view, m_instruments, ReductionJobs());
}

BatchPresenter::BatchPresenter(IBatchView *view,
                               std::vector<std::string> const &instruments,
                               ReductionJobs reductionJobs)
    : m_view(view), m_instruments(instruments),
      m_model(std::move(reductionJobs)) {
  m_view->subscribe(this);
}

ReductionJobs const& BatchPresenter::reductionJobs() const {
  return m_model;
}

void BatchPresenter::removeRowsFromModel(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> rows) {
  std::sort(rows.begin(), rows.end());
  for (auto row = rows.crbegin(); row != rows.crend(); ++row) {
    auto const groupIndex = groupOf(*row);
    auto const rowIndex = rowOf(*row);
    m_model.groups()[groupIndex].removeRow(rowIndex);
  }
}

void BatchPresenter::notifyDeleteRowRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (!selected.empty()) {
    if (!containsGroups(selected)) {
      removeRowsAndGroupsFromView(selected);
      removeRowsFromModel(selected);
    } else {
      // TODO: m_view->mustNotSelectGroupError();
    }
  } else {
    // TODO: m_view->mustSelectRowError();
  }
}

void BatchPresenter::notifyDeleteGroupRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groupIndicesOrderedLowToHigh = groupIndexesFromSelection(selected);
    removeGroupsFromModel(groupIndicesOrderedLowToHigh);
    removeGroupsFromView(groupIndicesOrderedLowToHigh);
  } else {
    // TODO: m_view->mustSelectGroupOrRowError();
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
    m_model.removeGroup(*it);
}

void BatchPresenter::notifyPauseRequested() {}

void BatchPresenter::notifyProcessRequested() {
  for (auto &&group : m_model.groups()) {
    std::cout << "Group (" << group.name() << ")\n";
    for (auto &&row : group.rows()) {
      if (row.is_initialized()) {
        if (row.get().runNumbers().empty())
          std::cout << "  Row (empty)\n";
        else
          std::cout << "  Row (run number: " << row.get().runNumbers()[0]
                    << ")\n";
      } else {
        std::cout << "  Row (invalid)\n";
      }
    }
  }
  std::cout << std::endl;
}

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
    // TODO: m_view->mustSelectGroupError();
  }
}

void BatchPresenter::appendRowsToGroupsInView(
    std::vector<int> const &groupIndices) {
  for (auto &&groupIndex : groupIndices)
    m_view->jobs().appendChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}));
}

void BatchPresenter::appendRowsToGroupsInModel(
    std::vector<int> const &groupIndices) {
  for (auto &&groupIndex : groupIndices)
    m_model.groups()[groupIndex].appendEmptyRow();
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

void BatchPresenter::appendEmptyGroupInModel() {
  m_model.appendGroup(UnslicedGroup(
      std::string(), std::vector<boost::optional<SingleRow>>(), std::string()));
}

void BatchPresenter::appendEmptyGroupInView() {
  auto location = m_view->jobs().appendChildRowOf(
      MantidQt::MantidWidgets::Batch::RowLocation());
  applyGroupStyling(location);
  // TODO: Consider using the other version of appendChildRowOf
}

void BatchPresenter::insertEmptyGroupInModel(int beforeGroup) {
  m_model.insertGroup(UnslicedGroup(std::string(),
                                    std::vector<boost::optional<SingleRow>>(),
                                    std::string()),
                      beforeGroup);
}

void BatchPresenter::insertEmptyRowInModel(int groupIndex, int beforeRow) {
  m_model.groups()[groupIndex].insertRow(boost::none, beforeRow);
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
    m_model.groups()[groupIndex].setName(newValue);
  } else {
    auto const groupIndex = groupOf(itemIndex);
    auto const rowIndex = rowOf(itemIndex);
    auto rowValidationResult =
        validateRow<SingleRow>(cellTextFromViewAt(itemIndex));
    m_model.groups()[groupIndex].updateRow(
        rowIndex, rowValidationResult.validRowElseNone());
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
      m_model.groups()[groupIndex].removeRow(rowIndex);
    } else if (isGroupLocation(*location)) {
      m_model.removeGroup(groupIndex);
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
    // TODO: m_view->invalidSelectionForCopy();
    return;
}

void BatchPresenter::notifyCutRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized()) {
    m_view->jobs().removeRows(m_view->jobs().selectedRowLocations());
    m_view->jobs().clearSelection();
  } else {
    // TODO: m_view->invalidSelectionForCut();
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
    // TODO: m_view->invalidSelectionForPaste();
  }
}

void BatchPresenter::notifyFilterReset() {}
}
}
