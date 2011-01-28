#ifdef WIN32
#include <windows.h>
#endif
#include "GL3DWidget.h"
#include "GLActor.h"
#include "GLColor.h"
#include "InstrumentActor.h"
#include "UnwrappedCylinder.h"
#include "UnwrappedSphere.h"
#include "OpenGLError.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Objects/Object.h"

#include "boost/shared_ptr.hpp"

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


GL3DWidget::GL3DWidget(QWidget* parent):
  QGLWidget(QGLFormat(QGL::DepthBuffer|QGL::NoAlphaChannel|QGL::SampleBuffers),parent)
{

  if (!this->format().depth())
    std::cout << "Warning! OpenGL Depth buffer could not be initialized.\n";
  //std::cout << "Depth buffer size is " << this->format().depthBufferSize() << "\n";

  makeCurrent();
  _viewport=new GLViewport;
  _trackball=new GLTrackball(_viewport);
  isKeyPressed=false;
  scene=boost::shared_ptr<GLActorCollection>(new GLActorCollection());
  mPickedActor=NULL;
  mPickingDraw=true;
  iInteractionMode=GL3DWidget::MoveMode;
  mPickBox=new GLGroupPickBox();
  setFocusPolicy(Qt::StrongFocus);
  setAutoFillBackground(false);
  bgColor=QColor(0,0,0,1);
  m3DAxesShown = 1;
  m_polygonMode = SOLID;
  m_lightingState = 0;
  m_firstFrame = true;
  m_renderMode = FULL3D;
  m_unwrappedSurface = NULL;
  m_unwrappedSurfaceChanged = true;
  m_unwrappedViewChanged = true;

  //Enable right-click in pick mode
  setContextMenuPolicy(Qt::DefaultContextMenu);

}

GL3DWidget::~GL3DWidget()
{
  delete _viewport;
  delete _trackball;
  if (m_unwrappedSurface)
  {
    delete m_unwrappedSurface;
  }
}

void GL3DWidget::setInteractionModePick()
{
  iInteractionMode = GL3DWidget::PickMode;// Pick mode
  setMouseTracking(true);
  mPickingDraw = true;
  update();
}

void GL3DWidget::setInteractionModeNormal()
{
  iInteractionMode = GL3DWidget::MoveMode;//Normal mode
  setMouseTracking(false);
  setCursor(Qt::PointingHandCursor);
  glEnable(GL_NORMALIZE);
  if (m_lightingState > 0)
  {
    glEnable(GL_LIGHTING);
  }
  update();
}

GLActor* GL3DWidget::getPickedActor()
{
  return mPickedActor;
}

/**
 * This method initializes the opengl settings. its invoked defaultly by Qt when the widget
 * is initialized.
 */
void GL3DWidget::initializeGL()
{
  setCursor(Qt::PointingHandCursor); // This is to set the initial window mouse cursor to Hand icon
  
  // Set the relevant OpenGL rendering options
  setRenderingOptions();
  
  // Clear the memory buffers
  glClearColor(bgColor.red()/255.0,bgColor.green()/255.0,bgColor.blue()/255.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GL3DWidget::setRenderingOptions()
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

}

/**
 * Toggles the use of high resolution lighting
 * @param state :: An integer indicating lighting state. (Note that this is not a boolean because Qt's CheckBox emits an integer signal)
 * Unchecked = 0, ,PartiallyChecked = 1, Checked = 2
 */
void GL3DWidget::setLightingModel(int state)
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

/** Draw 3D axes centered at the origin (if the option is selected)
 *
 */
void GL3DWidget::drawAxes(double axis_length)
{
  //Don't do anything if the checkbox is unchecked.
  if (!m3DAxesShown)
    return;

  glPointSize(3.0);
  glLineWidth(3.0);

  //To make sure the lines are colored
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_TEXTURE_2D);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  glColor3f(1.0, 0., 0.);
  glBegin(GL_LINES);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(axis_length, 0.0,0.0);
  glEnd();

  glColor3f(0., 1.0, 0.);
  glBegin(GL_LINES);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(0.0, axis_length, 0.0);
  glEnd();

  glColor3f(0., 0., 1.);
  glBegin(GL_LINES);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(0.0, 0.0, axis_length);
  glEnd();
}

/**
 * This method draws the scene onto the graphics context
 */
void GL3DWidget::drawDisplayScene()
{
  if (m_renderMode == FULL3D)
  {
    draw3D();
  }
  else
  {
    drawUnwrapped();
  }
}
/**
  * This method draws the scene onto the graphics context
  */
