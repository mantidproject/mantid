#include "MantidQtWidgets/Common/Batch/QtFilterLeafNodes.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include <iostream>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

bool RowPredicate::operator()(RowLocation const &rowLocation) const {
  return rowMeetsCriteria(rowLocation);
}

QtFilterLeafNodes::QtFilterLeafNodes(RowLocationAdapter rowLocationAdapter,
                                     QObject *parent)
    : QSortFilterProxyModel(parent), m_rowLocation(rowLocationAdapter) {
  resetPredicate();
}

void QtFilterLeafNodes::resetPredicate() {
  m_predicate = nullptr;
  invalidate();
}

bool QtFilterLeafNodes::isReset() const { return m_predicate == nullptr; }

void QtFilterLeafNodes::setPredicate(std::unique_ptr<RowPredicate> predicate) {
  m_predicate = std::move(predicate);
  invalidate();
}

RowLocation QtFilterLeafNodes::rowLocationAt(QModelIndex const &index) const {
  return m_rowLocation.atIndex(fromMainModel(index, *sourceModel()));
}

bool QtFilterLeafNodes::filterAcceptsRow(int row,
                                         const QModelIndex &parent) const {
  auto index = sourceModel()->index(row, 0, parent);
  if (index.isValid()) {
    if (m_predicate == nullptr || (*m_predicate)(rowLocationAt(index)))
      return true;

    int rows = sourceModel()->rowCount(index);
    for (auto r = 0; r < rows; r++)
      if (filterAcceptsRow(r, index))
        return true;
    return false;
  } else {
    return false;
  }
}
}
}
}
