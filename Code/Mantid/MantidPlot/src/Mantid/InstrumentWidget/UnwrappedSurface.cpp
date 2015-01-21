#include "UnwrappedSurface.h"
#include "GLColor.h"
#include "MantidGLWidget.h"
#include "OpenGLError.h"
#include "PeakMarker2D.h"
#include "InputController.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Instrument.h"

#include <QRgb>
#include <QSet>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <QMessageBox>
#include <QTransform>

#include <cfloat>
#include <limits>
#include <cmath>
#include "MantidKernel/Exception.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::Exception::NotFoundError;

UnwrappedDetector::UnwrappedDetector():
u(0), v(0), width(0), height(0), uscale(0), vscale(0), detector()
{
  color[0] = 0;
  color[1] = 0;
  color[2] = 0;
}

UnwrappedDetector::UnwrappedDetector(const unsigned char* c,
                     boost::shared_ptr<const IDetector> det
                     ):
u(0), v(0), width(0), height(0), uscale(0), vscale(0), detector(det)
{
  color[0] = *c;
  color[1] = *(c+1);
  color[2] = *(c+2);
}

/** Copy constructor */
UnwrappedDetector::UnwrappedDetector(const UnwrappedDetector & other)
{
  this->operator =(other);
}

/** Assignment operator */
UnwrappedDetector & UnwrappedDetector::operator=(const UnwrappedDetector & other)
{
  u = other.u;
  v = other.v;
  width = other.width;
  height = other.height;
  uscale = other.uscale;
  vscale = other.vscale;
  detector = other.detector;
  color[0] = other.color[0];
  color[1] = other.color[1];
  color[2] = other.color[2];
  return *this;
}

/**
 * Constructor.
 * @param rootActor :: The instrument actor.
 */
UnwrappedSurface::UnwrappedSurface(const InstrumentActor* rootActor):
    ProjectionSurface(rootActor),
    m_u_min(DBL_MAX),
    m_u_max(-DBL_MAX),
    m_v_min(DBL_MAX),
    m_v_max(-DBL_MAX),
    m_height_max(0),
    m_width_max(0),
    m_flippedView(false),
    m_startPeakShapes(false)
{
    // create and set the move input controller
    InputControllerMoveUnwrapped* moveController = new InputControllerMoveUnwrapped(this);
    setInputController(MoveMode,moveController);
    connect(moveController,SIGNAL(setSelectionRect(QRect)),this,SLOT(setSelectionRect(QRect)));
    connect(moveController,SIGNAL(zoom()),this,SLOT(zoom()));
    connect(moveController,SIGNAL(unzoom()),this,SLOT(unzoom()));
}

/**
  * Get information about the dimensions of the surface.
  */
QString UnwrappedSurface::getDimInfo() const
{
    return QString("U: [%1, %2] V: [%3, %4]").arg(m_viewRect.x0()).arg(m_viewRect.x1()).arg(m_viewRect.y0()).arg(m_viewRect.y1());
}


//------------------------------------------------------------------------------
/** Calculate the rectangular region in uv coordinates occupied by an assembly.
 *
 * @param comp :: A member of the assembly. The total area of the assembly is a sum of areas of its members
 * @param compRect :: A rect. area occupied by comp in uv space
 */
void UnwrappedSurface::calcAssemblies(const Mantid::Geometry::IComponent * comp,const QRectF& compRect)
{
  // We don't need the parametrized version = use the bare parent for speed
  const Mantid::Geometry::IComponent * parent = comp->getBareParent();
  if (parent)
  {
    QRectF& r = m_assemblies[parent->getComponentID()];
    r |= compRect;
    calcAssemblies(parent,r);
  }
}


