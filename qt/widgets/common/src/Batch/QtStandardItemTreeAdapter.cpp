#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include "MantidQtWidgets/Common/Batch/QtBasicNavigation.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/CellStandardItem.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

QModelIndexForMainModel rootModelIndex(QStandardItemModel const &) {
  return QModelIndexForMainModel(QModelIndex());
}

QModelIndexForFilteredModel rootModelIndex(QSortFilterProxyModel const &) {
  return QModelIndexForFilteredModel(QModelIndex());
}

QStandardItem const *modelItemFromIndex(QStandardItemModel const &model,
                                        QModelIndexForMainModel const &index) {
  if (index.isValid()) {
    auto *item = model.itemFromIndex(index.untyped());
    assertOrThrow(item != nullptr,
                  "modelItemFromIndex: Index must point to a valid item.");
    return item;
  } else
    return model.invisibleRootItem();
}

QStandardItem *modelItemFromIndex(QStandardItemModel &model,
                                  QModelIndexForMainModel const &index) {
  if (index.isValid()) {
    auto *item = model.itemFromIndex(index.untyped());
    assertOrThrow(item != nullptr,
                  "modelItemFromIndex: Index must point to a valid item.");
    return item;
  } else
    return model.invisibleRootItem();
}

QtStandardItemTreeModelAdapter::QtStandardItemTreeModelAdapter(
    QStandardItemModel &model, Cell const &emptyCellStyle)
    : m_model(model), m_emptyCellStyle(emptyCellStyle) {}

void QtStandardItemTreeModelAdapter::removeRowFrom(
    QModelIndexForMainModel const &index) {
  if (index.isValid()) {
    m_model.removeRows(index.row(), 1, m_model.parent(index.untyped()));
  } else {
    m_model.removeRows(0, m_model.rowCount());
  }
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::appendEmptySiblingRow(
    QModelIndexForMainModel const &index) {
  auto newIndex = appendEmptyChildRow(index.parent());
  return newIndex.sibling(newIndex.row(), index.column());
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::appendSiblingRow(
    QModelIndexForMainModel const &index, QList<QStandardItem *> cells) {
  auto newIndex = appendChildRow(index.parent(), cells);
  return newIndex.sibling(newIndex.row(), index.column());
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::appendEmptyChildRow(
    QModelIndexForMainModel const &parent) {
  return appendChildRow(parent, emptyRow(m_model.columnCount()));
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::appendChildRow(
    QModelIndexForMainModel const &parent, QList<QStandardItem *> cells) {
  auto parentRow = QModelIndexForMainModel(firstCellOnRowOf(parent.untyped()));
  auto *const parentItem = modelItemFromIndex(m_model, parentRow);
  parentItem->appendRow(cells);
  return QModelIndexForMainModel(lastChildRowOf(parentRow.untyped(), m_model));
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::insertChildRow(
    QModelIndexForMainModel const &parent, int row,
    QList<QStandardItem *> cells) {
  auto *const parentItem = modelItemFromIndex(m_model, parent);
  parentItem->insertRow(row, cells);
  return QModelIndexForMainModel(m_model.index(row, 0, parent.untyped()));
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::insertEmptyChildRow(
    QModelIndexForMainModel const &parent, int row) {
  return insertChildRow(parent, row, emptyRow(m_model.columnCount()));
}

QList<QStandardItem *>
QtStandardItemTreeModelAdapter::emptyRow(int columnCount) const {
  auto cells = QList<QStandardItem *>();
  for (auto i = 0; i < columnCount; ++i) {
    auto *cellItem = new QStandardItem();
    applyCellPropertiesToItem(m_emptyCellStyle, *cellItem);
    cells.append(cellItem);
  }
  return cells;
}

void QtStandardItemTreeModelAdapter::setCellAtCellIndex(
    QModelIndexForMainModel const &cellIndex, Cell const &cell) {
  auto *item = modelItemFromIndex(m_model, cellIndex);
  applyCellPropertiesToItem(cell, *item);
}

void QtStandardItemTreeModelAdapter::setCellsAtRow(
    QModelIndexForMainModel const &firstCellIndex,
    std::vector<Cell> const &cells) {
  enumerateCellsInRow(
      firstCellIndex, m_model.columnCount(),
      [this, &cells](QModelIndexForMainModel const &cellIndex, int i) -> void {
        auto *item = modelItemFromIndex(m_model, cellIndex);
        applyCellPropertiesToItem(cells[i], *item);
      });
}

QList<QStandardItem *> rowFromCells(std::vector<Cell> const &cells) {
  auto rowCells = QList<QStandardItem *>();
  for (auto &&cell : cells) {
    auto *item = new QStandardItem();
    applyCellPropertiesToItem(cell, *item);
    rowCells.append(item);
  }
  return rowCells;
}

QList<QStandardItem *> paddedRowFromCells(std::vector<Cell> const &cells,
                                          Cell const &paddingCell,
                                          int paddedWidth) {
  auto rowCells = rowFromCells(cells);

  for (auto i = static_cast<int>(cells.size()); i < paddedWidth; i++) {
    auto *item = new QStandardItem();
    applyCellPropertiesToItem(paddingCell, *item);
    rowCells.append(item);
  }

  return rowCells;
}

std::vector<Cell> QtStandardItemTreeModelAdapter::cellsAtRow(
    QModelIndexForMainModel const &firstCellIndex) const {
  auto cells = std::vector<Cell>();
  cells.reserve(m_model.columnCount());
  enumerateCellsInRow(
      firstCellIndex, m_model.columnCount(),
      [this, &cells](QModelIndexForMainModel const &cellIndex, int)
          -> void { cells.emplace_back(cellFromCellIndex(cellIndex)); });
  return cells;
}

Cell QtStandardItemTreeModelAdapter::cellFromCellIndex(
    QModelIndexForMainModel const &index) const {
  auto *cellItem = modelItemFromIndex(m_model, index);
  return extractCellPropertiesFromItem(*cellItem);
}
}
}
}
