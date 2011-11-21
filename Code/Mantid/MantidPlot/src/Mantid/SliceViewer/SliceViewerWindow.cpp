#include "../../ApplicationWindow.h"
#include "../../MdiSubWindow.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/VMD.h"
#include "MantidQtSliceViewer/LineViewer.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include "SliceViewerWindow.h"
#include <qlayout.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::SliceViewer;


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

  // Make a horizontal splitter
  m_splitter = new QSplitter(this);
  m_splitter->setObjectName(QString::fromUtf8("splitter"));
  m_splitter->setOrientation(Qt::Horizontal);
  m_splitter->setOpaqueResize(false);

  m_slicer = new SliceViewer(m_splitter);
  m_liner = new LineViewer(m_splitter);
  m_liner->setVisible(false);

  layout->addWidget(m_splitter);
  m_splitter->addWidget(m_slicer);
  m_splitter->addWidget(m_liner);


  // Connect closing signals
  connect(this, SIGNAL(closedWindow(MdiSubWindow*)), app, SLOT(closeWindow(MdiSubWindow*)));
  connect(this, SIGNAL(hiddenWindow(MdiSubWindow*)), app, SLOT(hideWindow(MdiSubWindow*)));
  connect(this, SIGNAL(showContextMenu()), app, SLOT(showWindowContextMenu()));

  // Connect WorkspaceObserver signals
  connect(this,SIGNAL(needToClose()),this,SLOT(closeWindow()));
  connect(this,SIGNAL(needToUpdate()),this,SLOT(updateWorkspace()));

  // Connect the SliceViewer and the LineViewer together
  QObject::connect( m_slicer, SIGNAL(changedShownDim(size_t, size_t)),
            m_liner, SLOT(setFreeDimensions(size_t, size_t)) );
  QObject::connect( m_slicer->getLineOverlay(), SIGNAL(lineChanging(QPointF, QPointF, double)),
            this, SLOT(lineChanging(QPointF, QPointF, double)) );
//  QObject::connect( m_slicer, SIGNAL(showLineViewer(bool)),
//            this, SLOT(lineChanging(QPointF, QPointF, double)) );
  //QObject::connect( m_slicer, SIGNAL(changedSlicePoint(size_t, size_t)), m_liner, SIGNAL(setFreeDimensions(size_t, size_t)) );

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
  m_liner->setWorkspace(m_ws);
}

//------------------------------------------------------------------------------------------------
/** Using the positions from the LineOverlay, set the values in the LineViewer,
 * but don't update view. */
void SliceViewerWindow::setLineViewerValues(QPointF start2D, QPointF end2D, double width)
{
  VMD start = m_slicer->getSlicePoint();
  VMD end = start;
  start[m_slicer->getDimX()] = start2D.x();
  start[m_slicer->getDimY()] = start2D.y();
  end[m_slicer->getDimX()] = end2D.x();
  end[m_slicer->getDimY()] = end2D.y();
  m_liner->setStart(start);
  m_liner->setEnd(end);
  VMD widthVec = start * 0;
  for (size_t d=0; d<widthVec.getNumDims(); d++)
    widthVec[d] = width;
  m_liner->setWidth(widthVec);
}

//------------------------------------------------------------------------------------------------
/** Slot called when the line overlay position is changing (being dragged) */
void SliceViewerWindow::lineChanging(QPointF start2D, QPointF end2D, double width)
{
  setLineViewerValues(start2D, end2D, width);
  m_liner->showPreview();
}

/** Slot called when the line overlay drag is released */
void SliceViewerWindow::lineChanged(QPointF start2D, QPointF end2D, double width)
{
  setLineViewerValues(start2D, end2D, width);
  m_liner->apply();
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