void GL3DWidget::draw3D()
{
  glGetError();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  // Clear the background to the bg color set previously.
  glClearColor(bgColor.red()/255.0,bgColor.green()/255.0,bgColor.blue()/255.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  OpenGLError::check("GL3DWidget::draw3D()[clear] ");

  // Issue the rotation, translation and zooming of the trackball to the object
  _trackball->IssueRotation();
  
  if (m_polygonMode == SOLID)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  else
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  glPushMatrix();
  if(isKeyPressed)
  {
    if (m_lightingState > 0)
    {
      setLightingModel(m_lightingState);
    }
    scene->draw();
    setLightingModel(0);
    this->drawAxes();
    OpenGLError::check("GL3DWidget::draw3D()[scene draw 1] ");
  }
  else
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    //Set the lighting
    if (m_lightingState > 0)
      setLightingModel(m_lightingState);
    else
      setLightingModel(0);

    scene->draw();
    OpenGLError::check("GL3DWidget::draw3D()[scene draw] ");

    //This draws a point at the origin, I guess
    glPointSize(3.0);
    glBegin(GL_POINTS);
    glVertex3d(0.0,0.0,0.0);
    glEnd();

    //Also some axes
    setLightingModel(0);
    this->drawAxes();


    QApplication::restoreOverrideCursor();


  }
  glPopMatrix();
  OpenGLError::check("GL3DWidget::draw3D()");
  QPainter painter(this);
  painter.end();
}

/**
 * This method draws the scenc when in pick mode i.e with reference colors to actors
 */
void GL3DWidget::drawPickingScene()
{
  makeCurrent();
  glGetError();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glClearColor(0.0,0.0,0.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // Issue the rotation, translation and zooming of the trackball to the object
  _trackball->IssueRotation();
  glPushMatrix();
  drawSceneUsingColorID();
  glPopMatrix();
}

/**
 * Switches to picking mode does all the operations to create and set the images to GLGroupPickBox
 */
void GL3DWidget::switchToPickingMode()
{
  if (m_renderMode == FULL3D)
  {
    //First we draw the regular scene and save it to display
    drawDisplayScene();
    glReadBuffer(GL_FRONT);
    mPickBox->setDisplayImage(grabFrameBuffer(false));

    // Now we draw the picking scene with the special colors

    glDisable(GL_MULTISAMPLE);  //This will disable antialiasing which is build in by default for samplebuffers
    glDisable(GL_NORMALIZE);
    drawPickingScene();
    glReadBuffer(GL_BACK);
    mPickBox->setPickImage(grabFrameBuffer(false));
    glEnable(GL_MULTISAMPLE);   //enable antialiasing
    mPickingDraw=false;
    OpenGLError::check("GL3DWidget::switchToPickingMode() ");
  }
  else
  {
    drawUnwrapped();
  }
}


/**
 * This is overridden function which is called by Qt when the widget needs to be repainted.
 */
void GL3DWidget::paintEvent(QPaintEvent *event)
{
  (void) event; //avoid compiler warning
  makeCurrent();
  if(iInteractionMode == GL3DWidget::PickMode)
  {
    if (m_renderMode == FULL3D)
    {
      if(mPickingDraw==true)
      {
        switchToPickingMode();
      }
      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);
      mPickBox->draw(&painter);
      painter.end();
    }
    else
    {
      drawUnwrapped();
    }
  }
  else
  {
    drawDisplayScene();
  }


  if (m_firstFrame)
  {
    update();
    m_firstFrame = false;
  }
}

/**
 * This function is also overridden from the parent. This method is invoked when the widget is resized
 * This method resizes the viewport according to the new widget width and height
 */
void GL3DWidget::resizeGL(int width, int height)
{
  makeCurrent();
  _viewport->resize(width,height);
  _viewport->issueGL();
  
  if( iInteractionMode == GL3DWidget::PickMode) //This is when in picking mode and the window is resized so update the image
  {
    mPickingDraw=true;
  }
  m_unwrappedViewChanged = true;

  OpenGLError::check("GL3DWidget::resizeGL");
}

/**
 * Mouse press callback method, It implements mouse button press initialize methods.
 * Left Button: Zoom
 * Right Button: Rotate
 * Middle Button: Translate
 * Key + Left Button: Pick (TODO: Yet to implement)
 * @param event :: This is the event variable which has the position and button states
 */
