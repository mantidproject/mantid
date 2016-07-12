#ifndef FloatingWindow_H
#define FloatingWindow_H

#include <QMainWindow>

class QEvent;
class ApplicationWindow;
class MdiSubWindow;
class QSize;

/**
 * Floating wrapper window for a MdiSubWindow.
 */
class FloatingWindow : public QMainWindow {
  Q_OBJECT
public:
  FloatingWindow(ApplicationWindow *appWindow, Qt::WindowFlags f = 0);
  ~FloatingWindow() override;
  void setStaysOnTopFlag();
  void removeStaysOnTopFlag();
  MdiSubWindow *mdiSubWindow() const;
  void setMdiSubWindow(MdiSubWindow *sw);
  void removeMdiSubWindow();
  QSize minimumSizeHint() const override;

public slots:
  void dragMousePress(QPoint);
  void dragMouseRelease(QPoint);
  void dragMouseMove(QPoint);

protected:
  void setWidget(QWidget *w);
  QWidget *widget() const;
  bool event(QEvent *ev) override;
  void moveEvent(QMoveEvent *ev) override;

private:
  ApplicationWindow *d_app; ///< Pointer to the main window
#ifdef Q_OS_WIN
  Qt::WindowFlags
      m_flags; ///< Keeps a copy of window flags, used when re-parenting
#endif
  bool m_draggingToTiledWindow;
  bool m_isInsideTiledWindow;

  bool m_dragMouseDown;
  QPoint m_dragStartPos;
};

#endif
