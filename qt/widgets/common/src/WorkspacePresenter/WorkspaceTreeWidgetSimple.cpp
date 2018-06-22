#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
#include <MantidQtWidgets/Common/MantidTreeWidget.h>
#include <MantidQtWidgets/Common/MantidTreeWidgetItem.h>
#include "MantidQtWidgets/Common/MantidTreeModel.h"

#include <MantidAPI/AlgorithmManager.h>
#include <MantidAPI/FileProperty.h>
#include <MantidAPI/ITableWorkspace.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>

#include <QMenu>
#include <QSignalMapper>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
namespace MantidWidgets {

WorkspaceTreeWidgetSimple::WorkspaceTreeWidgetSimple(QWidget *parent)
    : WorkspaceTreeWidget(new MantidTreeModel(), parent),
      m_plotSpectrum(new QAction("spectrum...", this)),
      m_overplotSpectrum(new QAction("overplot spectrum...", this)),
      m_plotSpectrumWithErrs(new QAction("spectrum with errors...", this)),
      m_overplotSpectrumWithErrs(
          new QAction("overplot spectrum with errors...", this)),
      m_plotColorfill(new QAction("colorfill", this)) {
  // connections
  connect(m_plotSpectrum, SIGNAL(triggered()), this,
          SLOT(onPlotSpectrumClicked()));
  connect(m_overplotSpectrum, SIGNAL(triggered()), this,
          SLOT(onOverplotSpectrumClicked()));
  connect(m_plotSpectrumWithErrs, SIGNAL(triggered()), this,
          SLOT(onPlotSpectrumWithErrorsClicked()));
  connect(m_overplotSpectrumWithErrs, SIGNAL(triggered()), this,
          SLOT(onOverplotSpectrumWithErrorsClicked()));
  connect(m_plotColorfill, SIGNAL(triggered()), this,
          SLOT(onPlotColorfillClicked()));
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
    plotSubMenu->addAction(m_plotSpectrum);
    plotSubMenu->addAction(m_overplotSpectrum);
    plotSubMenu->addAction(m_plotSpectrumWithErrs);
    plotSubMenu->addAction(m_overplotSpectrumWithErrs);
    plotSubMenu->addSeparator();
    plotSubMenu->addAction(m_plotColorfill);
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

void WorkspaceTreeWidgetSimple::onPlotSpectrumClicked() {
  emit plotSpectrumClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onOverplotSpectrumClicked() {
  emit overplotSpectrumClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotSpectrumWithErrorsClicked() {
  emit plotSpectrumWithErrorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onOverplotSpectrumWithErrorsClicked() {
  emit overplotSpectrumWithErrorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotColorfillClicked() {
  emit plotColorfillClicked(getSelectedWorkspaceNamesAsQList());
}

} // namespace MantidWidgets
} // namespace MantidQt
