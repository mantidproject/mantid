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
void enumerateCellsInRow(QModelIndexForMainModel const &startIndex, int columns,
                         Action const &action) {
  for (auto i = 0; i < columns; i++) {
    auto cellIndex = startIndex.sibling(startIndex.row(), i);
    action(cellIndex, i);
  }
}

EXPORT_OPT_MANTIDQT_COMMON QModelIndexForMainModel
rootModelIndex(QStandardItemModel const &model);
EXPORT_OPT_MANTIDQT_COMMON QModelIndexForFilteredModel
rootModelIndex(QSortFilterProxyModel const &model);

EXPORT_OPT_MANTIDQT_COMMON QStandardItem *
modelItemFromIndex(QStandardItemModel &model,
                   QModelIndexForMainModel const &index);

EXPORT_OPT_MANTIDQT_COMMON QStandardItem const *
modelItemFromIndex(QStandardItemModel const &model,
                   QModelIndexForMainModel const &index);

EXPORT_OPT_MANTIDQT_COMMON QList<QStandardItem *>
rowFromCells(std::vector<Cell> const &cells);

EXPORT_OPT_MANTIDQT_COMMON QList<QStandardItem *>
paddedRowFromCells(std::vector<Cell> const &cells, Cell const &paddingCell,
                   int paddedWidth);

EXPORT_OPT_MANTIDQT_COMMON QList<QStandardItem *> emptyRow(int columnCount);

class EXPORT_OPT_MANTIDQT_COMMON QtStandardItemTreeModelAdapter {
public:
  QtStandardItemTreeModelAdapter(QStandardItemModel &model,
                                 Cell const &emptyCellStyle);

  QModelIndexForMainModel rootIndex() const;
  QList<QStandardItem *> emptyRow(int columnCount) const;

  std::vector<Cell>
  cellsAtRow(QModelIndexForMainModel const &firstCellIndex) const;
  void setCellsAtRow(QModelIndexForMainModel const &rowIndex,
                     std::vector<Cell> const &cells);

  Cell cellFromCellIndex(QModelIndexForMainModel const &index) const;
  void setCellAtCellIndex(QModelIndexForMainModel const &index,
                          Cell const &newCellProperties);

  QModelIndexForMainModel appendSiblingRow(QModelIndexForMainModel const &index,
                                           QList<QStandardItem *> cells);
  QModelIndexForMainModel
  appendEmptySiblingRow(QModelIndexForMainModel const &index);

  QModelIndexForMainModel appendChildRow(QModelIndexForMainModel const &parent,
                                         QList<QStandardItem *> cells);
  QModelIndexForMainModel
  appendEmptyChildRow(QModelIndexForMainModel const &parent);

  QModelIndexForMainModel insertChildRow(QModelIndexForMainModel const &parent,
                                         int column,
                                         QList<QStandardItem *> cells);
  QModelIndexForMainModel
  insertEmptyChildRow(QModelIndexForMainModel const &parent, int column);

  void removeRowFrom(QModelIndexForMainModel const &index);

private:
  QStandardItemModel &m_model;
  Cell m_emptyCellStyle;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
