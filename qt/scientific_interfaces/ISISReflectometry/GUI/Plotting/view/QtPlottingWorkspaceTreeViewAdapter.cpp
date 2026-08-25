// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingWorkspaceTreeViewAdapter.h"
#include "QtPlottingWorkspaceTreeView.h"

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

QtPlottingWorkspaceTreeViewAdapter::QtPlottingWorkspaceTreeViewAdapter(
    QtPlottingWorkspaceTreeView *plottingWorkspaceTreeView, QObject *parent)
    : QObject(parent), m_plottingWorkspaceTreeView(plottingWorkspaceTreeView), m_updatingSelection(false) {
  m_model.setHorizontalHeaderLabels({QString("Item type"), QString("Output type"), QString("Item")});
  m_plottingWorkspaceTreeView->setModel(&m_model);
  m_plottingWorkspaceTreeView->setTreePosition(ItemColumn);
  m_plottingWorkspaceTreeView->setItemDelegate(new PlottingWorkspaceTreeItemDelegate(m_plottingWorkspaceTreeView));
  m_plottingWorkspaceTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_plottingWorkspaceTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_plottingWorkspaceTreeView->setExpandsOnDoubleClick(false);
  m_plottingWorkspaceTreeView->viewport()->installEventFilter(this);
}

void QtPlottingWorkspaceTreeViewAdapter::setPlottingWorkspaceTreeItemStates(
    std::vector<PlottingWorkspaceTreeItemState> const &itemStates) {
  m_model.removeRows(0, m_model.rowCount());
  for (auto const &itemState : itemStates) {
    addPlottingWorkspaceTreeItemState(m_model.invisibleRootItem(), itemState);
  }
  m_plottingWorkspaceTreeView->expandAll();
}

void QtPlottingWorkspaceTreeViewAdapter::clearSelection() {
  m_updatingSelection = true;
  m_plottingWorkspaceTreeView->selectionModel()->clearSelection();
  m_updatingSelection = false;
}

std::vector<std::string> QtPlottingWorkspaceTreeViewAdapter::selectedPlottingWorkspaceNames() const {
  auto workspaces = std::vector<std::string>{};
  for (auto const &index : m_plottingWorkspaceTreeView->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::Workspace && canContributeSelection(selectedIndex)) {
      workspaces.emplace_back(workspaceName(selectedIndex));
    }
  }
  return workspaces;
}

size_t QtPlottingWorkspaceTreeViewAdapter::selectedPlottingWorkspaceGroupCount() const {
  auto count = size_t{0};
  for (auto const &index : m_plottingWorkspaceTreeView->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::WorkspaceGroup && canSelectDirectly(selectedIndex)) {
      ++count;
    }
  }
  return count;
}

void QtPlottingWorkspaceTreeViewAdapter::setItemMuted(QStandardItem *parent, int row, bool muted) {
  auto const background =
      muted ? PlottingWorkspaceTreeView::mutedBackgroundBrush(m_plottingWorkspaceTreeView->palette()) : QBrush();
  for (auto column = 0; column < parent->columnCount(); ++column) {
    auto *item = parent->child(row, column);
    item->setBackground(background);
    item->setData(muted, PlottingWorkspaceTreeView::mutedRole);
  }
}