void GL3DWidget::mousePressEvent(QMouseEvent* event)
{
  if (m_renderMode != FULL3D && m_unwrappedSurface)
  {
    if(event->buttons() & Qt::RightButton)
    {
      if (getInteractionMode() == MoveMode)
      {
        m_unwrappedSurface->unzoom();
      }
    }
    else
    {
      m_unwrappedSurface->startSelection(event->x(),event->y());
    }
    update();
    OpenGLError::check("GL3DWidget::mousePressEvent");
    return;
  }

  // Pick Mode
  if( iInteractionMode == GL3DWidget::PickMode && (event->buttons() & Qt::LeftButton) )
  { 
    setCursor(Qt::CrossCursor);
    QRgb tmpColor = mPickBox->pickPoint(event->x(), event->y());
    emit actorHighlighted(tmpColor);
    mPickBox->mousePressed(event->buttons(), event->pos());
    return;
  } //end of pick mode and start of normal mode
  
  if (event->buttons() & Qt::MidButton)
  {
    setCursor(Qt::SizeVerCursor);
    _trackball->initZoomFrom(event->x(),event->y());
    isKeyPressed=true;
    setSceneLowResolution();
  }
  else if (event->buttons() & Qt::LeftButton)
  {
    setCursor(Qt::OpenHandCursor);
    _trackball->initRotationFrom(event->x(),event->y());
    isKeyPressed=true;
    setSceneLowResolution();
  }
  else if(event->buttons() & Qt::RightButton)
  {
    setCursor(Qt::CrossCursor);
    _trackball->initTranslateFrom(event->x(),event->y());
    isKeyPressed=true;
    setSceneLowResolution();
  }
  OpenGLError::check("GL3DWidget::mousePressEvent");
}

/**
 * Called when a context menu event is recieved
 */
