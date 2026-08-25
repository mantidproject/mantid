// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtWorkspaceTreeViewAdapter.h"
#include "WorkspaceTreeView.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <stdexcept>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr itemTypeRole = Qt::UserRole + 1;
auto constexpr reducedOutputTypeRole = Qt::UserRole + 2;
auto constexpr workspaceNameRole = Qt::UserRole + 3;
auto constexpr selectionModeRole = Qt::UserRole + 5;

template <typename Enum> int enumIndex(Enum value) { return static_cast<int>(value); }

QString displayName(PlottingWorkspaceTreeItemType itemType) {
  switch (itemType) {
  case PlottingWorkspaceTreeItemType::ReductionGroup:
    return "Group";
  case PlottingWorkspaceTreeItemType::Run:
    return "Run";
  case PlottingWorkspaceTreeItemType::WorkspaceGroup:
    return "WorkspaceGroup";
  case PlottingWorkspaceTreeItemType::Workspace:
    return "Workspace";
  }
  throw std::runtime_error("Unexpected plotting workspace tree item type.");
}

QString displayName(ReducedWorkspaceOutputType outputType) {
  switch (outputType) {
  case ReducedWorkspaceOutputType::None:
    return "";
  case ReducedWorkspaceOutputType::IvsQ:
    return "IvsQ";
  case ReducedWorkspaceOutputType::IvsLambda:
    return "IvsLambda";
  case ReducedWorkspaceOutputType::IvsQBinned:
    return "IvsQBinned";
  }
  throw std::runtime_error("Unexpected reduced workspace output type.");
}

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

/// Draws column separators for workspace tree rows.
class WorkspaceTreeItemDelegate : public QStyledItemDelegate {
public:
  explicit WorkspaceTreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {
    setObjectName("workspaceTreeItemDelegate");
  }

