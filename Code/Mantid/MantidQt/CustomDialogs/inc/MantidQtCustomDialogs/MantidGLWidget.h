#ifndef MANTIDQT_CUSTOMDIALOGS_MANTIDGLWIDGET_H_
#define MANTIDQT_CUSTOMDIALOGS_MANTIDGLWIDGET_H_

//-----------------------------------------
// Includes
//-----------------------------------------
#include <QGLWidget>
#include "boost/shared_ptr.hpp"

#include "MantidGeometry/V3D.h"

//-----------------------------------------
// Forward Declarations
//-----------------------------------------
namespace Mantid
{
namespace Geometry
{
  class Object;
}
}

namespace MantidQt
{
namespace CustomDialogs
{

/** 
    This class provides a widget to display a Mantid Geometry object using 
    OpenGL.

    @author Martyn Gigg, Tessella Support Services plc
    @date 18/05/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class MantidGLWidget : public QGLWidget
{

  Q_OBJECT

public:
  ///Default constructor
  MantidGLWidget(QWidget *parent = 0);
  ///Destructor
  ~MantidGLWidget();
  
  /// Set the Mantid geometry object
  void setDisplayObject(boost::shared_ptr<Mantid::Geometry::Object> object);

protected:
  /// Initialize the renderer
  void initializeGL();
  /// Set up the viewport
  void resizeGL(int width, int height);
  /// Render the scene
  void paintGL();

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
  void mousePressEvent(QMouseEvent *event);
  /// Handle a MouseMoveEvent
  void mouseMoveEvent(QMouseEvent *event);
  /// A Mantid geometry object
  boost::shared_ptr<Mantid::Geometry::Object> m_display_object;
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

}
}

#endif //MANTIDQT_CUSTOMDIALOGS_MANTIDGLWIDGET_H_
