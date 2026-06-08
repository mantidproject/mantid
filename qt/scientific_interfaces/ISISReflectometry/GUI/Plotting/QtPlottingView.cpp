// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingView.h"
#include "PlotOutputTypeProperties.h"
#include "WorkspaceTreeView.h"
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QTimer>
#include <stdexcept>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr itemTypeRole = Qt::UserRole + 1;
auto constexpr outputTypeRole = Qt::UserRole + 2;
auto constexpr workspaceNameRole = Qt::UserRole + 3;

int plotOutputTypeIndex(PlotOutputType outputType) { return static_cast<int>(outputType); }

auto const metadataTextColour = QColor(112, 112, 112);
auto constexpr minimumSelectedItemsForMultiPlot = size_t{2};

template <typename Enum> int enumIndex(Enum value) { return static_cast<int>(value); }

bool hasSelectedItems(size_t selectedItemCount) { return selectedItemCount > 0; }

bool hasEnoughSelectedItemsForMultiPlot(size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                                        PlotOutputTypeProperties const &plotProperties) {
  if (plotProperties.requiresWorkspaceGroupsForMultiPlot()) {
    return selectedWorkspaceGroupCount >= minimumSelectedItemsForMultiPlot;
  }
  return selectedItemCount >= minimumSelectedItemsForMultiPlot;
}

bool hasEnoughSelectedItemsForMultiOutputPlot(size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                                              PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot) {
  return addToExistingPlot
             ? hasSelectedItems(selectedItemCount)
             : hasEnoughSelectedItemsForMultiPlot(selectedItemCount, selectedWorkspaceGroupCount, plotProperties);
}

bool shouldEnablePlotIndividual(bool outputOptionsEnabled, size_t selectedItemCount, bool addToExistingPlot) {
  return outputOptionsEnabled && !addToExistingPlot && hasSelectedItems(selectedItemCount);
}

bool shouldEnablePlotOverplot(bool outputOptionsEnabled, size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                              PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot,
                              bool activePlotOverplotCompatible) {
  return outputOptionsEnabled && plotProperties.supportsOverplot() &&
         (!addToExistingPlot || activePlotOverplotCompatible) &&
         hasEnoughSelectedItemsForMultiOutputPlot(selectedItemCount, selectedWorkspaceGroupCount, plotProperties,
                                                  addToExistingPlot);
}

bool shouldEnablePlotTiled(bool outputOptionsEnabled, size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                           PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot) {
  return outputOptionsEnabled && hasEnoughSelectedItemsForMultiOutputPlot(
                                     selectedItemCount, selectedWorkspaceGroupCount, plotProperties, addToExistingPlot);
}

bool shouldEnablePlotTiledVertically(bool outputOptionsEnabled, size_t selectedItemCount) {
  return outputOptionsEnabled && hasSelectedItems(selectedItemCount);
}

bool shouldEnableAddToExistingPlot(bool outputOptionsEnabled, bool activePlotAvailable,
                                   bool activePlotOverplotCompatible, PlotOutputTypeProperties const &plotProperties) {
  return outputOptionsEnabled && activePlotAvailable && activePlotOverplotCompatible &&
         plotProperties.supportsAddToExistingPlot();
}

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

QtPlottingView::QtPlottingView(QWidget *parent)
    : QWidget(parent), m_notifyee(nullptr), m_outputOptionsEnabled(false), m_activePlotAvailable(false),
      m_activePlotOverplotCompatible(false), m_updatingSelection(false) {
  initLayout();
}

