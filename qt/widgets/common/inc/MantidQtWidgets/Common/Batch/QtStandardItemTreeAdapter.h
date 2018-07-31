/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
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
                                           std::vector<Cell> const &cells);
  QModelIndexForMainModel
  appendEmptySiblingRow(QModelIndexForMainModel const &index);

  QModelIndexForMainModel appendChildRow(QModelIndexForMainModel const &parent,
                                         std::vector<Cell> const &cells);
  QModelIndexForMainModel
  appendEmptyChildRow(QModelIndexForMainModel const &parent);

  QModelIndexForMainModel insertChildRow(QModelIndexForMainModel const &parent,
                                         int row,
                                         std::vector<Cell> const &cells);
  QModelIndexForMainModel
  insertEmptyChildRow(QModelIndexForMainModel const &parent, int column);

  void removeRowFrom(QModelIndexForMainModel const &index);

  template <typename Action>
  void enumerateCellsInRow(QModelIndexForMainModel const &startIndex,
                           int columns, Action const &action) const;

  void removeAllRows();

private:
  QList<QStandardItem *> rowItemsFromCells(std::vector<Cell> const &cells);
  QStringList getHeaderData() const;
  void setHeaderData(QStringList const &headerData);

  QStandardItemModel &m_model;
  Cell m_emptyCellStyle;
};

/**
 * Enumerates the first `columnCount` number of cells to the right of
 * startAtCell, moving left to right.
 */
template <typename Action>
void QtStandardItemTreeModelAdapter::enumerateCellsInRow(
    QModelIndexForMainModel const &startAtCell, int columnCount,
    Action const &action) const {
  for (auto i = 0; i < columnCount; i++) {
    auto cellIndex = startAtCell.sibling(startAtCell.row(), i);
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
