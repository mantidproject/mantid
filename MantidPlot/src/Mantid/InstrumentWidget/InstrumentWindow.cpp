#include "ApplicationWindow.h"
#include "InstrumentWindow.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "TSVSerialiser.h"

#include <InstrumentWidget.h>
#include <ProjectionSurface.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

InstrumentWindow::InstrumentWindow(const QString &wsName, const QString &label,
                                   ApplicationWindow *parent,
                                   const QString &name)
    : MdiSubWindow(parent, label, name) {

  m_instrumentWidget = new InstrumentWidget(wsName, this);
  this->setWidget(m_instrumentWidget);
  confirmClose(parent->confirmCloseInstrWindow);
}

InstrumentWindow::~InstrumentWindow() {}

void InstrumentWindow::loadFromProject(const std::string &lines,
                                       ApplicationWindow *app,
                                       const int fileVersion) {
  Q_UNUSED(fileVersion);

  TSVSerialiser tsv(lines);
  if (tsv.hasLine("geometry")) {
    const QString geometry =
        QString::fromStdString(tsv.lineAsString("geometry"));
    app->restoreWindowGeometry(app, this, geometry);
  }
}

std::string InstrumentWindow::saveToProject(ApplicationWindow *app) {
  TSVSerialiser tsv;
  tsv.writeRaw("<instrumentwindow>");
  tsv.writeLine("WorkspaceName")
      << m_instrumentWidget->getWorkspaceNameStdString();
  tsv.writeRaw(app->windowGeometryInfo(this));
  tsv.writeRaw("</instrumentwindow>");
  return tsv.outputLines();
}

void InstrumentWindow::selectTab(int tab) {
  return m_instrumentWidget->selectTab(tab);
}

/**
* Closes the window if the associated workspace is deleted.
* @param ws_name :: Name of the deleted workspace.
* @param workspace_ptr :: Pointer to the workspace to be deleted
*/
void InstrumentWindow::preDeleteHandle(
    const std::string &ws_name,
    const boost::shared_ptr<Workspace> workspace_ptr) {
  if (m_instrumentWidget->hasWorkspace(ws_name)) {
    confirmClose(false);
    close();
    return;
  }
  Mantid::API::IPeaksWorkspace_sptr pws =
      boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(workspace_ptr);
  if (pws) {
    m_instrumentWidget->deletePeaksWorkspace(pws);
    return;
  }
}

void InstrumentWindow::afterReplaceHandle(
    const std::string &wsName, const boost::shared_ptr<Workspace> workspace) {
  m_instrumentWidget->handleWorkspaceReplacement(wsName, workspace);
}

void InstrumentWindow::renameHandle(const std::string &oldName,
                                    const std::string &newName) {
  if (m_instrumentWidget->hasWorkspace(oldName)) {
    m_instrumentWidget->renameWorkspace(newName);
    setWindowTitle(QString("Instrument - ") +
                   m_instrumentWidget->getWorkspaceName());
  }
}

void InstrumentWindow::clearADSHandle() {
  confirmClose(false);
  close();
}
