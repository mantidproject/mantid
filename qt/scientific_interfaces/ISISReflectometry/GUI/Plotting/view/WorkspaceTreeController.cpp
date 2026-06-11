// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "WorkspaceTreeController.h"
#include "GUI/Plotting/model/PlotOutputTypeProperties.h"
#include "WorkspaceTreeView.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <stdexcept>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr itemTypeRole = Qt::UserRole + 1;
auto constexpr outputTypeRole = Qt::UserRole + 2;
auto constexpr workspaceNameRole = Qt::UserRole + 3;

auto const metadataTextColour = QColor(112, 112, 112);

template <typename Enum> int enumIndex(Enum value) { return static_cast<int>(value); }

QString displayName(PlottingWorkspaceTreeItemType itemType) {
  switch (itemType) {
  case PlottingWorkspaceTreeItemType::Group:
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

QString displayName(PlottingWorkspaceOutputType outputType) {
  switch (outputType) {
  case PlottingWorkspaceOutputType::None:
    return "";
  case PlottingWorkspaceOutputType::IvsQ:
    return "IvsQ";
  case PlottingWorkspaceOutputType::IvsLambda:
    return "IvsLambda";
  case PlottingWorkspaceOutputType::IvsQBinned:
    return "IvsQBinned";
  }
  throw std::runtime_error("Unexpected plotting workspace output type.");
}

QStandardItem *createNonEditableItem(QString const &text, bool muted = false) {
  auto item = new QStandardItem(text);
  item->setEditable(false);
  if (muted) {
    item->setForeground(QBrush(metadataTextColour));
  }
  return item;
}

/// Draws column separators for workspace tree rows.
class WorkspaceTreeItemDelegate : public QStyledItemDelegate {
public:
  explicit WorkspaceTreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {
    setObjectName("workspaceTreeItemDelegate");
  }

  void paint(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const override {
    QStyledItemDelegate::paint(painter, option, index);

    if (index.column() < index.model()->columnCount(index.parent()) - 1) {
      painter->save();
      painter->setPen(QColor(214, 214, 214));
      painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
      painter->restore();
    }
  }
};
} // namespace

WorkspaceTreeController::WorkspaceTreeController(WorkspaceTreeView *workspaceTree, QObject *parent)
    : QObject(parent), m_workspaceTree(workspaceTree), m_plotOutputType(PlotOutputType::ReflectivityCurve),
      m_updatingSelection(false) {
  m_model.setHorizontalHeaderLabels({QString("Item type"), QString("Output type"), QString("Item")});
  m_workspaceTree->setModel(&m_model);
  m_workspaceTree->setTreePosition(ItemColumn);
  m_workspaceTree->setItemDelegate(new WorkspaceTreeItemDelegate(m_workspaceTree));
  m_workspaceTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_workspaceTree->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_workspaceTree->setExpandsOnDoubleClick(false);
  m_workspaceTree->viewport()->installEventFilter(this);
}

void WorkspaceTreeController::setItems(std::vector<PlottingWorkspaceTreeItem> const &items) {
  m_model.removeRows(0, m_model.rowCount());
  for (auto const &item : items) {
    addTreeItem(m_model.invisibleRootItem(), item);
  }
  setItemsMutedForCurrentPlotOutputType();
  m_workspaceTree->expandAll();
}

void WorkspaceTreeController::clearSelection() {
  m_updatingSelection = true;
  m_workspaceTree->selectionModel()->clearSelection();
  m_updatingSelection = false;
}

void WorkspaceTreeController::setCurrentPlotOutputType(PlotOutputType outputType) {
  m_plotOutputType = outputType;
  setItemsMutedForCurrentPlotOutputType();
}

std::vector<std::string> WorkspaceTreeController::selectedWorkspaceNames() const {
  auto workspaces = std::vector<std::string>{};
  for (auto const &index : m_workspaceTree->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::Workspace) {
      workspaces.emplace_back(workspaceName(selectedIndex));
    }
  }
  return workspaces;
}

size_t WorkspaceTreeController::selectedWorkspaceGroupCount() const {
  auto count = size_t{0};
  for (auto const &index : m_workspaceTree->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::WorkspaceGroup &&
        isSelectableForCurrentPlotOutputType(selectedIndex)) {
      ++count;
    }
  }
  return count;
}

