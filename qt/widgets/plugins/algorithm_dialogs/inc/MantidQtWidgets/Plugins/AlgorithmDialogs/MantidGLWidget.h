// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMDIALOGS_MANTIDGLWIDGET_H_
#define MANTIDQT_CUSTOMDIALOGS_MANTIDGLWIDGET_H_

//-----------------------------------------
// Includes
//-----------------------------------------
#include <QGLWidget>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/V3D.h"

//-----------------------------------------
// Forward Declarations
//-----------------------------------------
namespace Mantid {
namespace Geometry {
class IObject;
}
} // namespace Mantid

namespace MantidQt {
namespace CustomDialogs {

/**
    This class provides a widget to display a Mantid Geometry object using
    OpenGL.

    @author Martyn Gigg, Tessella Support Services plc
    @date 18/05/2009
*/
class MantidGLWidget : public QGLWidget {

  Q_OBJECT

public:
  /// Default constructor
  MantidGLWidget(QWidget *parent = nullptr);
  /// Destructor
  ~MantidGLWidget() override;

  /// Set the Mantid geometry object
  void setDisplayObject(boost::shared_ptr<Mantid::Geometry::IObject> object);

protected:
  /// Initialize the renderer
  void initializeGL() override;
  /// Set up the viewport
  void resizeGL(int width, int height) override;
  /// Render the scene
  void paintGL() override;

private:
  /// Set the rotation angle around the X-axis
  void setXRotation(int angle);
  /// Set the rotation angle around the Y-axis
  void setYRotation(int angle);
  /// Set the rotation angle around the Z-axis
  void setZRotation(int angle);
  /// Calculate and set the orthographic projection matrix
  void setOrthoProjectionMatrix(GLdouble aspect_ratio);

private:
  /// Ensure the angle is in the range 0 < angle < 360
  void normalizeAngle(int *angle);
  /// Handle a MousePressEvent
  void mousePressEvent(QMouseEvent *event) override;
  /// Handle a MouseMoveEvent
  void mouseMoveEvent(QMouseEvent *event) override;
  /// A Mantid geometry object
  boost::shared_ptr<Mantid::Geometry::IObject> m_display_object;
  /// The current X, Y and Z rotations
  GLdouble m_x_rot, m_y_rot, m_z_rot;
  /// The scaling factor to use
  GLdouble m_scale_factor;
  /// The location of the cursor when the mouse button was clicked
  QPoint m_click_point;
  /// The separation of the bounding box sides in x,y,z respectively
  GLdouble m_bb_widths[3];
  /// The centre of the bounding box
  GLdouble m_bb_centres[3];
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMDIALOGS_MANTIDGLWIDGET_H_
