#include "FloatingWindow.h"
#include "MdiSubWindow.h"
#include "ApplicationWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QEvent>
#include <QCloseEvent>
#include <QString>
#include <QDateTime>
#include <QMenu>
#include <QTextStream>
#include <QTemporaryFile>
#include <QMdiArea>
#include <QSize>
#include <QDrag>

/**
 * Constructor.
 */
FloatingWindow::FloatingWindow(ApplicationWindow* appWindow, Qt::WindowFlags f):
#ifdef Q_OS_WIN
QMainWindow(appWindow,f),
#else
QMainWindow(NULL,f),
#endif
d_app(appWindow),
m_draggingToTiledWindow(false),
m_isInsideTiledWindow(false),
m_dragMouseDown(false)
{
  setFocusPolicy(Qt::StrongFocus);
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  connect(appWindow,SIGNAL(shutting_down()),this,SLOT(close()));
#ifdef Q_OS_WIN
  // remember the flags
  m_flags = windowFlags();
#endif

  // Window must NOT get deleted automatically when closed.
  // Instead, the ApplicationWindow->removeFloatingWindow() call takes care of
  // calling deleteLater().
  setAttribute(Qt::WA_DeleteOnClose, false);
}

FloatingWindow::~FloatingWindow()
{
  //std::cerr << "Deleted FloatingWindow\n";
}

/**
  * Returns the inner MdiSubWindow.
  */
MdiSubWindow* FloatingWindow::mdiSubWindow() const
{
  return static_cast<MdiSubWindow*>(widget());
}

/**
  * Returns the inner MdiSubWindow as a QWidget.(?)
  */
QWidget* FloatingWindow::widget() const
{
  return static_cast<MdiSubWindowParent_t*>(centralWidget())->widget();
}

/**
 * Process state change events such as activation, minimizing or maximizing.
 */
bool FloatingWindow::event(QEvent * e)
{
  if (e->type() == QEvent::WindowActivate)
  {
    // If FloatingWindow was activated by clicking on it we need to
    // let the application know about it
    MdiSubWindow* w = dynamic_cast<MdiSubWindow*>(widget());
    if (w && this != d_app->getActiveFloating())
    {
      // the second argument says that FloatingWindow must not be activated again
      d_app->activateWindow(w,false);
    }
  }
  else if (e->type() == QEvent::WindowStateChange)
  {
    if (this->isMinimized())
    {
#ifdef Q_OS_WIN
      // set parent to NULL wich makes it minimize nicely into a program bar icon
      this->setParent(NULL);
      this->showMinimized();
#endif
      mdiSubWindow()->setStatus(MdiSubWindow::Minimized);
      d_app->activateNewWindow();
    }
    else if ( !this->isMaximized() || !this->isMinimized() )
    {
#ifdef Q_OS_WIN
      // re-parent to the main window making the floating window stay on top of it
      if (this->parent() != d_app)
      {
        this->setParent(d_app);
        this->setWindowFlags(m_flags);
        this->showNormal();
      }
#endif
      mdiSubWindow()->setStatus(MdiSubWindow::Normal);
    }
    else if (this->isMaximized())
    {
#ifdef Q_OS_WIN
      // re-parent to the main window making the floating window stay on top of it
      if (this->parent() != d_app)
      {
        this->setParent(d_app);
        this->setWindowFlags(m_flags);
        this->showMaximized();
      }
#endif
      mdiSubWindow()->setStatus(MdiSubWindow::Maximized);
    }
  }
  else if (e->type() == QEvent::Close)
  {
    if (widget() && widget()->close())
    {
      // forget about me and close
      d_app->removeFloatingWindow(this);
    }
    else
    {
      // don't close
      e->ignore();
      return true;
    }
  }
  else if ( e->type() == QEvent::NonClientAreaMouseButtonPress )
  {
    // User clicked the window title bar
    m_draggingToTiledWindow = true;
    auto mu = static_cast<QMouseEvent*>(e);
    m_dragStartPos = mu->pos();
  }
  else if ( e->type() == QEvent::NonClientAreaMouseMove )
  {
    // For some reason this event is fired when the user releases the mouse over the title bar
    if ( m_draggingToTiledWindow && m_isInsideTiledWindow )
    {
      m_draggingToTiledWindow = false;
      m_isInsideTiledWindow = false;
      d_app->dropInTiledWindow( mdiSubWindow(), pos() + m_dragStartPos );
      return true;
    }
    m_draggingToTiledWindow = false;
    m_isInsideTiledWindow = false;
  }
  return QMainWindow::event(e);
}

void FloatingWindow::moveEvent(QMoveEvent *ev)
{
  if ( m_draggingToTiledWindow )
  {
    // we are here if the window is being moved by the user
    m_isInsideTiledWindow =  d_app->isInTiledWindow( ev->pos() + m_dragStartPos );
  }
  else
  {
    m_isInsideTiledWindow = false;
  }
}

/**
  * Make this window stay on top
  */
void FloatingWindow::setStaysOnTopFlag()
{
  Qt::WindowFlags flags = windowFlags();
  Qt::WindowFlags new_flags = flags | Qt::WindowStaysOnTopHint;
  if (new_flags != flags)
  {
    setWindowFlags(new_flags);
    show();
  }
}

/**
  * Make this window not stay on top
  */
void FloatingWindow::removeStaysOnTopFlag()
{
  Qt::WindowFlags flags = windowFlags();
  Qt::WindowFlags new_flags = flags ^ Qt::WindowStaysOnTopHint;
  if (new_flags != flags)
  {
    setWindowFlags(new_flags);
    show();
  }
}

/** Sets the underlying MdiSubWindow */
void FloatingWindow::setMdiSubWindow(MdiSubWindow* sw)
{
  setWidget(sw);
  setWindowIcon(sw->windowIcon());
  connect(sw,SIGNAL(dragMousePress(QPoint)),this,SLOT(dragMousePress(QPoint)));
  connect(sw,SIGNAL(dragMouseRelease(QPoint)),this,SLOT(dragMouseRelease(QPoint)));
  connect(sw,SIGNAL(dragMouseMove(QPoint)),this,SLOT(dragMouseMove(QPoint)));
}


/** removes the underlying MdiSubWindow */
void FloatingWindow::removeMdiSubWindow()
{
  MdiSubWindowParent_t* wrapper = dynamic_cast<MdiSubWindowParent_t*>(centralWidget());
  if (wrapper)
  {
    wrapper->setWidget(NULL);
  }
}

QSize FloatingWindow::minimumSizeHint() const
{
  return QSize(200, 200);
}

/** Sets the widget displayed in the FloatingWindow
 *
 * @param w :: MdiSubWindow being floated
 */
void FloatingWindow::setWidget(QWidget* w)
{
  MdiSubWindowParent_t* wrapper = new MdiSubWindowParent_t(this);
  wrapper->setWidget(w);
  setCentralWidget(wrapper);
}

void FloatingWindow::dragMousePress(QPoint pos)
{
  if ( d_app->hasTiledWindowOpen() )
  {
    m_dragMouseDown = true;
    m_dragStartPos = pos;
  }
}

void FloatingWindow::dragMouseRelease(QPoint)
{
  if ( m_dragMouseDown )
  {
    m_dragMouseDown = false;
  }
}

void FloatingWindow::dragMouseMove(QPoint pos)
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
