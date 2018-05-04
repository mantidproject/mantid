#ifndef MANTIDQTMANTIDWIDGETS_QTBASICNAVIGATION_H_
#define MANTIDQTMANTIDWIDGETS_QTBASICNAVIGATION_H_
#include <QModelIndex>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

QModelIndex below(QModelIndex const &index);
QModelIndex above(QModelIndex const &index);
QModelIndex leftOf(QModelIndex const &index);
QModelIndex rightOf(QModelIndex const &index);
QModelIndex firstCellOnRowOf(QModelIndex const &index);
QModelIndex lastChildRowOf(QModelIndex const &index,
                           QAbstractItemModel const &model);
bool hasCellOnTheLeft(QModelIndex const &index);
bool hasCellOnTheRight(QModelIndex const &index);
bool hasRowAbove(QModelIndex const &index);
bool hasRowBelow(QModelIndex const &index);
bool areOnSameRow(QModelIndex const &a, QModelIndex const &b);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_QTBASICNAVIGATION_H_
