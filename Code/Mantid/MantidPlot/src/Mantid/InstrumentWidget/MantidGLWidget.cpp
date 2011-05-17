 #ifdef WIN32
#include <windows.h>
#endif
#include "MantidGLWidget.h"
#include "OpenGLError.h"
#include "UnwrappedSurface.h"

#include <boost/shared_ptr.hpp>

#include <QtOpenGL>
#include <QSpinBox>
#include <QApplication>
#include <QTime>

#include <map>
#include <string>
#include <iostream>
#include <cfloat>
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

//NOTES:
//1) if the sample buffers are not available then the paint of image on the mdi windows
//   seems to not work on intel chipset


MantidGLWidget::MantidGLWidget(QWidget* parent):
  QGLWidget(QGLFormat(QGL::DepthBuffer|QGL::NoAlphaChannel|QGL::SampleBuffers),parent)
{

  if (!this->format().depth())
  {
    std::cout << "Warning! OpenGL Depth buffer could not be initialized.\n";
  }
  //std::cout << "Depth buffer size is " << this->format().depthBufferSize() << "\n";

  isKeyPressed=false;
  m_interactionMode=MantidGLWidget::MoveMode;
  setFocusPolicy(Qt::StrongFocus);
  setAutoFillBackground(false);
  m_bgColor=QColor(0,0,0,1);
  m_polygonMode = SOLID;
  m_lightingState = 0;
  m_firstFrame = true;
  m_renderMode = FULL3D;
  //m_unwrappedSurface = NULL;
  //m_unwrappedSurfaceChanged = true;
  //m_unwrappedViewChanged = true;

  //Enable right-click in pick mode
  setContextMenuPolicy(Qt::DefaultContextMenu);
  setRenderingOptions();

}

MantidGLWidget::~MantidGLWidget()
{
  //if (m_unwrappedSurface)
  //{
  //  delete m_unwrappedSurface;
  //}
}

void MantidGLWidget::setInteractionModePick()
{
  m_interactionMode = PickMode;
  setMouseTracking(true);
  setCursor(Qt::ArrowCursor);
  update();
}

void MantidGLWidget::setInteractionModeMove()
{
  m_interactionMode = MoveMode;
  setMouseTracking(false);
  //setCursor(Qt::PointingHandCursor);
  glEnable(GL_NORMALIZE);
  if (m_lightingState > 0)
  {
    glEnable(GL_LIGHTING);
  }
  update();
}

/**
 * This method initializes the opengl settings. its invoked defaultly by Qt when the widget
 * is initialized.
 */
void MantidGLWidget::initializeGL()
{
  setCursor(Qt::PointingHandCursor); // This is to set the initial window mouse cursor to Hand icon
  
  // Set the relevant OpenGL rendering options
  setRenderingOptions();
  
  // Clear the memory buffers
  glClearColor(GLclampf(m_bgColor.red()/255.0),GLclampf(m_bgColor.green()/255.0),GLclampf(m_bgColor.blue()/255.0),1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MantidGLWidget::setRenderingOptions()
{
  // Enable depth testing. This only draws points that are not hidden by other objects
  glEnable(GL_DEPTH_TEST);

  // Depth function for testing is Less than or equal                        
  glDepthFunc(GL_LEQUAL);

  // Disable colour blending
  glDisable(GL_BLEND);

  //Disable face culling because some polygons are visible from the back.
  glDisable(GL_CULL_FACE);

  //enablewriting into the depth buffer
  glDepthMask(GL_TRUE);

  OpenGLError::check("setRenderingOptions");
}

/**
 * Toggles the use of high resolution lighting
 * @param state :: An integer indicating lighting state. (Note that this is not a boolean because Qt's CheckBox emits an integer signal)
 * Unchecked = 0, ,PartiallyChecked = 1, Checked = 2
 */
void MantidGLWidget::setLightingModel(int state)
{
  // Basic lighting
  if( state == 0 )
  {
    glShadeModel(GL_FLAT);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LINE_SMOOTH);
  }
  // High end shading and lighting
  else if( state == 2 )
  {
    glShadeModel(GL_SMOOTH);           // Shade model is smooth (expensive but looks pleasing)
    glEnable (GL_LIGHTING);            // Enable light
    glEnable(GL_LIGHT0);               // Enable opengl first light
    glEnable(GL_LINE_SMOOTH);          // Set line should be drawn smoothly
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);  // This model lits both sides of the triangle
    // Set Light0 Attributes, Ambient, diffuse,specular and position
    // Its a directional light which follows camera position
    float lamp_ambient[4]={0.0,0.0,0.0,1.0};
    float lamp_diffuse[4]={1.0,1.0,1.0,1.0};
    float lamp_specular[4]={1.0,1.0,1.0,1.0};
    glLightfv(GL_LIGHT0, GL_AMBIENT,lamp_ambient );
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lamp_specular);
    float lamp_pos[4]={0.0,0.0,1.0,0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, lamp_pos);
  }
  else return;
}


