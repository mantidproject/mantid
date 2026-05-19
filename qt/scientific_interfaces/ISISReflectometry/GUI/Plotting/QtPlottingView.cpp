// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingView.h"
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr itemTypeRole = Qt::UserRole + 1;
auto constexpr outputTypeRole = Qt::UserRole + 2;
auto constexpr workspaceNameRole = Qt::UserRole + 3;

struct PlotSelectionBehaviour {
  std::vector<PlottingWorkspaceTreeItemType> selectableItemTypes;
  std::vector<PlottingWorkspaceOutputType> includedWorkspaceOutputTypes;
};

int plotOutputTypeIndex(PlotOutputType outputType) { return static_cast<int>(outputType); }

auto const metadataTextColour = QColor(112, 112, 112);

template <typename Enum> int enumIndex(Enum value) { return static_cast<int>(value); }

template <typename T> bool contains(std::vector<T> const &values, T value) {
  return std::find(values.cbegin(), values.cend(), value) != values.cend();
}

PlotSelectionBehaviour freeSelectionBehaviour() {
  return {{PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceTreeItemType::Run,
           PlottingWorkspaceTreeItemType::WorkspaceGroup, PlottingWorkspaceTreeItemType::Workspace},
          {PlottingWorkspaceOutputType::IvsQ, PlottingWorkspaceOutputType::IvsLambda,
           PlottingWorkspaceOutputType::IvsQBinned}};
}

PlotSelectionBehaviour selectionBehaviour(PlotOutputType outputType) {
  switch (outputType) {
  case PlotOutputType::SpinAsymmetry:
    return {{PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceTreeItemType::Run,
             PlottingWorkspaceTreeItemType::WorkspaceGroup},
            {PlottingWorkspaceOutputType::IvsQBinned}};
  case PlotOutputType::ReflectivityCurve:
    return {{PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceTreeItemType::Run,
             PlottingWorkspaceTreeItemType::WorkspaceGroup, PlottingWorkspaceTreeItemType::Workspace},
            {PlottingWorkspaceOutputType::IvsQ, PlottingWorkspaceOutputType::IvsQBinned}};
  case PlotOutputType::DetectorMap:
  case PlotOutputType::Alignment:
    return freeSelectionBehaviour();
  }
  throw std::runtime_error("Unexpected reflectometry plot output type.");
}

QString displayName(PlotOutputType outputType) {
  switch (outputType) {
  case PlotOutputType::ReflectivityCurve:
    return "Reflectivity Curve";
  case PlotOutputType::DetectorMap:
    return "Detector Map";
  case PlotOutputType::SpinAsymmetry:
    return "Spin Asymmetry";
  case PlotOutputType::Alignment:
    return "Alignment";
  }
  throw std::runtime_error("Unexpected reflectometry plot output type.");
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

QtPlottingView::QtPlottingView(QWidget *parent) : QWidget(parent), m_notifyee(nullptr), m_updatingSelection(false) {
  initLayout();
}

void QtPlottingView::initLayout() {
  m_ui.setupUi(this);
  m_ui.detectorMapYAxis->addItem("Detector ID", enumIndex(DetectorMapYAxis::DetectorId));
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
  setOutputOptionControlsEnabled(false);
}

void QtPlottingView::subscribe(PlottingViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtPlottingView::setOutputOptionsEnabled(bool enabled) { setOutputOptionControlsEnabled(enabled); }

void QtPlottingView::setAvailablePlotOutputTypes(std::vector<PlotOutputType> const &outputTypes) {
  auto const previouslySelected = selectedPlotOutputType();
  QSignalBlocker blocker(m_ui.plotPreset);
  m_ui.plotPreset->clear();
  for (auto const outputType : outputTypes) {
    m_ui.plotPreset->addItem(displayName(outputType), plotOutputTypeIndex(outputType));
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
  m_ui.plotTiled->setEnabled(enabled);
  m_ui.plotOverplot->setEnabled(enabled);
  m_ui.plotIndividual->setEnabled(enabled);
  m_ui.plotPreset->setEnabled(enabled);
  m_ui.detectorMapYAxis->setEnabled(enabled);
  m_ui.detectorMapXAxis->setEnabled(enabled);
  m_ui.alignmentXAxis->setEnabled(enabled);
}

void QtPlottingView::updatePlotOutputProperties() {
  auto const outputType = selectedPlotOutputType();
  auto const showDetectorMapProperties = outputType == PlotOutputType::DetectorMap;
  auto const showAlignmentProperties = outputType == PlotOutputType::Alignment;
  auto const showProperties = showDetectorMapProperties || showAlignmentProperties;

  m_ui.plotPropertiesTopSeparator->setVisible(showProperties);
  m_ui.plotPropertiesBottomSeparator->setVisible(showProperties);
  m_ui.detectorMapYAxisLabel->setVisible(showDetectorMapProperties);
  m_ui.detectorMapYAxis->setVisible(showDetectorMapProperties);
  m_ui.detectorMapXAxisLabel->setVisible(showDetectorMapProperties);
  m_ui.detectorMapXAxis->setVisible(showDetectorMapProperties);
  m_ui.alignmentXAxisLabel->setVisible(showAlignmentProperties);
  m_ui.alignmentXAxis->setVisible(showAlignmentProperties);
  setWorkspaceItemsMutedForCurrentPlotOutputType();
}

void QtPlottingView::clearWorkspaceSelection() {
  m_updatingSelection = true;
  m_ui.workspaceTree->selectionModel()->clearSelection();
  m_updatingSelection = false;
}

void QtPlottingView::setWorkspaceItemsMutedForCurrentPlotOutputType() {
  setWorkspaceItemsMutedForCurrentPlotOutputType(m_workspaceModel.invisibleRootItem());
}

void QtPlottingView::setWorkspaceItemsMutedForCurrentPlotOutputType(QStandardItem *parent) {
  for (auto row = 0; row < parent->rowCount(); ++row) {
    auto const itemTypeItem = parent->child(row, ItemTypeColumn);
    auto const itemLabel = parent->child(row, ItemColumn);
    itemLabel->setForeground(isSelectableForCurrentPlotOutputType(itemTypeItem->index()) ? QBrush()
                                                                                         : QBrush(metadataTextColour));
    setWorkspaceItemsMutedForCurrentPlotOutputType(itemTypeItem);
  }
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
  auto const behaviour = selectionBehaviour(selectedPlotOutputType());
  auto const indexItemType = itemType(index);
  if (!contains(behaviour.selectableItemTypes, indexItemType)) {
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
  return contains(selectionBehaviour(selectedPlotOutputType()).includedWorkspaceOutputTypes, outputType(index));
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
