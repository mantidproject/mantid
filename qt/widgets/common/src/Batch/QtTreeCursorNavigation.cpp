#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"

namespace MantidQt {
namespace MantidWidgets {

QtTreeCursorNavigation::QtTreeCursorNavigation(QAbstractItemModel const *model) : model(model) {}

QtTreeCursorNavigationResult
QtTreeCursorNavigation::withoutAppendedRow(QModelIndex const &index) const {
  return std::make_pair(false, index);
}

QtTreeCursorNavigationResult
QtTreeCursorNavigation::withAppendedRow(QModelIndex const &index) const {
  return std::make_pair(true, index);
}

std::pair<bool, QModelIndex>
QtTreeCursorNavigation::moveCursorNext(QModelIndex const &currentIndex) const {
  if (currentIndex.isValid()) {
    if (isNotLastCellOnThisRow(currentIndex))
      return withoutAppendedRow(nextCellOnThisRow(currentIndex));
    else if (isNotLastRowInThisNode(currentIndex))
      return withoutAppendedRow(firstCellOnNextRow(currentIndex));
    else
      return withAppendedRow(currentIndex);
  } else
    return withoutAppendedRow(QModelIndex());
}

QModelIndex QtTreeCursorNavigation::moveCursorPrevious(
    QModelIndex const &currentIndex) const {
  if (currentIndex.isValid()) {
    if (isNotFirstCellInThisRow(currentIndex))
      return previousCellInThisRow(currentIndex);
    else if (isNotFirstRowInThisNode(currentIndex))
      return lastCellInPreviousRow(currentIndex);
    else
      return lastCellInParentRowElseNone(currentIndex);
  } else
    return QModelIndex();
}

bool QtTreeCursorNavigation::isNotFirstCellInThisRow(
    QModelIndex const &index) const {
  return index.column() > 0;
}

bool QtTreeCursorNavigation::isNotFirstRowInThisNode(
    QModelIndex const &index) const {
  return index.row() > 0;
}

QModelIndex
QtTreeCursorNavigation::previousCellInThisRow(QModelIndex const &index) const {
  return index.sibling(index.row(), index.column() - 1);
}

QModelIndex
QtTreeCursorNavigation::lastCellInPreviousRow(QModelIndex const &index) const {
  return index.sibling(index.row() - 1, model->columnCount() - 1);
}

QModelIndex
QtTreeCursorNavigation::lastRowInThisNode(QModelIndex const &parent) const {
  return model->index(model->rowCount(parent) - 1, 0, parent);
}

QModelIndex QtTreeCursorNavigation::lastCellInParentRowElseNone(
    QModelIndex const &index) const {
  auto parent = model->parent(index);
  if (parent.isValid())
    return parent.sibling(parent.row(), model->columnCount() - 1);
  else
    return QModelIndex();
}

QModelIndex
QtTreeCursorNavigation::firstCellOnNextRow(QModelIndex const &index) const {
  return index.sibling(index.row() + 1, 0);
}

QModelIndex
QtTreeCursorNavigation::nextCellOnThisRow(QModelIndex const &index) const {
  return index.sibling(index.row(), index.column() + 1);
}

bool QtTreeCursorNavigation::isNotLastCellOnThisRow(
    QModelIndex const &index) const {
  return index.column() + 1 < model->columnCount();
}

bool QtTreeCursorNavigation::isNotLastRowInThisNode(
    QModelIndex const &index) const {
  return index.row() + 1 < model->rowCount(index.parent());
}
}
}