/**
 * This is overridden function which is called by Qt when the widget needs to be repainted.
 */
void MantidGLWidget::paintEvent(QPaintEvent *event)
{
  //(void) event; //avoid compiler warning
  //makeCurrent();
  //if(iInteractionMode == MantidGLWidget::PickMode)
  //{
  //  if (m_renderMode == FULL3D)
  //  {
  //    if(mPickingDraw==true)
  //    {
  //      switchToPickingMode();
  //    }
  //    QPainter painter(this);
  //    painter.setRenderHint(QPainter::Antialiasing);
  //    mPickBox->draw(&painter);
  //    painter.end();
  //  }
  //  else
  //  {
  //    drawUnwrapped();
  //  }
  //}
  //else
  //{
  //  drawDisplayScene();
  //}


  //if (m_firstFrame)
  //{
  //  update();
  //  m_firstFrame = false;
  //}
}

/**
 * This function is also overridden from the parent. This method is invoked when the widget is resized
 * This method resizes the viewport according to the new widget width and height
 */
void MantidGLWidget::resizeGL(int width, int height)
{
}

/**
 * Mouse press callback method, It implements mouse button press initialize methods.
 * Left Button: Zoom
 * Right Button: Rotate
 * Middle Button: Translate
 * Key + Left Button: Pick (TODO: Yet to implement)
 * @param event :: This is the event variable which has the position and button states
 */
void MantidGLWidget::mousePressEvent(QMouseEvent* event)
{
}

/**
 * Called when a context menu event is recieved
 */
void MantidGLWidget::contextMenuEvent(QContextMenuEvent * event)
{
  (void) event; //avoid compiler warning
  if( m_interactionMode == MantidGLWidget::PickMode )
  {
    //mPickBox->mousePressed(Qt::RightButton, QCursor::pos());
    //mPickBox->mouseReleased(Qt::RightButton, QCursor::pos());
    //std::set<QRgb> result=mPickBox->getListOfColorsPicked();
    //if(!result.empty())
    //{
    //  emit actorsPicked(result);
    //}

  }
}

/**
 * This is mouse move callback method. It implements the actions to be taken when the mouse is
 * moved with a particular button is pressed.
 * Left Button: Zoom
 * Right Button: Rotate
 * Middle Button: Translate
 * Key + Left Button: Pick (TODO: Yet to implement)
 * @param event :: This is the event variable which has the position and button states
 */
void MantidGLWidget::mouseMoveEvent(QMouseEvent* event)
{

}

/**
 * This is mouse button release callback method. This resets the cursor to pointing hand cursor
 * @param event :: This is the event variable which has the position and button states
 */
void MantidGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
}

/**
 * Mouse wheel event to set the zooming in and out
 * @param event :: This is the event variable which has the status of the wheel
 */
void MantidGLWidget::wheelEvent(QWheelEvent* event)
{
  OpenGLError::check("MantidGLWidget::wheelEvent");
}

/**
 * This method is to handle keyboard events to mimic the mouse operations of click and move
 * @param event :: This is the event variable which has the status of the keyboard
 */
void MantidGLWidget::keyPressEvent(QKeyEvent *event)
{

}

/**
 * This method is to handle keyboard events to mimic the mouse operations of mouse button up.
 * @param event :: This is the event variable which has the status of the keyboard
 */
void MantidGLWidget::keyReleaseEvent(QKeyEvent *event)
{
  releaseKeyboard();
  setCursor(Qt::PointingHandCursor);
  isKeyPressed=false;
  if(!event->isAutoRepeat())
  {
    update();
  }
  OpenGLError::check("MantidGLWidget::keyReleaseEvent");
}


/**
 * This method set the background color.
 */
