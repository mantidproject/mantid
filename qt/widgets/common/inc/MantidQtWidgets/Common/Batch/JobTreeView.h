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
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include "MantidQtWidgets/Common/Batch/FilteredTreeModel.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
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

class EXPORT_OPT_MANTIDQT_COMMON JobTreeView : public QTreeView, public IJobTreeView {
  Q_OBJECT
public:
  JobTreeView(QStringList const &columnHeadings, Cell const &defaultCellStyle, QWidget *parent = nullptr);

  void filterRowsBy(std::unique_ptr<RowPredicate> predicate) override;
  void filterRowsBy(RowPredicate *predicate) override;
  void resetFilter() override;
  bool hasFilter() const override;

  void setHintsForColumn(int column, std::unique_ptr<HintStrategy> hintStrategy) override;
  void setHintsForColumn(int column, HintStrategy *hintStrategy) override;

  void subscribe(JobTreeViewSubscriber *subscriber) override;

  RowLocation insertChildRowOf(RowLocation const &parent, int beforeRow, std::vector<Cell> const &rowText) override;
  RowLocation insertChildRowOf(RowLocation const &parent, int beforeRow) override;
  RowLocation appendChildRowOf(RowLocation const &parent) override;
  RowLocation appendChildRowOf(RowLocation const &parentLocation, std::vector<Cell> const &rowText) override;
  void appendAndEditAtChildRow() override;
  void appendAndEditAtRowBelow() override;
  void editAtRowAbove() override;

  void removeRowAt(RowLocation const &location) override;
  void removeAllRows() override;
  void removeRows(std::vector<RowLocation> rowsToRemove) override;
  bool isOnlyChildOfRoot(RowLocation const &location) const override;

  void replaceRows(std::vector<RowLocation> replacementPoints, std::vector<Subtree> replacements) override;

  void appendSubtreesAt(RowLocation const &parent, std::vector<Subtree> subtrees) override;
  void appendSubtreeAt(RowLocation const &parent, Subtree const &subtree) override;

  void replaceSubtreeAt(RowLocation const &rootToRemove, Subtree const &toInsert) override;
  void insertSubtreeAt(RowLocation const &parent, int index, Subtree const &subtree) override;

  std::vector<Cell> cellsAt(RowLocation const &location) const override;
  void setCellsAt(RowLocation const &location, std::vector<Cell> const &rowText) override;

  Cell cellAt(RowLocation location, int column) const override;
  void setCellAt(RowLocation location, int column, Cell const &cellText) override;

  void clearSelection() override;
  void expandAll() override;
  void collapseAll() override;

  QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
  std::vector<RowLocation> selectedRowLocations() const override;
  boost::optional<std::vector<Subtree>> selectedSubtrees() const override;
  boost::optional<std::vector<RowLocation>> selectedSubtreeRoots() const override;

  bool hasNoSelectedDescendants(QModelIndex const &index) const;
  void appendAllUnselectedDescendants(QModelIndexList &selectedRows, QModelIndex const &index) const;
  QModelIndexList findImplicitlySelected(QModelIndexList const &selectedRows) const;

  int currentColumn() const override;

  Cell deadCell() const override;
  using QTreeView::edit;

protected:
  void keyPressEvent(QKeyEvent *event) override;
  bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) override;
  void setHeaderLabels(QStringList const &columnHeadings);
  void appendAndEditAtChildRowRequested();
  void appendAndEditAtRowBelowRequested();
  void editAtRowAboveRequested();
  void removeSelectedRequested();
  void copySelectedRequested();
  void cutSelectedRequested();
  void pasteSelectedRequested();
  void enableFiltering();

protected slots:
  void commitData(QWidget * /*editor*/) override;
  void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

private:
  // The view property values for an uneditable, unselectable cell.
  static Cell const g_deadCell;

  bool indexesAreOnSameRow(QModelIndex const &a, QModelIndex const &b) const;

  QModelIndexForMainModel mapToMainModel(QModelIndexForFilteredModel const &filteredModelIndex) const;
  QModelIndexForFilteredModel mapToFilteredModel(QModelIndexForMainModel const &mainModelIndex) const;

  QModelIndexForMainModel fromMainModel(QModelIndex const &mainModelIndex) const;
  QModelIndexForFilteredModel fromFilteredModel(QModelIndex const &filteredModelIndex) const;

  bool isOnlyChild(QModelIndexForMainModel const &index) const;
  bool isOnlyChildOfRoot(QModelIndexForMainModel const &index) const;
  QModelIndex siblingIfExistsElseParent(QModelIndex const &index) const;
  bool rowRemovalWouldBeIneffective(QModelIndexForMainModel const &indexToRemove) const;

  bool isBeingEdited(QModelIndexForFilteredModel const &cellIndex) const;
  bool isEditable(const QModelIndex &index) const;
  void closeEditorIfOpenAtCell(QModelIndexForFilteredModel const &cellIndex);
  void closeAnyOpenEditorsOnUneditableCells(QModelIndexForMainModel const &firstCellOnRow,
                                            std::vector<Cell> const &cells);
  void closeEditorIfCellIsUneditable(QModelIndexForMainModel const &cellIndex, Cell const &cell);

  QModelIndexForFilteredModel expanded(QModelIndexForFilteredModel const &index);
  void editAt(QModelIndexForFilteredModel const &index);

  QtTreeCursorNavigationResult moveNextUntilEditable(QModelIndex const &startingPoint);
  QModelIndex movePreviousUntilEditable(QModelIndex const &startingPoint);
  QModelIndex applyNavigationResult(QtTreeCursorNavigationResult const &result);
  std::pair<QModelIndexForFilteredModel, bool> findOrMakeCellBelow(QModelIndexForFilteredModel const &index);
  bool hasEditorOpen() const;

  QtTreeCursorNavigation navigation() const;
  RowLocationAdapter rowLocation() const;

  JobTreeViewSubscriber *m_notifyee;
  QStandardItemModel m_mainModel;
  QtStandardItemTreeModelAdapter m_adaptedMainModel;

  FilteredTreeModel m_filteredModel;
  QModelIndexForMainModel m_lastEdited;
  bool m_hasEditorOpen;
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