void QtPlottingView::initLayout() {
  m_ui.setupUi(this);
  m_ui.detectorMapYAxis->addItem("Detector Index", enumIndex(DetectorMapYAxis::DetectorId));
  m_ui.detectorMapYAxis->addItem("Detector angle, theta", enumIndex(DetectorMapYAxis::Theta));
  m_ui.detectorMapXAxis->addItem("Time of Flight", enumIndex(DetectorMapXAxis::TimeOfFlight));
  m_ui.detectorMapXAxis->addItem("Lambda", enumIndex(DetectorMapXAxis::Lambda));
  m_ui.alignmentXAxis->addItem("Detector Index", enumIndex(AlignmentXAxis::DetectorId));
  m_ui.alignmentXAxis->addItem("Detector angle, theta", enumIndex(AlignmentXAxis::Theta));
  m_workspaceModel.setHorizontalHeaderLabels({QString("Item type"), QString("Output type"), QString("Item")});
  m_ui.workspaceTree->setModel(&m_workspaceModel);
  m_ui.workspaceTree->setTreePosition(ItemColumn);
  m_ui.workspaceTree->setItemDelegate(new WorkspaceTreeItemDelegate(m_ui.workspaceTree));
  m_ui.workspaceTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_ui.workspaceTree->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_ui.workspaceTree->setExpandsOnDoubleClick(false);
  m_ui.workspaceTree->viewport()->installEventFilter(this);
  connect(m_ui.workspaceTree->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          [this](QItemSelection const &selected, QItemSelection const &deselected) {
            updateChildSelection(deselected, QItemSelectionModel::Deselect);
            updateChildSelection(selected, QItemSelectionModel::Select);
            if (m_notifyee) {
              m_notifyee->notifyPlotSelectionChanged();
            }
            updatePlotButtonEnabledStates();
          });
  connect(m_ui.plotPreset, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int) {
    clearWorkspaceSelection();
    updatePlotOutputProperties();
  });
  setAvailablePlotOutputTypes({PlotOutputType::ReflectivityCurve});
  connect(m_ui.plotTiled, &QPushButton::clicked, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyPlotTiledClicked();
    }
  });
  connect(m_ui.plotOverplot, &QPushButton::clicked, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyPlotOverplotClicked();
    }
  });
  connect(m_ui.plotIndividual, &QPushButton::clicked, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyPlotIndividualClicked();
    }
  });
  connect(m_ui.addToExistingPlot, &QCheckBox::stateChanged, this, [this](int) {
    if (m_notifyee) {
      m_notifyee->notifyAddToExistingPlotChanged();
    }
    updatePlotButtonEnabledStates();
  });
  auto *activePlotAvailabilityTimer = new QTimer(this);
  activePlotAvailabilityTimer->setInterval(1000);
  connect(activePlotAvailabilityTimer, &QTimer::timeout, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyPlotSelectionChanged();
    }
  });
  activePlotAvailabilityTimer->start();
  setOutputOptionControlsEnabled(false);
}