void GL3DWidget::contextMenuEvent(QContextMenuEvent * event)
{
  (void) event; //avoid compiler warning
  if( iInteractionMode == GL3DWidget::PickMode )
  {
    mPickBox->mousePressed(Qt::RightButton, QCursor::pos());
    mPickBox->mouseReleased(Qt::RightButton, QCursor::pos());
    std::set<QRgb> result=mPickBox->getListOfColorsPicked();
    if(!result.empty())
    {
      emit actorsPicked(result);
    }

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
void GL3DWidget::mouseMoveEvent(QMouseEvent* event)
{
  makeCurrent();
  if (m_renderMode != FULL3D && m_unwrappedSurface)
  {
    if (event->buttons() & Qt::LeftButton)
    {
      m_unwrappedSurface->moveSelection(event->x(),event->y());
      update();
    }
    else
    {
      int detId = m_unwrappedSurface->getDetectorID(event->x(),event->y());
      emit actorHighlighted(detId);
    }
    OpenGLError::check("GL3DWidget::mouseMoveEvent");
    return;
  }

  if(iInteractionMode == GL3DWidget::PickMode)
  {
    setCursor(Qt::CrossCursor);
    QRgb tmpColor = mPickBox->pickPoint(event->x(), event->y());
    if (event->buttons() & Qt::LeftButton)
    {
      emit increaseSelection(tmpColor);
      mPickBox->mouseMoveEvent(event);
      update();
    }
    else
    {
      emit actorHighlighted(tmpColor);
    }
  }
  else
  {
    if (event->buttons() & Qt::LeftButton)
    {
      setCursor(Qt::ClosedHandCursor);
      _trackball->generateRotationTo(event->x(),event->y());
      update();
      _trackball->initRotationFrom(event->x(),event->y());
    }
    else if(event->buttons() & Qt::RightButton)
    { //Translate
      setCursor(Qt::CrossCursor);
      _trackball->generateTranslationTo(event->x(),event->y());
      update();
      _trackball->initTranslateFrom(event->x(),event->y());
    }
    else if(event->buttons() & Qt::MidButton)
    { //Zoom
      setCursor(Qt::SizeVerCursor);
      _trackball->generateZoomTo(event->x(),event->y());
      update();
      _trackball->initZoomFrom(event->x(),event->y());
    }
  }
  OpenGLError::check("GL3DWidget::mouseMoveEvent");
}

/**
 * This is mouse button release callback method. This resets the cursor to pointing hand cursor
 * @param event :: This is the event variable which has the position and button states
 */
void GL3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (m_renderMode != FULL3D && m_unwrappedSurface)
  {
    if (getInteractionMode() == PickMode && m_unwrappedSurface->hasSelection())
    {
      showUnwrappedContextMenu();
    }
    m_unwrappedSurface->endSelection(event->x(),event->y());
    update();
    return;
  }

  setCursor(Qt::PointingHandCursor);
  isKeyPressed=false;
  setSceneHighResolution();
  if(iInteractionMode == GL3DWidget::PickMode)
  {
    mPickBox->mouseReleased(event->buttons(), event->pos());
    std::set<QRgb> result=mPickBox->getListOfColorsPicked();
    if(!result.empty())
    {
      emit actorsPicked(result);
    }
  }
  update();
}

/**
 * Mouse wheel event to set the zooming in and out
 * @param event :: This is the event variable which has the status of the wheel
 */
void GL3DWidget::wheelEvent(QWheelEvent* event)
{
  makeCurrent();
  setCursor(Qt::SizeVerCursor);
  _trackball->initZoomFrom(event->x(),event->y());
  _trackball->generateZoomTo(event->x(),event->y()-event->delta());
  update();
  setCursor(Qt::PointingHandCursor);
  OpenGLError::check("GL3DWidget::wheelEvent");
}

/**
 * This method is to handle keyboard events to mimic the mouse operations of click and move
 * @param event :: This is the event variable which has the status of the keyboard
 */
void GL3DWidget::keyPressEvent(QKeyEvent *event)
{
  makeCurrent();
  grabKeyboard();
  // Ignore keyboard event when in pick mode
  if( iInteractionMode== GL3DWidget::PickMode) return; 
  int width,height;
  _viewport->getViewport(&width,&height);
  int halfwidth=width/2;
  int halfheight=height/2;
  switch(event->key())
    {
      //-----------------------Translation-----------------
    case Qt::Key_Left:
      isKeyPressed=true;
      setCursor(Qt::CrossCursor);
      _trackball->initTranslateFrom(1,0);
      _trackball->generateTranslationTo(0,0);
      update();
      break;
    case Qt::Key_Right:
      isKeyPressed=true;
      setCursor(Qt::CrossCursor);
      _trackball->initTranslateFrom(0,0);
      _trackball->generateTranslationTo(1,0);
      update();
      break;
    case Qt::Key_Up:
      isKeyPressed=true;
      setCursor(Qt::CrossCursor);
      _trackball->initTranslateFrom(0,1);
      _trackball->generateTranslationTo(0,0);
      update();
      break;
    case Qt::Key_Down:
      isKeyPressed=true;
      setCursor(Qt::CrossCursor);
      _trackball->initTranslateFrom(0,0);
      _trackball->generateTranslationTo(0,1);
      update();
      break;
      //--------------------End of Translation---------------
      //--------------------Rotation-------------------------
    case Qt::Key_1:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth-1,halfheight+1);
      update();
      break;
    case Qt::Key_2:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth,halfheight+1);
      update();
      break;
    case Qt::Key_3:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth+1,halfheight+1);
      update();
      break;
    case Qt::Key_4:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth-1,halfheight);
      update();
      break;
    case Qt::Key_6:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth+1,halfheight);
      update();
      break;
    case Qt::Key_7:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth-1,halfheight-1);
      update();
      break;
    case Qt::Key_8:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth,halfheight-1);
      update();
      break;
    case Qt::Key_9:
      isKeyPressed=true;
      setCursor(Qt::ClosedHandCursor);
      _trackball->initRotationFrom(halfwidth,halfheight);
      _trackball->generateRotationTo(halfwidth+1,halfheight-1);
      update();
      break;
      //---------------------------------End of Rotation--------------
      //---------------------------------Zoom-------------------------
    case Qt::Key_PageUp:
      isKeyPressed=true;
      setCursor(Qt::SizeVerCursor);
      _trackball->initZoomFrom(halfwidth,halfheight);
      _trackball->generateZoomTo(halfwidth,halfheight-1);
      update();
      break;
    case Qt::Key_PageDown:
      isKeyPressed=true;
      setCursor(Qt::SizeVerCursor);
      _trackball->initZoomFrom(halfwidth,halfheight);
      _trackball->generateZoomTo(halfwidth,halfheight+1);
      update();
      break;
    }
  OpenGLError::check("GL3DWidget::keyPressEvent");
}

/**
 * This method is to handle keyboard events to mimic the mouse operations of mouse button up.
 * @param event :: This is the event variable which has the status of the keyboard
 */
