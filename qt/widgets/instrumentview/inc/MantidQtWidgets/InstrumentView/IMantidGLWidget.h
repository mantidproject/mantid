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
\class  IMantidGLWidget
\brief  Interface for the Qt Widget which renders Mantid Geometry ObjComponents
*/

class IMantidGLWidget : public QGLWidget {
  Q_OBJECT
public:
  IMantidGLWidget() {}
  template <typename T1, typename T2> IMantidGLWidget(T1 arg1, T2 arg2) : QGLWidget(arg1, arg2) {}

  virtual void setSurface(std::shared_ptr<ProjectionSurface> surface) = 0;
  virtual std::shared_ptr<ProjectionSurface> getSurface() = 0;

  virtual void setBackgroundColor(const QColor & /*input*/) = 0;
  virtual QColor currentBackgroundColor() const = 0;
  virtual void saveToFile(const QString &filename) = 0;

public slots:
  virtual void enableLighting(bool /*on*/) = 0;
  virtual void updateView(bool picking = true) = 0;
  virtual void updateDetectors() = 0;
  virtual void componentSelected(size_t componentIndex) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
