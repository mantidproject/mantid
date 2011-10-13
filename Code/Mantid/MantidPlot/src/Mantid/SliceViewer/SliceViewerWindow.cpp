#include "../../ApplicationWindow.h"
#include "../../MdiSubWindow.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/SingletonHolder.h"
#include "SliceViewer.h"
#include "SliceViewerWindow.h"
#include <qlayout.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;


SliceViewerWindow::SliceViewerWindow(const QString& wsName, ApplicationWindow *app , const QString& label, Qt::WFlags f)
 : MdiSubWindow(QString("Slice Viewer (") + wsName + QString(")") + label,
                 app, QString("Slice Viewer (") + wsName + QString(")"), f),
   WorkspaceObserver()
{
  // Get the workspace
  m_wsName = wsName.toStdString();
  m_ws = boost::dynamic_pointer_cast<IMDWorkspace>( AnalysisDataService::Instance().retrieve(m_wsName) );

  // Watch for the deletion of the associated workspace
  observeAfterReplace();
  observeDelete();
  observeADSClear();

  // Set up the window
  this->setCaption(QString("Slice Viewer (") + wsName + QString(")") + label);
  this->resize(500, 500);

  // Create the m_slicer and add it to the MDI window
  QLayout * layout = this->layout();
  m_slicer = new SliceViewer(this);
  layout->addWidget(m_slicer);

  // Connect closing signals
  connect(this, SIGNAL(closedWindow(MdiSubWindow*)), app, SLOT(closeWindow(MdiSubWindow*)));
  connect(this, SIGNAL(hiddenWindow(MdiSubWindow*)), app, SLOT(hideWindow(MdiSubWindow*)));
  connect(this, SIGNAL(showContextMenu()), app, SLOT(showWindowContextMenu()));

  // Connect WorkspaceObserver signals
  connect(this,SIGNAL(needToClose()),this,SLOT(closeWindow()));
  connect(this,SIGNAL(needToUpdate()),this,SLOT(updateWorkspace()));

  // Set the current workspace
  this->updateWorkspace();
}


SliceViewerWindow::~SliceViewerWindow()
{

}

//------------------------------------------------------------------------------------------------
/** Slot to close the window */
void SliceViewerWindow::closeWindow()
{
  askOnCloseEvent(false);
  close();
}

//------------------------------------------------------------------------------------------------
/** Slot to replace the workspace being looked at. */
void SliceViewerWindow::updateWorkspace()
{
  m_slicer->setWorkspace(m_ws);
}



//------------------------------------------------------------------------------------------------
/** Signal to close this window if the workspace has just been deleted */
void SliceViewerWindow::deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::IMDWorkspace * ws_ptr = dynamic_cast<Mantid::API::IMDWorkspace*>(ws.get());
  if (!ws_ptr) return;
  if (ws_ptr == m_ws.get() || wsName == m_wsName)
  {
    emit needToClose();
  }
}

//------------------------------------------------------------------------------------------------
/** Signal that the workspace being looked at was just replaced with a different one */
void SliceViewerWindow::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::IMDWorkspace_sptr new_ws = boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(ws);
  if (!new_ws) return;
  if (new_ws.get() == m_ws.get() || wsName == m_wsName)
  {
    m_ws = new_ws;
    emit needToUpdate();
  }
}