void WorkspaceTreeController::setItemsMutedForCurrentPlotOutputType() {
  setItemsMutedForCurrentPlotOutputType(m_model.invisibleRootItem());
}

void WorkspaceTreeController::setItemsMutedForCurrentPlotOutputType(QStandardItem *parent) {
  for (auto row = 0; row < parent->rowCount(); ++row) {
    auto const itemTypeItem = parent->child(row, ItemTypeColumn);
    setItemMuted(parent, row, !isSelectableForCurrentPlotOutputType(itemTypeItem->index()));
    setItemsMutedForCurrentPlotOutputType(itemTypeItem);
  }
}

void WorkspaceTreeController::setItemMuted(QStandardItem *parent, int row, bool muted) {
  auto const background = muted ? QBrush(WorkspaceTree::mutedBackgroundColour()) : QBrush();
  for (auto column = 0; column < parent->columnCount(); ++column) {
    auto *item = parent->child(row, column);
    item->setBackground(background);
    item->setData(muted, WorkspaceTree::mutedRole);
  }

  auto const itemLabel = parent->child(row, ItemColumn);
  itemLabel->setForeground(muted ? QBrush(metadataTextColour) : QBrush());
}

bool WorkspaceTreeController::eventFilter(QObject *watched, QEvent *event) {
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

void WorkspaceTreeController::addTreeItem(QStandardItem *parent, PlottingWorkspaceTreeItem const &item) {
  auto treeItem = createNonEditableItem(displayName(item.itemType), true);
  auto outputTypeItem = createNonEditableItem(displayName(item.outputType), true);
  auto itemLabel = createNonEditableItem(QString::fromStdString(item.label));
  for (auto *rowItem : {treeItem, outputTypeItem, itemLabel}) {
    rowItem->setData(enumIndex(item.itemType), itemTypeRole);
    rowItem->setData(enumIndex(item.outputType), outputTypeRole);
    rowItem->setData(QString::fromStdString(item.workspaceName), workspaceNameRole);
  }
  parent->appendRow({treeItem, outputTypeItem, itemLabel});
  for (auto const &child : item.children) {
    addTreeItem(treeItem, child);
  }
}

QModelIndex WorkspaceTreeController::itemIndex(QModelIndex const &index) const {
  return index.sibling(index.row(), ItemTypeColumn);
}

PlottingWorkspaceTreeItemType WorkspaceTreeController::itemType(QModelIndex const &index) const {
  return static_cast<PlottingWorkspaceTreeItemType>(itemIndex(index).data(itemTypeRole).toInt());
}

PlottingWorkspaceOutputType WorkspaceTreeController::outputType(QModelIndex const &index) const {
  return static_cast<PlottingWorkspaceOutputType>(itemIndex(index).data(outputTypeRole).toInt());
}

std::string WorkspaceTreeController::workspaceName(QModelIndex const &index) const {
  return itemIndex(index).data(workspaceNameRole).toString().toStdString();
}

bool WorkspaceTreeController::isWorkspaceItem(QModelIndex const &index) const { return m_model.rowCount(index) == 0; }

bool WorkspaceTreeController::hasWorkspaceDescendant(QModelIndex const &index) const {
  if (isWorkspaceItem(index)) {
    return itemType(index) == PlottingWorkspaceTreeItemType::Workspace;
  }

  auto const rows = m_model.rowCount(index);
  for (auto row = 0; row < rows; ++row) {
    if (hasWorkspaceDescendant(m_model.index(row, 0, index))) {
      return true;
    }
  }
  return false;
}

bool WorkspaceTreeController::allWorkspaceDescendantsIncludedForCurrentPlotOutputType(QModelIndex const &index) const {
  if (isWorkspaceItem(index)) {
    return itemType(index) != PlottingWorkspaceTreeItemType::Workspace ||
           isWorkspaceIncludedForCurrentPlotOutputType(index);
  }

  auto const rows = m_model.rowCount(index);
  for (auto row = 0; row < rows; ++row) {
    if (!allWorkspaceDescendantsIncludedForCurrentPlotOutputType(m_model.index(row, 0, index))) {
      return false;
    }
  }
  return true;
}

bool WorkspaceTreeController::isSelectableForCurrentPlotOutputType(QModelIndex const &index) const {
  auto const &plotProperties = plotOutputTypeProperties(m_plotOutputType);
  auto const indexItemType = itemType(index);
  if (isPostprocessedGroupOutputExcludedForCurrentPlotOutputType(index)) {
    return false;
  }
  if (!plotProperties.allowsItemType(indexItemType)) {
    return false;
  }
  if (indexItemType == PlottingWorkspaceTreeItemType::Workspace) {
    return isWorkspaceIncludedForCurrentPlotOutputType(index);
  }
  if (indexItemType == PlottingWorkspaceTreeItemType::WorkspaceGroup) {
    return hasWorkspaceDescendant(index) && allWorkspaceDescendantsIncludedForCurrentPlotOutputType(index);
  }
  return true;
}

bool WorkspaceTreeController::isWorkspaceIncludedForCurrentPlotOutputType(QModelIndex const &index) const {
  if (isPostprocessedGroupOutputExcludedForCurrentPlotOutputType(index)) {
    return false;
  }
  return plotOutputTypeProperties(m_plotOutputType).includesWorkspaceOutput(outputType(index));
}

bool WorkspaceTreeController::isPostprocessedGroupOutputExcludedForCurrentPlotOutputType(
    QModelIndex const &index) const {
  return plotOutputTypeProperties(m_plotOutputType).excludesPostprocessedGroupOutputs() &&
         isPostprocessedGroupOutputItem(index);
}

bool WorkspaceTreeController::isPostprocessedGroupOutputItem(QModelIndex const &index) const {
  auto const indexItemType = itemType(index);
  if (indexItemType != PlottingWorkspaceTreeItemType::WorkspaceGroup &&
      indexItemType != PlottingWorkspaceTreeItemType::Workspace) {
    return false;
  }

  auto const parentIndex = itemIndex(index).parent();
  if (!parentIndex.isValid()) {
    return false;
  }
  if (itemType(parentIndex) == PlottingWorkspaceTreeItemType::Group) {
    return true;
  }

  auto const grandparentIndex = parentIndex.parent();
  return grandparentIndex.isValid() && itemType(parentIndex) == PlottingWorkspaceTreeItemType::WorkspaceGroup &&
         itemType(grandparentIndex) == PlottingWorkspaceTreeItemType::Group;
}

bool WorkspaceTreeController::handleWorkspaceTreeClick(QMouseEvent const &event) {
  if (event.button() != Qt::LeftButton) {
    return false;
  }

  auto const clickedIndex = m_workspaceTree->indexAt(event.pos());
  if (!clickedIndex.isValid()) {
    return false;
  }

  auto const index = itemIndex(clickedIndex);
  if (!isSelectableForCurrentPlotOutputType(index)) {
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

bool WorkspaceTreeController::isAdditiveSelectionModifier(QMouseEvent const &event) const {
  auto const modifiers = event.modifiers();
  return modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier) ||
         modifiers.testFlag(Qt::ShiftModifier);
}

bool WorkspaceTreeController::hasSelectedAncestor(QModelIndex const &index) const {
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

bool WorkspaceTreeController::isSubtreeSelected(QModelIndex const &parentIndex) const {
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

void WorkspaceTreeController::selectSubtree(QModelIndex const &parentIndex,
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

void WorkspaceTreeController::updateChildSelection(QItemSelection const &selection,
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

void WorkspaceTreeController::updateChildSelection(QModelIndex const &parentIndex,
                                                   QItemSelectionModel::SelectionFlags selectionFlags) {
  auto const rows = m_model.rowCount(parentIndex);
  for (auto row = 0; row < rows; ++row) {
    auto const childIndex = m_model.index(row, 0, parentIndex);
    if (selectionFlags.testFlag(QItemSelectionModel::Deselect) || isSelectableForCurrentPlotOutputType(childIndex) ||
        isWorkspaceIncludedForCurrentPlotOutputType(childIndex)) {
      m_workspaceTree->selectionModel()->select(childIndex, selectionFlags | QItemSelectionModel::Rows);
    }
    updateChildSelection(childIndex, selectionFlags);
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
