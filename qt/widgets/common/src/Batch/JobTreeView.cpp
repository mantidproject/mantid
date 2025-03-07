// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/BuildSubtreeItems.h"
#include "MantidQtWidgets/Common/Batch/CellDelegate.h"
#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include "MantidQtWidgets/Common/Batch/FindSubtreeRoots.h"
#include "MantidQtWidgets/Common/Batch/QtBasicNavigation.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include <QKeyEvent>
#include <QStandardItemModel>
#include <algorithm>

namespace {
QAbstractItemView::EditTriggers getEditTriggers() {
  auto trigger =
      QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed;
  return trigger;
}
} // namespace

namespace MantidQt::MantidWidgets::Batch {

JobTreeView::JobTreeView(QStringList const &columnHeadings, Cell const &emptyCellStyle, QWidget *parent)
    : QTreeView(parent), m_notifyee(nullptr), m_mainModel(0, columnHeadings.size(), this),
      m_adaptedMainModel(m_mainModel, emptyCellStyle), m_filteredModel(RowLocationAdapter(m_mainModel), this),
      m_lastEdited(QModelIndex()) {
  setModel(&m_mainModel);
  setHeaderLabels(columnHeadings);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setEditTriggers(getEditTriggers());
  setItemDelegate(new CellDelegate(this, *this, m_filteredModel, m_mainModel));
  setContextMenuPolicy(Qt::ActionsContextMenu);
  enableFiltering();
}

void JobTreeView::commitData(QWidget *editor) {
  auto current_filtered_index = fromFilteredModel(currentIndex());
  auto cellTextBefore = m_adaptedMainModel.cellFromCellIndex(m_lastEdited).contentText();
  QTreeView::commitData(editor);
  auto cellText = m_adaptedMainModel.cellFromCellIndex(m_lastEdited).contentText();
  // cppcheck-suppress knownConditionTrueFalse
  if (cellText != cellTextBefore) {
    resizeColumnToContents(m_lastEdited.column());
    m_hasEditorOpen = false;
    m_notifyee->notifyCellTextChanged(rowLocation().atIndex(m_lastEdited), m_lastEdited.column(), cellTextBefore,
                                      cellText);
    editAt(current_filtered_index);
  }
}

void JobTreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  QTreeView::selectionChanged(selected, deselected);
  if (!m_notifyee)
    return;
  m_notifyee->notifySelectionChanged();
}

void JobTreeView::filterRowsBy(std::unique_ptr<RowPredicate> predicate) {
  m_filteredModel.setPredicate(std::move(predicate));
  expandAll();
}

void JobTreeView::filterRowsBy(RowPredicate *predicate) {
  m_filteredModel.setPredicate(std::unique_ptr<RowPredicate>(predicate));
  expandAll();
}

void JobTreeView::resetFilter() {
  m_filteredModel.resetPredicate();
  m_notifyee->notifyFilterReset();
}

bool JobTreeView::hasFilter() const { return m_filteredModel.isReset(); }

bool JobTreeView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) {
  m_lastEdited = mapToMainModel(fromFilteredModel(index));
  m_hasEditorOpen = true;
  return QTreeView::edit(index, trigger, event);
}

Cell JobTreeView::deadCell() const { return g_deadCell; }

boost::optional<std::vector<Subtree>> JobTreeView::selectedSubtrees() const {
  auto selected = selectedRowLocations();
  std::sort(selected.begin(), selected.end());

  auto selectedRows = std::vector<Row>();
  selectedRows.reserve(selected.size());

  std::transform(selected.cbegin(), selected.cend(), std::back_inserter(selectedRows),
                 [&](RowLocation const &location) -> Row { return Row(location, cellsAt(location)); });

  auto extractSubtrees = ExtractSubtrees();
  return extractSubtrees(selectedRows);
}

boost::optional<std::vector<RowLocation>> JobTreeView::selectedSubtreeRoots() const {
  auto findSubtreeRoots = FindSubtreeRoots();
  return findSubtreeRoots(selectedRowLocations());
}