void MantidGLWidget::setBackgroundColor(QColor input)
{
  makeCurrent();
  m_bgColor = input;
  glClearColor(GLclampf(m_bgColor.red()/255.0),GLclampf(m_bgColor.green()/255.0),GLclampf(m_bgColor.blue()/255.0),1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  OpenGLError::check("MantidGLWidget::setBackgroundColor");
  //m_unwrappedSurfaceChanged = true;
  update();
}

QColor MantidGLWidget::currentBackgroundColor() const
{
  return m_bgColor;
}

/**
 * This saves the GL scene to a file.
 * @param filename :: The name of the file
 */
void MantidGLWidget::saveToFile(const QString & filename)
{
  if( filename.isEmpty() ) return;
  //  QPixmap pm = renderPixmap();
  //pm.save(filename);
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
void MantidGLWidget::resetWidget()
{
  //setActorCollection(boost::shared_ptr<GLActorCollection>(new GLActorCollection()));
}

/**
  * Enables / disables lighting
  * @param on :: Set true to turn lighting on or false to turn it off.
  */
void MantidGLWidget::enableLighting(bool on)
{
  m_lightingState = on? 2 : 0;
  setLightingModel(m_lightingState);
  //if (m_unwrappedSurface)
  //{
  //  m_unwrappedSurface->updateView();
  //}
  update();
}

/**
  * Set wireframe view on or off
  * @param on :: If true set wireframe, otherwise it's SOLID
  */
void MantidGLWidget::setWireframe(bool on)
{
  m_polygonMode = on ? WIREFRAME : SOLID;
  update();
}

void MantidGLWidget::setRenderMode(int mode)
{
  makeCurrent();
  if (mode < RENDERMODE_SIZE)
  {
    m_renderMode = RenderMode(mode);
    resetUnwrappedViews();
  }
  //if (mode == FULL3D)
  //{
  //  _viewport->issueGL();
  //}
  update();
}

void MantidGLWidget::resetUnwrappedViews()
{
  if (m_unwrappedSurface)
  {
    delete m_unwrappedSurface;
    m_unwrappedSurface = NULL;
  }
  m_unwrappedSurfaceChanged = true;
}

void MantidGLWidget::draw()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  //if (m_unwrappedSurfaceChanged)
  //{
  //  GLActor* actor = scene->getActor(0);
  //  if (!actor) return;
  //  InstrumentActor* instrActor = dynamic_cast<InstrumentActor*>(actor);
  //  if (!instrActor) return;
  //  boost::shared_ptr<Mantid::Geometry::IInstrument> instr = instrActor->getInstrument();
  //  Mantid::Geometry::IObjComponent_sptr sample = instr->getSample();
  //  Mantid::Geometry::V3D sample_pos = sample->getPos();

  //  QTime time;
  //  time.start();
  //  if (m_unwrappedSurface) delete m_unwrappedSurface;
  //  Mantid::Geometry::V3D axis;
  //  if (m_renderMode == SPHERICAL_Y || m_renderMode == CYLINDRICAL_Y)
  //  {
  //    axis = Mantid::Geometry::V3D(0,1,0);
  //  }
  //  else if (m_renderMode == SPHERICAL_Z || m_renderMode == CYLINDRICAL_Z)
  //  {
  //    axis = Mantid::Geometry::V3D(0,0,1);
  //  }
  //  else // SPHERICAL_X || CYLINDRICAL_X
  //  {
  //    axis = Mantid::Geometry::V3D(1,0,0);
  //  }

  //  if (m_renderMode <= CYLINDRICAL_X)
  //  {
  //    m_unwrappedSurface = new UnwrappedCylinder(instrActor,sample_pos,axis);
  //  }
  //  else // SPHERICAL
  //  {
  //    m_unwrappedSurface = new UnwrappedSphere(instrActor,sample_pos,axis);
  //  }

  //  m_unwrappedSurfaceChanged = false;
  //}

  //m_unwrappedSurface->draw(this);

  QApplication::restoreOverrideCursor();
  OpenGLError::check("MantidGLWidget::drawUnwrapped()");
}

//void MantidGLWidget::componentSelected(Mantid::Geometry::ComponentID id)
//{
//  if (m_unwrappedSurface)
//  {
//    m_unwrappedSurface->componentSelected(id);
//    update();
//  }
//}

void MantidGLWidget::refreshView()
{
  if( m_interactionMode == PickMode) //This is when in picking mode and the window is resized so update the image
  {
    //mPickingDraw=true;
  }
  m_unwrappedViewChanged = true;
  update();
}
