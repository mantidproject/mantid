#ifndef MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
#define MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include "MantidQtWidgets/Common/Batch/Row.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

template <typename Action>
void enumerateCellsInRow(QModelIndexForMainModel const& startIndex, int columns, Action const& action) {
  for (auto i = 0; i < columns; i++) {
    auto cellIndex = startIndex.sibling(startIndex.row(), i);
    action(cellIndex, i);
  }
}

QModelIndexForMainModel rootModelIndex(QStandardItemModel const &model);
QModelIndexForFilteredModel rootModelIndex(QSortFilterProxyModel const &model);

EXPORT_OPT_MANTIDQT_COMMON QList<QStandardItem *>
rowFromCells(std::vector<Cell> const &cells);

QList<QStandardItem *> emptyRow(int columnCount);

EXPORT_OPT_MANTIDQT_COMMON std::vector<Cell>
cellsAtRow(QStandardItemModel const &model,
           QModelIndexForMainModel const &firstCellIndex);

EXPORT_OPT_MANTIDQT_COMMON void
setCellsAtRow(QStandardItemModel &model,
              QModelIndexForMainModel const &firstCellIndex,
              std::vector<Cell> const &cells);

EXPORT_OPT_MANTIDQT_COMMON Cell
cellFromCellIndex(QStandardItemModel const &model,
                  QModelIndexForMainModel const &index);

void setCellAtCellIndex(QStandardItemModel &model,
                        QModelIndexForMainModel const &index,
                        Cell const &newCellProperties);

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
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