bool JobTreeView::hasNoSelectedDescendants(QModelIndex const &index) const {
  for (auto row = 0; row < m_filteredModel.rowCount(index); ++row) {
    auto childIndex = m_filteredModel.index(row, 0, index);
    if (selectionModel()->isSelected(childIndex))
      return false;
    else if (!hasNoSelectedDescendants(childIndex))
      return false;
  }
  return true;
}

void JobTreeView::appendAllUnselectedDescendants(QModelIndexList &descendantsList, QModelIndex const &index) const {
  for (auto row = 0; row < m_filteredModel.rowCount(index); ++row) {
    auto childIndex = m_filteredModel.index(row, 0, index);
    if (!selectionModel()->isSelected(childIndex))
      descendantsList.append(childIndex);
    appendAllUnselectedDescendants(descendantsList, childIndex);
  }
}

void JobTreeView::setHintsForColumn(int column, std::unique_ptr<HintStrategy> hintStrategy) {
  setItemDelegateForColumn(column, new HintingLineEditFactory(itemDelegate(), std::move(hintStrategy)));
}

void JobTreeView::setHintsForColumn(int column, HintStrategy *hintStrategy) {
  setHintsForColumn(column, std::unique_ptr<HintStrategy>(hintStrategy));
}

QModelIndexList JobTreeView::findImplicitlySelected(QModelIndexList const &selectedRows) const {
  auto implicitlySelected = QModelIndexList();
  for (auto &&row : selectedRows) {
    if (isExpanded(row)) {
      if (hasNoSelectedDescendants(row))
        appendAllUnselectedDescendants(implicitlySelected, row);
    } else {
      appendAllUnselectedDescendants(implicitlySelected, row);
    }
  }
  return implicitlySelected;
}

std::vector<RowLocation> JobTreeView::selectedRowLocations() const {
  auto selection = selectionModel()->selectedRows();
  selection.append(findImplicitlySelected(selection));

  std::vector<RowLocation> rowSelection;
  rowSelection.reserve(selection.size());
  std::transform(selection.begin(), selection.end(), std::back_inserter(rowSelection),
                 [&](QModelIndex const &index) -> RowLocation {
                   auto indexForMainModel = mapToMainModel(fromFilteredModel(index));
                   return rowLocation().atIndex(indexForMainModel);
                 });
  return rowSelection;
}

void JobTreeView::appendAndEditAtChildRowRequested() { m_notifyee->notifyAppendAndEditAtChildRowRequested(); }

void JobTreeView::appendAndEditAtRowBelowRequested() { m_notifyee->notifyAppendAndEditAtRowBelowRequested(); }

void JobTreeView::editAtRowAboveRequested() { m_notifyee->notifyEditAtRowAboveRequested(); }

void JobTreeView::removeSelectedRequested() { m_notifyee->notifyRemoveRowsRequested(selectedRowLocations()); }

void JobTreeView::copySelectedRequested() { m_notifyee->notifyCopyRowsRequested(); }

void JobTreeView::cutSelectedRequested() { m_notifyee->notifyCutRowsRequested(); }

void JobTreeView::pasteSelectedRequested() { m_notifyee->notifyPasteRowsRequested(); }

void JobTreeView::removeRows(std::vector<RowLocation> rowsToRemove) {
  std::sort(rowsToRemove.begin(), rowsToRemove.end());
  for (auto rowIt = rowsToRemove.crbegin(); rowIt < rowsToRemove.crend(); ++rowIt)
    removeRowAt(*rowIt);
}

Cell JobTreeView::cellAt(RowLocation location, int column) const {
  auto const cellIndex = rowLocation().indexAt(location, column);
  return m_adaptedMainModel.cellFromCellIndex(cellIndex);
}

bool JobTreeView::isBeingEdited(QModelIndexForFilteredModel const &cellIndex) const {
  return hasEditorOpen() && fromFilteredModel(currentIndex()) == cellIndex;
}

void JobTreeView::closeEditorIfOpenAtCell(QModelIndexForFilteredModel const &cellIndex) {
  if (cellIndex.isValid() && isBeingEdited(cellIndex))
    closePersistentEditor(cellIndex.untyped());
}

