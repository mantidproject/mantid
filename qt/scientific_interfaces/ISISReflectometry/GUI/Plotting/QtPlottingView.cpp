// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingView.h"
#include <QComboBox>
#include <QMouseEvent>
#include <QPushButton>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
int plotOutputTypeIndex(PlotOutputType outputType) { return static_cast<int>(outputType); }
} // namespace

QtPlottingView::QtPlottingView(QWidget *parent) : QWidget(parent), m_notifyee(nullptr), m_updatingSelection(false) {
  initLayout();
}

void QtPlottingView::initLayout() {
  m_ui.setupUi(this);
  m_workspaceModel.setHorizontalHeaderLabels({QString("Workspace")});
  m_ui.workspaceTree->setModel(&m_workspaceModel);
  m_ui.workspaceTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_ui.workspaceTree->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_ui.workspaceTree->setExpandsOnDoubleClick(false);
  m_ui.workspaceTree->viewport()->installEventFilter(this);
  connect(m_ui.workspaceTree->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          [this](QItemSelection const &selected, QItemSelection const &deselected) {
            updateChildSelection(deselected, QItemSelectionModel::Deselect);
            updateChildSelection(selected, QItemSelectionModel::Select);
          });
  m_ui.plotPreset->addItem("Reflectivity Curve", plotOutputTypeIndex(PlotOutputType::ReflectivityCurve));
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
  setOutputOptionsEnabled(false);
}

void QtPlottingView::subscribe(PlottingViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtPlottingView::setOutputOptionsEnabled(bool enabled) {
  m_ui.plotTiled->setEnabled(enabled);
  m_ui.plotOverplot->setEnabled(enabled);
  m_ui.plotIndividual->setEnabled(enabled);
  m_ui.plotPreset->setEnabled(enabled);
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
  m_ui.workspaceTree->expandAll();
}

void QtPlottingView::addTreeItem(QStandardItem *parent, PlottingWorkspaceTreeItem const &item) {
  auto treeItem = new QStandardItem(QString::fromStdString(item.label));
  treeItem->setEditable(false);
  parent->appendRow(treeItem);
  for (auto const &child : item.children) {
    addTreeItem(treeItem, child);
  }
}

std::vector<std::string> QtPlottingView::selectedWorkspaces() const {
  auto workspaces = std::vector<std::string>{};
  for (auto const &index : m_ui.workspaceTree->selectionModel()->selectedRows()) {
    if (isWorkspaceItem(index)) {
      workspaces.emplace_back(index.data().toString().toStdString());
    }
  }
  return workspaces;
}

PlotOutputType QtPlottingView::selectedPlotOutputType() const {
  return static_cast<PlotOutputType>(m_ui.plotPreset->currentData().toInt());
}

bool QtPlottingView::isWorkspaceItem(QModelIndex const &index) const { return m_workspaceModel.rowCount(index) == 0; }

bool QtPlottingView::handleWorkspaceTreeClick(QMouseEvent const &event) {
  if (event.button() != Qt::LeftButton) {
    return false;
  }

  auto const index = m_ui.workspaceTree->indexAt(event.pos());
  if (!index.isValid()) {
    return false;
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
    updateChildSelection(index, selectionFlags);
  }
  m_updatingSelection = false;
}

void QtPlottingView::updateChildSelection(QModelIndex const &parentIndex,
                                          QItemSelectionModel::SelectionFlags selectionFlags) {
  auto const rows = m_workspaceModel.rowCount(parentIndex);
  for (auto row = 0; row < rows; ++row) {
    auto const childIndex = m_workspaceModel.index(row, 0, parentIndex);
    m_ui.workspaceTree->selectionModel()->select(childIndex, selectionFlags | QItemSelectionModel::Rows);
    updateChildSelection(childIndex, selectionFlags);
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