//------------------------------------------------------------------------------
/** If needed, recalculate the cached bounding rectangles of all assemblies. */
void UnwrappedSurface::cacheAllAssemblies()
{
  if (!m_assemblies.empty())
    return;

  for(size_t i=0;i<m_unwrappedDetectors.size();++i)
  {
    const UnwrappedDetector& udet = m_unwrappedDetectors[i];

    if (! udet.detector ) continue;
    // Get the BARE parent (not parametrized) to speed things up.
    const Mantid::Geometry::IComponent * bareDet = udet.detector->getComponentID();
    const Mantid::Geometry::IComponent * parent = bareDet->getBareParent();
    if (parent)
    {
      QRectF detRect;
      detRect.setLeft(udet.u - udet.width);
      detRect.setRight(udet.u + udet.width);
      detRect.setBottom(udet.v - udet.height);
      detRect.setTop(udet.v + udet.height);
      Mantid::Geometry::ComponentID id = parent->getComponentID();
      QRectF& r = m_assemblies[id];
      r |= detRect;
      calcAssemblies(parent,r);
    }
  }
}

//------------------------------------------------------------------------------
/**
  * Draw the unwrapped instrument onto the screen
  * @param widget :: The widget to draw it on.
  * @param picking :: True if detector is being drawn in the picking mode.
  */
