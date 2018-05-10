#ifndef SIMPLEWIDGET_H
#define SIMPLEWIDGET_H

#include <QWidget>

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {

class ProjectionSurface;

/**
 * A simple widget for drawing unwrapped instrument images.
 */
class SimpleWidget : public QWidget {
public:
  /// Constructor
  explicit SimpleWidget(QWidget *parent);
  ~SimpleWidget() override;
  /// Assign a surface to draw on
  void setSurface(boost::shared_ptr<ProjectionSurface> surface);
  /// Return the surface
  boost::shared_ptr<ProjectionSurface> getSurface() { return m_surface; }
  /// Redraw the view
  void updateView(bool picking = true);
  /// Update the detector information (count values) and redraw
  void updateDetectors();
  /// Save the image into a file
  void saveToFile(const QString &filename);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;
  void keyPressEvent(QKeyEvent *) override;
  void enterEvent(QEvent *) override;
  void leaveEvent(QEvent *) override;
  ///< The projection surface
  boost::shared_ptr<ProjectionSurface> m_surface;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // SIMPLEWIDGET_H
