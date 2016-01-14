#include "InstrumentWindow.h"
#include "ApplicationWindow.h"
#include "TSVSerialiser.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <MdiSubWindow.h>
#include <ProjectionSurface.h>

using namespace Mantid::API;

InstrumentWindow::InstrumentWindow(MdiSubWindow *parent, const QString &wsName)
    : InstrumentWidget(wsName, parent),
	m_mdiSubWindowParent(parent)
{
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
    app->restoreWindowGeometry(app, m_mdiSubWindowParent, geometry);
  }
}

std::string InstrumentWindow::saveToProject(ApplicationWindow *app) {
  TSVSerialiser tsv;
  tsv.writeRaw("<instrumentwindow>");
  tsv.writeLine("WorkspaceName") << m_workspaceName.toStdString();
  tsv.writeRaw(app->windowGeometryInfo(m_mdiSubWindowParent));
  tsv.writeRaw("</instrumentwindow>");
  return tsv.outputLines();
}

void InstrumentWindow::closeEvent(QCloseEvent *e)
{
	if (m_mdiSubWindowParent && m_mdiSubWindowParent->close())
		e->accept();
	else
		e->ignore();
}

/**
* Closes the window if the associated workspace is deleted.
* @param ws_name :: Name of the deleted workspace.
* @param workspace_ptr :: Pointer to the workspace to be deleted
*/
void InstrumentWindow::preDeleteHandle(
    const std::string &ws_name,
    const boost::shared_ptr<Workspace> workspace_ptr) {
  if (ws_name == m_workspaceName.toStdString()) {
    m_mdiSubWindowParent->confirmClose(false);
    m_mdiSubWindowParent->close();
    return;
  }
  Mantid::API::IPeaksWorkspace_sptr pws =
      boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(workspace_ptr);
  if (pws) {
    getSurface()->deletePeaksWorkspace(pws);
    updateInstrumentView();
    return;
  }
}

void InstrumentWindow::afterReplaceHandle(
    const std::string &wsName, const boost::shared_ptr<Workspace> workspace) {
  // Replace current workspace
  if (wsName == m_workspaceName.toStdString()) {
    if (m_instrumentActor) {
      // Check if it's still the same workspace underneath (as well as having
      // the same name)
      auto matrixWS =
          boost::dynamic_pointer_cast<const MatrixWorkspace>(workspace);
      bool sameWS = false;
      try {
        sameWS = (matrixWS == m_instrumentActor->getWorkspace());
      } catch (std::runtime_error &) {
        // Carry on, sameWS should stay false
      }

      // try to detect if the instrument changes (unlikely if the workspace
      // hasn't, but theoretically possible)
      bool resetGeometry = matrixWS->getInstrument()->getNumberDetectors() !=
                           m_instrumentActor->ndetectors();

      // if workspace and instrument don't change keep the scaling
      if (sameWS && !resetGeometry) {
        m_instrumentActor->updateColors();
      } else {
        delete m_instrumentActor;
        m_instrumentActor = NULL;
        init(resetGeometry, true, 0.0, 0.0, false);
        updateInstrumentDetectors();
      }
    }
  }
}

void InstrumentWindow::renameHandle(const std::string &oldName,
                                          const std::string &newName) {
  if (oldName == m_workspaceName.toStdString()) {
    m_workspaceName = QString::fromStdString(newName);
    m_mdiSubWindowParent->setWindowTitle(QString("Instrument - ") +
                                         m_workspaceName);
  }
}

void InstrumentWindow::clearADSHandle() {
  m_mdiSubWindowParent->confirmClose(false);
  m_mdiSubWindowParent->close();
}
