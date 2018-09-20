#include "MantidQtWidgets/Common/Batch/FilteredTreeModel.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

FilteredTreeModel::FilteredTreeModel(RowLocationAdapter rowLocationAdapter,
                                     QObject *parent)
    : QSortFilterProxyModel(parent), m_rowLocation(rowLocationAdapter) {
  resetPredicate();
}

void FilteredTreeModel::resetPredicate() {
  m_predicate = nullptr;
  invalidate();
}

bool FilteredTreeModel::isReset() const { return m_predicate == nullptr; }

void FilteredTreeModel::setPredicate(std::unique_ptr<RowPredicate> predicate) {
  m_predicate = std::move(predicate);
  invalidate();
}

RowLocation FilteredTreeModel::rowLocationAt(QModelIndex const &index) const {
  return m_rowLocation.atIndex(fromMainModel(index, *sourceModel()));
}

bool FilteredTreeModel::filterAcceptsRow(int row,
                                         const QModelIndex &parent) const {
  auto index = sourceModel()->index(row, 0, parent);
  if (index.isValid()) {
    if (m_predicate == nullptr || (*m_predicate)(rowLocationAt(index))) {
      return true;
    } else {
      int rows = sourceModel()->rowCount(index);
      for (auto r = 0; r < rows; r++)
        if (filterAcceptsRow(r, index))
          return true;
      return false;
    }
  } else {
    return false;
  }
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
