#include "ApplicationWindow.h"
#include "InstrumentWindow.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "TSVSerialiser.h"

#include <InstrumentWidget.h>
#include <ProjectionSurface.h>

using namespace Mantid::API;

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
      << m_instrumentWidget->getWorkspaceName().toStdString();
  tsv.writeRaw(app->windowGeometryInfo(this));
  tsv.writeRaw("</instrumentwindow>");
  return tsv.outputLines();
}

/**
* Closes the window if the associated workspace is deleted.
* @param ws_name :: Name of the deleted workspace.
* @param workspace_ptr :: Pointer to the workspace to be deleted
*/
void InstrumentWindow::preDeleteHandle(
    const std::string &ws_name,
    const boost::shared_ptr<Workspace> workspace_ptr) {
  if (ws_name == m_instrumentWidget->getWorkspaceName().toStdString()) {
    confirmClose(false);
    close();
    return;
  }
  Mantid::API::IPeaksWorkspace_sptr pws =
      boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(workspace_ptr);
  if (pws) {
    m_instrumentWidget->getSurface()->deletePeaksWorkspace(pws);
    m_instrumentWidget->updateInstrumentView();
    return;
  }
}

void InstrumentWindow::afterReplaceHandle(
    const std::string &wsName, const boost::shared_ptr<Workspace> workspace) {
  // Replace current workspace
  if (wsName == m_instrumentWidget->getWorkspaceName().toStdString()) {
    if (m_instrumentWidget->getInstrumentActor()) {
      // Check if it's still the same workspace underneath (as well as having
      // the same name)
      auto matrixWS =
          boost::dynamic_pointer_cast<const MatrixWorkspace>(workspace);
      bool sameWS = false;
      try {
        sameWS = (matrixWS ==
                  m_instrumentWidget->getInstrumentActor()->getWorkspace());
      } catch (std::runtime_error &) {
        // Carry on, sameWS should stay false
      }

      // try to detect if the instrument changes (unlikely if the workspace
      // hasn't, but theoretically possible)
      bool resetGeometry =
          matrixWS->getInstrument()->getNumberDetectors() !=
          m_instrumentWidget->getInstrumentActor()->ndetectors();

      // if workspace and instrument don't change keep the scaling
      if (sameWS && !resetGeometry) {
        m_instrumentWidget->getInstrumentActor()->updateColors();
      } else {
        m_instrumentWidget->resetInstrument(resetGeometry);
      }
    }
  }
}

void InstrumentWindow::renameHandle(const std::string &oldName,
                                    const std::string &newName) {
  if (oldName == m_instrumentWidget->getWorkspaceName().toStdString()) {
    m_instrumentWidget->renameWorkspace(QString::fromStdString(newName));
    setWindowTitle(QString("Instrument - ") +
                   m_instrumentWidget->getWorkspaceName());
  }
}

void InstrumentWindow::clearADSHandle() {
  confirmClose(false);
  close();
}