void JobTreeView::closeEditorIfCellIsUneditable(QModelIndexForMainModel const &cellIndex, Cell const &cell) {
  if (!cell.isEditable())
    closeEditorIfOpenAtCell(mapToFilteredModel(cellIndex));
}

void JobTreeView::setCellAt(RowLocation location, int column, Cell const &cell) {
  auto const cellIndex = rowLocation().indexAt(location, column);
  m_adaptedMainModel.setCellAtCellIndex(cellIndex, cell);
  closeEditorIfCellIsUneditable(cellIndex, cell);
}

std::vector<Cell> JobTreeView::cellsAt(RowLocation const &location) const {
  return m_adaptedMainModel.cellsAtRow(rowLocation().indexAt(location));
}

void JobTreeView::closeAnyOpenEditorsOnUneditableCells(QModelIndexForMainModel const &firstCellOnRow,
                                                       std::vector<Cell> const &cells) {
  m_adaptedMainModel.enumerateCellsInRow(firstCellOnRow, m_mainModel.columnCount(),
                                         [&](QModelIndexForMainModel const &cellIndex, int column) -> void {
                                           closeEditorIfCellIsUneditable(cellIndex, cells[column]);
                                         });
}

void JobTreeView::setCellsAt(RowLocation const &location, std::vector<Cell> const &cells) {
  auto firstCellOnRow = rowLocation().indexAt(location);
  m_adaptedMainModel.setCellsAtRow(firstCellOnRow, cells);
  closeAnyOpenEditorsOnUneditableCells(firstCellOnRow, cells);
}

void JobTreeView::replaceSubtreeAt(RowLocation const &rootToRemove, Subtree const &toInsert) {
  auto const insertionParent = rootToRemove.parent();
  auto insertionIndex = rootToRemove.rowRelativeToParent();
  // Insert the new row first (to avoid possibility of an empty table, which
  // will cause problems because it is not allowed and will insert an empty
  // group which we don't want)
  insertSubtreeAt(insertionParent, insertionIndex, toInsert);
  // Now find the new index of the root we're replacing and remove it. It is
  // now the sibling below.
  auto originalRootIndexToRemove = rowLocation().indexAt(rootToRemove);
  auto newRootIndexToRemove = below(originalRootIndexToRemove.untyped());
  removeRowAt(rowLocation().atIndex(fromMainModel(newRootIndexToRemove)));
}

void JobTreeView::insertSubtreeAt(RowLocation const &parent, int index, Subtree const &subtree) {
  auto build = BuildSubtreeItems(m_adaptedMainModel, rowLocation());
  build(parent, index, subtree);
}

void JobTreeView::appendSubtreesAt(RowLocation const &parent, std::vector<Subtree> subtrees) {
  for (auto &&subtree : subtrees)
    appendSubtreeAt(parent, subtree);
}

void JobTreeView::appendSubtreeAt(RowLocation const &parent, Subtree const &subtree) {
  auto parentIndex = rowLocation().indexAt(parent);
  insertSubtreeAt(parent, m_mainModel.rowCount(parentIndex.untyped()), subtree);
}

void JobTreeView::replaceRows(std::vector<RowLocation> replacementPoints, std::vector<Subtree> replacements) {
  assertOrThrow(replacementPoints.size() > 0, "replaceRows: Passed an empty list of replacement points."
                                              "At least one replacement point is required.");
  auto replacementPoint = replacementPoints.cbegin();
  auto replacement = replacements.cbegin();

  for (; replacementPoint != replacementPoints.cend() && replacement != replacements.cend();
       ++replacementPoint, ++replacement) {
    replaceSubtreeAt(*replacementPoint, *replacement);
  }

  if (replacementPoints.size() > replacements.size())
    for (; replacementPoint != replacementPoints.cend(); ++replacementPoint)
      removeRowAt(*replacementPoint);
  else if (replacementPoints.size() < replacements.size())
    for (; replacement != replacements.cend(); ++replacement)
      appendSubtreeAt(replacementPoints.back().parent(), *replacement);
}

