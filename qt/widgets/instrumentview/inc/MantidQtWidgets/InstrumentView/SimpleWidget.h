// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  void paintEvent(QPaintEvent * /*unused*/) override;
  void resizeEvent(QResizeEvent * /*unused*/) override;
  void mousePressEvent(QMouseEvent * /*event*/) override;
  void mouseMoveEvent(QMouseEvent * /*event*/) override;
  void mouseReleaseEvent(QMouseEvent * /*event*/) override;
  void wheelEvent(QWheelEvent * /*event*/) override;
  void keyPressEvent(QKeyEvent * /*event*/) override;
  void enterEvent(QEvent * /*event*/) override;
  void leaveEvent(QEvent * /*event*/) override;
  ///< The projection surface
  boost::shared_ptr<ProjectionSurface> m_surface;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // SIMPLEWIDGET_H
