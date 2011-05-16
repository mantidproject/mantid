//-----------------------------------------
// Includes
//-----------------------------------------
#include "MantidQtCustomDialogs/MantidGLWidget.h"
#include "MantidGeometry/Objects/Object.h"
//#include "MantidGeometry/V3D.h"
//#include "MantidGeometry/Quat.h"

#include <QtOpenGL>
#include <QMessageBox>

#include <cfloat>

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
  m_bb_widths[0] = 0.0;
  m_bb_widths[1] = 0.0;
  m_bb_widths[2] = 0.0;

  m_bb_centres[0] = 0.0;
  m_bb_centres[1] = 0.0;
  m_bb_centres[2] = 0.0;
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
 * @param object :: A pointer to the Mantid::Geometry::Object
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
  const double bb_large(1e10);
  double bbox[6] = { bb_large, bb_large, bb_large, -bb_large, -bb_large, -bb_large };
  m_display_object->getBoundingBox(bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]);

  // The abs function kept casting my doubles to integers, hence the next two lines
  //double max_bb = bbox[0];
  //if( max_bb < 0. ) max_bb *=-1;
  //for( int i = 1; i < 6; ++i )
  //{
  //  double d = bbox[i];
  //  if( d < 0. ) d *= -1.;
  //  if( d > max_bb && d < 1e10)
  //  {
  //    max_bb = d;
  //  }
  //}
//  m_min_bb *= 2.0; m_max_bb *= 2.0;
  //std::cerr << "\n";
  //Calculate the widths and save for resize events
  for( int i = 0; i < 3; ++i )
  {

    //    std::cerr << "Bounding box max:" << bbox[i] << "  min: " << bbox[i+3] << "\n"; 
    m_bb_widths[i] = 1.1*(bbox[i] - bbox[i + 3]);
    if( m_bb_widths[i] < 0.0 ) m_bb_widths[i] *= -1.0;
    if( std::fabs(bbox[i]) < 1e10 && std::fabs(bbox[i+3]) < 1e10 )
    {
      m_bb_centres[i] = (bbox[i] + bbox[i + 3]) / 2.0;
    }
    else
    {
      m_bb_centres[i] = 0.0;
    }
    if( m_bb_centres[i] < 0.0 ) m_bb_centres[i] *= -1.0;
  }
  
//  std::cerr << "centres: " << m_bb_centres[0] << " " << m_bb_centres[1] << " " << m_bb_centres[2] << "\n";

  GLdouble aspect_ratio((GLdouble)this->width() / (GLdouble)this->height());
  setOrthoProjectionMatrix(aspect_ratio);

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
  glClearColor(0.0,0.0,0.0, 0.0);
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  setCursor(Qt::PointingHandCursor); // This is to set the initial window mouse cursor to Hand icon
  glEnable(GL_DEPTH_TEST);           // Enable opengl depth test to render 3D object properly
  glShadeModel(GL_SMOOTH);           // Shade model is smooth (expensive but looks pleasing)
  glEnable(GL_LINE_SMOOTH);          // Set line should be drawn smoothly
  
  //glEnable(GL_NORMALIZE);

  glEnable (GL_LIGHTING);            // Enable light
  glEnable(GL_LIGHT0);               // Enable opengl first light
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);  // This model lits both sides of the triangle
  //Set Light0 Attributes, Ambient, diffuse,specular and position
 //Its a directional light which follows camera position
  GLfloat lamp_ambient[4] = {0.40f, 0.0f, 1.0f, 0.0f};
  GLfloat lamp_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat lamp_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  glLightfv(GL_LIGHT0, GL_AMBIENT,lamp_ambient );
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lamp_specular);
  GLfloat lamp_pos[4] = {0.0f, 0.0f, 1.0f, 0.0};
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

  // The factor of 16 is due to Qt using angles that are in
  // 1/16ths of a degree
  glRotated(m_x_rot / 16.0, 1.0, 0.0, 0.0);
  glRotated(m_y_rot / 16.0, 0.0, 1.0, 0.0);
  glRotated(m_z_rot / 16.0, 0.0, 0.0, 1.0);


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
  glPopMatrix();  
}

/**
 * Handle a resize event
 * @param width :: The width of the resized viewport
 * @param height :: The height of the resized viewport
 */
void MantidGLWidget::resizeGL(int width, int height)
{
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
  if( height == 0 ) height = 1;

  GLdouble aspect_ratio = (GLdouble)width/(GLdouble)height;
  setOrthoProjectionMatrix(aspect_ratio);
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
  // Points in Qt only have integer precision
  int x_rot = static_cast<int>(m_x_rot);
  
  if (event->buttons() & Qt::LeftButton) {
    setXRotation(x_rot + 8 * dy);
    setYRotation(static_cast<int>(m_y_rot) + 8 * dx);
  } else if (event->buttons() & Qt::RightButton) {
    setXRotation(x_rot + 8 * dy);
    setZRotation(static_cast<int>(m_z_rot) + 8 * dx);
  }
  m_click_point = event->pos();

}

/**
 * Set the current rotation angle around the X-axis.
 * @param angle :: The angle of rotation
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
 * @param angle :: The angle of rotation
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
 * @param angle :: The angle of rotation
 */
void MantidGLWidget::setZRotation(int angle)
{
  normalizeAngle(&angle);
  if (angle != m_z_rot) {
    m_z_rot = angle;
    updateGL();
  }
}

void MantidGLWidget::setOrthoProjectionMatrix(GLdouble aspect_ratio)
{
  GLdouble left = - m_bb_widths[0]/2.0;
  GLdouble right = m_bb_widths[0]/2.0;
  GLdouble bottom = - m_bb_widths[1]/2.0;
  GLdouble top = + m_bb_widths[1]/2.0;
  
  //std::cerr << "Projection volume points: " << left << " " << right << " " << bottom << " " << top << " " << znear << " " << zfar << "\n";

  // Window taller than wide
  if( aspect_ratio < 1.0 )
  {
    top /= aspect_ratio;
    bottom /= aspect_ratio;
  }
  // Window wider than tall 
  else
  {
    left *= aspect_ratio;
    right *= aspect_ratio;
  }

  left += m_bb_centres[0];
  right += m_bb_centres[0];
  bottom += m_bb_centres[1];
  top += m_bb_centres[1];

  //std::cerr << "Projection volume points 2: " << left << " " << right << " " << bottom << " " << top << "\n";

  // Set the correct projection 
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(left, right, bottom, top, -10.0, 10000.0);//znear, zfar);
  glMatrixMode(GL_MODELVIEW);
}
  
  
  /**
 * Adjust the angle given so that it is within the range 0 < x < (360 * 16)
 * (Note: The factor of 16 is due to Qt using angles in 1/16th of a degree)
 * @param angle :: The angle of rotation
 */
void MantidGLWidget::normalizeAngle(int *angle)
{
  while (*angle < 0)
    *angle += 360 * 16;
  while (*angle > 360 * 16)
    *angle -= 360 * 16;
}