void JobTreeView::setHeaderLabels(QStringList const &columnHeadings) {
  m_mainModel.setHorizontalHeaderLabels(columnHeadings);

  for (auto i = 0; i < model()->columnCount(); ++i)
    resizeColumnToContents(i);
}

void JobTreeView::subscribe(JobTreeViewSubscriber *subscriber) { m_notifyee = subscriber; }

bool JobTreeView::hasEditorOpen() const { return m_lastEdited.isValid(); }

bool JobTreeView::rowRemovalWouldBeIneffective(QModelIndexForMainModel const &indexToRemove) const {
  return areOnSameRow(mapToMainModel(fromFilteredModel(currentIndex())).untyped(), indexToRemove.untyped()) &&
         hasEditorOpen();
}

QModelIndex JobTreeView::siblingIfExistsElseParent(QModelIndex const &index) const {
  if (hasRowAbove(index)) {
    return above(index);
  } else if (hasRowBelow(index)) {
    return below(index);
  } else {
    return index.parent();
  }
}

bool JobTreeView::isOnlyChild(QModelIndexForMainModel const &index) const {
  auto parentIndex = index.parent();
  return m_mainModel.rowCount(parentIndex.untyped()) == 1;
}

bool JobTreeView::isOnlyChildOfRoot(RowLocation const &index) const {
  return isOnlyChildOfRoot(rowLocation().indexAt(index));
}

bool JobTreeView::isOnlyChildOfRoot(QModelIndexForMainModel const &index) const {
  return !index.parent().isValid() && isOnlyChild(index);
}

void JobTreeView::removeRowAt(RowLocation const &location) {
  auto indexToRemove = rowLocation().indexAt(location);
  assertOrThrow(indexToRemove.isValid(), "removeRowAt: Attempted to remove the invisible root item.");
  // We can't delete the only child of the invisible root
  // for the main model unless we use removeAllRows()
  if (isOnlyChildOfRoot(indexToRemove)) {
    removeAllRows();
    return;
  }

  if (rowRemovalWouldBeIneffective(indexToRemove)) {
    // implies that indexToRemove corresponds to an index in the filtered model.
    auto rowIndexToSwitchTo = siblingIfExistsElseParent(mapToFilteredModel(indexToRemove).untyped());
    if (rowIndexToSwitchTo.isValid()) {
      setCurrentIndex(rowIndexToSwitchTo);
    } else {
      resetFilter();
      rowIndexToSwitchTo = siblingIfExistsElseParent(mapToFilteredModel(indexToRemove).untyped());
      setCurrentIndex(rowIndexToSwitchTo);
    }
  }
  m_adaptedMainModel.removeRowFrom(indexToRemove);
}

void JobTreeView::removeAllRows() {
  appendChildRowOf({});
  auto firstChild = std::vector<int>{0};
  while (!isOnlyChildOfRoot(firstChild)) {
    removeRowAt(firstChild);
  }
  clearSelection();
}

RowLocation JobTreeView::insertChildRowOf(RowLocation const &parent, int beforeRow) {
  auto const child = m_adaptedMainModel.insertEmptyChildRow(rowLocation().indexAt(parent), beforeRow);
  editAt(expanded(mapToFilteredModel(child)));
  return rowLocation().atIndex(child);
}

RowLocation JobTreeView::insertChildRowOf(RowLocation const &parent, int beforeRow, std::vector<Cell> const &cells) {
  assertOrThrow(static_cast<int>(cells.size()) <= m_mainModel.columnCount(),
                "Attempted to add row with more cells than columns. Increase "
                "the number of columns by increasing the number of headings.");
  auto child = m_adaptedMainModel.insertChildRow(rowLocation().indexAt(parent), beforeRow,
                                                 paddedCellsToWidth(cells, g_deadCell, m_mainModel.columnCount()));
  editAt(expanded(mapToFilteredModel(child)));
  return rowLocation().atIndex(child);
}

Cell const JobTreeView::g_deadCell = Cell("", "white", 0, "transparent", 0, false);