void UnwrappedSurface::drawSurface(MantidGLWidget *widget,bool picking)const
{
  // dimensions of the screen to draw on
  int widget_width = widget->width();
  int widget_height = widget->height();

  // view rectangle in the OpenGL coordinates
  double view_left = m_viewRect.x0();
  double view_top  = m_viewRect.y1();
  double view_right  = m_viewRect.x1();
  double view_bottom = m_viewRect.y0();

  // make sure the view rectangle has a finite area
  if (view_left == view_right)
  {
      view_left  -= m_width_max / 2;
      view_right += m_width_max / 2;
  }
  if (view_top == view_bottom)
  {
      view_top    += m_height_max / 2;
      view_bottom -= m_height_max / 2;
  }

  const double dw = fabs((view_right - view_left) / widget_width);
  const double dh = fabs((view_top - view_bottom) / widget_height);

  if (m_startPeakShapes)
  {
    createPeakShapes(widget->rect());
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, widget_width, widget_height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho( view_left, view_right, view_bottom, view_top, -10,10 );

  if (OpenGLError::hasError("UnwrappedSurface::drawSurface"))
  {
    OpenGLError::log() << "glOrtho arguments:\n";
    OpenGLError::log() << view_left << ',' << view_right << ','
                       << view_bottom << ',' << view_top << ','
                       << -10 << ',' << 10 << '\n';
  }
  glMatrixMode(GL_MODELVIEW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLfloat oldLineWidth;
  glGetFloatv(GL_LINE_WIDTH,&oldLineWidth);
  glLineWidth(1.0f);

  glLoadIdentity();

  if ( m_isLightingOn && !picking )
  {
    glShadeModel(GL_SMOOTH);           // Shade model is smooth
    glEnable(GL_LINE_SMOOTH);          // Set line should be drawn smoothly
    glEnable(GL_LIGHT0);               // Enable opengl second light
    float diffuse[4]={1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    float direction[3]={0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);

    glEnable (GL_LIGHTING);            // Enable overall lighting
  }
  else
  {
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glDisable(GL_LINE_SMOOTH);
    glShadeModel(GL_FLAT);
  }

  for(size_t i=0;i<m_unwrappedDetectors.size();++i)
  {
    const UnwrappedDetector& udet = m_unwrappedDetectors[i];

    if (!udet.detector) continue;

    int iw = int(udet.width / dw);
    int ih = int(udet.height / dh);
    double w = (iw == 0)?  dw : udet.width/2;
    double h = (ih == 0)?  dh : udet.height/2;

    // check that the detector is visible in the current view
    if (!(m_viewRect.contains(udet.u-w, udet.v-h) || m_viewRect.contains(udet.u+w, udet.v+h))) continue;
    //QRectF detectorRect(udet.u-w,udet.v+h,w*2,h*2);
    //if ( !m_viewRect.intersects(detectorRect) ) continue;

    // apply the detector's colour
    setColor(int(i),picking);

    // if the detector is too small to see its shape draw a rectangle
    if (iw < 6 || ih < 6)
    {
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      glRectd(udet.u-w,udet.v-h,udet.u+w,udet.v+h);
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      if (iw > 2 || ih > 2 )
      {
        glRectd(udet.u-w,udet.v-h,udet.u+w,udet.v+h);
      }
    }
    // else draw the correct shape
    else
    {
      glPushMatrix();

      glTranslated(udet.u,udet.v,0.);
      glScaled(udet.uscale,udet.vscale,1);

      Mantid::Kernel::Quat rot;
      this->rotate(udet,rot);
      double deg,ax0,ax1,ax2;
      rot.getAngleAxis(deg,ax0,ax1,ax2);
      glRotated(deg,ax0,ax1,ax2);

      Mantid::Kernel::V3D scaleFactor = udet.detector->getScaleFactor();
      glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);

      udet.detector->shape()->draw();

      glPopMatrix();

    }

  }

  OpenGLError::check("UnwrappedSurface::drawSurface");

  glLineWidth(oldLineWidth);

  if (OpenGLError::check("UnwrappedSurface::drawSurface"))
  {
    OpenGLError::log()<<"oldLineWidth="<<oldLineWidth<<'\n';
  }

}


/**
  * Set detector color in OpenGL context.
  * @param index :: Detector's index in m_unwrappedDetectors
  * @param picking :: True if detector is being drawn in the picking mode.
  *   In this case index is transformed into color
  */
void UnwrappedSurface::setColor(int index,bool picking)const
{
  if (picking)
  {
    GLColor c = GLActor::makePickColor(index);
    unsigned char r,g,b;
    c.get(r,g,b);
    glColor3ub(r,g,b);
  }
  else
  {
    glColor3ubv(&m_unwrappedDetectors[index].color[0]);
  }
}

bool hasParent(boost::shared_ptr<const Mantid::Geometry::IComponent> comp,Mantid::Geometry::ComponentID id)
{
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = comp->getParent();
  if (!parent) return false;
  if (parent->getComponentID() == id) return true;
  return hasParent(parent,id);
}

//------------------------------------------------------------------------------
/** This method is called when a component is selected in the InstrumentTreeWidget
 * and zooms into that spot on the view.
 *
 * @param id :: ComponentID to zoom to.
 */
void UnwrappedSurface::componentSelected(Mantid::Geometry::ComponentID id)
{
  boost::shared_ptr<const Mantid::Geometry::Instrument> instr = m_instrActor->getInstrument();
  if (id == NULL)
  {
    id = instr->getComponentID();
  }
  boost::shared_ptr<const Mantid::Geometry::IComponent> comp = instr->getComponentByID(id);
  boost::shared_ptr<const Mantid::Geometry::ICompAssembly> ass =
      boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(comp);
  boost::shared_ptr<const Mantid::Geometry::IDetector> det =
      boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector>(comp);
  if (det)
  {
    int detID = det->getID();

    std::vector<UnwrappedDetector>::const_iterator it;
    for (it = m_unwrappedDetectors.begin(); it != m_unwrappedDetectors.end(); ++it)
    {
      const UnwrappedDetector& udet = *it;
      if (udet.detector && udet.detector->getID() == detID)
      {
        double w = udet.width;
        if (w > m_width_max) w = m_width_max;
        double h = udet.height;
        if (h > m_height_max) h = m_height_max;
        QRectF area(udet.u - w,udet.v - h,w*2,h*2);
        zoom(area);
        break;
      }
    }
  }
  if (ass)
  {
    this->cacheAllAssemblies();
    QMap<Mantid::Geometry::ComponentID,QRectF>::iterator assRect = m_assemblies.find(ass->getComponentID());
    if (assRect != m_assemblies.end())
      zoom(*assRect);
    else
    {
      // std::cout << "Assembly not found " << std::endl;
    }
  }
}

void UnwrappedSurface::getSelectedDetectors(QList<int>& dets)
{
  if (m_selectRect.isNull())
  {
    return;
  }
  QRect rect = selectionRect();

  double vtop = m_v_min;
  double vbottom = m_v_min;
  double uleft = m_u_min;
  double uright = m_u_min;

  // find the first picking colours different from black (0,0,0) to get the top-left
  // and bottom-right detectors
  int rwidth = rect.width();
  int rheight = rect.height();
  for(int i=0;i<rwidth;++i)
  {
    bool stop = false;
    for(int j=0;j<rheight;++j)
    {
      int x = rect.x() + i;
      int y = rect.y() + j;
      size_t ind = getPickID( x, y );
      if (ind < m_unwrappedDetectors.size() )
      {
        uleft = m_unwrappedDetectors[ind].u - m_unwrappedDetectors[ind].width / 2;
        vtop = m_unwrappedDetectors[ind].v + m_unwrappedDetectors[ind].height / 2;
        stop = true;
        break;
      }
    }
    if (stop) break;
  }

  for(int i=rwidth-1;i >= 0; --i)
  {
    bool stop = false;
    for(int j=rheight-1;j >= 0;--j)
    {
      int x = rect.x() + i;
      int y = rect.y() + j;
      size_t ind = getPickID( x, y );
      if (ind < m_unwrappedDetectors.size() )
      {
        uright = m_unwrappedDetectors[ind].u + m_unwrappedDetectors[ind].width / 2;
        vbottom = m_unwrappedDetectors[ind].v - m_unwrappedDetectors[ind].height / 2;
        stop = true;
        break;
      }
    }
    if (stop) break;
  }

  // select detectors with u,v within the allowed boundaries
  for(size_t i = 0; i < m_unwrappedDetectors.size(); ++i)
  {
    UnwrappedDetector& udet = m_unwrappedDetectors[i];
    if (! udet.detector ) continue;
    if (udet.u >= uleft && udet.u <= uright && udet.v >= vbottom && udet.v <= vtop)
    {
      dets.push_back(udet.detector->getID());
    }
  }

}

void UnwrappedSurface::getMaskedDetectors(QList<int>& dets)const
{
  dets.clear();
  if (m_maskShapes.isEmpty()) return;
  for(size_t i = 0; i < m_unwrappedDetectors.size(); ++i)
  {
    const UnwrappedDetector& udet = m_unwrappedDetectors[i];
    if (! udet.detector ) continue;
    if (m_maskShapes.isMasked(udet.u, udet.v))
    {
      dets.append(udet.detector->getID());
    }
  }
}

void UnwrappedSurface::changeColorMap()
{
  for(size_t i = 0; i < m_unwrappedDetectors.size(); ++i)
  {
    UnwrappedDetector& udet = m_unwrappedDetectors[i];
    if (! udet.detector ) continue;
    unsigned char color[3];
    m_instrActor->getColor(udet.detector->getID()).getUB3(&color[0]);
    udet.color[0] = color[0];
    udet.color[1] = color[1];
    udet.color[2] = color[2];
  }
}

QString UnwrappedSurface::getInfoText()const
{
  if (m_interactionMode == MoveMode)
  {
    //return getDimInfo() +
    return "Left mouse click and drag to zoom in. Right mouse click to zoom out.";
  }
  return ProjectionSurface::getInfoText();
}

RectF UnwrappedSurface::getSurfaceBounds()const
{
  return m_viewRect;
}

/**
 * Set a peaks workspace to be drawn ontop of the workspace.
 * @param pws :: A shared pointer to the workspace.
 */
void UnwrappedSurface::setPeaksWorkspace(boost::shared_ptr<Mantid::API::IPeaksWorkspace> pws)
{
  if (!pws)
  {
    return;
  }
  PeakOverlay* po = new PeakOverlay( this, pws );
  po->setPrecision(m_peakLabelPrecision);
  po->setShowRowsFlag(m_showPeakRows);
  po->setShowLabelsFlag(m_showPeakLabels);
  m_peakShapes.append(po);
  m_startPeakShapes = true;
  connect(po,SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)),this,SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)));
  emit peaksWorkspaceAdded();
}

