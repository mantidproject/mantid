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
#ifndef MANTIDQTMANTIDWIDGETS_QTTREECURSORNAVIGATION_H_
#define MANTIDQTMANTIDWIDGETS_QTTREECURSORNAVIGATION_H_
#include <QModelIndex>
#include <utility>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

using QtTreeCursorNavigationResult = std::pair<bool, QModelIndex>;
class QtTreeCursorNavigation {
public:
  QtTreeCursorNavigation(QAbstractItemModel const *model);
  QModelIndex moveCursorPrevious(QModelIndex const &currentIndex) const;
  QtTreeCursorNavigationResult
  moveCursorNext(QModelIndex const &currentIndex) const;

  QModelIndex previousCellInThisRow(QModelIndex const &index) const;
  QModelIndex lastCellInPreviousRow(QModelIndex const &index) const;
  QModelIndex lastCellInParentRowElseNone(QModelIndex const &index) const;
  QModelIndex firstCellOnNextRow(QModelIndex const &rowAbove) const;
  QModelIndex nextCellOnThisRow(QModelIndex const &index) const;
  QModelIndex lastRowInThisNode(QModelIndex const &index) const;

  bool isNotLastCellOnThisRow(QModelIndex const &index) const;
  bool isNotLastRowInThisNode(QModelIndex const &index) const;
  bool isNotFirstCellInThisRow(QModelIndex const &index) const;
  bool isNotFirstRowInThisNode(QModelIndex const &index) const;

private:
  QAbstractItemModel const *m_model;
  QtTreeCursorNavigationResult
  withoutAppendedRow(QModelIndex const &index) const;
  QtTreeCursorNavigationResult withAppendedRow(QModelIndex const &index) const;
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_QTTREECURSORNAVIGATION_H_