RowLocation JobTreeView::appendChildRowOf(RowLocation const &parent) {
  return rowLocation().atIndex(m_adaptedMainModel.appendEmptyChildRow(rowLocation().indexAt(parent)));
}

RowLocation JobTreeView::appendChildRowOf(RowLocation const &parent, std::vector<Cell> const &cells) {
  auto parentIndex = rowLocation().indexAt(parent);
  return rowLocation().atIndex(
      m_adaptedMainModel.appendChildRow(parentIndex, paddedCellsToWidth(cells, g_deadCell, m_mainModel.columnCount())));
}

void JobTreeView::editAt(QModelIndexForFilteredModel const &index) {
  if (isEditable(index.untyped())) {
    QTreeView::clearSelection();
    setCurrentIndex(index.untyped());
    edit(index.untyped());
  }
}

void JobTreeView::clearSelection() { QTreeView::clearSelection(); }

void JobTreeView::expandAll() { QTreeView::expandAll(); }

void JobTreeView::collapseAll() { QTreeView::collapseAll(); }

QModelIndexForFilteredModel JobTreeView::expanded(QModelIndexForFilteredModel const &index) {
  auto expandAt = index.untyped();
  while (expandAt.isValid()) {
    setExpanded(expandAt, true);
    expandAt = m_filteredModel.parent(expandAt);
  }
  return index;
}

QtTreeCursorNavigation JobTreeView::navigation() const { return QtTreeCursorNavigation(&m_filteredModel); }

RowLocationAdapter JobTreeView::rowLocation() const { return RowLocationAdapter(m_mainModel); }

std::pair<QModelIndexForFilteredModel, bool>
JobTreeView::findOrMakeCellBelow(QModelIndexForFilteredModel const &index) {
  if (hasRowBelow(index.untyped())) {
    return std::make_pair(fromFilteredModel(below(index.untyped())), false);
  } else {
    auto indexForModel = mapToMainModel(index);
    resetFilter();
    auto newIndex = m_adaptedMainModel.appendEmptySiblingRow(indexForModel);
    return std::make_pair(mapToFilteredModel(newIndex), true);
  }
}

QModelIndexForMainModel JobTreeView::mapToMainModel(QModelIndexForFilteredModel const &filteredModelIndex) const {
  return QModelIndexForMainModel(m_filteredModel.mapToSource(filteredModelIndex.untyped()));
}

QModelIndexForFilteredModel JobTreeView::mapToFilteredModel(QModelIndexForMainModel const &mainModelIndex) const {
  return QModelIndexForFilteredModel(m_filteredModel.mapFromSource(mainModelIndex.untyped()));
}

void JobTreeView::appendAndEditAtChildRow() {
  resetFilter();
  auto const parent = mapToMainModel(fromFilteredModel(currentIndex()));
  auto const child = m_adaptedMainModel.appendEmptyChildRow(parent);
  m_notifyee->notifyRowInserted(rowLocation().atIndex(child));
  editAt(expanded(mapToFilteredModel(child)));
}

void JobTreeView::appendAndEditAtRowBelow() {
  auto current = currentIndex();
  setCurrentIndex(QModelIndex());
  setCurrentIndex(current);
  if (current != m_mainModel.index(-1, -1)) {
    auto const cell = findOrMakeCellBelow(fromFilteredModel(current));
    auto index = cell.first;
    auto isNew = cell.second;

    if (isNew) {
      m_notifyee->notifyRowInserted(rowLocation().atIndex(mapToMainModel(index)));
    }
    editAt(index);
  }
}

void JobTreeView::editAtRowAbove() {
  if (hasRowAbove(currentIndex()))
    editAt(fromFilteredModel(above(currentIndex())));
}

void JobTreeView::enableFiltering() {
  m_filteredModel.setDynamicSortFilter(false);
  m_filteredModel.setSourceModel(&m_mainModel);
  setModel(&m_filteredModel);
}

