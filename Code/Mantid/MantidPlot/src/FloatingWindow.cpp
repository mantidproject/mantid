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

/**
 * Constructor.
 */
FloatingWindow::FloatingWindow(ApplicationWindow* appWindow, Qt::WindowFlags f):
//QMainWindow(NULL,f),
QMainWindow(appWindow,f),
d_app(appWindow)
{
  setFocusPolicy(Qt::StrongFocus);
  connect(appWindow,SIGNAL(shutting_down()),this,SLOT(close()));
#ifdef _WIN32
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
    if (w && this != w->d_app->getActiveFloating())
    {
      // the second argument says that FloatingWindow must not be activated again
      w->d_app->activateWindow(w,false);
    }
  }
  else if (e->type() == QEvent::WindowStateChange)
  {
    if (this->isMinimized())
    {
#ifdef _WIN32
      // set parent to NULL wich makes it minimize nicely into a program bar icon
      this->setParent(NULL);
      this->showMinimized();
#endif
      mdiSubWindow()->setStatus(MdiSubWindow::Minimized);
      d_app->activateNewWindow();
    }
    else if ( !this->isMaximized() || !this->isMinimized() )
    {
#ifdef _WIN32
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
#ifdef _WIN32
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
      d_app->removeFloatingWindow(this);
    }
  }
  return QMainWindow::event(e);
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

//
///**
// * Handle the close event.
// * @param e :: A QCloseEvent event.
// */
//void FloatingWindow::closeEvent( QCloseEvent *e )
//{
//  // Default result = do close.
//  int result = 0;
//
////  // If you need to confirm the close, ask the user
////  if (d_confirm_close)
////  {
////    result = QMessageBox::information(this, tr("MantidPlot"),
////        tr("Do you want to hide or delete") + "<p><b>'" + objectName() + "'</b> ?",
////        tr("Delete"), tr("Hide"), tr("Cancel"), 0, 2);
////  }
//
//  switch(result)
//  {
//  case 0:
//    if (widget()->close())
//    {
//      e->accept();
//      // Continue; the mdi window should close (?)
//    }
//    else
//    {
//      QMessageBox::critical(parentWidget(),"MantidPlot - Error", "Window cannot be closed");
//      e->ignore();
//    }
//    break;
//
////  case 1:
////    e->ignore();
////    emit hiddenWindow(this);
////    break;
////
////  case 2:
////    e->ignore();
////    break;
//  }
//
//}
