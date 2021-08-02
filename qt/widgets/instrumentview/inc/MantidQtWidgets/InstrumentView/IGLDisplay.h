// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/IComponent.h"

#include <QGLWidget>
#include <QString>

#include <memory>
#include <stddef.h>

namespace MantidQt {
namespace MantidWidgets {
class ProjectionSurface;
/**
\class  IGLDisplay
\brief  Interface for the Qt Widget which renders Mantid Geometry ObjComponents
*/

class IGLDisplay : public QGLWidget {
public:
  IGLDisplay() {}
  template <typename... Params> IGLDisplay(Params &&...params) : QGLWidget(std::forward<Params>(params)...) {}
  virtual ~IGLDisplay() = default;

  virtual void setSurface(std::shared_ptr<ProjectionSurface> surface) = 0;
  virtual std::shared_ptr<ProjectionSurface> getSurface() = 0;

  virtual void setBackgroundColor(const QColor & /*input*/) = 0;
  virtual QColor currentBackgroundColor() const = 0;
  virtual void saveToFile(const QString &filename) = 0;

  // Qt overrides
  virtual void qtInstallEventFilter(QObject *arg) { installEventFilter(arg); }
  virtual void qtUpdate() { update(); }
  virtual void qtSetMinimumWidth(int arg) { setMinimumWidth(arg); }
public slots:
  virtual void enableLighting(bool /*on*/) = 0;
  virtual void updateView(bool picking = true) = 0;
  virtual void updateDetectors() = 0;
  virtual void componentSelected(size_t componentIndex) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
