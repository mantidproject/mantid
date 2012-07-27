 #ifdef WIN32
#include <windows.h>
#endif
#include "Projection3D.h"
#include "GLActor.h"
#include "GLColor.h"

#include "UnwrappedCylinder.h"
#include "UnwrappedSphere.h"
#include "OpenGLError.h"
#include "DetSelector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <QtOpenGL>
#include <QSpinBox>
#include <QApplication>
#include <QTime>

#include <map>
#include <string>
#include <iostream>
#include <cfloat>
#include <algorithm>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

Projection3D::Projection3D(const InstrumentActor* rootActor,int winWidth,int winHeight)
  :ProjectionSurface(rootActor,Mantid::Kernel::V3D(),Mantid::Kernel::V3D(0,0,1)),
  m_viewport(new GLViewport),
  m_drawAxes(true),
  m_wireframe(false),
  m_isKeyPressed(false),
  m_isLightingOn(false)
{

  Instrument_const_sptr instr = rootActor->getInstrument();

  m_viewport->resize(winWidth,winHeight);
  V3D minBounds,maxBounds;
  m_instrActor->getBoundingBox(minBounds,maxBounds);

  double radius = minBounds.norm();
  double tmp = maxBounds.norm();
  if (tmp > radius) radius = tmp;

  m_viewport->setOrtho(minBounds.X(),maxBounds.X(),
                       minBounds.Y(),maxBounds.Y(),
                       -radius,radius);
  m_trackball = new GLTrackball(m_viewport);
  changeColorMap();
  rootActor->invalidateDisplayLists();
}

Projection3D::~Projection3D()
{
  delete m_trackball;
  delete m_viewport;
}

void Projection3D::init()
{
}

void Projection3D::resize(int w, int h)
{
  if (m_viewport)
  {
    m_viewport->resize(w,h);
    m_viewport->issueGL();
  }
}

void Projection3D::drawSurface(MantidGLWidget*,bool picking)const
{
  OpenGLError::check("GL3DWidget::draw3D()[begin]");

  glEnable(GL_DEPTH_TEST);

  if (m_wireframe)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }
  else
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  setLightingModel(picking);

  m_viewport->issueGL();

  // fill the buffer with background colour
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // if actor is undefined leave it with clear screen
  if ( !m_instrActor ) return;

  // Reset the rendering options just in case
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Issue the rotation, translation and zooming of the trackball to the object
  m_trackball->IssueRotation();
  
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if (m_viewChanged)
  {
    m_viewChanged = false;
  }
  m_instrActor->draw(picking);
  OpenGLError::check("GL3DWidget::draw3D()[scene draw] ");

  //Also some axes
  if (m_drawAxes && !picking)
  {
    //This draws a point at the origin, I guess
    glPointSize(3.0);
    glBegin(GL_POINTS);
    glVertex3d(0.0,0.0,0.0);
    glEnd();

    drawAxes();
  }

  QApplication::restoreOverrideCursor();

  OpenGLError::check("GL3DWidget::draw3D()");
}

/** Draw 3D axes centered at the origin (if the option is selected)
 *
 */
