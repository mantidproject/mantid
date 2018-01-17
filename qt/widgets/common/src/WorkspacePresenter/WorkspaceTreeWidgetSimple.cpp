#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
#include <MantidQtWidgets/Common/MantidTreeWidget.h>
#include <MantidQtWidgets/Common/MantidTreeWidgetItem.h>

#include <MantidAPI/AlgorithmManager.h>
#include <MantidAPI/FileProperty.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>
#include <MantidAPI/ITableWorkspace.h>

#include <QMenu>
#include <QSignalMapper>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
namespace MantidWidgets {

WorkspaceTreeWidgetSimple::WorkspaceTreeWidgetSimple(MantidDisplayBase *mdb,
                                                     QWidget *parent)
    : WorkspaceTreeWidget(mdb, parent), m_plot1D(new QAction("1D...", this)) {
  // connections
  connect(m_plot1D, SIGNAL(triggered()), this, SLOT(onPlot1DClicked()));
}

WorkspaceTreeWidgetSimple::~WorkspaceTreeWidgetSimple() {}

void WorkspaceTreeWidgetSimple::popupContextMenu() {
  QTreeWidgetItem *treeItem = m_tree->itemAt(m_menuPosition);
  selectedWsName = "";
  if (treeItem)
    selectedWsName = treeItem->text(0);
  else
    m_tree->selectionModel()->clear();

  QMenu *menu(nullptr);

  // If no workspace is here then have load items
  if (selectedWsName.isEmpty())
    menu = m_loadMenu;
  else {
    menu = new QMenu(this);
    menu->setObjectName("WorkspaceContextMenu");

    // plot submenu first
    QMenu *plotSubMenu(new QMenu("Plot", menu));
    plotSubMenu->addAction(m_plot1D);
    menu->addMenu(plotSubMenu);

    menu->addSeparator();
    menu->addAction(m_rename);
    menu->addAction(m_saveNexus);

    menu->addSeparator();
    menu->addAction(m_delete);
  }

  // Show the menu at the cursor's current position
  menu->popup(QCursor::pos());
}

void WorkspaceTreeWidgetSimple::onPlot1DClicked() {
  emit plot1DClicked(selectedWsName);
}

} // namespace MantidWidgets
} // namespace MantidQt