void JobTreeView::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_I:
    if (event->modifiers() & Qt::ControlModifier) {
      appendAndEditAtChildRowRequested();
    }
    break;
  case Qt::Key_Return:
  case Qt::Key_Enter: {
    if (event->modifiers() & Qt::ShiftModifier) {
      editAtRowAboveRequested();
    } else {
      appendAndEditAtRowBelowRequested();
    }
    break;
  }
  case Qt::Key_Delete:
    removeSelectedRequested();
    break;
  case Qt::Key_C: {
    if (event->modifiers() & Qt::ControlModifier) {
      copySelectedRequested();
    }
    break;
  }
  case Qt::Key_V: {
    if (event->modifiers() & Qt::ControlModifier) {
      pasteSelectedRequested();
    }
    break;
  }
  case Qt::Key_X: {
    if (event->modifiers() & Qt::ControlModifier) {
      cutSelectedRequested();
    }
    break;
  }
  default:
    QTreeView::keyPressEvent(event);
  }
}

QModelIndexForMainModel JobTreeView::fromMainModel(QModelIndex const &mainModelIndex) const {
  return ::MantidQt::MantidWidgets::Batch::fromMainModel(mainModelIndex, m_mainModel);
}

QModelIndexForFilteredModel JobTreeView::fromFilteredModel(QModelIndex const &filteredModelIndex) const {
  return ::MantidQt::MantidWidgets::Batch::fromFilteredModel(filteredModelIndex, m_filteredModel);
}

bool JobTreeView::isEditable(QModelIndex const &index) const { return index.flags() & Qt::ItemIsEditable; }

QtTreeCursorNavigationResult JobTreeView::moveNextUntilEditable(QModelIndex const &startingPoint) {
  auto result = navigation().moveCursorNext(startingPoint);
  while (!result.first && result.second.isValid() && !isEditable(result.second))
    result = navigation().moveCursorNext(result.second);
  return result;
}

QModelIndex JobTreeView::movePreviousUntilEditable(QModelIndex const &startingPoint) {
  auto currentIndex = navigation().moveCursorPrevious(startingPoint);
  while (currentIndex.isValid() && !isEditable(currentIndex))
    currentIndex = navigation().moveCursorPrevious(currentIndex);
  return currentIndex;
}

QModelIndex JobTreeView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) {
  if (cursorAction == QAbstractItemView::MoveNext)
    return applyNavigationResult(moveNextUntilEditable(currentIndex()));
  else if (cursorAction == QAbstractItemView::MovePrevious)
    return movePreviousUntilEditable(currentIndex());
  else
    return QTreeView::moveCursor(cursorAction, modifiers);
}

QModelIndex JobTreeView::applyNavigationResult(QtTreeCursorNavigationResult const &result) {
  auto shouldMakeNewRowBelow = result.first;
  if (shouldMakeNewRowBelow) {
    // `newCellIndex` is the model index of the cell in the new row with a
    // column which matches
    // the column currently selected by the user. To correctly get the
    // RowLocation we need the
    // model index of the first cell in the new row.
    auto newCellIndex = m_adaptedMainModel.appendEmptySiblingRow(mapToMainModel(fromFilteredModel(result.second)));
    auto newRowIndex = fromMainModel(firstCellOnRowOf(newCellIndex.untyped()));

    // Resetting the filter ensures that the new row is visible and has a
    // corresponding index in
    // the filtered model.
    resetFilter();

    auto newRowLocation = rowLocation().atIndex(newRowIndex);
    m_notifyee->notifyRowInserted(newRowLocation);

    // The subscriber is entitled to remove the row at `newRowIndex` when we
    // call
    // `notifyRowInserted` hence we have to assume they did and try to get the
    // index again.
    auto maybeIndexOfNewRow = rowLocation().indexIfExistsAt(newRowLocation);
    if (maybeIndexOfNewRow.is_initialized()) {
      return expanded(mapToFilteredModel(maybeIndexOfNewRow.get())).untyped();
    } else {
      return QModelIndex();
    }
  } else {
    return result.second;
  }
}

int JobTreeView::currentColumn() const { return currentIndex().column(); }

} // namespace MantidQt::MantidWidgets::Batch
