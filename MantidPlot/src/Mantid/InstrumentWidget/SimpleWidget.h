#ifndef SIMPLEWIDGET_H
#define SIMPLEWIDGET_H

#include <QWidget>

#include <boost/shared_ptr.hpp>

class ProjectionSurface;

/**
 * A simple widget for drawing unwrapped instrument images.
 */
class SimpleWidget : public QWidget
{
public:
  /// Constructor
  SimpleWidget(QWidget* parent);
  ~SimpleWidget();
  /// Assign a surface to draw on
  void setSurface(boost::shared_ptr<ProjectionSurface> surface);
  /// Return the surface
  boost::shared_ptr<ProjectionSurface> getSurface(){return m_surface;}
  /// Redraw the view
  void updateView(bool picking = true);
  /// Update the detector information (count values) and redraw
  void updateDetectors();
  /// Save the image into a file
  void saveToFile(const QString & filename);
protected:
  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void keyPressEvent(QKeyEvent*);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);
  ///< The projection surface
  boost::shared_ptr<ProjectionSurface> m_surface;
};

#endif // SIMPLEWIDGET_H
