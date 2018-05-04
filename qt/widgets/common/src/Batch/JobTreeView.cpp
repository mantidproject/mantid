#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/CellDelegate.h"
#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include "MantidQtWidgets/Common/Batch/FindSubtreeRoots.h"
#include "MantidQtWidgets/Common/Batch/QtBasicNavigation.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include "MantidQtWidgets/Common/Batch/BuildSubtreeItems.h"
#include <QKeyEvent>
#include <QStandardItemModel>
#include <algorithm>
#include <iostream>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

JobTreeView::JobTreeView(QStringList const &columnHeadings,
                         Cell const &emptyCellStyle, QWidget *parent)
    : QTreeView(parent), m_mainModel(0, columnHeadings.size(), this),
      m_adaptedMainModel(m_mainModel, emptyCellStyle),
      m_filteredModel(RowLocationAdapter(m_mainModel), this),
      m_lastEdited(QModelIndex()) {

  setModel(&m_mainModel);
  setHeaderLabels(columnHeadings);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setItemDelegate(new CellDelegate(this, *this, m_filteredModel, m_mainModel));
  enableFiltering();
}

void JobTreeView::commitData(QWidget *editor) {
  QTreeView::commitData(editor);

  auto location = rowLocation().atIndex(m_lastEdited);
  auto *const cell = modelItemFromIndex(m_mainModel, m_lastEdited);
  auto cellText = cell->text().toStdString();

  m_notifyee->notifyCellChanged(location, m_lastEdited.column(), cellText);
  m_hasEditorOpen = false;
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
  m_notifyee->notifyFilterReset();
  m_filteredModel.resetPredicate();
}

bool JobTreeView::hasFilter() const { return m_filteredModel.isReset(); }

bool JobTreeView::edit(const QModelIndex &index, EditTrigger trigger,
                       QEvent *event) {
  m_lastEdited = mapToMainModel(fromFilteredModel(index));
  m_hasEditorOpen = true;
  return QTreeView::edit(index, trigger, event);
}

boost::optional<std::vector<Subtree>> JobTreeView::selectedSubtrees() const {
  auto rows = selectedRowLocations();
  std::sort(rows.begin(), rows.end());

  auto selectedRowData = std::vector<std::vector<Cell>>();
  selectedRowData.reserve(rows.size());

  std::transform(rows.cbegin(), rows.cend(),
                 std::back_inserter(selectedRowData),
                 [&](RowLocation const &location)
                     -> std::vector<Cell> { return cellsAt(location); });

  auto extractSubtrees = ExtractSubtrees();
  return extractSubtrees(rows, selectedRowData);
}

boost::optional<std::vector<RowLocation>>
JobTreeView::selectedSubtreeRoots() const {
  auto findSubtreeRoots = FindSubtreeRoots();
  return findSubtreeRoots(selectedRowLocations());
}

std::vector<RowLocation> JobTreeView::selectedRowLocations() const {
  auto selection = selectionModel()->selectedRows();
  std::vector<RowLocation> rowSelection;
  rowSelection.reserve(selection.size());
  std::transform(
      selection.begin(), selection.end(), std::back_inserter(rowSelection),
      [&](QModelIndex const &index) -> RowLocation {
        auto indexForMainModel = mapToMainModel(fromFilteredModel(index));
        return rowLocation().atIndex(indexForMainModel);
      });
  return rowSelection;
}

void JobTreeView::removeSelectedRequested() {
  m_notifyee->notifyRemoveRowsRequested(selectedRowLocations());
}

void JobTreeView::copySelectedRequested() {
  m_notifyee->notifyCopyRowsRequested();
}

void JobTreeView::pasteSelectedRequested() {
  m_notifyee->notifyPasteRowsRequested();
}

void JobTreeView::removeRows(std::vector<RowLocation> rowsToRemove) {
  std::sort(rowsToRemove.begin(), rowsToRemove.end());
  for (auto rowIt = rowsToRemove.crbegin(); rowIt < rowsToRemove.crend();
       ++rowIt)
    removeRowAt(*rowIt);
}

Cell JobTreeView::cellAt(RowLocation location, int column) const {
  auto const cellIndex = rowLocation().indexAt(location, column);
  return m_adaptedMainModel.cellFromCellIndex(cellIndex);
}

void JobTreeView::setCellAt(RowLocation location, int column,
                            Cell const &cell) {
  auto const cellIndex = rowLocation().indexAt(location, column);
  m_adaptedMainModel.setCellAtCellIndex(cellIndex, cell);
}

std::vector<Cell> JobTreeView::cellsAt(RowLocation const &location) const {
  return m_adaptedMainModel.cellsAtRow(rowLocation().indexAt(location));
}

void JobTreeView::setCellsAt(RowLocation const &location,
                             std::vector<Cell> const &cells) {
  return m_adaptedMainModel.setCellsAtRow(rowLocation().indexAt(location),
                                          cells);
}

void JobTreeView::replaceSubtreeAt(RowLocation const &rootToRemove,
                                   Subtree const &toInsert) {
  auto const insertionParent = rootToRemove.parent();
  auto insertionIndex = rootToRemove.rowRelativeToParent();
  removeRowAt(rootToRemove);
  insertSubtreeAt(insertionParent, insertionIndex, toInsert);
}

