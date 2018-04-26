#include "MantidQtWidgets/Common/Batch/QtFilterLeafNodes.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
QtFilterLeafNodes::QtFilterLeafNodes(QObject *parent)
    : QSortFilterProxyModel(parent){}

bool QtFilterLeafNodes::filterAcceptsRow(int row,
                                         const QModelIndex &parent) const {
  auto index = sourceModel()->index(row, 0, parent);
  if (index.isValid()) {
    if (index.data().toString().contains(filterRegExp()))
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
