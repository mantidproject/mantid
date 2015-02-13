#include "DockedWindow.h"
#include "MdiSubWindow.h"
#include "ApplicationWindow.h"

#include <QApplication>

/**
 * Constructor.
 */
DockedWindow::DockedWindow(ApplicationWindow* appWindow):
  QMdiSubWindow(appWindow),
  d_app(appWindow),
  m_draggingToTiledWindow(false),
  m_isInsideTiledWindow(false),
  m_dragMouseDown(false)
{
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_DeleteOnClose);
}

/**
  * Returns the inner MdiSubWindow.
  */
MdiSubWindow* DockedWindow::mdiSubWindow() const
{
  return static_cast<MdiSubWindow*>(widget());
}

/**
 * Set the innner MdiSubWindow.
 * @param sw :: A subwindow to set.
 */
void DockedWindow::setMdiSubWindow(MdiSubWindow* sw)
{
  setWidget(sw);
  //setWindowIcon(sw->windowIcon());
  connect(sw,SIGNAL(dragMousePress(QPoint)),this,SLOT(dragMousePress(QPoint)));
  connect(sw,SIGNAL(dragMouseRelease(QPoint)),this,SLOT(dragMouseRelease(QPoint)));
  connect(sw,SIGNAL(dragMouseMove(QPoint)),this,SLOT(dragMouseMove(QPoint)));
}

/**
 * Process state change events such as activation, minimizing or maximizing.
 */
bool DockedWindow::event(QEvent * e)
{
  //std::cerr << "Docked event " << e->type() << std::endl;
  if ( e->type() == QEvent::NonClientAreaMouseButtonPress )
  {
    // User clicked the window title bar
    m_draggingToTiledWindow = true;
  }
  else if ( e->type() == QEvent::NonClientAreaMouseMove )
  {
    // For some reason this event is fired when the user releases the mouse over the title bar
    if ( m_draggingToTiledWindow )
    {
      d_app->dropInTiledWindow( mdiSubWindow(), pos() - d_app->pos() );
      return true;
    }
    m_draggingToTiledWindow = false;
    m_isInsideTiledWindow = false;
  }
  return QMdiSubWindow::event(e);
}

void DockedWindow::moveEvent(QMoveEvent *ev)
{
  if ( m_draggingToTiledWindow )
  {
    // we are here if the window is being moved by the user
    m_isInsideTiledWindow =  d_app->isInTiledWindow( ev->pos() - d_app->pos() );
  }
  else
  {
    m_isInsideTiledWindow = false;
  }
}

void DockedWindow::dragMousePress(QPoint pos)
{
  if ( d_app->hasTiledWindowOpen() )
  {
    m_dragMouseDown = true;
    m_dragStartPos = pos;
  }
}

void DockedWindow::dragMouseRelease(QPoint)
{
  if ( m_dragMouseDown )
  {
    m_dragMouseDown = false;
  }
}

void DockedWindow::dragMouseMove(QPoint pos)
{
  if ( m_dragMouseDown )
  {
    if ((pos - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
    {
      return;
    }

    QDrag *drag = new QDrag(d_app);
    QMimeData *mimeData = new QMimeData;

    MdiSubWindow *ptr = mdiSubWindow();
    auto d = QByteArray::fromRawData( (const char*)ptr, 1 );
    mimeData->setData("TiledWindow",d);

    drag->setMimeData(mimeData);
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
    (void) dropAction;
  }
}
