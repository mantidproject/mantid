// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IQtDisplay.h"

#include <QString>
#include <QWidget>
#include <memory>

namespace MantidQt {
namespace MantidWidgets {

class ProjectionSurface;

/**
 * A simple widget for drawing unwrapped instrument images.
 */
class QtDisplay final : public IQtDisplay {
public:
  /// Constructor
  explicit QtDisplay(QWidget *parent = nullptr);
  ~QtDisplay() override;
  /// Assign a surface to draw on
  void setSurface(std::shared_ptr<ProjectionSurface> surface) override;
  /// Return the surface
  std::shared_ptr<ProjectionSurface> getSurface() override { return m_surface; }
  /// Redraw the view
  void updateView(bool picking = true) override;
  /// Update the detector information (count values) and redraw
  void updateDetectors() override;
  /// Save the image into a file
  void saveToFile(const QString &filename) override;

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
  std::shared_ptr<ProjectionSurface> m_surface;
};
} // namespace MantidWidgets
} // namespace MantidQt
