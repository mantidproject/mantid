#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/CellDelegate.h"
#include <QKeyEvent>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <algorithm>
#include <iostream>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

JobTreeView::JobTreeView(QStringList const &columnHeadings, QWidget *parent)
    : QTreeView(parent), m_model(this) {
  setModel(&m_model);
  setHeaderLabels(columnHeadings);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setItemDelegate(new CellDelegate(this, *this));
}

void JobTreeView::commitData(QWidget *editor) {
  QTreeView::commitData(editor);

  auto location = rowLocationAt(m_lastEdited);
  auto *const cell = adaptedModel().modelItemFromIndex(m_lastEdited);
  auto cellText = cell->text().toStdString();

  m_notifyee->notifyCellChanged(rowLocationAt(m_lastEdited),
                                m_lastEdited.column(), cellText);
  m_hasEditorOpen = false;
}

bool JobTreeView::edit(const QModelIndex &index, EditTrigger trigger,
                       QEvent *event) {
  m_lastEdited = index;
  m_hasEditorOpen = true;
  return QTreeView::edit(index, trigger, event);
}

std::vector<RowLocation> JobTreeView::selectedRowLocations() const {
  auto selection = selectionModel()->selectedRows();
  std::vector<RowLocation> rowSelection;
  rowSelection.reserve(selection.size());
  std::transform(selection.begin(), selection.end(),
                 std::back_inserter(rowSelection),
                 [&](QModelIndex const &index)
                     -> RowLocation { return rowLocationAt(index); });
  return rowSelection;
}

void JobTreeView::removeSelectedRequested() {
  m_notifyee->notifyRemoveRowsRequested(selectedRowLocations());
}

void JobTreeView::copySelectedRequested() {
  m_notifyee->notifyCopyRowsRequested(selectedRowLocations());
}

void JobTreeView::removeRows(std::vector<RowLocation> rowsToRemove) {
  std::sort(rowsToRemove.begin(), rowsToRemove.end());
  for (auto rowIt = rowsToRemove.crbegin(); rowIt < rowsToRemove.crend();
       ++rowIt)
    removeRowAt(*rowIt);
}

std::string JobTreeView::textAt(RowLocation location, int column) const {
  auto const cellIndex = modelIndexAt(location, column);
  return adaptedModel().textFromCell(cellIndex);
}

void JobTreeView::setTextAt(RowLocation location, int column,
                            std::string const &cellText) {
  auto const cellIndex = modelIndexAt(location, column);
  adaptedModel().setTextAtCell(cellIndex, cellText);
}

void JobTreeView::setRowTextAt(RowLocation const &location,
                               std::vector<std::string> const &rowText) {}

std::vector<std::string>
JobTreeView::rowTextAt(RowLocation const &location) const {
  return std::vector<std::string>();
}

void JobTreeView::setHeaderLabels(QStringList const &columnHeadings) {
  m_model.setHorizontalHeaderLabels(columnHeadings);

  for (auto i = 0; i < model()->columnCount(); ++i)
    resizeColumnToContents(i);
}

void JobTreeView::subscribe(JobTreeViewSubscriber &subscriber) {
  m_notifyee = &subscriber;
}

boost::optional<QModelIndex>
JobTreeView::modelIndexIfExistsAt(RowLocation const &location,
                                  int column) const {
  auto parentIndex = adaptedModel().rootModelIndex();
  if (location.isRoot()) {
    return parentIndex;
  } else {
    auto &path = location.path();
    for (auto it = path.cbegin(); it != path.cend() - 1; ++it)
      parentIndex = model()->index(*it, column, parentIndex);
    if (model()->hasIndex(location.rowRelativeToParent(), column, parentIndex))
      return model()->index(location.rowRelativeToParent(), column,
                            parentIndex);
    else
      return boost::none;
  }
}

QModelIndex JobTreeView::modelIndexAt(RowLocation const &location,
                                      int column) const {
  auto maybeIndex = modelIndexIfExistsAt(location, column);
  if (maybeIndex.is_initialized())
    return maybeIndex.get();
  else
    throw std::runtime_error("modelIndexAt: Attempted to get model index for "
                             "row location which does not exist.");
}

RowLocation JobTreeView::rowLocationAt(QModelIndex const &index) const {
  if (index.isValid()) {
    auto pathComponents = RowPath();
    auto currentIndex = index;
    while (currentIndex.isValid()) {
      pathComponents.insert(pathComponents.begin(), currentIndex.row());
      currentIndex = currentIndex.parent();
    }
    return RowLocation(pathComponents);
  } else {
    return RowLocation();
  }
}

bool JobTreeView::indexesAreOnSameRow(QModelIndex const &a,
                                      QModelIndex const &b) const {
  return a.parent() == b.parent() && a.row() == b.row() &&
         a.model() == b.model();
}

bool JobTreeView::hasEditorOpen() const { return m_lastEdited.isValid(); }

bool JobTreeView::rowRemovalWouldBeIneffective(
    QModelIndex const &indexToRemove) const {
  return indexesAreOnSameRow(currentIndex(), indexToRemove) && hasEditorOpen();
}

QModelIndex
JobTreeView::siblingIfExistsElseParent(QModelIndex const &index) const {
  if (navigation().isNotFirstRowInThisNode(index)) {
    return index.sibling(index.row() - 1, index.column());
  } else if (navigation().isNotLastRowInThisNode(index)) {
    return index.sibling(index.row() + 1, index.column());
  } else {
    return index.parent();
  }
}