//-----------------------------------------------------------------------------
/** Create the peak labels from the peaks set by setPeaksWorkspace.
 * The method is called from the draw(...) method
 *
 * @param window :: The screen window rectangle in pixels.
 */
void UnwrappedSurface::createPeakShapes(const QRect& window)const
{
    if ( !m_peakShapes.isEmpty() )
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        PeakOverlay& peakShapes = *m_peakShapes.last();
        PeakMarker2D::Style style = peakShapes.getDefaultStyle(m_peakShapesStyle);
        m_peakShapesStyle++;
        peakShapes.setWindow(getSurfaceBounds(),window);
        peakShapes.createMarkers( style );
        QApplication::restoreOverrideCursor();
    }
    m_startPeakShapes = false;
    setPeakVisibility();
}

/**
 * Toggle between flipped and straight view.
 */
void UnwrappedSurface::setFlippedView(bool on)
{
    if ( m_flippedView != on )
    {
        m_flippedView = on;
        m_viewRect.xFlip();
        for(int i = 0;i < m_zoomStack.size(); ++i)
        {
            m_zoomStack[i].xFlip();
        }
    }
}

/**
 * Draw the surface onto an image without OpenGL
 * @param image :: Image to draw on.
 * @param picking :: If true draw a picking image.
 */
