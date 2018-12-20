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

  void setBackgroundColor(QColor);
  QColor currentBackgroundColor() const;
  void saveToFile(const QString &filename);
  // int getLightingState() const {return m_lightingState;}

public slots:
  void enableLighting(bool);
  void updateView(bool picking = true);
  void updateDetectors();
  void componentSelected(size_t componentIndex);

protected:
  void initializeGL() override;
  void resetWidget();
  void MakeObject();
  void paintEvent(QPaintEvent *event) override;
  void resizeGL(int, int) override;
  void contextMenuEvent(QContextMenuEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;
  void keyPressEvent(QKeyEvent *) override;
  void keyReleaseEvent(QKeyEvent *) override;
  void enterEvent(QEvent *) override;
  void leaveEvent(QEvent *) override;
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
