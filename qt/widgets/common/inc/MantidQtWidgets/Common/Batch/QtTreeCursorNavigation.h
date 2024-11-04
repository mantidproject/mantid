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
  QtTreeCursorNavigationResult moveCursorNext(QModelIndex const &currentIndex) const;

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
  QtTreeCursorNavigationResult withoutAppendedRow(QModelIndex const &index) const;
  QtTreeCursorNavigationResult withAppendedRow(QModelIndex const &index) const;
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
