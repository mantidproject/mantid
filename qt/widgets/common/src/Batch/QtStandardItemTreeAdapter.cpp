#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

QtStandardItemMutableTreeAdapter::QtStandardItemMutableTreeAdapter(
    QStandardItemModel &model)
    : QtStandardItemTreeAdapter(model), m_model(&model) {}

QtStandardItemTreeAdapter::QtStandardItemTreeAdapter(
    QStandardItemModel const &model)
    : m_model(&model) {}

QModelIndex QtStandardItemTreeAdapter::rootModelIndex() const {
  return QModelIndex();
}

QStandardItem const *
QtStandardItemTreeAdapter::modelItemFromIndex(QModelIndex const &index) const {
  if (index.isValid()) {
    auto *item = model().itemFromIndex(index);
    assertOrThrow(item != nullptr,
                  "modelItemFromIndex: Index must point to a valid item.");
    return item;
  } else
    return model().invisibleRootItem();
}

QStandardItem *
QtStandardItemMutableTreeAdapter::modelItemFromIndex(QModelIndex const &index) {
  if (index.isValid()) {
    auto *item = model().itemFromIndex(index);
    assertOrThrow(item != nullptr,
                  "modelItemFromIndex: Index must point to a valid item.");
    return item;
  } else
    return model().invisibleRootItem();
}

void QtStandardItemMutableTreeAdapter::removeRowAt(QModelIndex const &index) {
  if (index.isValid()) {
    std::cout << "Index is valid" << std::endl;
    model().removeRows(index.row(), 1, model().parent(index));
  } else {
    model().removeRows(0, model().rowCount());
  }
}

QModelIndex QtStandardItemMutableTreeAdapter::appendEmptySiblingRow(
    QModelIndex const &index) {
  auto newIndex = appendEmptyChildRow(model().parent(index));
  return newIndex.sibling(newIndex.row(), index.column());
}

QModelIndex QtStandardItemMutableTreeAdapter::appendSiblingRow(
    QModelIndex const &index, QList<QStandardItem *> cells) {
  auto newIndex = appendChildRow(model().parent(index), cells);
  return newIndex.sibling(newIndex.row(), index.column());
}

QModelIndex QtStandardItemMutableTreeAdapter::appendEmptyChildRow(
    QModelIndex const &parent) {
  return appendChildRow(parent, emptyRow());
}

QModelIndex
QtStandardItemMutableTreeAdapter::appendChildRow(QModelIndex const &parent,
                                                 QList<QStandardItem *> cells) {
  auto parentRow = firstCellOnRowOf(parent);
  auto *const parentItem = modelItemFromIndex(parentRow);
  parentItem->appendRow(cells);
  return lastChildRowOf(parentRow, model());
}

QModelIndex QtStandardItemMutableTreeAdapter::insertChildRow(
    QModelIndex const &parent, int row, QList<QStandardItem *> cells) {
  auto *const parentItem = modelItemFromIndex(parent);
  parentItem->insertRow(row, cells);
  return model().index(row, 0, parent);
}

QModelIndex
QtStandardItemMutableTreeAdapter::insertEmptyChildRow(QModelIndex const &parent,
                                                      int row) {
  return insertChildRow(parent, row, emptyRow());
}

void QtStandardItemMutableTreeAdapter::setTextAtCell(QModelIndex index, std::string const& newText) {
  modelItemFromIndex(index)->setText(QString::fromStdString(newText));
}

QList<QStandardItem *> QtStandardItemTreeAdapter::emptyRow() const {
  auto cells = QList<QStandardItem *>();
  for (auto i = 0; i < model().columnCount(); ++i)
    cells.append(new QStandardItem(""));
  return cells;
}

QList<QStandardItem *> QtStandardItemTreeAdapter::rowFromRowText(
    std::vector<std::string> const &rowText) const {
  auto rowCells = QList<QStandardItem *>();
  for (auto &cellText : rowText)
    rowCells.append(new QStandardItem(QString::fromStdString(cellText)));
  return rowCells;
}

std::vector<std::string>
QtStandardItemTreeAdapter::rowTextFromRow(QModelIndex firstCellIndex) const {
  auto rowText = std::vector<std::string>();
  rowText.reserve(model().columnCount());

  for (auto i = 0; i < model().columnCount(); i++) {
    auto cellIndex = firstCellIndex.sibling(firstCellIndex.row(), i);
    rowText.emplace_back(textFromCell(cellIndex));
  }
  return rowText;
}

std::string QtStandardItemTreeAdapter::textFromCell(QModelIndex index) const {
  return modelItemFromIndex(index)->text().toStdString();
}

QStandardItemModel const &QtStandardItemTreeAdapter::model() const {
  return *m_model;
}

QStandardItemModel &QtStandardItemMutableTreeAdapter::model() {
  return *m_model;
}
}
}
}
