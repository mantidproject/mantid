#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"
#include "MantidQtWidgets/Common/Batch/QtBasicNavigation.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

QtTreeCursorNavigation::QtTreeCursorNavigation(QAbstractItemModel const *model)
    : m_model(model) {}

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
  return hasCellOnTheLeft(index);
}

bool QtTreeCursorNavigation::isNotFirstRowInThisNode(
    QModelIndex const &index) const {
  return hasRowAbove(index);
}

QModelIndex
QtTreeCursorNavigation::previousCellInThisRow(QModelIndex const &index) const {
  return leftOf(index);
}

QModelIndex
QtTreeCursorNavigation::lastCellInPreviousRow(QModelIndex const &index) const {
  return index.sibling(index.row() - 1, m_model->columnCount() - 1);
}

QModelIndex lastChildRowOf(QModelIndex const &parent,
                           QAbstractItemModel const &model) {
  return model.index(model.rowCount(parent) - 1, 0, parent);
}

QModelIndex
QtTreeCursorNavigation::lastRowInThisNode(QModelIndex const &parent) const {
  return lastChildRowOf(parent, *m_model);
}

QModelIndex QtTreeCursorNavigation::lastCellInParentRowElseNone(
    QModelIndex const &index) const {
  auto parent = m_model->parent(index);
  if (parent.isValid())
    return parent.sibling(parent.row(), m_model->columnCount() - 1);
  else
    return QModelIndex();
}

QModelIndex
QtTreeCursorNavigation::firstCellOnNextRow(QModelIndex const &index) const {
  return index.sibling(index.row() + 1, 0);
}

QModelIndex
QtTreeCursorNavigation::nextCellOnThisRow(QModelIndex const &index) const {
  return rightOf(index);
}

bool QtTreeCursorNavigation::isNotLastCellOnThisRow(
    QModelIndex const &index) const {
  return hasCellOnTheRight(index);
}

bool QtTreeCursorNavigation::isNotLastRowInThisNode(
    QModelIndex const &index) const {
  return hasRowBelow(index);
}
}
}
}
