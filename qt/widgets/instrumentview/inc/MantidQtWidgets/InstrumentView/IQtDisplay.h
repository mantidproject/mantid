// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QWidget>

#include <memory>

namespace MantidQt {
namespace MantidWidgets {

class ProjectionSurface;

/**
 * An interface for the widget for drawing unwrapped instrument images.
 */
class IQtDisplay : public QWidget {
public:
  template <typename... Params> IQtDisplay(Params &&...params) : QWidget(std::forward<Params>(params)...) {}
  virtual ~IQtDisplay() = default;

  /// Assign a surface to draw on
  virtual void setSurface(std::shared_ptr<ProjectionSurface> surface) = 0;
  /// Return the surface
  virtual std::shared_ptr<ProjectionSurface> getSurface() = 0;
  /// Redraw the view
  virtual void updateView(bool picking = true) = 0;
  /// Update the detector information (count values) and redraw
  virtual void updateDetectors() = 0;
  /// Save the image into a file
  virtual void saveToFile(const QString &filename) = 0;

  // Qt overrides
  virtual void qtInstallEventFilter(QObject *arg) { installEventFilter(arg); }
  virtual void qtUpdate() { update(); }
};
} // namespace MantidWidgets
} // namespace MantidQt
