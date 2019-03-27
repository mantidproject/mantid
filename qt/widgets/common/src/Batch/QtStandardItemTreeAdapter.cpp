// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/CellStandardItem.h"
#include "MantidQtWidgets/Common/Batch/QtBasicNavigation.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

QtStandardItemTreeModelAdapter::QtStandardItemTreeModelAdapter(
    QStandardItemModel &model, Cell const &emptyCellStyle)
    : m_model(model), m_emptyCellStyle(emptyCellStyle) {}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::rootIndex() const {
  return QModelIndexForMainModel();
}

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
    QModelIndexForMainModel const &index, std::vector<Cell> const &cells) {
  auto newIndex = appendChildRow(index.parent(), cells);
  return newIndex.sibling(newIndex.row(), index.column());
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::appendEmptyChildRow(
    QModelIndexForMainModel const &parent) {
  return appendChildRow(parent, emptyRow(m_model.columnCount()));
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::appendChildRow(
    QModelIndexForMainModel const &parent, std::vector<Cell> const &cells) {
  auto parentRow = QModelIndexForMainModel(firstCellOnRowOf(parent.untyped()));
  auto *const parentItem = modelItemFromIndex(m_model, parentRow);
  parentItem->appendRow(rowItemsFromCells(cells));
  return QModelIndexForMainModel(lastChildRowOf(parentRow.untyped(), m_model));
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::insertChildRow(
    QModelIndexForMainModel const &parent, int row,
    std::vector<Cell> const &cells) {
  auto *const parentItem = modelItemFromIndex(m_model, parent);
  parentItem->insertRow(row, rowItemsFromCells(cells));
  return QModelIndexForMainModel(m_model.index(row, 0, parent.untyped()));
}

QModelIndexForMainModel QtStandardItemTreeModelAdapter::insertEmptyChildRow(
    QModelIndexForMainModel const &parent, int row) {
  return insertChildRow(parent, row, emptyRow(m_model.columnCount()));
}

std::vector<Cell>
QtStandardItemTreeModelAdapter::emptyRow(int columnCount) const {
  return std::vector<Cell>(columnCount, m_emptyCellStyle);
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

QList<QStandardItem *> QtStandardItemTreeModelAdapter::rowItemsFromCells(
    std::vector<Cell> const &cells) {
  auto rowCells = QList<QStandardItem *>();
  for (auto &&cell : cells) {
    auto *item = new QStandardItem();
    applyCellPropertiesToItem(cell, *item);
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
      [this, &cells](QModelIndexForMainModel const &cellIndex, int /*unused*/) -> void {
        cells.emplace_back(cellFromCellIndex(cellIndex));
      });
  return cells;
}

Cell QtStandardItemTreeModelAdapter::cellFromCellIndex(
    QModelIndexForMainModel const &index) const {
  auto *cellItem = modelItemFromIndex(m_model, index);
  return extractCellPropertiesFromItem(*cellItem);
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
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