void UnwrappedSurface::drawSimpleToImage(QImage* image,bool picking)const
{
  if ( !image ) return;

  QPainter paint(image);

  int vwidth = image->width();
  int vheight = image->height();

  paint.fillRect(0, 0, vwidth, vheight, m_backgroundColor);

  const double dw = fabs(m_viewRect.width() / vwidth);
  const double dh = fabs(m_viewRect.height()/ vheight);

  //std::cerr << m_viewRect.left() << ' ' << m_viewRect.right() << " : " <<  m_viewRect.bottom() << ' ' << m_viewRect.top() << std::endl;

  if (m_startPeakShapes)
  {
    createPeakShapes(image->rect());
  }

  for(size_t i=0;i<m_unwrappedDetectors.size();++i)
  {
    const UnwrappedDetector& udet = m_unwrappedDetectors[i];

    if (!udet.detector) continue;

    int iw = int(udet.width / dw);
    int ih = int(udet.height / dh);
    if ( iw < 4 ) iw = 4;
    if ( ih < 4 ) ih = 4;
    
    double w = (iw == 0)?  dw : udet.width/2;
    double h = (ih == 0)?  dh : udet.height/2;

    if (!(m_viewRect.contains(udet.u-w, udet.v-h) || m_viewRect.contains(udet.u+w, udet.v+h))) continue;

    int u = 0;
    if ( !isFlippedView() )
    {
      u = static_cast<int>( ( udet.u - m_viewRect.x0() ) / dw );
    }
    else
    {
      u =  static_cast<int>( vwidth - ( udet.u - m_viewRect.x1() ) / dw );
    }

    int v = vheight - static_cast<int>(( udet.v - m_viewRect.y0() ) / dh );

    QColor color;
    int index = int( i );
    if (picking)
    {
      GLColor c = GLActor::makePickColor(index);
      unsigned char r,g,b;
      c.get(r,g,b);
      color = QColor(r,g,b);
    }
    else
    {
      auto c = &m_unwrappedDetectors[index].color[0];
      color = QColor(c[0],c[1],c[2]);
    }

    paint.fillRect(u - iw/2, v - ih/2, iw, ih, color);

  }

  // draw custom stuff
  if ( !picking )
  {
      // TODO: this transform should be done for drawing the detectors
      QTransform transform;
      m_viewRect.findTransform( transform, QRectF(0, 0, vwidth, vheight) );
      paint.setTransform(transform);
      drawCustom(&paint);
  }
}

/**
  * Zooms to the specified area. The previous zoom stack is cleared.
  */