  void paint(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const override {
    auto itemOption = option;
    if (index.data(WorkspaceTree::mutedRole).toBool()) {
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

QtWorkspaceTreeViewAdapter::QtWorkspaceTreeViewAdapter(WorkspaceTreeView *workspaceTree, QObject *parent)
    : QObject(parent), m_workspaceTree(workspaceTree), m_updatingSelection(false) {
  m_model.setHorizontalHeaderLabels({QString("Item type"), QString("Output type"), QString("Item")});
  m_workspaceTree->setModel(&m_model);
  m_workspaceTree->setTreePosition(ItemColumn);
  m_workspaceTree->setItemDelegate(new WorkspaceTreeItemDelegate(m_workspaceTree));
  m_workspaceTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_workspaceTree->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_workspaceTree->setExpandsOnDoubleClick(false);
  m_workspaceTree->viewport()->installEventFilter(this);
}

void QtWorkspaceTreeViewAdapter::setItems(std::vector<PlottingWorkspaceTreeDisplayItem> const &items) {
  m_model.removeRows(0, m_model.rowCount());
  for (auto const &item : items) {
    addTreeItem(m_model.invisibleRootItem(), item);
  }
  m_workspaceTree->expandAll();
}

void QtWorkspaceTreeViewAdapter::clearSelection() {
  m_updatingSelection = true;
  m_workspaceTree->selectionModel()->clearSelection();
  m_updatingSelection = false;
}

std::vector<std::string> QtWorkspaceTreeViewAdapter::selectedWorkspaceNames() const {
  auto workspaces = std::vector<std::string>{};
  for (auto const &index : m_workspaceTree->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::Workspace && canContributeSelection(selectedIndex)) {
      workspaces.emplace_back(workspaceName(selectedIndex));
    }
  }
  return workspaces;
}

size_t QtWorkspaceTreeViewAdapter::selectedWorkspaceGroupCount() const {
  auto count = size_t{0};
  for (auto const &index : m_workspaceTree->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::WorkspaceGroup && canSelectDirectly(selectedIndex)) {
      ++count;
    }
  }
  return count;
}

void QtWorkspaceTreeViewAdapter::setItemMuted(QStandardItem *parent, int row, bool muted) {
  auto const background = muted ? WorkspaceTree::mutedBackgroundBrush(m_workspaceTree->palette()) : QBrush();
  for (auto column = 0; column < parent->columnCount(); ++column) {
    auto *item = parent->child(row, column);
    item->setBackground(background);
    item->setData(muted, WorkspaceTree::mutedRole);
  }
}

bool QtWorkspaceTreeViewAdapter::eventFilter(QObject *watched, QEvent *event) {
  if (watched != m_workspaceTree->viewport()) {
    return QObject::eventFilter(watched, event);
  }

  if (event->type() == QEvent::MouseButtonPress) {
    return handleWorkspaceTreeClick(*static_cast<QMouseEvent const *>(event));
  }
  if (event->type() == QEvent::MouseMove) {
    auto const mouseEvent = static_cast<QMouseEvent const *>(event);
    return mouseEvent->buttons().testFlag(Qt::LeftButton);
  }
  if (event->type() == QEvent::MouseButtonDblClick) {
    return true;
  }
  return QObject::eventFilter(watched, event);
}

void QtWorkspaceTreeViewAdapter::addTreeItem(QStandardItem *parent, PlottingWorkspaceTreeDisplayItem const &item) {
  auto treeItem = createNonEditableItem(displayName(item.itemType));
  auto outputTypeItem = createNonEditableItem(displayName(item.reducedOutputType));
  auto itemLabel = createNonEditableItem(QString::fromStdString(item.label));
  for (auto *rowItem : {treeItem, outputTypeItem, itemLabel}) {
    rowItem->setData(enumIndex(item.itemType), itemTypeRole);
    rowItem->setData(enumIndex(item.reducedOutputType), reducedOutputTypeRole);
    rowItem->setData(QString::fromStdString(item.workspaceName), workspaceNameRole);
    rowItem->setData(enumIndex(item.selectionMode), selectionModeRole);
  }
  parent->appendRow({treeItem, outputTypeItem, itemLabel});
  setItemMuted(parent, parent->rowCount() - 1, item.muted);
  for (auto const &child : item.children) {
    addTreeItem(treeItem, child);
  }
}

QModelIndex QtWorkspaceTreeViewAdapter::itemIndex(QModelIndex const &index) const {
  return index.sibling(index.row(), ItemTypeColumn);
}

PlottingWorkspaceTreeItemType QtWorkspaceTreeViewAdapter::itemType(QModelIndex const &index) const {
  return static_cast<PlottingWorkspaceTreeItemType>(itemIndex(index).data(itemTypeRole).toInt());
}

std::string QtWorkspaceTreeViewAdapter::workspaceName(QModelIndex const &index) const {
  return itemIndex(index).data(workspaceNameRole).toString().toStdString();
}

bool QtWorkspaceTreeViewAdapter::canSelectDirectly(QModelIndex const &index) const {
  auto const selectionMode =
      static_cast<PlottingWorkspaceTreeSelectionMode>(itemIndex(index).data(selectionModeRole).toInt());
  return selectionMode == PlottingWorkspaceTreeSelectionMode::Direct ||
         selectionMode == PlottingWorkspaceTreeSelectionMode::DirectAndParent;
}

bool QtWorkspaceTreeViewAdapter::canSelectViaParent(QModelIndex const &index) const {
  auto const selectionMode =
      static_cast<PlottingWorkspaceTreeSelectionMode>(itemIndex(index).data(selectionModeRole).toInt());
  return selectionMode == PlottingWorkspaceTreeSelectionMode::ParentOnly ||
         selectionMode == PlottingWorkspaceTreeSelectionMode::DirectAndParent;
}

bool QtWorkspaceTreeViewAdapter::canContributeSelection(QModelIndex const &index) const {
  auto const selectionMode =
      static_cast<PlottingWorkspaceTreeSelectionMode>(itemIndex(index).data(selectionModeRole).toInt());
  return selectionMode != PlottingWorkspaceTreeSelectionMode::None;
}

bool QtWorkspaceTreeViewAdapter::handleWorkspaceTreeClick(QMouseEvent const &event) {
  if (event.button() != Qt::LeftButton) {
    return false;
  }

  auto const clickedIndex = m_workspaceTree->indexAt(event.pos());
  if (!clickedIndex.isValid()) {
    return false;
  }

  auto const index = itemIndex(clickedIndex);
  if (!canSelectDirectly(index)) {
    return true;
  }
  if (hasSelectedAncestor(index)) {
    if (!event.modifiers().testFlag(Qt::ShiftModifier)) {
      selectSubtree(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    return true;
  }

  auto const subtreeSelected = isSubtreeSelected(index);
  auto selectionFlags = QItemSelectionModel::SelectionFlags{subtreeSelected ? QItemSelectionModel::Deselect
                                                                            : QItemSelectionModel::Select};
  if (!isAdditiveSelectionModifier(event) && !subtreeSelected) {
    selectionFlags |= QItemSelectionModel::Clear;
  }

  selectSubtree(index, selectionFlags | QItemSelectionModel::Rows);
  return true;
}

bool QtWorkspaceTreeViewAdapter::isAdditiveSelectionModifier(QMouseEvent const &event) const {
  auto const modifiers = event.modifiers();
  return modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier) ||
         modifiers.testFlag(Qt::ShiftModifier);
}

bool QtWorkspaceTreeViewAdapter::hasSelectedAncestor(QModelIndex const &index) const {
  auto const selectionModel = m_workspaceTree->selectionModel();
  auto ancestor = index.parent();
  while (ancestor.isValid()) {
    if (selectionModel->isSelected(ancestor)) {
      return true;
    }
    ancestor = ancestor.parent();
  }
  return false;
}

bool QtWorkspaceTreeViewAdapter::isSubtreeSelected(QModelIndex const &parentIndex) const {
  auto const selectionModel = m_workspaceTree->selectionModel();
  if (!selectionModel->isSelected(parentIndex)) {
    return false;
  }

  auto const rows = m_model.rowCount(parentIndex);
  for (auto row = 0; row < rows; ++row) {
    auto const childIndex = m_model.index(row, 0, parentIndex);
    if (!isSubtreeSelected(childIndex)) {
      return false;
    }
  }
  return true;
}

void QtWorkspaceTreeViewAdapter::selectSubtree(QModelIndex const &parentIndex,
                                               QItemSelectionModel::SelectionFlags selectionFlags) {
  m_updatingSelection = true;
  if (selectionFlags.testFlag(QItemSelectionModel::Clear)) {
    m_workspaceTree->selectionModel()->clearSelection();
    selectionFlags &= ~QItemSelectionModel::Clear;
  }
  m_workspaceTree->selectionModel()->select(parentIndex, selectionFlags);
  updateChildSelection(parentIndex, selectionFlags);
  m_updatingSelection = false;
}

void QtWorkspaceTreeViewAdapter::updateChildSelection(QItemSelection const &selection,
                                                      QItemSelectionModel::SelectionFlags selectionFlags) {
  if (m_updatingSelection) {
    return;
  }

  m_updatingSelection = true;
  for (auto const &index : selection.indexes()) {
    if (index.column() == ItemTypeColumn) {
      updateChildSelection(index, selectionFlags);
    }
  }
  m_updatingSelection = false;
}

void QtWorkspaceTreeViewAdapter::updateChildSelection(QModelIndex const &parentIndex,
                                                      QItemSelectionModel::SelectionFlags selectionFlags) {
  auto const rows = m_model.rowCount(parentIndex);
  for (auto row = 0; row < rows; ++row) {
    auto const childIndex = m_model.index(row, 0, parentIndex);
    if (selectionFlags.testFlag(QItemSelectionModel::Deselect) || canSelectDirectly(childIndex) ||
        canSelectViaParent(childIndex)) {
      m_workspaceTree->selectionModel()->select(childIndex, selectionFlags | QItemSelectionModel::Rows);
    }
    updateChildSelection(childIndex, selectionFlags);
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
