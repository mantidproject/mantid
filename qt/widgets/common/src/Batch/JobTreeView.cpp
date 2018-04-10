#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <QKeyEvent>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

JobTreeView::JobTreeView(QStringList const &columnHeadings, QWidget *parent)
    : QTreeView(parent), m_model(this) {
  setModel(&m_model);
  setHeaderLabels(columnHeadings);
}

void JobTreeView::setHeaderLabels(QStringList const &columnHeadings) {
  m_model.setHorizontalHeaderLabels(columnHeadings);

  for (auto i = 0; i < model()->columnCount(); ++i)
    resizeColumnToContents(i);
}

void JobTreeView::subscribe(IJobTreeViewSubscriber &subscriber) {
  m_notifyee = &subscriber;
}

QModelIndex JobTreeView::modelIndexAt(RowLocation const &location,
                                      int column) const {
  auto parentIndex = adaptedModel().rootModelIndex();
  if (location.isRoot()) {
    return parentIndex;
  } else {
    auto &path = location.path();
    for (auto it = path.cbegin(); it != path.cend() - 1; ++it)
      parentIndex = model()->index(*it, column, parentIndex);
    assertOrThrow(
        model()->hasIndex(location.rowRelativeToParent(), 0, parentIndex),
        "modelIndexAt: Location refers to an index which does not "
        "exist in the model.");
    return model()->index(location.rowRelativeToParent(), column, parentIndex);
  }
}

RowLocation JobTreeView::rowLocationAt(QModelIndex const &index) const {
  if (index.isValid()) {
    auto pathComponents = RowPath();
    auto currentIndex = index;
    while (currentIndex.isValid()) {
      pathComponents.insert(pathComponents.begin(), currentIndex.row());
      currentIndex = index.parent();
    }
    return RowLocation(pathComponents);
  } else {
    return RowLocation({});
  }
}

void JobTreeView::removeRowAt(RowLocation const &location) {
  adaptedModel().removeRowAt(modelIndexAt(location));
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

QModelIndex JobTreeView::editAt(QModelIndex const &index) {
  clearSelection();
  setCurrentIndex(index);
  edit(index);
  return index;
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

QModelIndex JobTreeView::findOrMakeCellBelow(QModelIndex const &index) {
  if (navigation().isNotLastRowInThisNode(index)) {
    return moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);
  } else {
    return adaptedModel().appendEmptySiblingRow(index);
  }
}

void JobTreeView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Return) {
    event->accept();
    if (event->modifiers() & Qt::ControlModifier)
      editAt(adaptedModel().appendEmptyChildRow(currentIndex()));
    else {
      auto below = findOrMakeCellBelow(currentIndex());
      editAt(expanded(below));
    }
  } else {
    QTreeView::keyPressEvent(event);
  }
}

QModelIndex
JobTreeView::applyNavigationResult(QtTreeCursorNavigationResult const &result) {
  auto shouldMakeNewRowBelow = result.first;
  if (shouldMakeNewRowBelow)
    return expanded(adaptedModel().appendEmptySiblingRow(result.second));
  else
    return result.second;
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
