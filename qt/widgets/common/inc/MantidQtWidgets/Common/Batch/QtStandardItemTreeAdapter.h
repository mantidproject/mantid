#ifndef MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
#define MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

QModelIndexForMainModel rootModelIndex(QStandardItemModel const &model);
QModelIndexForFilteredModel rootModelIndex(QSortFilterProxyModel const &model);

EXPORT_OPT_MANTIDQT_COMMON QList<QStandardItem *>
rowFromRowText(std::vector<std::string> const &rowText);
QList<QStandardItem *> emptyRow(int columnCount);

std::vector<std::string>
rowTextAtRow(QStandardItemModel const &model,
             QModelIndexForMainModel const &firstCellIndex);
std::string textFromCell(QStandardItemModel const &model,
                         QModelIndexForMainModel const &index);

QModelIndexForMainModel
appendEmptySiblingRow(QStandardItemModel &model,
                      QModelIndexForMainModel const &index);
QModelIndexForMainModel appendSiblingRow(QStandardItemModel &model,
                                         QModelIndexForMainModel const &index,
                                         QList<QStandardItem *> cells);

QModelIndexForMainModel
appendEmptyChildRow(QStandardItemModel &model,
                    QModelIndexForMainModel const &parent);
QModelIndexForMainModel appendChildRow(QStandardItemModel &model,
                                       QModelIndexForMainModel const &parent,
                                       QList<QStandardItem *> cells);

QModelIndexForMainModel insertChildRow(QStandardItemModel &model,
                                       QModelIndexForMainModel const &parent,
                                       int column,
                                       QList<QStandardItem *> cells);
QModelIndexForMainModel
insertEmptyChildRow(QStandardItemModel &model,
                    QModelIndexForMainModel const &parent, int column);

QStandardItem *modelItemFromIndex(QStandardItemModel &model,
                                  QModelIndexForMainModel const &index);

QStandardItem const *modelItemFromIndex(QStandardItemModel const &model,
                                        QModelIndexForMainModel const &index);

void removeRowFrom(QStandardItemModel &model,
                   QModelIndexForMainModel const &index);
void setTextAtCell(QStandardItemModel &model,
                   QModelIndexForMainModel const &index,
                   std::string const &newText);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
