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
#ifndef MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
#define MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include "MantidQtWidgets/Common/Batch/FilteredTreeModel.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>

#include <QTreeView>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON JobTreeViewSubscriber {
public:
  virtual void notifyCellTextChanged(RowLocation const &itemIndex, int column,
                                     std::string const &oldValue,
                                     std::string const &newValue) = 0;
  virtual void notifyRowInserted(RowLocation const &newRowLocation) = 0;
  virtual void notifyRemoveRowsRequested(
      std::vector<RowLocation> const &locationsOfRowsToRemove) = 0;
  virtual void notifyCopyRowsRequested() = 0;
  virtual void notifyPasteRowsRequested() = 0;
  virtual void notifyFilterReset() = 0;
  virtual ~JobTreeViewSubscriber() = default;
};

class EXPORT_OPT_MANTIDQT_COMMON JobTreeView : public QTreeView {
  Q_OBJECT
public:
  JobTreeView(QStringList const &columnHeadings, Cell const &defaultCellStyle,
              QWidget *parent = nullptr);

  void filterRowsBy(std::unique_ptr<RowPredicate> predicate);
  void filterRowsBy(RowPredicate *predicate);
  void resetFilter();
  bool hasFilter() const;

  void subscribe(JobTreeViewSubscriber &subscriber);

  void insertChildRowOf(RowLocation const &parent, int beforeRow,
                        std::vector<Cell> const &rowText);
  void insertChildRowOf(RowLocation const &parent, int beforeRow);
  void appendChildRowOf(RowLocation const &parent);
  void appendChildRowOf(RowLocation const &parentLocation,
                        std::vector<Cell> const &rowText);

  void removeRowAt(RowLocation const &location);
  void removeRows(std::vector<RowLocation> rowsToRemove);
  void removeAllRows();
  bool isOnlyChildOfRoot(RowLocation const &index) const;

  template <typename InputIterator>
  void removeRows(InputIterator begin, InputIterator end);

  void replaceRows(std::vector<RowLocation> replacementPoints,
                   std::vector<Subtree> replacements);

  void appendSubtreesAt(RowLocation const &parent,
                        std::vector<Subtree> subtrees);
  void appendSubtreeAt(RowLocation const &parent, Subtree const &subtree);

  void replaceSubtreeAt(RowLocation const &rootToRemove,
                        Subtree const &toInsert);
  void insertSubtreeAt(RowLocation const &parent, int index,
                       Subtree const &subtree);

  std::vector<Cell> cellsAt(RowLocation const &location) const;
  void setCellsAt(RowLocation const &location,
                  std::vector<Cell> const &rowText);

  Cell cellAt(RowLocation location, int column) const;
  void setCellAt(RowLocation location, int column, Cell const &cellText);

  QModelIndex moveCursor(CursorAction cursorAction,
                         Qt::KeyboardModifiers modifiers) override;
  std::vector<RowLocation> selectedRowLocations() const;
  boost::optional<std::vector<Subtree>> selectedSubtrees() const;
  boost::optional<std::vector<RowLocation>> selectedSubtreeRoots() const;

  bool hasNoSelectedDescendants(QModelIndex const &index) const;
  void appendAllUnselectedDescendants(QModelIndexList &selectedRows,
                                      QModelIndex const &index) const;
  QModelIndexList
  findImplicitlySelected(QModelIndexList const &selectedRows) const;

  Cell deadCell() const;
  using QTreeView::edit;

protected:
  void keyPressEvent(QKeyEvent *event) override;
  bool edit(const QModelIndex &index, EditTrigger trigger,
            QEvent *event) override;
  void setHeaderLabels(QStringList const &columnHeadings);
  void removeSelectedRequested();
  void copySelectedRequested();
  void pasteSelectedRequested();

protected slots:
  void commitData(QWidget *) override;

private:
  // The view property values for an uneditable, unselectable cell.
  static Cell const g_deadCell;

  void appendAndEditAtChildRow();
  void appendAndEditAtRowBelow();
  void editAtRowAbove();
  bool indexesAreOnSameRow(QModelIndex const &a, QModelIndex const &b) const;

  QModelIndexForMainModel
  mapToMainModel(QModelIndexForFilteredModel const &filteredModelIndex) const;
  QModelIndexForFilteredModel
  mapToFilteredModel(QModelIndexForMainModel const &mainModelIndex) const;

  QModelIndexForMainModel
  fromMainModel(QModelIndex const &mainModelIndex) const;
  QModelIndexForFilteredModel
  fromFilteredModel(QModelIndex const &filteredModelIndex) const;

  bool isOnlyChild(QModelIndexForMainModel const &index) const;
  bool isOnlyChildOfRoot(QModelIndexForMainModel const &index) const;
  QModelIndex siblingIfExistsElseParent(QModelIndex const &index) const;
  bool rowRemovalWouldBeIneffective(
      QModelIndexForMainModel const &indexToRemove) const;

  bool isBeingEdited(QModelIndexForFilteredModel const &cellIndex) const;
  bool isEditable(const QModelIndex &index) const;
  void closeEditorIfOpenAtCell(QModelIndexForFilteredModel const &cellIndex);
  void closeAnyOpenEditorsOnUneditableCells(
      QModelIndexForMainModel const &firstCellOnRow,
      std::vector<Cell> const &cells);
  void closeEditorIfCellIsUneditable(QModelIndexForMainModel const &cellIndex,
                                     Cell const &cell);

  QModelIndexForFilteredModel
  expanded(QModelIndexForFilteredModel const &index);
  void editAt(QModelIndexForFilteredModel const &index);

  QtTreeCursorNavigationResult
  moveNextUntilEditable(QModelIndex const &startingPoint);
  QModelIndex movePreviousUntilEditable(QModelIndex const &startingPoint);
  QModelIndex applyNavigationResult(QtTreeCursorNavigationResult const &result);
  std::pair<QModelIndexForFilteredModel, bool>
  findOrMakeCellBelow(QModelIndexForFilteredModel const &index);
  bool hasEditorOpen() const;

  void enableFiltering();

  QtTreeCursorNavigation navigation() const;
  RowLocationAdapter rowLocation() const;

  JobTreeViewSubscriber *m_notifyee;
  QStandardItemModel m_mainModel;
  QtStandardItemTreeModelAdapter m_adaptedMainModel;

  FilteredTreeModel m_filteredModel;
  QModelIndexForMainModel m_lastEdited;
  bool m_hasEditorOpen;
};

template <typename InputIterator>
void JobTreeView::removeRows(InputIterator begin, InputIterator end) {
  removeRows(std::vector<RowLocation>(begin, end));
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