void UnwrappedSurface::zoom(const QRectF& area)
{
  if (!m_zoomStack.isEmpty())
  {
    m_viewRect = m_zoomStack.first();
    m_zoomStack.clear();
  }
  m_zoomStack.push(m_viewRect);

  double left = area.left();
  double top  = area.top();
  double width = area.width();
  double height = area.height();

  if (width * m_viewRect.width() < 0)
  {
    left += width;
    width = -width;
  }
  if (height * m_viewRect.height() < 0)
  {
    top += height;
    height = -height;
  }
  m_viewRect = RectF( QPointF(left,top), QPointF(left+width,top+height) );
  updateView();

}

void UnwrappedSurface::unzoom()
{
  if (!m_zoomStack.isEmpty())
  {
    m_viewRect = m_zoomStack.pop();
    updateView();
    emit updateInfoText();
  }
}

void UnwrappedSurface::zoom()
{
  if (!m_viewImage) return;
  RectF newView = selectionRectUV();
  if ( newView.isEmpty() ) return;
  m_zoomStack.push(m_viewRect);
  m_viewRect = newView;
  updateView();
  emptySelectionRect();
  emit updateInfoText();
}

//------------------------------------------------------------------------------
/** Calculate the UV and size of the given detector
 * Calls the pure virtual project() and calcSize() methods that
 * depend on the type of projection
 *
 * @param udet :: detector to unwrap.
 * @param pos :: detector position relative to the sample origin
 */
void UnwrappedSurface::calcUV(UnwrappedDetector& udet, Mantid::Kernel::V3D & pos )
{
  this->project(pos, udet.u, udet.v, udet.uscale, udet.vscale);
  calcSize(udet);
}

//------------------------------------------------------------------------------
/** Calculate the size of the detector in U/V
 *
 * @param udet :: UwrappedDetector struct to calculate the size for. udet's size fields 
 * are updated by this method.
 */
void UnwrappedSurface::calcSize(UnwrappedDetector& udet)
{
  // U is the horizontal axis on the screen
  const Mantid::Kernel::V3D U(-1,0,0);
  // V is the vertical axis on the screen
  const Mantid::Kernel::V3D V(0,1,0);

  // find the detector's rotation
  Mantid::Kernel::Quat R;
  this->rotate(udet,R);

  Mantid::Geometry::BoundingBox bbox = udet.detector->shape()->getBoundingBox();
  Mantid::Kernel::V3D scale = udet.detector->getScaleFactor();

  // sizes of the detector along each 3D axis
  Mantid::Kernel::V3D size = bbox.maxPoint() - bbox.minPoint();
  size *= scale;

  Mantid::Kernel::V3D s1(size);
  Mantid::Kernel::V3D s2 = size + Mantid::Kernel::V3D(-size.X(),0,0) - Mantid::Kernel::V3D(size.X(),0,0);
  Mantid::Kernel::V3D s3 = size + Mantid::Kernel::V3D(0,-size.Y(),0) - Mantid::Kernel::V3D(0,size.Y(),0);
  // rotate the size vectors to get the dimensions along axes U and V
  R.rotate(s1);
  R.rotate(s2);
  R.rotate(s3);

  // get the larges projection to the U axis which is the visible width
  double d = fabs(s1.scalar_prod(U));
  udet.width = d;
  d = fabs(s2.scalar_prod(U));
  if (d > udet.width) udet.width = d;
  d = fabs(s3.scalar_prod(U));
  if (d > udet.width) udet.width = d;

  // get the larges projection to the V axis which is the visible height
  d = fabs(s1.scalar_prod(V));
  udet.height = d;
  d = fabs(s2.scalar_prod(V));
  if (d > udet.height) udet.height = d;
  d = fabs(s3.scalar_prod(V));
  if (d > udet.height) udet.height = d;

  // apply the scale factors
  udet.width *= udet.uscale;
  udet.height *= udet.vscale;

  // don't let them be too large
  if (udet.width > m_width_max) m_width_max = udet.width;
  if (udet.height > m_height_max) m_height_max = udet.height;

}