void JobTreeView::insertSubtreeAt(RowLocation const &parent, int index,
                                  Subtree const &subtree) {
  auto build = BuildSubtreeItems(m_mainModel);
  auto parentIndex = rowLocation().indexAt(parent);
  build(modelItemFromIndex(m_mainModel, parentIndex), parent, index, subtree);
}

void JobTreeView::appendSubtreesAt(RowLocation const &parent,
                                   std::vector<Subtree> subtrees) {
  for (auto &&subtree : subtrees)
    appendSubtreeAt(parent, subtree);
}

void JobTreeView::appendSubtreeAt(RowLocation const &parent,
                                  Subtree const &subtree) {
  auto parentIndex = rowLocation().indexAt(parent);
  insertSubtreeAt(parent, model()->rowCount(parentIndex.untyped()), subtree);
}

void JobTreeView::replaceRows(std::vector<RowLocation> replacementPoints,
                              std::vector<Subtree> replacements) {
  assertOrThrow(replacementPoints.size() > 0,
                "replaceRows: Passed an empty list of replacement points."
                "At least one replacement point is required.");

  auto replacementPoint = replacementPoints.cbegin();
  auto replacement = replacements.cbegin();

  for (; replacementPoint != replacementPoints.cend() &&
             replacement != replacements.cend();
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

void JobTreeView::subscribe(JobTreeViewSubscriber &subscriber) {
  m_notifyee = &subscriber;
}

bool JobTreeView::hasEditorOpen() const { return m_lastEdited.isValid(); }

bool JobTreeView::rowRemovalWouldBeIneffective(
    QModelIndexForMainModel const &indexToRemove) const {
  return areOnSameRow(
             mapToMainModel(fromFilteredModel(currentIndex())).untyped(),
             indexToRemove.untyped()) &&
         hasEditorOpen();
}

QModelIndex
JobTreeView::siblingIfExistsElseParent(QModelIndex const &index) const {
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

bool JobTreeView::isOnlyChildOfRoot(
    QModelIndexForMainModel const &index) const {
  return !index.parent().isValid() && isOnlyChild(index);
}

void JobTreeView::removeRowAt(RowLocation const &location) {
  auto indexToRemove = rowLocation().indexAt(location);
  assertOrThrow(indexToRemove.isValid(),
                "removeRowAt: Attempted to remove the invisible root item.");
  assertOrThrow(!isOnlyChildOfRoot(indexToRemove),
                "Attempted to delete the only child of the invisible root"
                " for the main model. Try removeAllRows() instead.");
  if (rowRemovalWouldBeIneffective(indexToRemove)) {
    // implies that indexToRemove corresponds to an index in filtered model.
    auto rowIndexToSwitchTo =
        siblingIfExistsElseParent(mapToFilteredModel(indexToRemove).untyped());
    if (rowIndexToSwitchTo.isValid()) {
      setCurrentIndex(rowIndexToSwitchTo);
    } else {
      resetFilter();
      rowIndexToSwitchTo = siblingIfExistsElseParent(
          mapToFilteredModel(indexToRemove).untyped());
      setCurrentIndex(rowIndexToSwitchTo);
    }
  }
  m_adaptedMainModel.removeRowFrom(indexToRemove);
}

void JobTreeView::insertChildRowOf(RowLocation const &parent, int beforeRow) {
  m_adaptedMainModel.insertEmptyChildRow(rowLocation().indexAt(parent),
                                         beforeRow);
}

void JobTreeView::insertChildRowOf(RowLocation const &parent, int beforeRow,
                                   std::vector<Cell> const &cells) {
  assertOrThrow(cells.size() <= m_mainModel.columnCount(),
                "Attempted to add row with more cells than columns. Increase "
                "the number of columns by increasing the number of headings.");
  m_adaptedMainModel.insertChildRow(
      rowLocation().indexAt(parent), beforeRow,
      paddedRowFromCells(cells, deadCell, m_mainModel.columnCount()));
}

Cell const JobTreeView::deadCell =
    Cell("", "white", 0, "transparent", 0, false);

void JobTreeView::appendChildRowOf(RowLocation const &parent) {
  m_adaptedMainModel.appendEmptyChildRow(rowLocation().indexAt(parent));
}

void JobTreeView::appendChildRowOf(RowLocation const &parent,
                                   std::vector<Cell> const &cells) {
  auto parentIndex = rowLocation().indexAt(parent);
  m_adaptedMainModel.appendChildRow(
      parentIndex,
      paddedRowFromCells(cells, deadCell, m_mainModel.columnCount()));
}

void JobTreeView::editAt(QModelIndexForFilteredModel const &index) {
  clearSelection();
  setCurrentIndex(index.untyped());
  edit(index.untyped());
}

QModelIndexForFilteredModel
JobTreeView::expanded(QModelIndexForFilteredModel const &index) {
  auto expandAt = index.untyped();
  while (expandAt.isValid()) {
    setExpanded(expandAt, true);
    expandAt = m_filteredModel.parent(expandAt);
  }
  return index;
}

QtTreeCursorNavigation JobTreeView::navigation() const {
  return QtTreeCursorNavigation(&m_filteredModel);
}

RowLocationAdapter JobTreeView::rowLocation() const {
  return RowLocationAdapter(m_mainModel);
}

std::pair<QModelIndexForFilteredModel, bool>
JobTreeView::findOrMakeCellBelow(QModelIndexForFilteredModel const &index) {
  if (hasRowBelow(index.untyped()))
    return std::make_pair(fromFilteredModel(below(index.untyped())), false);
  else {
    auto indexForModel = mapToMainModel(index);
    resetFilter();
    auto newIndex = m_adaptedMainModel.appendEmptySiblingRow(indexForModel);
    return std::make_pair(mapToFilteredModel(newIndex), true);
  }
}

QModelIndexForMainModel JobTreeView::mapToMainModel(
    QModelIndexForFilteredModel const &filteredModelIndex) const {
  return QModelIndexForMainModel(
      m_filteredModel.mapToSource(filteredModelIndex.untyped()));
}

QModelIndexForFilteredModel JobTreeView::mapToFilteredModel(
    QModelIndexForMainModel const &mainModelIndex) const {
  return QModelIndexForFilteredModel(
      m_filteredModel.mapFromSource(mainModelIndex.untyped()));
}

void JobTreeView::appendAndEditAtChildRow() {
  resetFilter();
  auto const parent = mapToMainModel(fromFilteredModel(currentIndex()));
  auto const child = m_adaptedMainModel.appendEmptyChildRow(parent);
  editAt(expanded(mapToFilteredModel(child)));
  m_notifyee->notifyRowInserted(rowLocation().atIndex(child));
}

void JobTreeView::appendAndEditAtRowBelow() {
  auto const below = findOrMakeCellBelow(fromFilteredModel(currentIndex()));
  auto index = below.first;
  auto isNew = below.second;
  editAt(index);
  if (isNew)
    m_notifyee->notifyRowInserted(rowLocation().atIndex(mapToMainModel(index)));
}

void JobTreeView::enableFiltering() {
  m_filteredModel.setDynamicSortFilter(false);
  m_filteredModel.setSourceModel(&m_mainModel);
  setModel(&m_filteredModel);
}

void JobTreeView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Return) {
    if (event->modifiers() & Qt::ControlModifier) {
      appendAndEditAtChildRow();
    } else if (event->modifiers() & Qt::ShiftModifier) {
      // TODO Go to row above
      // prependAndEditAtRowAbove();
    } else {
      appendAndEditAtRowBelow();
    }
  } else if (event->key() == Qt::Key_Delete) {
    removeSelectedRequested();
  } else if (event->key() == Qt::Key_C) {
    if (event->modifiers() & Qt::ControlModifier) {
      copySelectedRequested();
    }
  } else if (event->key() == Qt::Key_V) {
    if (event->modifiers() & Qt::ControlModifier) {
      pasteSelectedRequested();
    }
  } else {
    QTreeView::keyPressEvent(event);
  }
}

QModelIndexForMainModel
JobTreeView::fromMainModel(QModelIndex const &mainModelIndex) const {
  return ::MantidQt::MantidWidgets::Batch::fromMainModel(mainModelIndex,
                                                         m_mainModel);
}

QModelIndexForFilteredModel
JobTreeView::fromFilteredModel(QModelIndex const &filteredModelIndex) const {
  return ::MantidQt::MantidWidgets::Batch::fromFilteredModel(filteredModelIndex,
                                                             m_filteredModel);
}

QModelIndex JobTreeView::moveCursor(CursorAction cursorAction,
                                    Qt::KeyboardModifiers modifiers) {
  if (cursorAction == QAbstractItemView::MoveNext) {
    return applyNavigationResult(navigation().moveCursorNext(currentIndex()));
  } else if (cursorAction == QAbstractItemView::MovePrevious) {
    return navigation().moveCursorPrevious(currentIndex());
  } else {
    return QTreeView::moveCursor(cursorAction, modifiers);
  }
}

QModelIndex
JobTreeView::applyNavigationResult(QtTreeCursorNavigationResult const &result) {
  auto shouldMakeNewRowBelow = result.first;
  if (shouldMakeNewRowBelow) {
    auto newCellIndex = m_adaptedMainModel.appendEmptySiblingRow(
        mapToMainModel(fromFilteredModel(result.second)));
    auto newRowIndex = fromMainModel(firstCellOnRowOf(newCellIndex.untyped()));
    resetFilter();
    auto newRowLocation = rowLocation().atIndex(newRowIndex);

    m_notifyee->notifyRowInserted(newRowLocation);

    auto maybeIndexOfNewRow = rowLocation().indexIfExistsAt(newRowLocation);
    if (maybeIndexOfNewRow.is_initialized()) {
      return expanded(mapToFilteredModel(maybeIndexOfNewRow.value())).untyped();
    } else {
      return QModelIndex();
    }
  } else {
    return result.second;
  }
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
