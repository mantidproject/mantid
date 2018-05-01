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

void removeRowFrom(QStandardItemModel &model,
                   QModelIndexForMainModel const &index) {
  if (index.isValid()) {
    model.removeRows(index.row(), 1, index.parent().untyped());
  } else {
    model.removeRows(0, model.rowCount());
  }
}

QModelIndexForMainModel
appendEmptySiblingRow(QStandardItemModel &model,
                      QModelIndexForMainModel const &index) {
  auto newIndex = appendEmptyChildRow(model, index.parent());
  return newIndex.sibling(newIndex.row(), index.column());
}

QModelIndexForMainModel appendSiblingRow(QStandardItemModel &model,
                                         QModelIndexForMainModel const &index,
                                         QList<QStandardItem *> cells) {
  auto newIndex = appendChildRow(model, index.parent(), cells);
  return newIndex.sibling(newIndex.row(), index.column());
}

QModelIndexForMainModel
appendEmptyChildRow(QStandardItemModel &model,
                    QModelIndexForMainModel const &parent) {
  return appendChildRow(model, parent, emptyRow(model.columnCount()));
}

QModelIndexForMainModel appendChildRow(QStandardItemModel &model,
                                       QModelIndexForMainModel const &parent,
                                       QList<QStandardItem *> cells) {
  auto parentRow = QModelIndexForMainModel(firstCellOnRowOf(parent.untyped()));
  auto *const parentItem = modelItemFromIndex(model, parentRow);
  parentItem->appendRow(cells);
  return QModelIndexForMainModel(lastChildRowOf(parentRow.untyped(), model));
}

QModelIndexForMainModel insertChildRow(QStandardItemModel &model,
                                       QModelIndexForMainModel const &parent,
                                       int row, QList<QStandardItem *> cells) {
  auto *const parentItem = modelItemFromIndex(model, parent);
  parentItem->insertRow(row, cells);
  return QModelIndexForMainModel(model.index(row, 0, parent.untyped()));
}

QModelIndexForMainModel
insertEmptyChildRow(QStandardItemModel &model,
                    QModelIndexForMainModel const &parent, int row) {
  return insertChildRow(model, parent, row, emptyRow(model.columnCount()));
}

QList<QStandardItem *> emptyRow(int columnCount) {
  auto cells = QList<QStandardItem *>();
  for (auto i = 0; i < columnCount; ++i)
    cells.append(new QStandardItem(""));
  return cells;
}

QList<QStandardItem *> rowFromRowText(std::vector<std::string> const &rowText) {
  auto rowCells = QList<QStandardItem *>();
  for (auto &cellText : rowText)
    rowCells.append(new QStandardItem(QString::fromStdString(cellText)));
  return rowCells;
}

void setCellAtCellIndex(QStandardItemModel &model,
                        QModelIndexForMainModel const &cellIndex,
                        Cell const &cell) {
  auto *item = modelItemFromIndex(model, cellIndex);
  applyCellPropertiesToItem(cell, *item);
}

void setCellsAtRow(QStandardItemModel &model,
                   QModelIndexForMainModel const &firstCellIndex,
                   std::vector<Cell> const &cells) {
  enumerateCellsInRow(firstCellIndex, model.columnCount(),
                      [&model, &cells](QModelIndexForMainModel const &cellIndex, int i) -> void {
                        auto *item = modelItemFromIndex(model, cellIndex);
                        applyCellPropertiesToItem(cells[i], *item);
                      });
}

QList<QStandardItem *> rowFromCells(std::vector<Cell> const &cells) {
  auto rowCells = QList<QStandardItem *>();
  for (auto &&cell : cells) {
    auto *item = new QStandardItem(QString::fromStdString(cell.contentText()));
    applyCellPropertiesToItem(cell, *item);
    rowCells.append(item);
  }
  return rowCells;
}

std::vector<Cell> cellsAtRow(QStandardItemModel const &model,
                             QModelIndexForMainModel const &firstCellIndex) {
  auto cells = std::vector<Cell>();
  cells.reserve(model.columnCount());
  enumerateCellsInRow(firstCellIndex, model.columnCount(),
                      [&model, &cells](QModelIndexForMainModel const &cellIndex, int) -> void {
                        cells.emplace_back(cellFromCellIndex(model, cellIndex));
                      });
  return cells;
}

Cell cellFromCellIndex(QStandardItemModel const &model,
                       QModelIndexForMainModel const &index) {
  auto *cellItem = modelItemFromIndex(model, index);
  return extractCellPropertiesFromItem(*cellItem);
}
}
}
}
