#ifdef _WIN32
#include <windows.h>
#endif
#include "MantidQtWidgets/InstrumentView/MantidGLWidget.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"
#include "MantidQtWidgets/InstrumentView/Projection3D.h"

#include <boost/shared_ptr.hpp>

#include <QtOpenGL>
#include <QSpinBox>
#include <QApplication>
#include <QTime>

#include <map>
#include <string>
#include <cfloat>
#include <typeinfo>

namespace MantidQt {
namespace MantidWidgets {
//#ifndef GL_MULTISAMPLE
//#define GL_MULTISAMPLE  0x809D
//#endif

// NOTES:
// 1) if the sample buffers are not available then the paint of image on the mdi
// windows
//   seems to not work on intel chipset

const Qt::CursorShape cursorShape = Qt::ArrowCursor;

MantidGLWidget::MantidGLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::DepthBuffer | QGL::NoAlphaChannel), parent),
      // m_polygonMode(SOLID),
      // m_lightingState(0),
      m_isKeyPressed(false), m_firstFrame(true) {

  if (!this->format().depth()) {
    std::cout << "Warning! OpenGL Depth buffer could not be initialized.\n";
  }

  setFocusPolicy(Qt::StrongFocus);
  setAutoFillBackground(false);
  // Enable right-click in pick mode
  setContextMenuPolicy(Qt::DefaultContextMenu);
  setMouseTracking(true);
}

MantidGLWidget::~MantidGLWidget() {}

void MantidGLWidget::setSurface(boost::shared_ptr<ProjectionSurface> surface) {
  m_surface = surface;
  connect(m_surface.get(), SIGNAL(redrawRequired()), this, SLOT(repaint()),
          Qt::QueuedConnection);
  m_firstFrame = true;
}

/**
* This method initializes the opengl settings. its invoked defaultly by Qt when
* the widget
* is initialized.
*/
void MantidGLWidget::initializeGL() {
  setCursor(cursorShape); // This is to set the initial window mouse cursor to
                          // Hand icon

  // Set the relevant OpenGL rendering options
  setRenderingOptions();
  glViewport(0, 0, width(), height());

  // Clear the memory buffers
  QColor bgColor = currentBackgroundColor();
  glClearColor(GLclampf(bgColor.red() / 255.0),
               GLclampf(bgColor.green() / 255.0),
               GLclampf(bgColor.blue() / 255.0), 1.0);
}

void MantidGLWidget::setRenderingOptions() {
  // Enable depth testing. This only draws points that are not hidden by other
  // objects
  glEnable(GL_DEPTH_TEST);

  // Depth function for testing is Less than or equal
  glDepthFunc(GL_LEQUAL);

  // Disable colour blending
  glDisable(GL_BLEND);

  // Disable face culling because some polygons are visible from the back.
  glDisable(GL_CULL_FACE);

  // enablewriting into the depth buffer
  glDepthMask(GL_TRUE);

  OpenGLError::check("setRenderingOptions");
}

/**
* This is overridden function which is called by Qt when the widget needs to be
* repainted.
*/
void MantidGLWidget::paintEvent(QPaintEvent *event) {
  UNUSED_ARG(event)
  makeCurrent();
  if (m_surface) {
    m_surface->draw(this);
  }

  OpenGLError::check("paintEvent");

  if (m_firstFrame) {
    m_firstFrame = false;
  }
}

/**
* This function is also overridden from the parent. This method is invoked when
* the widget is resized
* This method resizes the viewport according to the new widget width and height
*/
void MantidGLWidget::resizeGL(int width, int height) {
  if (m_surface) {
    m_surface->resize(width, height);
  }
}

/**
* Called when a context menu event is recieved
*/
void MantidGLWidget::contextMenuEvent(QContextMenuEvent *event) {
  UNUSED_ARG(event) // avoid compiler warning
}

/**
* Mouse press callback method, It implements mouse button press initialize
* methods.
* Left Button: Zoom
* Right Button: Rotate
* Middle Button: Translate
* Key + Left Button: Pick (TODO: Yet to implement)
* @param event :: This is the event variable which has the position and button
* states
*/
void MantidGLWidget::mousePressEvent(QMouseEvent *event) {
  if (m_surface) {
    m_surface->mousePressEvent(event);
  }
  update();
}

/**
* This is mouse move callback method. It implements the actions to be taken when
* the mouse is
* moved with a particular button is pressed.
* Left Button: Zoom
* Right Button: Rotate
* Middle Button: Translate
* Key + Left Button: Pick (TODO: Yet to implement)
* @param event :: This is the event variable which has the position and button
* states
*/
void MantidGLWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_surface) {
    m_surface->mouseMoveEvent(event);
  }
  repaint();
}