bool QtPlottingWorkspaceTreeViewAdapter::eventFilter(QObject *watched, QEvent *event) {
  if (watched != m_plottingWorkspaceTreeView->viewport()) {
    return QObject::eventFilter(watched, event);
  }

  if (event->type() == QEvent::MouseButtonPress) {
    return handlePlottingWorkspaceTreeClick(*static_cast<QMouseEvent const *>(event));
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

void QtPlottingWorkspaceTreeViewAdapter::addPlottingWorkspaceTreeItemState(
    QStandardItem *parent, PlottingWorkspaceTreeItemState const &itemState) {
  auto treeItem = createNonEditableItem(displayName(itemState.itemType));
  auto outputTypeItem = createNonEditableItem(displayName(itemState.reducedOutputType));
  auto itemLabel = createNonEditableItem(QString::fromStdString(itemState.label));
  for (auto *rowItem : {treeItem, outputTypeItem, itemLabel}) {
    rowItem->setData(enumIndex(itemState.itemType), itemTypeRole);
    rowItem->setData(enumIndex(itemState.reducedOutputType), reducedOutputTypeRole);
    rowItem->setData(QString::fromStdString(itemState.workspaceName), workspaceNameRole);
    rowItem->setData(enumIndex(itemState.selectionMode), selectionModeRole);
  }
  parent->appendRow({treeItem, outputTypeItem, itemLabel});
  setItemMuted(parent, parent->rowCount() - 1, itemState.muted);
  for (auto const &child : itemState.children) {
    addPlottingWorkspaceTreeItemState(treeItem, child);
  }
}

QModelIndex QtPlottingWorkspaceTreeViewAdapter::itemIndex(QModelIndex const &index) const {
  return index.sibling(index.row(), ItemTypeColumn);
}

PlottingWorkspaceTreeItemType QtPlottingWorkspaceTreeViewAdapter::itemType(QModelIndex const &index) const {
  return static_cast<PlottingWorkspaceTreeItemType>(itemIndex(index).data(itemTypeRole).toInt());
}

std::string QtPlottingWorkspaceTreeViewAdapter::workspaceName(QModelIndex const &index) const {
  return itemIndex(index).data(workspaceNameRole).toString().toStdString();
}

bool QtPlottingWorkspaceTreeViewAdapter::canSelectDirectly(QModelIndex const &index) const {
  auto const selectionMode =
      static_cast<PlottingWorkspaceTreeSelectionMode>(itemIndex(index).data(selectionModeRole).toInt());
  return selectionMode == PlottingWorkspaceTreeSelectionMode::Direct ||
         selectionMode == PlottingWorkspaceTreeSelectionMode::DirectAndParent;
}

bool QtPlottingWorkspaceTreeViewAdapter::canSelectViaParent(QModelIndex const &index) const {
  auto const selectionMode =
      static_cast<PlottingWorkspaceTreeSelectionMode>(itemIndex(index).data(selectionModeRole).toInt());
  return selectionMode == PlottingWorkspaceTreeSelectionMode::ParentOnly ||
         selectionMode == PlottingWorkspaceTreeSelectionMode::DirectAndParent;
}

bool QtPlottingWorkspaceTreeViewAdapter::canContributeSelection(QModelIndex const &index) const {
  auto const selectionMode =
      static_cast<PlottingWorkspaceTreeSelectionMode>(itemIndex(index).data(selectionModeRole).toInt());
  return selectionMode != PlottingWorkspaceTreeSelectionMode::None;
}

bool QtPlottingWorkspaceTreeViewAdapter::handlePlottingWorkspaceTreeClick(QMouseEvent const &event) {
  if (event.button() != Qt::LeftButton) {
    return false;
  }

  auto const clickedIndex = m_plottingWorkspaceTreeView->indexAt(event.pos());
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

bool QtPlottingWorkspaceTreeViewAdapter::isAdditiveSelectionModifier(QMouseEvent const &event) const {
  auto const modifiers = event.modifiers();
  return modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier) ||
         modifiers.testFlag(Qt::ShiftModifier);
}

bool QtPlottingWorkspaceTreeViewAdapter::hasSelectedAncestor(QModelIndex const &index) const {
  auto const selectionModel = m_plottingWorkspaceTreeView->selectionModel();
  auto ancestor = index.parent();
  while (ancestor.isValid()) {
    if (selectionModel->isSelected(ancestor)) {
      return true;
    }
    ancestor = ancestor.parent();
  }
  return false;
}

bool QtPlottingWorkspaceTreeViewAdapter::isSubtreeSelected(QModelIndex const &parentIndex) const {
  auto const selectionModel = m_plottingWorkspaceTreeView->selectionModel();
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

void QtPlottingWorkspaceTreeViewAdapter::selectSubtree(QModelIndex const &parentIndex,
                                                       QItemSelectionModel::SelectionFlags selectionFlags) {
  m_updatingSelection = true;
  if (selectionFlags.testFlag(QItemSelectionModel::Clear)) {
    m_plottingWorkspaceTreeView->selectionModel()->clearSelection();
    selectionFlags &= ~QItemSelectionModel::Clear;
  }
  m_plottingWorkspaceTreeView->selectionModel()->select(parentIndex, selectionFlags);
  updateChildSelection(parentIndex, selectionFlags);
  m_updatingSelection = false;
}

void QtPlottingWorkspaceTreeViewAdapter::updateChildSelection(QItemSelection const &selection,
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

void QtPlottingWorkspaceTreeViewAdapter::updateChildSelection(QModelIndex const &parentIndex,
                                                              QItemSelectionModel::SelectionFlags selectionFlags) {
  auto const rows = m_model.rowCount(parentIndex);
  for (auto row = 0; row < rows; ++row) {
    auto const childIndex = m_model.index(row, 0, parentIndex);
    if (selectionFlags.testFlag(QItemSelectionModel::Deselect) || canSelectDirectly(childIndex) ||
        canSelectViaParent(childIndex)) {
      m_plottingWorkspaceTreeView->selectionModel()->select(childIndex, selectionFlags | QItemSelectionModel::Rows);
    }
    updateChildSelection(childIndex, selectionFlags);
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
