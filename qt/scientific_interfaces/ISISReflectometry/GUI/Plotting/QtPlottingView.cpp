// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingView.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

QtPlottingView::QtPlottingView(QWidget *parent) : QWidget(parent), m_notifyee(nullptr) { initLayout(); }

void QtPlottingView::initLayout() {
  m_ui.setupUi(this);
  m_workspaceModel.setHorizontalHeaderLabels({QString("Workspace")});
  m_ui.workspaceTree->setModel(&m_workspaceModel);
  m_ui.plotPreset->addItem("Reflectivity Curve");
  m_ui.plotPreset->addItem("Stitched Reflectivity Curve");
  setOutputOptionsEnabled(false);
}

void QtPlottingView::subscribe(PlottingViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtPlottingView::setOutputOptionsEnabled(bool enabled) {
  m_ui.keepSelected->setEnabled(enabled);
  m_ui.plotTiled->setEnabled(enabled);
  m_ui.plotOverplot->setEnabled(enabled);
  m_ui.plotIndividual->setEnabled(enabled);
  m_ui.plotPreset->setEnabled(enabled);
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

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
