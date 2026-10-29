// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingWorkspaceTreeView.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <algorithm>
#include <iterator>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
QStandardItem *createNonEditableItem(QString const &text) {
  auto item = new QStandardItem(text);
  item->setEditable(false);
  return item;
}

void applyMutedTextPalette(QStyleOptionViewItem &option) {
  auto const mutedText = option.palette.brush(QPalette::Disabled, QPalette::Text);
  option.palette.setBrush(QPalette::Active, QPalette::Text, mutedText);
  option.palette.setBrush(QPalette::Inactive, QPalette::Text, mutedText);
  option.palette.setBrush(QPalette::Active, QPalette::WindowText, mutedText);
  option.palette.setBrush(QPalette::Inactive, QPalette::WindowText, mutedText);
}

/// Draws column separators for plotting workspace tree rows.
class PlottingWorkspaceTreeItemDelegate : public QStyledItemDelegate {
public:
  explicit PlottingWorkspaceTreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {
    setObjectName("plottingWorkspaceTreeItemDelegate");
  }

  void paint(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const override {
    auto itemOption = option;
    if (index.data(PlottingWorkspaceTreeView::mutedRole).toBool()) {
      applyMutedTextPalette(itemOption);
    }
    QStyledItemDelegate::paint(painter, itemOption, index);

    if (index.column() < index.model()->columnCount(index.parent()) - 1) {
      painter->save();
      painter->setPen(QColor(214, 214, 214));
      painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
      painter->restore();
    }
  }
};
} // namespace

QtPlottingWorkspaceTreeView::QtPlottingWorkspaceTreeView(QWidget *parent) : QTreeView(parent) {
  m_model.setHorizontalHeaderLabels({QString("Item type"), QString("Output type"), QString("Item")});
  QTreeView::setModel(&m_model);
  setTreePosition(ItemColumn);
  setItemDelegate(new PlottingWorkspaceTreeItemDelegate(this));
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setExpandsOnDoubleClick(false);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          [this](QItemSelection const &selected, QItemSelection const &deselected) {
            if (m_notifyee) {
              m_notifyee->notifySelectionChanged(pathsForSelection(deselected), pathsForSelection(selected));
            }
            emit workspaceSelectionChanged();
          });
}

void QtPlottingWorkspaceTreeView::setItemMuted(QStandardItem *parent, int row, bool muted) {
  auto const background = muted ? PlottingWorkspaceTreeView::mutedBackgroundBrush(palette()) : QBrush();
  for (auto column = 0; column < parent->columnCount(); ++column) {
    auto *item = parent->child(row, column);
    item->setBackground(background);
    item->setData(muted, PlottingWorkspaceTreeView::mutedRole);
  }
}

bool QtPlottingWorkspaceTreeView::viewportEvent(QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress &&
      handlePlottingWorkspaceTreeClick(*static_cast<QMouseEvent const *>(event))) {
    return true;
  }
  if (event->type() == QEvent::MouseMove &&
      static_cast<QMouseEvent const *>(event)->buttons().testFlag(Qt::LeftButton)) {
    return true;
  }
  if (event->type() == QEvent::MouseButtonDblClick) {
    return true;
  }
  return QTreeView::viewportEvent(event);
}

void QtPlottingWorkspaceTreeView::addItem(QStandardItem *parent, PlottingWorkspaceTreeViewItem const &itemState) {
  auto treeItem = createNonEditableItem(QString::fromStdString(itemState.itemTypeLabel));
  auto outputTypeItem = createNonEditableItem(QString::fromStdString(itemState.outputTypeLabel));
  auto itemLabel = createNonEditableItem(QString::fromStdString(itemState.label));
  parent->appendRow({treeItem, outputTypeItem, itemLabel});
  setItemMuted(parent, parent->rowCount() - 1, itemState.muted);
  for (auto const &child : itemState.children) {
    addItem(treeItem, child);
  }
}