void GL3DWidget::keyReleaseEvent(QKeyEvent *event)
{
  releaseKeyboard();
  setCursor(Qt::PointingHandCursor);
  isKeyPressed=false;
  if(!event->isAutoRepeat())
  {
    update();
  }
  OpenGLError::check("GL3DWidget::keyReleaseEvent");
}
/**
 * This method sets the collection of actors that widget needs to display
 * @param col :: input collection of actors
 */
void GL3DWidget::setActorCollection(boost::shared_ptr<GLActorCollection> col)
{
  scene=col;
  update();
  OpenGLError::check("GL3DWidget::setActorCollection");
}

/**
 * This default object initialization method. usually used for testing or added a default
 * object to all the widgets.
 */
void GL3DWidget::MakeObject()
{
}

/**
 * Sets the default view to input provided
 */
void GL3DWidget::setViewDirection(AxisDirection dir)
{
  makeCurrent();
  Mantid::Geometry::V3D minPoint,maxPoint;
  double _bbmin[3],_bbmax[3];
  getBoundingBox(minPoint,maxPoint);
  Mantid::Geometry::V3D centre=(maxPoint+minPoint)/2.0;
  defaultProjection();
  _viewport->getProjection(_bbmin[0],_bbmax[0],_bbmin[1],_bbmax[1],_bbmin[2],_bbmax[2]);
  switch(dir)
  {
  case XPOSITIVE:
    _trackball->setViewToXPositive();
    _trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
    _viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
    break;
  case YPOSITIVE:
    _trackball->setViewToYPositive();
    _trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
    _viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
    break;
  case ZPOSITIVE:
    _trackball->setViewToZPositive();
    _viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
    break;
  case XNEGATIVE:
    _trackball->setViewToXNegative();
    _trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
    _viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
    break;
  case YNEGATIVE:
    _trackball->setViewToYNegative();
    _trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
    _viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
    break;
  case ZNEGATIVE:
    _trackball->setViewToZNegative();
    _trackball->rotateBoundingBox(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minPoint[2],maxPoint[2]);
    _viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],_bbmin[2],_bbmax[2]);
    break;
  }
  OpenGLError::check("GL3DWidget::setViewDirection");
  _viewport->issueGL();
  update();
}

/**
 * This method calculates the default projection
 */
void GL3DWidget::defaultProjection()
{
  makeCurrent();
  // Getting the bounding box of the scene and setting the orthagonal projection
  // such that the orthogonal projection places the object completly in the screen
  // Its a simplified version of placing the object completly in screen with same
  // min and max values in all directions.
  Mantid::Geometry::V3D minPoint,maxPoint;
  getBoundingBox(minPoint,maxPoint);
  if(minPoint[0]==DBL_MAX||minPoint[1]==DBL_MAX||minPoint[2]==DBL_MAX||maxPoint[0]==-DBL_MAX||maxPoint[1]==-DBL_MAX||maxPoint[2]==-DBL_MAX)
  {
    minPoint=Mantid::Geometry::V3D(-1.0,-1.0,-1.0);
    maxPoint=Mantid::Geometry::V3D( 1.0, 1.0, 1.0);
  }
  double minValue,maxValue;
  minValue=minPoint[0];
  if(minValue>minPoint[1])minValue=minPoint[1];
  if(minValue>minPoint[2])minValue=minPoint[2];
  maxValue=maxPoint[0];
  if(maxValue<maxPoint[1])maxValue=maxPoint[1];
  if(maxValue<maxPoint[2])maxValue=maxPoint[2];
  if(minValue>maxValue)
  {
    double tmp;
    tmp=maxValue;
    maxValue=minValue;
    minValue=maxValue;
  }
  double temp=minValue-fabs(maxValue-minValue);
  maxValue=maxValue+fabs(maxValue-minValue);
  minValue=temp;
  
  _viewport->setOrtho(minPoint[0],maxPoint[0],minPoint[1],maxPoint[1],minValue*-1,maxValue*-1);
  OpenGLError::check("GL3DWidget::defaultProjection()");
  Mantid::Geometry::V3D center;
  center=(minPoint+maxPoint)/2.0;
  _viewport->issueGL();
}

/**
 * Sets the flag indicating the visibility of the orientation axes
 *
 */
void GL3DWidget::set3DAxesState(int state)
{
  m3DAxesShown = state;
  // Update the display
  update();
}



/**
 * This method set the background color.
 */
