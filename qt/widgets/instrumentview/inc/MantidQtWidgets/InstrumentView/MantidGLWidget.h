// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDGLWIDGET_H_
#define MANTIDGLWIDGET_H_

#include "MantidGeometry/IComponent.h"

#include <QGLWidget>
#include <QString>

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {
class ProjectionSurface;

/**
\class  MantidGLWidget
\brief  OpenGL Qt Widget which renders Mantid Geometry ObjComponents
*/

class MantidGLWidget : public QGLWidget {
  Q_OBJECT
public:
  explicit MantidGLWidget(QWidget *parent = nullptr); ///< Constructor
  ~MantidGLWidget() override;                         ///< Destructor
  void setSurface(boost::shared_ptr<ProjectionSurface> surface);
  boost::shared_ptr<ProjectionSurface> getSurface() { return m_surface; }

  void setBackgroundColor(QColor /*input*/);
  QColor currentBackgroundColor() const;
  void saveToFile(const QString &filename);
  // int getLightingState() const {return m_lightingState;}

public slots:
  void enableLighting(bool /*on*/);
  void updateView(bool picking = true);
  void updateDetectors();
  void componentSelected(size_t componentIndex);

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
  boost::shared_ptr<ProjectionSurface> m_surface;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDGLWIDGET_H_*/