bool JobTreeView::isOnlyChild(QModelIndex const &index) const {
  auto parentIndex = model()->parent(index);
  return model()->rowCount(parentIndex) == 1;
}

bool JobTreeView::isOnlyChildOfRoot(QModelIndex const &index) const {
  return !model()->parent(index).isValid() && isOnlyChild(index);
}

void JobTreeView::removeRowAt(RowLocation const &location) {
  auto indexToRemove = modelIndexAt(location);
  assertOrThrow(indexToRemove.isValid(),
                "removeRowAt: Attempted to remove the invisible root item.");
  assertOrThrow(!isOnlyChildOfRoot(indexToRemove),
                "Attempted to delete the only child of the invisible root "
                "item. Try removeAllRows() instead.");
  if (rowRemovalWouldBeIneffective(indexToRemove)) {
    auto loc = rowLocationAt(siblingIfExistsElseParent(indexToRemove));
    std::cout << "Setting current index to " << loc << std::endl;
    setCurrentIndex(siblingIfExistsElseParent(indexToRemove));
  }
  adaptedModel().removeRowAt(indexToRemove);
}

void JobTreeView::insertChildRowOf(RowLocation const &parent, int beforeRow) {
  adaptedModel().insertEmptyChildRow(modelIndexAt(parent), beforeRow);
}

void JobTreeView::insertChildRowOf(RowLocation const &parent, int beforeRow,
                                   std::vector<std::string> const &rowText) {
  adaptedModel().insertChildRow(modelIndexAt(parent), beforeRow,
                                adaptedModel().rowFromRowText(rowText));
}

void JobTreeView::appendChildRowOf(RowLocation const &parent) {
  adaptedModel().appendEmptyChildRow(modelIndexAt(parent));
}

void JobTreeView::appendChildRowOf(RowLocation const &parent,
                                   std::vector<std::string> const &rowText) {
  adaptedModel().appendChildRow(modelIndexAt(parent),
                                adaptedModel().rowFromRowText(rowText));
}

void JobTreeView::editAt(QModelIndex const &index) {
  clearSelection();
  setCurrentIndex(index);
  edit(index);
}

QModelIndex JobTreeView::expanded(QModelIndex const &index) {
  auto expandAt = index;
  while (expandAt.isValid()) {
    setExpanded(expandAt, true);
    expandAt = model()->parent(expandAt);
  }
  return index;
}

QtStandardItemMutableTreeAdapter JobTreeView::adaptedModel() {
  return QtStandardItemMutableTreeAdapter(m_model);
}

QtStandardItemTreeAdapter const JobTreeView::adaptedModel() const {
  return QtStandardItemTreeAdapter(m_model);
}

QtTreeCursorNavigation JobTreeView::navigation() const {
  return QtTreeCursorNavigation(model());
}

std::pair<QModelIndex, bool>
JobTreeView::findOrMakeCellBelow(QModelIndex const &index) {
  if (navigation().isNotLastRowInThisNode(index)) {
    return std::make_pair(index.sibling(index.row() + 1, index.column()),
                          false);
  } else {
    std::cout << "input index for appendEmptySiblingRow is " << index.column()
              << std::endl;
    return std::make_pair(adaptedModel().appendEmptySiblingRow(index), true);
  }
}

void JobTreeView::appendAndEditAtChildRow() {
  auto const parent = currentIndex();
  auto const child = adaptedModel().appendEmptyChildRow(parent);
  editAt(expanded(child));
  m_notifyee->notifyRowInserted(rowLocationAt(child));
}

void JobTreeView::appendAndEditAtRowBelow() {
  auto const below = findOrMakeCellBelow(currentIndex());
  auto index = below.first;
  auto isNew = below.second;
  editAt(index);
  if (isNew)
    m_notifyee->notifyRowInserted(rowLocationAt(index));
}

void JobTreeView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Return) {
    if (event->modifiers() & Qt::ControlModifier) {
      appendAndEditAtChildRow();
    } else if (event->modifiers() & Qt::ShiftModifier) {
      // Go to row above
    } else {
      appendAndEditAtRowBelow();
    }
  } else if (event->key() == Qt::Key_Delete) {
    removeSelectedRequested();
  } else if (event->key() == Qt::Key_C) {
    if (event->modifiers() & Qt::ControlModifier) {
      copySelectedRequested();
    }
  } else {
    QTreeView::keyPressEvent(event);
  }
}

QModelIndex
JobTreeView::applyNavigationResult(QtTreeCursorNavigationResult const &result) {
  auto shouldMakeNewRowBelow = result.first;
  if (shouldMakeNewRowBelow) {
    auto newIndex = adaptedModel().appendEmptySiblingRow(result.second);
    auto newRowIndex = newIndex.sibling(newIndex.row(), 0);
    auto newRowLocation = rowLocationAt(newRowIndex);

    m_notifyee->notifyRowInserted(newRowLocation);

    if (modelIndexIfExistsAt(newRowLocation).is_initialized())
      return expanded(newRowIndex);
    else
      return QModelIndex();
  } else {
    return result.second;
  }
}

QModelIndex JobTreeView::moveCursor(CursorAction cursorAction,
                                    Qt::KeyboardModifiers modifiers) {
  if (cursorAction == QAbstractItemView::MoveNext) {
    return applyNavigationResult(navigation().moveCursorNext(currentIndex()));
  } else if (cursorAction == QAbstractItemView::MovePrevious) {
    return navigation().moveCursorPrevious(currentIndex());
  } else {
    return QTreeView::moveCursor(cursorAction, modifiers);
  }
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
