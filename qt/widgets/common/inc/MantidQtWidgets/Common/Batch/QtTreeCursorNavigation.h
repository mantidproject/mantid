#ifndef MANTIDQTMANTIDWIDGETS_QTTREECURSORNAVIGATION_H_
#define MANTIDQTMANTIDWIDGETS_QTTREECURSORNAVIGATION_H_
#include <QModelIndex>
#include <utility>
namespace MantidQt {
namespace MantidWidgets {

using QtTreeCursorNavigationResult = std::pair<bool, QModelIndex>;
class QtTreeCursorNavigation {
public:
  QtTreeCursorNavigation(QAbstractItemModel const* model);
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
  QAbstractItemModel const* model;
  QtTreeCursorNavigationResult withoutAppendedRow(QModelIndex const &index) const;
  QtTreeCursorNavigationResult withAppendedRow(QModelIndex const& index) const;
};
}
}
#endif // MANTIDQTMANTIDWIDGETS_QTTREECURSORNAVIGATION_H_
