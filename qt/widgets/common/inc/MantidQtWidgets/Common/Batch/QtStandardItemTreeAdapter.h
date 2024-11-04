// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/Row.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON QtStandardItemTreeModelAdapter {
public:
  QtStandardItemTreeModelAdapter(QStandardItemModel &model, Cell emptyCellStyle);

  QModelIndexForMainModel rootIndex() const;
  std::vector<Cell> emptyRow(int columnCount) const;

  std::vector<Cell> cellsAtRow(QModelIndexForMainModel const &firstCellIndex) const;
  void setCellsAtRow(QModelIndexForMainModel const &rowIndex, std::vector<Cell> const &cells);

  Cell cellFromCellIndex(QModelIndexForMainModel const &index) const;
  void setCellAtCellIndex(QModelIndexForMainModel const &index, Cell const &newCellProperties);

  QModelIndexForMainModel appendSiblingRow(QModelIndexForMainModel const &index, std::vector<Cell> const &cells);
  QModelIndexForMainModel appendEmptySiblingRow(QModelIndexForMainModel const &index);

  QModelIndexForMainModel appendChildRow(QModelIndexForMainModel const &parent, std::vector<Cell> const &cells);
  QModelIndexForMainModel appendEmptyChildRow(QModelIndexForMainModel const &parent);

  QModelIndexForMainModel insertChildRow(QModelIndexForMainModel const &parent, int row,
                                         std::vector<Cell> const &cells);
  QModelIndexForMainModel insertEmptyChildRow(QModelIndexForMainModel const &parent, int column);

  void removeRowFrom(QModelIndexForMainModel const &index);

  template <typename Action>
  void enumerateCellsInRow(QModelIndexForMainModel const &startIndex, int columns, Action const &action) const;

private:
  QList<QStandardItem *> rowItemsFromCells(std::vector<Cell> const &cells);

  QStandardItemModel &m_model;
  Cell m_emptyCellStyle;
};

/**
 * Enumerates the first `columnCount` number of cells to the right of
 * startAtCell, moving left to right.
 */
template <typename Action>
void QtStandardItemTreeModelAdapter::enumerateCellsInRow(QModelIndexForMainModel const &startAtCell, int columnCount,
                                                         Action const &action) const {
  for (auto i = 0; i < columnCount; i++) {
    auto cellIndex = startAtCell.sibling(startAtCell.row(), i);
    action(cellIndex, i);
  }
}

EXPORT_OPT_MANTIDQT_COMMON QStandardItem *modelItemFromIndex(QStandardItemModel &model,
                                                             QModelIndexForMainModel const &index);

EXPORT_OPT_MANTIDQT_COMMON QStandardItem const *modelItemFromIndex(QStandardItemModel const &model,
                                                                   QModelIndexForMainModel const &index);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