void Projection3D::drawAxes(double axis_length)const
{
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

void Projection3D::mousePressEventMove(QMouseEvent* event)
{
  if (event->buttons() & Qt::MidButton)
  {
    m_trackball->initZoomFrom(event->x(),event->y());
    m_isKeyPressed=true;
    //setSceneLowResolution();
  }
  else if (event->buttons() & Qt::LeftButton)
  {
    m_trackball->initRotationFrom(event->x(),event->y());
    m_isKeyPressed=true;
    //setSceneLowResolution();
  }
  else if(event->buttons() & Qt::RightButton)
  {
    m_trackball->initTranslateFrom(event->x(),event->y());
    m_isKeyPressed=true;
    //setSceneLowResolution();
  }
  OpenGLError::check("GL3DWidget::mousePressEvent");
}

void Projection3D::mouseMoveEventMove(QMouseEvent* event)
{
    m_viewChanged = false;
    if (event->buttons() & Qt::LeftButton)
    {
      m_trackball->generateRotationTo(event->x(),event->y());
      m_trackball->initRotationFrom(event->x(),event->y());
      m_viewChanged = true;
    }
    else if(event->buttons() & Qt::RightButton)
    { //Translate
      m_trackball->generateTranslationTo(event->x(),event->y());
      m_trackball->initTranslateFrom(event->x(),event->y());
      m_viewChanged = true;
    }
    else if(event->buttons() & Qt::MidButton)
    { //Zoom
      m_trackball->generateZoomTo(event->x(),event->y());
      m_trackball->initZoomFrom(event->x(),event->y());
      m_viewChanged = true;
    }
    OpenGLError::check("GL3DWidget::mouseMoveEvent");
}

void Projection3D::mouseReleaseEventMove(QMouseEvent*)
{
  m_isKeyPressed=false;
  m_viewChanged = true;
}

void Projection3D::wheelEventMove(QWheelEvent* event)
{
  m_trackball->initZoomFrom(event->x(),event->y());
  m_trackball->generateZoomTo(event->x(),event->y()-event->delta());
  m_viewChanged = true;
}

void Projection3D::changeColorMap()
{
}

void Projection3D::setViewDirection(const QString& input)
{
	if(input.toUpper().compare("X+")==0)
	{
		m_trackball->setViewToXPositive();
	}
	else if(input.toUpper().compare("X-")==0)
	{
		m_trackball->setViewToXNegative();
	}
	else if(input.toUpper().compare("Y+")==0)
	{
		m_trackball->setViewToYPositive();
	}
	else if(input.toUpper().compare("Y-")==0)
	{
		m_trackball->setViewToYNegative();
	}
	else if(input.toUpper().compare("Z+")==0)
	{
		m_trackball->setViewToZPositive();
	}
	else if(input.toUpper().compare("Z-")==0)
	{
		m_trackball->setViewToZNegative();
	}
  updateView();
}

void Projection3D::set3DAxesState(bool on)
{
  m_drawAxes = on;
}

void Projection3D::setWireframe(bool on)
{
  m_wireframe = on;
}

//-----------------------------------------------------------------------------
/** This seems to be called when the user has selected a rectangle
 * using the mouse.
 *
 * @param dets :: returns a list of detector IDs selected.
 */
void Projection3D::getSelectedDetectors(QList<int>& dets)
{
  dets.clear();
  if (!hasSelection()) return;
  double xmin,xmax,ymin,ymax,zmin,zmax;
  m_viewport->getInstantProjection(xmin,xmax,ymin,ymax,zmin,zmax);
  QRect rect = selectionRect();
  int w,h;
  m_viewport->getViewport(&w,&h);

  double xLeft = xmin + (xmax - xmin) * rect.left() / w;
  double xRight = xmin + (xmax - xmin) * rect.right() / w;
  double yBottom = ymin + (ymax - ymin) * (h - rect.bottom()) / h;
  double yTop = ymin  + (ymax - ymin) * (h - rect.top()) / h;
  //std::cerr 
  //                       << xLeft << ' ' << xRight << '\n'
  //                       << yBottom << ' ' << yTop << '\n'
  //                       << zmin << ' ' << zmax << "\n\n";
  size_t ndet = m_instrActor->ndetectors();
  Quat rot = m_trackball->getRotation();

  // Cache all the detector positions if needed. This is slow, but just once.
  m_instrActor->cacheDetPos();

  for(size_t i = 0; i < ndet; ++i)
  {
    detid_t detId = m_instrActor->getDetID(i);
    V3D pos = m_instrActor->getDetPos(i);
    rot.rotate(pos);
    if (pos.X() >= xLeft && pos.X() <= xRight &&
        pos.Y() >= yBottom && pos.Y() <= yTop)
    {
      dets.push_back(detId);
    }
  }
}

//-----------------------------------------------------------------------------
/** Select detectors to mask, using the mouse.
 * From the Instrument Window's mask tab.
 *
 * @param dets :: returns a list of detector IDs to mask.
 */
void Projection3D::getMaskedDetectors(QList<int>& dets)const
{
  Quat rot = m_trackball->getRotation();

  // Cache all the detector positions if needed. This is slow, but just once.
  m_instrActor->cacheDetPos();

  // find the layer of visible detectors
  QList<QPoint> pixels;
  m_maskShapes.getMaskedPixels(pixels);
  double zmin = 1.0;
  double zmax = 0.0;
  QSet<int> ids;
  foreach(const QPoint& p,pixels)
  {
    int id = getDetectorID(p.x(),p.y());
    if (ids.contains(id)) continue;
    ids.insert(id);
    V3D pos = this->getDetectorPos(p.x(), p.y());
    rot.rotate(pos);
    double z = pos.Z();
    if (zmin > zmax)
    {
      zmin = zmax = z;
    }
    else
    {
      if (zmin > z) zmin = z;
      if (zmax < z) zmax = z;
    }

  }

  // find masked detector in that layer
  dets.clear();
  if (m_maskShapes.isEmpty()) return;
  size_t ndet = m_instrActor->ndetectors();
  for(size_t i = 0; i < ndet; ++i)
  {
    // Find the cached ID and position. This is much faster than getting the detector.
    V3D pos = m_instrActor->getDetPos(i);
    detid_t id = m_instrActor->getDetID(i);
    rot.rotate(pos);
    if (pos.Z() < zmin || pos.Z() > zmax) continue;
    if (m_maskShapes.isMasked(pos.X(),pos.Y()))
    {
      dets.push_back(int(id));
    }
  }
}

void Projection3D::componentSelected(Mantid::Geometry::ComponentID id)
{

  Instrument_const_sptr instr = m_instrActor->getInstrument();

  if (id == NULL || id == instr->getComponentID())
  {
    V3D minBounds,maxBounds;
    m_instrActor->getBoundingBox(minBounds,maxBounds);

    double radius = minBounds.norm();
    double tmp = maxBounds.norm();
    if (tmp > radius) radius = tmp;

    m_viewport->setOrtho(minBounds.X(),maxBounds.X(),
      minBounds.Y(),maxBounds.Y(),
      -radius,radius);
    return;
  }

  IComponent_const_sptr comp = instr->getComponentByID(id);
  V3D pos = comp->getPos();

  V3D compDir = comp->getPos() - instr->getSample()->getPos();
  compDir.normalize();
  V3D up(0,0,1);
  V3D x = up.cross_prod(compDir);
  up = compDir.cross_prod(x);
  Quat rot;
  InstrumentActor::BasisRotation(x,up,compDir,V3D(-1,0,0),V3D(0,1,0),V3D(0,0,-1),rot);

  BoundingBox bbox;
  if (comp->getComponentID() == instr->getSample()->getComponentID())
  {
    bbox = m_instrActor->getWorkspace()->sample().getShape().getBoundingBox();
    bbox.moveBy(comp->getPos());
  }
  else
  {
    comp->getBoundingBox(bbox);
  }
  V3D minBounds = bbox.minPoint() + pos;
  V3D maxBounds = bbox.maxPoint() + pos;
  rot.rotate(minBounds);
  rot.rotate(maxBounds);

  m_viewport->setOrtho(minBounds.X(),maxBounds.X(),
                       minBounds.Y(),maxBounds.Y(),
                       -1000.,1000);


  m_trackball->reset();
  m_trackball->setRotation(rot);
  //m_trackball->setModelCenter(comp->getPos());

}

QString Projection3D::getInfoText()const
{
  if (m_interactionMode == PickMode)
  {
    return getPickInfoText();
  }
  QString text = "Mouse Buttons: Left -- Rotation, Middle -- Zoom, Right -- Translate";
  if( m_drawAxes )
  {
    text += "\nAxes: X = Red; Y = Green; Z = Blue";
  }
  return text;
}

QRectF Projection3D::getSurfaceBounds()const
{
  double xmin,xmax,ymin,ymax,zmin,zmax;
  m_viewport->getInstantProjection(xmin,xmax,ymin,ymax,zmin,zmax);
  return QRectF(QPointF(xmin,ymin),QPointF(xmax,ymax));
}

/**
 * Enable or disable lighting in non-picking mode
 * @param on :: True for enabling, false for disabling.
 */
void Projection3D::enableLighting(bool on)
{
  m_isLightingOn = on;
}

/**
 * Define lighting of the scene
 */
void Projection3D::setLightingModel(bool picking) const
{
  // Basic lighting
  if ( m_isLightingOn && !picking )
  {
    glShadeModel(GL_SMOOTH);           // Shade model is smooth (expensive but looks pleasing)
    glEnable(GL_LIGHT0);               // Enable opengl first light
    glEnable(GL_LINE_SMOOTH);          // Set line should be drawn smoothly
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);  // This model lits both sides of the triangle
    // Set Light0 Attributes, Ambient, diffuse,specular and position
    // Its a directional light which follows camera position
    float lamp_ambient[4]={0.30f, 0.30f, 0.30f, 1.0f};
    float lamp_diffuse[4]={1.0f, 1.0f, 1.0f, 1.0f};
    float lamp_specular[4]={1.0f,1.0f,1.0f,1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT,lamp_ambient );
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lamp_specular);
    float lamp_pos[4]={0.0f, 0.0f, 0.0f, 1.0f}; // spot light at the origin
    glLightfv(GL_LIGHT0, GL_POSITION, lamp_pos);
    glEnable (GL_LIGHTING);            // Enable light
  }
  else
  {
    glShadeModel(GL_FLAT);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LINE_SMOOTH);
  }
}

