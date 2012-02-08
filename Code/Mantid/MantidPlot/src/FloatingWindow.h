#ifndef FloatingWindow_H
#define FloatingWindow_H

#include <QMainWindow>

class QEvent;
class ApplicationWindow;
class MdiSubWindow;

/**
 * Floating wrapper window for a MdiSubWindow.
 */
class FloatingWindow: public QMainWindow
{
  Q_OBJECT
public:
  FloatingWindow(ApplicationWindow* appWindow, Qt::WindowFlags f = 0);
  ~FloatingWindow();
  void setStaysOnTopFlag();
  void removeStaysOnTopFlag();
  MdiSubWindow* mdiSubWindow() const;
  void setMdiSubWindow(MdiSubWindow* sw);
  void removeMdiSubWindow();

protected:

  void setWidget(QWidget* w);
  QWidget* widget() const;
  virtual bool event(QEvent * e);
  ApplicationWindow* d_app; ///< Pointer to the main window
#ifdef Q_OS_WIN
  Qt::WindowFlags m_flags;  ///< Keeps a copy of window flags, used when re-parenting
#endif
};

#endif
