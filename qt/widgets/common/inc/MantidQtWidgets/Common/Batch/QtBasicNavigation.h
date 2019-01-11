// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
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
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_QTBASICNAVIGATION_H_
