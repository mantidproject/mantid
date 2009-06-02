//-----------------------------------------
// Includes
//-----------------------------------------
#include "MantidQtCustomDialogs/MantidGLWidget.h"
#include "MantidGeometry/Object.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"

#include <QtOpenGL>
#include <QMessageBox>

using namespace MantidQt::CustomDialogs;

//---------------------------------------
// Public member functions
//---------------------------------------
/**
 * Constructor
 */
MantidGLWidget::MantidGLWidget(QWidget *parent) : 
  QGLWidget(QGLFormat(QGL::DepthBuffer|QGL::NoAlphaChannel|QGL::SampleBuffers), parent), 
  m_display_object(boost::shared_ptr<Mantid::Geometry::Object>()),
  m_x_rot(0.0), m_y_rot(0.0), m_z_rot(0.0), m_scale_factor(1.0)
{
  setAutoFillBackground(false);
}

/**
 * Destructor
 */
MantidGLWidget::~MantidGLWidget()
{
  makeCurrent();
}

/**
 * Set the shape object
 * @param object A pointer to the Mantid::Geometry::Object
 */
void MantidGLWidget::setDisplayObject(boost::shared_ptr<Mantid::Geometry::Object> object)
{
  m_display_object = object;
  m_x_rot = 0.0;
  m_y_rot = 0.0;
  m_z_rot = 0.0;
  
  // Calculate a scale factor from the objects bounding box
  // The bounding box function seems to require maxima to be set for each of 
  // the numbers or else it returns rubbish
  // I'll set them to big numbers here
  const double bb_large(1e8);
  double bbox[6] = { bb_large, bb_large, bb_large, -bb_large, -bb_large, -bb_large };
  m_display_object->getBoundingBox(bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]);
  // The abs function kept casting my doubles to integers, hence the next two lines
  double max_bb = bbox[0];
  if( max_bb < 0. ) max_bb *=-1;
  for( int i = 1; i < 6; ++i )
  {
    double d = bbox[i];
    if( d < 0. ) d *= -1.;
    if( d > max_bb )
    {
      max_bb = d;
    }
  }

  m_scale_factor = 0.65 / max_bb;
  updateGL();
}

//---------------------------------------
// Protected member functions
//---------------------------------------
/**
 * Initialize the OpenGL display
 */
void MantidGLWidget::initializeGL()
{
  // Without this the initial display draws random rubbish from the graphics memory
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glClearColor(0.0,0.0, 0.0, 0.0);
  setCursor(Qt::PointingHandCursor); // This is to set the initial window mouse cursor to Hand icon
  glEnable(GL_DEPTH_TEST);           // Enable opengl depth test to render 3D object properly
  glShadeModel(GL_SMOOTH);           // Shade model is smooth (expensive but looks pleasing)
  glEnable(GL_LINE_SMOOTH);          // Set line should be drawn smoothly
  
  glEnable(GL_NORMALIZE);

  glEnable (GL_LIGHTING);            // Enable light
  glEnable(GL_LIGHT0);               // Enable opengl first light
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);  // This model lits both sides of the triangle
  // Set Light0 Attributes, Ambient, diffuse,specular and position
  // Its a directional light which follows camera position
  GLfloat lamp_ambient[4] = {0.40, 0.0, 1.0, 0.0};
  GLfloat lamp_diffuse[4] = {1.0,1.0,1.0,1.0};
  GLfloat lamp_specular[4] = {1.0,1.0,1.0,1.0};

  glLightfv(GL_LIGHT0, GL_AMBIENT,lamp_ambient );
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lamp_specular);
  GLfloat lamp_pos[4] = {0.0,0.0,1.0,0.0};
  glLightfv(GL_LIGHT0, GL_POSITION, lamp_pos);
}

/**
 * Render the 3D scene
 */
void MantidGLWidget::paintGL()
{
  // Nothing to draw
  if( m_display_object == boost::shared_ptr<Mantid::Geometry::Object>() ) return;

  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPushMatrix();
  QApplication::setOverrideCursor(Qt::WaitCursor);

  // The factor of 16 is due to Qt using angles that are in
  // 1/16ths of a degree
  glRotated(m_x_rot / 16.0, 1.0, 0.0, 0.0);
  glRotated(m_y_rot / 16.0, 0.0, 1.0, 0.0);
  glRotated(m_z_rot / 16.0, 0.0, 0.0, 1.0);

  glScalef(m_scale_factor, m_scale_factor, m_scale_factor);

  try 
  {
    m_display_object->draw();
  }
  catch( ... )
  {
    QMessageBox::information(this,"MantidGLWidget", 
			     QString("An error occurred while attempting to render the shape.\n") +
			     "Please check that all objects intersect each other.");
  }
  
  QApplication::restoreOverrideCursor();
  glPopMatrix();
}

/**
 * Handle a resize event
 * @param width The width of the resized viewport
 * @param height The height of the resized viewport
 */
void MantidGLWidget::resizeGL(int /*width*/, int /*height*/)
{
  // NEED TO IMPLEMENT
}

/**
 * Handle an event when a mouse button is pressed.
 * @prarm event A pointer to the QMouseEvent
 */
void MantidGLWidget::mousePressEvent(QMouseEvent *event)
{
  m_click_point = event->pos();
}

/**
 * Handle an event where the cursor is moved with the mouse
 * @prarm event A pointer to the QMouseEvent
 */
void MantidGLWidget::mouseMoveEvent(QMouseEvent *event)
{
  int dx = event->x() - m_click_point.x();
  int dy = event->y() - m_click_point.y();
  
  if (event->buttons() & Qt::LeftButton) {
    setXRotation(m_x_rot + 8 * dy);
    setYRotation(m_y_rot + 8 * dx);
  } else if (event->buttons() & Qt::RightButton) {
    setXRotation(m_x_rot + 8 * dy);
    setZRotation(m_z_rot + 8 * dx);
  }
  m_click_point = event->pos();

}

/**
 * Set the current rotation angle around the X-axis.
 * @param angle The angle of rotation
 */
void MantidGLWidget::setXRotation(int angle)
{
  normalizeAngle(&angle);
  if (angle != m_x_rot) {
    m_x_rot = angle;
    updateGL();
  }
}

/**
 * Set the current rotation angle around the Y-axis.
 * @param angle The angle of rotation
 */
void MantidGLWidget::setYRotation(int angle)
{
  normalizeAngle(&angle);
  if (angle != m_y_rot) {
    m_y_rot = angle;
    updateGL();
  }
}

/**
 * Set the current rotation angle around the Z-axis.
 * @param angle The angle of rotation
 */
void MantidGLWidget::setZRotation(int angle)
{
  normalizeAngle(&angle);
  if (angle != m_z_rot) {
    m_z_rot = angle;
    updateGL();
  }
}

/**
 * Adjust the angle given so that it is within the range 0 < x < (360 * 16)
 * (Note: The factor of 16 is due to Qt using angles in 1/16th of a degree)
 * @param angle The angle of rotation
 */
void MantidGLWidget::normalizeAngle(int *angle)
{
  while (*angle < 0)
    *angle += 360 * 16;
  while (*angle > 360 * 16)
    *angle -= 360 * 16;
}