void QtPlottingWorkspaceTreeView::subscribe(PlottingWorkspaceTreeAdapterViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QtPlottingWorkspaceTreeView::setItems(std::vector<PlottingWorkspaceTreeViewItem> const &items) {
  m_model.removeRows(0, m_model.rowCount());
  for (auto const &itemState : items) {
    addItem(m_model.invisibleRootItem(), itemState);
  }
}

void QtPlottingWorkspaceTreeView::expandAll() { QTreeView::expandAll(); }

void QtPlottingWorkspaceTreeView::setVisible(PlottingWorkspaceTreePath const &path, bool visible) {
  auto const index = indexForPath(path);
  setRowHidden(index.row(), index.parent(), !visible);
}

bool QtPlottingWorkspaceTreeView::isVisible(PlottingWorkspaceTreePath const &path) const {
  auto const index = indexForPath(path);
  return !isRowHidden(index.row(), index.parent());
}

void QtPlottingWorkspaceTreeView::clearSelection() { selectionModel()->clearSelection(); }

void QtPlottingWorkspaceTreeView::setSelected(PlottingWorkspaceTreePath const &path, bool selected) {
  auto const action = selected ? QItemSelectionModel::Select : QItemSelectionModel::Deselect;
  selectionModel()->select(indexForPath(path), action | QItemSelectionModel::Rows);
}

bool QtPlottingWorkspaceTreeView::isSelected(PlottingWorkspaceTreePath const &path) const {
  return selectionModel()->isSelected(indexForPath(path));
}

std::vector<PlottingWorkspaceTreePath> QtPlottingWorkspaceTreeView::selectedItems() const {
  auto paths = std::vector<PlottingWorkspaceTreePath>{};
  auto const rows = selectionModel()->selectedRows();
  std::transform(rows.cbegin(), rows.cend(), std::back_inserter(paths),
                 [this](QModelIndex const &index) { return pathForIndex(index); });
  return paths;
}

PlottingWorkspaceTreePath QtPlottingWorkspaceTreeView::pathForIndex(QModelIndex index) const {
  auto path = PlottingWorkspaceTreePath{};
  while (index.isValid()) {
    path.emplace_back(index.row());
    index = index.parent();
  }
  std::reverse(path.begin(), path.end());
  return path;
}

QModelIndex QtPlottingWorkspaceTreeView::indexForPath(PlottingWorkspaceTreePath const &path) const {
  auto index = QModelIndex{};
  for (auto row : path) {
    index = m_model.index(row, ItemTypeColumn, index);
  }
  return index;
}

std::vector<PlottingWorkspaceTreePath>
QtPlottingWorkspaceTreeView::pathsForSelection(QItemSelection const &selection) const {
  auto paths = std::vector<PlottingWorkspaceTreePath>{};
  for (auto const &index : selection.indexes()) {
    if (index.column() == ItemTypeColumn) {
      paths.emplace_back(pathForIndex(index));
    }
  }
  return paths;
}

bool QtPlottingWorkspaceTreeView::handlePlottingWorkspaceTreeClick(QMouseEvent const &event) {
  if (event.button() != Qt::LeftButton || !m_notifyee) {
    return false;
  }
  auto const index = indexAt(event.pos());
  if (!index.isValid()) {
    return false;
  }
  auto const modifiers = event.modifiers();
  auto const shift = modifiers.testFlag(Qt::ShiftModifier);
  auto const additive = modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier) || shift;
  m_notifyee->notifyItemClicked(pathForIndex(index), additive, shift);
  return true;
}

void QtPlottingWorkspaceTreeView::drawRow(QPainter *painter, QStyleOptionViewItem const &option,
                                          QModelIndex const &index) const {
  if (index.data(PlottingWorkspaceTreeView::mutedRole).toBool()) {
    painter->fillRect(option.rect, PlottingWorkspaceTreeView::mutedBackgroundBrush(palette()));
  }

  QTreeView::drawRow(painter, option, index);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