void GL3DWidget::setBackgroundColor(QColor input)
{
  makeCurrent();
  bgColor = input;
  glClearColor(bgColor.red()/255.0,bgColor.green()/255.0,bgColor.blue()/255.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  OpenGLError::check("GL3DWidget::setBackgroundColor");
  update();
}

QColor GL3DWidget::currentBackgroundColor() const
{
  return bgColor;
}

/**
 * This saves the GL scene to a file.
 * @param filename :: The name of the file
 */
void GL3DWidget::saveToFile(const QString & filename)
{
  if( filename.isEmpty() ) return;
  //  QPixmap pm = renderPixmap();
  //pm.save(filename);
  // It seems QGLWidget grabs the back buffer
  this->swapBuffers(); // temporarily swap the buffers
  QImage image = this->grabFrameBuffer();
  this->swapBuffers(); // swap them back
  OpenGLError::check("GL3DWidget::saveToFile");
  image.save(filename);
}

/**
 * Resets the widget for new instrument definition
 */
void GL3DWidget::resetWidget()
{
  setActorCollection(boost::shared_ptr<GLActorCollection>(new GLActorCollection()));
}

/**
  * Enables / disables lighting
  * @param on :: Set true to turn lighting on or false to turn it off.
  */
void GL3DWidget::enableLighting(bool on)
{
  m_lightingState = on? 2 : 0;
  setLightingModel(m_lightingState);
  if (m_unwrappedSurface)
  {
    m_unwrappedSurface->updateView();
  }
  update();
}

/**
  * Set wireframe view on or off
  * @param on :: If true set wireframe, otherwise it's SOLID
  */
void GL3DWidget::setWireframe(bool on)
{
  m_polygonMode = on ? WIREFRAME : SOLID;
  update();
}

void GL3DWidget::setRenderMode(int mode)
{
  makeCurrent();
  if (mode < RENDERMODE_SIZE)
  {
    m_renderMode = RenderMode(mode);
    resetUnwrappedViews();
  }
  if (mode == FULL3D)
  {
    _viewport->issueGL();
  }
  update();
}

void GL3DWidget::resetUnwrappedViews()
{
  if (m_unwrappedSurface)
  {
    delete m_unwrappedSurface;
    m_unwrappedSurface = NULL;
  }
  m_unwrappedSurfaceChanged = true;
}

void GL3DWidget::drawUnwrapped()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  if (m_unwrappedSurfaceChanged)
  {
    GLActor* actor = scene->getActor(0);
    if (!actor) return;
    InstrumentActor* instrActor = dynamic_cast<InstrumentActor*>(actor);
    if (!instrActor) return;
    boost::shared_ptr<Mantid::Geometry::IInstrument> instr = instrActor->getInstrument();
    Mantid::Geometry::IObjComponent_sptr sample = instr->getSample();
    Mantid::Geometry::V3D sample_pos = sample->getPos();

    QTime time;
    time.start();
    if (m_unwrappedSurface) delete m_unwrappedSurface;
    Mantid::Geometry::V3D axis;
    if (m_renderMode == SPHERICAL_Y || m_renderMode == CYLINDRICAL_Y)
    {
      axis = Mantid::Geometry::V3D(0,1,0);
    }
    else if (m_renderMode == SPHERICAL_Z || m_renderMode == CYLINDRICAL_Z)
    {
      axis = Mantid::Geometry::V3D(0,0,1);
    }
    else // SPHERICAL_X || CYLINDRICAL_X
    {
      axis = Mantid::Geometry::V3D(1,0,0);
    }

    if (m_renderMode <= CYLINDRICAL_X)
    {
      m_unwrappedSurface = new UnwrappedCylinder(instrActor,sample_pos,axis);
    }
    else // SPHERICAL
    {
      m_unwrappedSurface = new UnwrappedSphere(instrActor,sample_pos,axis);
    }

    m_unwrappedSurfaceChanged = false;
  }

  m_unwrappedSurface->draw(this);

  QApplication::restoreOverrideCursor();
  OpenGLError::check("GL3DWidget::drawUnwrapped()");
}

void GL3DWidget::redrawUnwrapped()
{
  if (m_unwrappedSurface)
  {
    m_unwrappedSurface->updateDetectors();
  }
}

void GL3DWidget::componentSelected(Mantid::Geometry::ComponentID id)
{
  if (m_unwrappedSurface)
  {
    m_unwrappedSurface->componentSelected(id);
    update();
  }
}

