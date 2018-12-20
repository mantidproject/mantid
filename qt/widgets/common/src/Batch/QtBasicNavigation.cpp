#include "MantidQtWidgets/Common/Batch/QtBasicNavigation.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

QModelIndex below(QModelIndex const &index) {
  return index.sibling(index.row() + 1, index.column());
}

QModelIndex above(QModelIndex const &index) {
  return index.sibling(index.row() - 1, index.column());
}

QModelIndex leftOf(QModelIndex const &index) {
  return index.sibling(index.row(), index.column() - 1);
}

QModelIndex rightOf(QModelIndex const &index) {
  return index.sibling(index.row(), index.column() + 1);
}

bool hasCellOnTheLeft(QModelIndex const &index) { return index.column() > 0; }

bool hasCellOnTheRight(QModelIndex const &index) {
  return index.column() + 1 < index.model()->columnCount();
}

bool hasRowAbove(QModelIndex const &index) { return index.row() > 0; }

bool hasRowBelow(QModelIndex const &index) {
  return index.row() + 1 < index.model()->rowCount(index.parent());
}

QModelIndex firstCellOnRowOf(QModelIndex const &index) {
  return index.sibling(index.row(), 0);
}

bool areOnSameRow(QModelIndex const &a, QModelIndex const &b) {
  return a.parent() == b.parent() && a.row() == b.row() &&
         a.model() == b.model();
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
