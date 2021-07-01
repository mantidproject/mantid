// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IGLDisplay.h"
#include "MantidGeometry/IComponent.h"

#include <QGLWidget>
#include <QString>

#include <memory>

namespace MantidQt {
namespace MantidWidgets {
class ProjectionSurface;

/**
\class  GLDisplay
\brief  OpenGL Qt Widget which renders Mantid Geometry ObjComponents
*/

class GLDisplay final : public IGLDisplay {
  Q_OBJECT
public:
  explicit GLDisplay(QWidget *parent = nullptr); ///< Constructor
  ~GLDisplay() override;                         ///< Destructor
  void setSurface(std::shared_ptr<ProjectionSurface> surface) override;
  std::shared_ptr<ProjectionSurface> getSurface() override { return m_surface; }

  void setBackgroundColor(const QColor & /*input*/) override;
  QColor currentBackgroundColor() const override;
  void saveToFile(const QString &filename) override;
  // int getLightingState() const {return m_lightingState;}

public slots:
  void enableLighting(bool /*on*/) override;
  void updateView(bool picking = true) override;
  void updateDetectors() override;
  void componentSelected(size_t componentIndex) override;

protected:
  void initializeGL() override;
  void resetWidget();
  void MakeObject();
  void paintEvent(QPaintEvent *event) override;
  void resizeGL(int /*w*/, int /*h*/) override;
  void contextMenuEvent(QContextMenuEvent * /*unused*/) override;
  void mousePressEvent(QMouseEvent * /*unused*/) override;
  void mouseMoveEvent(QMouseEvent * /*unused*/) override;
  void mouseReleaseEvent(QMouseEvent * /*unused*/) override;
  void wheelEvent(QWheelEvent * /*unused*/) override;
  void keyPressEvent(QKeyEvent * /*unused*/) override;
  void keyReleaseEvent(QKeyEvent * /*unused*/) override;
  void enterEvent(QEvent * /*unused*/) override;
  void leaveEvent(QEvent * /*unused*/) override;
  void draw();
  void checkGLError(const QString &funName);

private:
  void setRenderingOptions();

  // int m_lightingState;           ///< 0 = light off; 2 = light on
  bool m_isKeyPressed;
  bool m_firstFrame;

  /// Surface
  std::shared_ptr<ProjectionSurface> m_surface;
};
} // namespace MantidWidgets
} // namespace MantidQt