void QtPlottingView::subscribe(PlottingViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtPlottingView::setOutputOptionsEnabled(bool enabled) { setOutputOptionControlsEnabled(enabled); }

void QtPlottingView::setAvailablePlotOutputTypes(std::vector<PlotOutputType> const &outputTypes) {
  auto const previouslySelected = selectedPlotOutputType();
  QSignalBlocker blocker(m_ui.plotPreset);
  m_ui.plotPreset->clear();
  for (auto const outputType : outputTypes) {
    m_ui.plotPreset->addItem(plotOutputTypeProperties(outputType).displayName(), plotOutputTypeIndex(outputType));
  }
  auto const previousIndex = m_ui.plotPreset->findData(plotOutputTypeIndex(previouslySelected));
  if (previousIndex >= 0) {
    m_ui.plotPreset->setCurrentIndex(previousIndex);
  }
  if (previousIndex < 0 && !outputTypes.empty()) {
    clearWorkspaceSelection();
  }
  updatePlotOutputProperties();
}

void QtPlottingView::setOutputOptionControlsEnabled(bool enabled) {
  m_outputOptionsEnabled = enabled;
  m_ui.plotPreset->setEnabled(enabled);
  m_ui.detectorMapYAxis->setEnabled(enabled);
  m_ui.detectorMapXAxis->setEnabled(enabled);
  m_ui.alignmentXAxis->setEnabled(enabled);
  updatePlotButtonEnabledStates();
}

void QtPlottingView::updatePlotButtonEnabledStates() {
  auto const selectedWorkspaceCount = selectedWorkspaceNames().size();
  auto const selectedWorkspaceGroupCount = selectedWorkspaceGroupCountForCurrentPlotOutputType();
  auto const &plotProperties = plotOutputTypeProperties(selectedPlotOutputType());
  auto const addToExistingEnabled = shouldEnableAddToExistingPlot(m_outputOptionsEnabled, m_activePlotAvailable,
                                                                  m_activePlotOverplotCompatible, plotProperties);
  if (!addToExistingEnabled && m_ui.addToExistingPlot->isChecked()) {
    QSignalBlocker blocker(m_ui.addToExistingPlot);
    m_ui.addToExistingPlot->setChecked(false);
  }
  auto const addToExisting = addToExistingPlot();
  m_ui.addToExistingPlot->setEnabled(addToExistingEnabled);
  m_ui.plotIndividual->setEnabled(
      shouldEnablePlotIndividual(m_outputOptionsEnabled, selectedWorkspaceCount, addToExisting));
  m_ui.plotOverplot->setEnabled(shouldEnablePlotOverplot(m_outputOptionsEnabled, selectedWorkspaceCount,
                                                         selectedWorkspaceGroupCount, plotProperties, addToExisting,
                                                         m_activePlotOverplotCompatible));
  auto const plotTiledEnabled = shouldEnablePlotTiled(m_outputOptionsEnabled, selectedWorkspaceCount,
                                                      selectedWorkspaceGroupCount, plotProperties, addToExisting);
  m_ui.plotTiled->setEnabled(plotTiledEnabled);
  m_ui.plotTiledVertically->setEnabled(shouldEnablePlotTiledVertically(m_outputOptionsEnabled, selectedWorkspaceCount));
}

void QtPlottingView::updatePlotOutputProperties() {
  auto const &plotProperties = plotOutputTypeProperties(selectedPlotOutputType());

  m_ui.plotPropertiesTopSeparator->setVisible(plotProperties.showsPlotProperties());
  m_ui.plotPropertiesBottomSeparator->setVisible(plotProperties.showsPlotProperties());
  m_ui.detectorMapYAxisLabel->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.detectorMapYAxis->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.detectorMapXAxisLabel->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.detectorMapXAxis->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.alignmentXAxisLabel->setVisible(plotProperties.showsAlignmentProperties());
  m_ui.alignmentXAxis->setVisible(plotProperties.showsAlignmentProperties());
  setWorkspaceItemsMutedForCurrentPlotOutputType();
  updatePlotButtonEnabledStates();
}

void QtPlottingView::clearWorkspaceSelection() {
  m_updatingSelection = true;
  m_ui.workspaceTree->selectionModel()->clearSelection();
  m_updatingSelection = false;
  updatePlotButtonEnabledStates();
}

void QtPlottingView::setWorkspaceItemsMutedForCurrentPlotOutputType() {
  setWorkspaceItemsMutedForCurrentPlotOutputType(m_workspaceModel.invisibleRootItem());
}

void QtPlottingView::setWorkspaceItemsMutedForCurrentPlotOutputType(QStandardItem *parent) {
  for (auto row = 0; row < parent->rowCount(); ++row) {
    auto const itemTypeItem = parent->child(row, ItemTypeColumn);
    setWorkspaceItemMuted(parent, row, !isSelectableForCurrentPlotOutputType(itemTypeItem->index()));
    setWorkspaceItemsMutedForCurrentPlotOutputType(itemTypeItem);
  }
}

void QtPlottingView::setWorkspaceItemMuted(QStandardItem *parent, int row, bool muted) {
  auto const background = muted ? QBrush(WorkspaceTree::mutedBackgroundColour()) : QBrush();
  for (auto column = 0; column < parent->columnCount(); ++column) {
    auto *item = parent->child(row, column);
    item->setBackground(background);
    item->setData(muted, WorkspaceTree::mutedRole);
  }

  auto const itemLabel = parent->child(row, ItemColumn);
  itemLabel->setForeground(muted ? QBrush(metadataTextColour) : QBrush());
}

bool QtPlottingView::eventFilter(QObject *watched, QEvent *event) {
  if (watched != m_ui.workspaceTree->viewport()) {
    return QWidget::eventFilter(watched, event);
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
  return QWidget::eventFilter(watched, event);
}

void QtPlottingView::setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem> const &items) {
  m_workspaceModel.removeRows(0, m_workspaceModel.rowCount());
  for (auto const &item : items) {
    addTreeItem(m_workspaceModel.invisibleRootItem(), item);
  }
  setWorkspaceItemsMutedForCurrentPlotOutputType();
  m_ui.workspaceTree->expandAll();
  updatePlotButtonEnabledStates();
}

void QtPlottingView::addTreeItem(QStandardItem *parent, PlottingWorkspaceTreeItem const &item) {
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

size_t QtPlottingView::selectedWorkspaceGroupCountForCurrentPlotOutputType() const {
  auto count = size_t{0};
  for (auto const &index : m_ui.workspaceTree->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::WorkspaceGroup &&
        isSelectableForCurrentPlotOutputType(selectedIndex)) {
      ++count;
    }
  }
  return count;
}

std::vector<std::string> QtPlottingView::selectedWorkspaceNames() const {
  auto workspaces = std::vector<std::string>{};
  for (auto const &index : m_ui.workspaceTree->selectionModel()->selectedRows()) {
    auto const selectedIndex = itemIndex(index);
    if (itemType(selectedIndex) == PlottingWorkspaceTreeItemType::Workspace) {
      workspaces.emplace_back(workspaceName(selectedIndex));
    }
  }
  return workspaces;
}

PlotOutputType QtPlottingView::selectedPlotOutputType() const {
  return static_cast<PlotOutputType>(m_ui.plotPreset->currentData().toInt());
}

PlotOutputOptions QtPlottingView::selectedPlotOutputOptions() const {
  return {selectedPlotOutputType(), static_cast<DetectorMapXAxis>(m_ui.detectorMapXAxis->currentData().toInt()),
          static_cast<DetectorMapYAxis>(m_ui.detectorMapYAxis->currentData().toInt()),
          static_cast<AlignmentXAxis>(m_ui.alignmentXAxis->currentData().toInt())};
}

bool QtPlottingView::addToExistingPlot() const { return m_ui.addToExistingPlot->isChecked(); }

bool QtPlottingView::plotTiledVertically() const { return m_ui.plotTiledVertically->isChecked(); }

void QtPlottingView::setActivePlotAvailable(bool available) {
  m_activePlotAvailable = available;
  if (!available) {
    QSignalBlocker blocker(m_ui.addToExistingPlot);
    m_ui.addToExistingPlot->setChecked(false);
  }
  updatePlotButtonEnabledStates();
}

void QtPlottingView::setActivePlotOverplotCompatible(bool compatible) {
  m_activePlotOverplotCompatible = compatible;
  updatePlotButtonEnabledStates();
}

QWidget *QtPlottingView::plotParent() { return window(); }

bool QtPlottingView::confirmPlottingMultipleItems(size_t plotCount) const {
  auto const message = QString("This will plot %1 items. Continue?").arg(plotCount);
  return QMessageBox::warning(const_cast<QtPlottingView *>(this), "Create multiple plots", message,
                              QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok;
}

bool QtPlottingView::isWorkspaceItem(QModelIndex const &index) const { return m_workspaceModel.rowCount(index) == 0; }

bool QtPlottingView::hasWorkspaceDescendant(QModelIndex const &index) const {
  if (isWorkspaceItem(index)) {
    return itemType(index) == PlottingWorkspaceTreeItemType::Workspace;
  }

  auto const rows = m_workspaceModel.rowCount(index);
  for (auto row = 0; row < rows; ++row) {
    if (hasWorkspaceDescendant(m_workspaceModel.index(row, 0, index))) {
      return true;
    }
  }
  return false;
}

bool QtPlottingView::allWorkspaceDescendantsIncludedForCurrentPlotOutputType(QModelIndex const &index) const {
  if (isWorkspaceItem(index)) {
    return itemType(index) != PlottingWorkspaceTreeItemType::Workspace ||
           isWorkspaceIncludedForCurrentPlotOutputType(index);
  }

  auto const rows = m_workspaceModel.rowCount(index);
  for (auto row = 0; row < rows; ++row) {
    if (!allWorkspaceDescendantsIncludedForCurrentPlotOutputType(m_workspaceModel.index(row, 0, index))) {
      return false;
    }
  }
  return true;
}

QModelIndex QtPlottingView::itemIndex(QModelIndex const &index) const {
  return index.sibling(index.row(), ItemTypeColumn);
}

PlottingWorkspaceTreeItemType QtPlottingView::itemType(QModelIndex const &index) const {
  return static_cast<PlottingWorkspaceTreeItemType>(itemIndex(index).data(itemTypeRole).toInt());
}

PlottingWorkspaceOutputType QtPlottingView::outputType(QModelIndex const &index) const {
  return static_cast<PlottingWorkspaceOutputType>(itemIndex(index).data(outputTypeRole).toInt());
}

std::string QtPlottingView::workspaceName(QModelIndex const &index) const {
  return itemIndex(index).data(workspaceNameRole).toString().toStdString();
}

bool QtPlottingView::isSelectableForCurrentPlotOutputType(QModelIndex const &index) const {
  auto const &plotProperties = plotOutputTypeProperties(selectedPlotOutputType());
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

bool QtPlottingView::isWorkspaceIncludedForCurrentPlotOutputType(QModelIndex const &index) const {
  if (isPostprocessedGroupOutputExcludedForCurrentPlotOutputType(index)) {
    return false;
  }
  return plotOutputTypeProperties(selectedPlotOutputType()).includesWorkspaceOutput(outputType(index));
}

bool QtPlottingView::isPostprocessedGroupOutputExcludedForCurrentPlotOutputType(QModelIndex const &index) const {
  return plotOutputTypeProperties(selectedPlotOutputType()).excludesPostprocessedGroupOutputs() &&
         isPostprocessedGroupOutputItem(index);
}

bool QtPlottingView::isPostprocessedGroupOutputItem(QModelIndex const &index) const {
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

bool QtPlottingView::handleWorkspaceTreeClick(QMouseEvent const &event) {
  if (event.button() != Qt::LeftButton) {
    return false;
  }

  auto const clickedIndex = m_ui.workspaceTree->indexAt(event.pos());
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

bool QtPlottingView::isAdditiveSelectionModifier(QMouseEvent const &event) const {
  auto const modifiers = event.modifiers();
  return modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier) ||
         modifiers.testFlag(Qt::ShiftModifier);
}

bool QtPlottingView::hasSelectedAncestor(QModelIndex const &index) const {
  auto const selectionModel = m_ui.workspaceTree->selectionModel();
  auto ancestor = index.parent();
  while (ancestor.isValid()) {
    if (selectionModel->isSelected(ancestor)) {
      return true;
    }
    ancestor = ancestor.parent();
  }
  return false;
}

bool QtPlottingView::isSubtreeSelected(QModelIndex const &parentIndex) const {
  auto const selectionModel = m_ui.workspaceTree->selectionModel();
  if (!selectionModel->isSelected(parentIndex)) {
    return false;
  }

  auto const rows = m_workspaceModel.rowCount(parentIndex);
  for (auto row = 0; row < rows; ++row) {
    auto const childIndex = m_workspaceModel.index(row, 0, parentIndex);
    if (!isSubtreeSelected(childIndex)) {
      return false;
    }
  }
  return true;
}

void QtPlottingView::selectSubtree(QModelIndex const &parentIndex, QItemSelectionModel::SelectionFlags selectionFlags) {
  m_updatingSelection = true;
  if (selectionFlags.testFlag(QItemSelectionModel::Clear)) {
    m_ui.workspaceTree->selectionModel()->clearSelection();
    selectionFlags &= ~QItemSelectionModel::Clear;
  }
  m_ui.workspaceTree->selectionModel()->select(parentIndex, selectionFlags);
  updateChildSelection(parentIndex, selectionFlags);
  m_updatingSelection = false;
  updatePlotButtonEnabledStates();
}

void QtPlottingView::updateChildSelection(QItemSelection const &selection,
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

void QtPlottingView::updateChildSelection(QModelIndex const &parentIndex,
                                          QItemSelectionModel::SelectionFlags selectionFlags) {
  auto const rows = m_workspaceModel.rowCount(parentIndex);
  for (auto row = 0; row < rows; ++row) {
    auto const childIndex = m_workspaceModel.index(row, 0, parentIndex);
    if (selectionFlags.testFlag(QItemSelectionModel::Deselect) || isSelectableForCurrentPlotOutputType(childIndex) ||
        isWorkspaceIncludedForCurrentPlotOutputType(childIndex)) {
      m_ui.workspaceTree->selectionModel()->select(childIndex, selectionFlags | QItemSelectionModel::Rows);
    }
    updateChildSelection(childIndex, selectionFlags);
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
