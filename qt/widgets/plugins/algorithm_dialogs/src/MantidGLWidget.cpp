//-----------------------------------------
// Includes
//-----------------------------------------
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/MantidGLWidget.h"
#include "MantidGeometry/Objects/CSGObject.h"

#include <QMessageBox>
#include <QtOpenGL>

#include <cfloat>

using namespace MantidQt::CustomDialogs;

//---------------------------------------
// Public member functions
//---------------------------------------
/**
 * Constructor
 */
MantidGLWidget::MantidGLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::DepthBuffer | QGL::NoAlphaChannel |
                          QGL::SampleBuffers),
                parent),
      m_display_object(boost::shared_ptr<Mantid::Geometry::CSGObject>()),
      m_x_rot(0.0), m_y_rot(0.0), m_z_rot(0.0), m_scale_factor(1.0) {
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
MantidGLWidget::~MantidGLWidget() { makeCurrent(); }

/**
 * Set the shape object
 * @param object :: A pointer to the Mantid::Geometry::Object
 */
void MantidGLWidget::setDisplayObject(
    boost::shared_ptr<Mantid::Geometry::IObject> object) {
  m_display_object = object;
  m_x_rot = 0.0;
  m_y_rot = 0.0;
  m_z_rot = 0.0;

  auto boundingBox = m_display_object->getBoundingBox();
  double bbox[6] = {boundingBox.xMax(), boundingBox.yMax(), boundingBox.zMax(),
                    boundingBox.xMin(), boundingBox.yMin(), boundingBox.zMin()};

  // Calculate the widths and save for resize events
  for (int i = 0; i < 3; ++i) {

    m_bb_widths[i] = 1.1 * (bbox[i] - bbox[i + 3]);
    if (m_bb_widths[i] < 0.0)
      m_bb_widths[i] *= -1.0;
    if (std::fabs(bbox[i]) < 1e10 && std::fabs(bbox[i + 3]) < 1e10) {
      m_bb_centres[i] = (bbox[i] + bbox[i + 3]) / 2.0;
    } else {
      m_bb_centres[i] = 0.0;
    }
    if (m_bb_centres[i] < 0.0)
      m_bb_centres[i] *= -1.0;
  }

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
void MantidGLWidget::initializeGL() {
  // Without this the initial display draws random rubbish from the graphics
  // memory
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  setCursor(Qt::PointingHandCursor); // This is to set the initial window mouse
                                     // cursor to Hand icon
  glEnable(
      GL_DEPTH_TEST); // Enable opengl depth test to render 3D object properly
  glShadeModel(
      GL_SMOOTH); // Shade model is smooth (expensive but looks pleasing)
  glEnable(GL_LINE_SMOOTH); // Set line should be drawn smoothly

  // glEnable(GL_NORMALIZE);

  glEnable(GL_LIGHTING); // Enable light
  glEnable(GL_LIGHT0);   // Enable opengl first light
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,
                GL_TRUE); // This model lits both sides of the triangle
  // Set Light0 Attributes, Ambient, diffuse,specular and position
  // Its a directional light which follows camera position
  GLfloat lamp_ambient[4] = {0.40f, 0.0f, 1.0f, 0.0f};
  GLfloat lamp_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat lamp_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  glLightfv(GL_LIGHT0, GL_AMBIENT, lamp_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lamp_specular);
  GLfloat lamp_pos[4] = {0.0f, 0.0f, 1.0f, 0.0};
  glLightfv(GL_LIGHT0, GL_POSITION, lamp_pos);
}

/**
 * Render the 3D scene
 */
void MantidGLWidget::paintGL() {
  // Nothing to draw
  if (m_display_object == boost::shared_ptr<Mantid::Geometry::CSGObject>())
    return;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPushMatrix();

  // The factor of 16 is due to Qt using angles that are in
  // 1/16ths of a degree
  glRotated(m_x_rot / 16.0, 1.0, 0.0, 0.0);
  glRotated(m_y_rot / 16.0, 0.0, 1.0, 0.0);
  glRotated(m_z_rot / 16.0, 0.0, 0.0, 1.0);

  try {
    m_display_object->draw();
  } catch (...) {
    QMessageBox::information(
        this, "MantidGLWidget",
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
void MantidGLWidget::resizeGL(int width, int height) {
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
  if (height == 0)
    height = 1;

  GLdouble aspect_ratio = (GLdouble)width / (GLdouble)height;
  setOrthoProjectionMatrix(aspect_ratio);
}

/**
 * Handle an event when a mouse button is pressed.
 * @param event A pointer to the QMouseEvent
 */
void MantidGLWidget::mousePressEvent(QMouseEvent *event) {
  m_click_point = event->pos();
}

/**
 * Handle an event where the cursor is moved with the mouse
 * @param event A pointer to the QMouseEvent
 */
void MantidGLWidget::mouseMoveEvent(QMouseEvent *event) {
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
void MantidGLWidget::setXRotation(int angle) {
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
void MantidGLWidget::setYRotation(int angle) {
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
void MantidGLWidget::setZRotation(int angle) {
  normalizeAngle(&angle);
  if (angle != m_z_rot) {
    m_z_rot = angle;
    updateGL();
  }
}

void MantidGLWidget::setOrthoProjectionMatrix(GLdouble aspect_ratio) {
  GLdouble left = -m_bb_widths[0] / 2.0;
  GLdouble right = m_bb_widths[0] / 2.0;
  GLdouble bottom = -m_bb_widths[1] / 2.0;
  GLdouble top = +m_bb_widths[1] / 2.0;

  // std::cerr << "Projection volume points: " << left << " " << right << " " <<
  // bottom << " " << top << '\n';

  // width / height ratio in world coordinates must be equal to aspect_ratio
  auto ratio = m_bb_widths[0] / m_bb_widths[1];

  if (ratio < aspect_ratio) {
    auto width = m_bb_widths[1] * aspect_ratio;
    left = -width / 2.0;
    right = width / 2.0;
  } else {
    auto height = m_bb_widths[0] / aspect_ratio;
    bottom = -height / 2.0;
    top = height / 2.0;
  }

  left += m_bb_centres[0];
  right += m_bb_centres[0];
  bottom += m_bb_centres[1];
  top += m_bb_centres[1];

  // std::cerr << "Projection volume points 2: " << left << " " << right << " "
  // << bottom << " " << top << '\n';
  // std::cerr << "Aspect ratio: " << aspect_ratio << " == " << (right - left) /
  // (top - bottom) << '\n';

  // Set the correct projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(left, right, bottom, top, -10.0, 10000.0);
  glMatrixMode(GL_MODELVIEW);
}

/**
 *Adjust the angle given so that it is within the range 0 < x < (360 * 16)
 *(Note: The factor of 16 is due to Qt using angles in 1/16th of a degree)
 *@param angle :: The angle of rotation
 */
void MantidGLWidget::normalizeAngle(int *angle) {
  while (*angle < 0)
    *angle += 360 * 16;
  while (*angle > 360 * 16)
    *angle -= 360 * 16;
}