/**
* This is mouse button release callback method. This resets the cursor to
* pointing hand cursor
* @param event :: This is the event variable which has the position and button
* states
*/
void MantidGLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (m_surface) {
    m_surface->mouseReleaseEvent(event);
  }
  repaint();
}

/**
* Mouse wheel event to set the zooming in and out
* @param event :: This is the event variable which has the status of the wheel
*/
void MantidGLWidget::wheelEvent(QWheelEvent *event) {
  if (m_surface) {
    m_surface->wheelEvent(event);
  }
  update();
}

/**
* This method is to handle keyboard events to mimic the mouse operations of
* click and move
* @param event :: This is the event variable which has the status of the
* keyboard
*/
void MantidGLWidget::keyPressEvent(QKeyEvent *event) {
  if (m_surface) {
    m_surface->keyPressEvent(event);
  }
  update();
}

/**
* This method is to handle keyboard events to mimic the mouse operations of
* mouse button up.
* @param event :: This is the event variable which has the status of the
* keyboard
*/
void MantidGLWidget::keyReleaseEvent(QKeyEvent *event) {
  releaseKeyboard();
  setCursor(cursorShape);
  m_isKeyPressed = false;
  if (!event->isAutoRepeat()) {
    update();
  }
  OpenGLError::check("MantidGLWidget::keyReleaseEvent");
}

/**
* This method set the background color.
*/
void MantidGLWidget::setBackgroundColor(QColor input) {
  makeCurrent();
  glClearColor(GLclampf(input.red() / 255.0), GLclampf(input.green() / 255.0),
               GLclampf(input.blue() / 255.0), 1.0);
  OpenGLError::check("MantidGLWidget::setBackgroundColor");
  if (m_surface) {
    m_surface->setBackgroundColor(input);
    m_surface->updateView(false);
  }
  update();
}

QColor MantidGLWidget::currentBackgroundColor() const {
  return m_surface ? m_surface->getBackgroundColor() : QColor(Qt::black);
}

/**
* This saves the GL scene to a file.
* @param filename :: The name of the file
*/
void MantidGLWidget::saveToFile(const QString &filename) {
  if (filename.isEmpty())
    return;
  // It seems QGLWidget grabs the back buffer
  this->swapBuffers(); // temporarily swap the buffers
  QImage image = this->grabFrameBuffer();
  this->swapBuffers(); // swap them back
  OpenGLError::check("MantidGLWidget::saveToFile");
  image.save(filename);
}

/**
* Resets the widget for new instrument definition
*/
void MantidGLWidget::resetWidget() {
  // setActorCollection(boost::shared_ptr<GLActorCollection>(new
  // GLActorCollection()));
}

/**
* Enables / disables lighting on the surfaces that support it.
* @param on :: Set true to turn lighting on or false to turn it off.
*/
void MantidGLWidget::enableLighting(bool on) {
  if (m_surface) {
    m_surface->enableLighting(on);
    updateView();
  }
}

void MantidGLWidget::draw() {
  QApplication::setOverrideCursor(Qt::WaitCursor);
  if (m_surface) {
    m_surface->draw(this);
  }
  QApplication::restoreOverrideCursor();
  OpenGLError::check("MantidGLWidget::drawUnwrapped()");
}

void MantidGLWidget::componentSelected(size_t componentIndex) {
  if (m_surface) {
    m_surface->componentSelected(componentIndex);
    m_surface->updateView();
    update();
  }
}

/// Redraw the view
/// @param picking :: Set to true to update the picking image regardless the
/// interaction
///   mode of the surface.
void MantidGLWidget::updateView(bool picking) {
  if (m_surface) {
    m_surface->updateView(picking);
    update();
  }
}

void MantidGLWidget::updateDetectors() {
  if (m_surface) {
    m_surface->updateDetectors();
    update();
  }
}

void MantidGLWidget::enterEvent(QEvent *ev) {
  if (m_surface) {
    m_surface->enterEvent(ev);
  }
  update();
}

void MantidGLWidget::leaveEvent(QEvent *ev) {
  // Restore possible override cursor
  while (QApplication::overrideCursor()) {
    QApplication::restoreOverrideCursor();
  }
  if (m_surface) {
    m_surface->leaveEvent(ev);
  }
  update();
}

} // MantidWidgets
} // MantidQt
