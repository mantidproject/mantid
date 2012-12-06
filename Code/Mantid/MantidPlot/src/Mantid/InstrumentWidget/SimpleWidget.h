#ifndef SIMPLEWIDGET_H
#define SIMPLEWIDGET_H

#include <QWidget>

class ProjectionSurface;

/**
 * A simple widget for drawing unwrapped instrument images.
 */
class SimpleWidget : public QWidget
{
public:
  /// Constructor
  SimpleWidget(QWidget* parent=0);
  /// Assign a surface to draw on
  void setSurface(ProjectionSurface* surface);
  /// Return the surface
  ProjectionSurface* getSurface(){return m_surface;}
  /// Redraw the view
  void updateView();
  /// Update the detector information (count values) and redraw
  void updateDetectors();
protected:
  void paintEvent(QPaintEvent*);
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  void keyPressEvent(QKeyEvent *event);
  ///< The projection surface
  ProjectionSurface* m_surface;
};

#endif // SIMPLEWIDGET_H
