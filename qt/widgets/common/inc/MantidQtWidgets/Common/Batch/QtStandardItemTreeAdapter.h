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

class EXPORT_OPT_MANTIDQT_COMMON QtStandardItemTreeModelAdapter {
public:
  QtStandardItemTreeModelAdapter(QStandardItemModel &model,
                                 Cell const &emptyCellStyle);

  QModelIndexForMainModel rootIndex() const;
  std::vector<Cell> emptyRow(int columnCount) const;

  std::vector<Cell>
  cellsAtRow(QModelIndexForMainModel const &firstCellIndex) const;
  void setCellsAtRow(QModelIndexForMainModel const &rowIndex,
                     std::vector<Cell> const &cells);

  Cell cellFromCellIndex(QModelIndexForMainModel const &index) const;
  void setCellAtCellIndex(QModelIndexForMainModel const &index,
                          Cell const &newCellProperties);

  QModelIndexForMainModel appendSiblingRow(QModelIndexForMainModel const &index,
                                           std::vector<Cell> cells);
  QModelIndexForMainModel
  appendEmptySiblingRow(QModelIndexForMainModel const &index);

  QModelIndexForMainModel appendChildRow(QModelIndexForMainModel const &parent,
                                         std::vector<Cell> cells);
  QModelIndexForMainModel
  appendEmptyChildRow(QModelIndexForMainModel const &parent);

  QModelIndexForMainModel insertChildRow(QModelIndexForMainModel const &parent,
                                         int column, std::vector<Cell> cells);
  QModelIndexForMainModel
  insertEmptyChildRow(QModelIndexForMainModel const &parent, int column);

  void removeRowFrom(QModelIndexForMainModel const &index);
private:
  QList<QStandardItem *> rowItemsFromCells(std::vector<Cell> const &cells);

  template <typename Action>
  void enumerateCellsInRow(QModelIndexForMainModel const &startIndex,
                           int columns, Action const &action) const;

  QStandardItemModel &m_model;
  Cell m_emptyCellStyle;
};

template <typename Action>
void QtStandardItemTreeModelAdapter::enumerateCellsInRow(
    QModelIndexForMainModel const &startIndex, int columns,
    Action const &action) const {
  for (auto i = 0; i < columns; i++) {
    auto cellIndex = startIndex.sibling(startIndex.row(), i);
    action(cellIndex, i);
  }
}

EXPORT_OPT_MANTIDQT_COMMON QStandardItem *
modelItemFromIndex(QStandardItemModel &model,
                   QModelIndexForMainModel const &index);

EXPORT_OPT_MANTIDQT_COMMON QStandardItem const *
modelItemFromIndex(QStandardItemModel const &model,
                   QModelIndexForMainModel const &index);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
