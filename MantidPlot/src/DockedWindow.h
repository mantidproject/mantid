// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DOCKEDWINDOW_H
#define DOCKEDWINDOW_H

#include <QMdiSubWindow>

class QEvent;
class ApplicationWindow;
class MdiSubWindow;
class QSize;

/**
 * Docked (MDI) wrapper window for a MdiSubWindow.
 */
class DockedWindow : public QMdiSubWindow {
  Q_OBJECT
public:
  explicit DockedWindow(ApplicationWindow *appWindow);
  MdiSubWindow *mdiSubWindow() const;
  void setMdiSubWindow(MdiSubWindow *sw);

public slots:
  void dragMousePress(QPoint);
  void dragMouseRelease(QPoint);
  void dragMouseMove(QPoint);

protected:
  bool event(QEvent *ev) override;
  void moveEvent(QMoveEvent *ev) override;

private:
  ApplicationWindow *d_app; ///< Pointer to the main window
  bool m_draggingToTiledWindow;
  bool m_isInsideTiledWindow;

  bool m_dragMouseDown;
  QPoint m_dragStartPos;
};

#endif // DOCKEDWINDOW_H
