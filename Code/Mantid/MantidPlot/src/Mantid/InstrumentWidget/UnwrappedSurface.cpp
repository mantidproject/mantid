#include "UnwrappedSurface.h"
#include "GLColor.h"
#include "MantidGLWidget.h"
#include "OpenGLError.h"
#include "PeakMarker2D.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Instrument.h"

#include <QRgb>
#include <QSet>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <QMessageBox>

#include <cfloat>
#include <limits>
#include <cmath>

UnwrappedDetector::UnwrappedDetector(const unsigned char* c,
                     boost::shared_ptr<const Mantid::Geometry::IDetector> det
                     ):
u(0), v(0), width(0), height(0), uscale(0), vscale(0), detector(det)
{
  color[0] = *c;
  color[1] = *(c+1);
  color[2] = *(c+2);
}


UnwrappedSurface::UnwrappedSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis):
    ProjectionSurface(rootActor,origin,axis),
    m_instrActor(rootActor),
    m_u_min(DBL_MAX),
    m_u_max(-DBL_MAX),
    m_v_min(DBL_MAX),
    m_v_max(-DBL_MAX),
    m_height_max(0),
    m_width_max(0),
    m_flippedView(false),
    m_startPeakShapes(false)
{
}

UnwrappedSurface::~UnwrappedSurface()
{
}

void UnwrappedSurface::init()
{
  // the actor calls this->callback for each detector
  m_unwrappedDetectors.clear();
  m_u_min =  DBL_MAX;
  m_u_max = -DBL_MAX;
  m_v_min =  DBL_MAX;
  m_v_max = -DBL_MAX;

  size_t ndet = m_instrActor->ndetectors();
  for(size_t i = 0; i < ndet; ++i)
  {
    unsigned char color[3];
    boost::shared_ptr<const Mantid::Geometry::IDetector> det = m_instrActor->getDetector(i);
    if (!det || det->isMonitor())
    {
      m_unwrappedDetectors.append(UnwrappedDetector(&color[0],boost::shared_ptr<Mantid::Geometry::IDetector>()));
      continue;
    }
    Mantid::detid_t id = -1;
    id = m_instrActor->getDetID(i);
    if (id < 0)
    {
      m_unwrappedDetectors.append(UnwrappedDetector(&color[0],boost::shared_ptr<Mantid::Geometry::IDetector>()));
      continue;
    }
    m_instrActor->getColor(id).getUB3(&color[0]);
    // first detector defines the surface's x axis
    if (m_xaxis.nullVector())
    {
      Mantid::Kernel::V3D pos = det->getPos() - m_pos;
      double z = pos.scalar_prod(m_zaxis);
      if (z == 0.0)
      {
        // find the sortest projection of m_zaxis and direct m_xaxis along it
        bool isY = false;
        bool isZ = false;
        if (fabs(m_zaxis.Y()) < fabs(m_zaxis.X())) isY = true;
        if (fabs(m_zaxis.Z()) < fabs(m_zaxis.Y())) isZ = true;
        if (isZ)
        {
          m_xaxis = Mantid::Kernel::V3D(0,0,1);
        }
        else if (isY)
        {
          m_xaxis = Mantid::Kernel::V3D(0,1,0);
        }
        else
        {
          m_xaxis = Mantid::Kernel::V3D(1,0,0);
        }
      }
      else
      {
        m_xaxis = pos - m_zaxis * z;
        m_xaxis.normalize();
      }
      m_yaxis = m_zaxis.cross_prod(m_xaxis);
    }
    UnwrappedDetector udet(&color[0],det);
    this->calcUV(udet);
    if (udet.u < m_u_min) m_u_min = udet.u;
    if (udet.u > m_u_max) m_u_max = udet.u;
    if (udet.v < m_v_min) m_v_min = udet.v;
    if (udet.v > m_v_max) m_v_max = udet.v;
    m_unwrappedDetectors.append(udet);
  }

  findAndCorrectUGap();

  foreach(const UnwrappedDetector& udet,m_unwrappedDetectors)
  {
    if (! udet.detector ) continue;
    boost::shared_ptr<const Mantid::Geometry::IComponent> parent = udet.detector->getParent();
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

  double dU = fabs(m_u_max - m_u_min);
  double dV = fabs(m_v_max - m_v_min);
  double du = dU * 0.05;
  double dv = dV * 0.05;
  if (m_width_max > du && m_width_max != std::numeric_limits<double>::infinity())
  {
    if (du > 0 && !(dU >= m_width_max))
    {
      m_width_max = dU;
    }
    du = m_width_max;
  }
  if (m_height_max > dv && m_height_max != std::numeric_limits<double>::infinity())
  {
    if (dv > 0 && !(dV >= m_height_max))
    {
      m_height_max = dV;
    }
    dv = m_height_max;
  }
  m_u_min -= du;
  m_u_max += du;
  m_v_min -= dv;
  m_v_max += dv;
  m_viewRect = QRectF(QPointF(m_u_min,m_v_max),
                           QPointF(m_u_max,m_v_min));

}

/**
  * Calculate the rectangular region in uv coordinates occupied by an assembly.
  * @param comp :: A member of the assebmly. The total area of the assembly is a sum of areas of its members
  * @param compRect :: A rect. area occupied by comp in uv space
  */
void UnwrappedSurface::calcAssemblies(boost::shared_ptr<const Mantid::Geometry::IComponent> comp,const QRectF& compRect)
{
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = comp->getParent();
  if (parent)
  {
    QRectF& r = m_assemblies[parent->getComponentID()];
    r |= compRect;
    calcAssemblies(parent,r);
  }
}

/**
  * Draw the unwrapped instrument onto the screen
  * @param widget :: The widget to draw it on.
  * @param picking :: True if detector is being drawn in the picking mode.
  */
void UnwrappedSurface::drawSurface(MantidGLWidget *widget,bool picking)const
{
  int vwidth = widget->width();
  int vheight = widget->height();
  const double dw = fabs(m_viewRect.width() / vwidth);
  const double dh = fabs(m_viewRect.height()/ vheight);

  //std::cerr << m_viewRect.left() << ' ' << m_viewRect.right() << " : " <<  m_viewRect.bottom() << ' ' << m_viewRect.top() << std::endl;

  if (m_startPeakShapes)
  {
    createPeakShapes(widget->rect());
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //glDisable(GL_DEPTH_TEST);
  glViewport(0, 0, vwidth, vheight);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(m_viewRect.left(),m_viewRect.right(),
    m_viewRect.bottom(),m_viewRect.top(),
    -10,10);
  if (OpenGLError::hasError("UnwrappedSurface::drawSurface"))
  {
    OpenGLError::log() << "glOrtho arguments:\n";
    OpenGLError::log() << m_viewRect.left()<<','<<m_viewRect.right()<<','<<
      m_viewRect.bottom()<<','<<m_viewRect.top()<<','<<
      -10<<','<<10<<'\n';
  }
  glMatrixMode(GL_MODELVIEW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLfloat oldLineWidth;
  glGetFloatv(GL_LINE_WIDTH,&oldLineWidth);
  glLineWidth(1.0f);

  glLoadIdentity();

  for(int i=0;i<m_unwrappedDetectors.size();++i)
  {
    const UnwrappedDetector& udet = m_unwrappedDetectors[i];

    if (!udet.detector || !m_viewRect.contains(udet.u,udet.v)) continue;

    setColor(i,picking);

    int iw = int(udet.width / dw);
    int ih = int(udet.height / dh);
    if (iw < 6 || ih < 6)
    {
      double w = (iw == 0)?  dw : udet.width/2;
      double h = (ih == 0)?  dh : udet.height/2;
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      glRectd(udet.u-w,udet.v-h,udet.u+w,udet.v+h);
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      if (iw > 2 || ih > 2 )
      {
        glRectd(udet.u-w,udet.v-h,udet.u+w,udet.v+h);
      }
    }
    else
    {
      glPushMatrix();

      glTranslated(udet.u,udet.v,0.);
      glScaled(udet.uscale,udet.vscale,1);

      Mantid::Kernel::Quat rot;
      this->calcRot(udet,rot);
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

void UnwrappedSurface::calcSize(UnwrappedDetector& udet,const Mantid::Kernel::V3D& X,
              const Mantid::Kernel::V3D& Y)
{
  Mantid::Kernel::Quat R;
  this->calcRot(udet,R);

  Mantid::Geometry::BoundingBox bbox = udet.detector->shape()->getBoundingBox();
  Mantid::Kernel::V3D scale = udet.detector->getScaleFactor();

//  udet.minPoint = bbox.minPoint();
//  udet.maxPoint = bbox.maxPoint();

  Mantid::Kernel::V3D size = bbox.maxPoint() - bbox.minPoint();
  size *= scale;
  Mantid::Kernel::V3D s1(size);
  Mantid::Kernel::V3D s2 = size + Mantid::Kernel::V3D(-size.X(),0,0) - Mantid::Kernel::V3D(size.X(),0,0);
  Mantid::Kernel::V3D s3 = size + Mantid::Kernel::V3D(0,-size.Y(),0) - Mantid::Kernel::V3D(0,size.Y(),0);
  R.rotate(s1);
  R.rotate(s2);
  R.rotate(s3);

  double d = fabs(s1.scalar_prod(X));
  udet.width = d;
  d = fabs(s2.scalar_prod(X));
  if (d > udet.width) udet.width = d;
  d = fabs(s3.scalar_prod(X));
  if (d > udet.width) udet.width = d;

  d = fabs(s1.scalar_prod(Y));
  udet.height = d;
  d = fabs(s2.scalar_prod(Y));
  if (d > udet.height) udet.height = d;
  d = fabs(s3.scalar_prod(Y));
  if (d > udet.height) udet.height = d;

  udet.width *= udet.uscale;
  udet.height *= udet.vscale;

  if (udet.width > m_width_max) m_width_max = udet.width;
  if (udet.height > m_height_max) m_height_max = udet.height;

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

void UnwrappedSurface::showPickedDetector()
{
  if (m_selectRect.isNull())
  {
    return;
  }
  QRect rect = selectionRect();
  QSet<int> detIDs;
  for(int i=0;i<rect.width();++i)
  {
    for(int j=0;j<rect.height();++j)
    {
      int x = rect.x() + i;
      int y = rect.y() + j;
      QRgb pixel = m_pickImage->pixel(x,y);
      int detID = getDetectorID((unsigned char)qRed(pixel),(unsigned char)qGreen(pixel),(unsigned char)qBlue(pixel));
      if (detID >= 0)
      {
        detIDs.insert(detID);
      }
    }
  }
  foreach(int id,detIDs)
  {
    std::cerr<<"det ID = "<<id<<'\n';

  }
}

bool hasParent(boost::shared_ptr<const Mantid::Geometry::IComponent> comp,Mantid::Geometry::ComponentID id)
{
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = comp->getParent();
  if (!parent) return false;
  if (parent->getComponentID() == id) return true;
  return hasParent(parent,id);
}

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
    foreach(const UnwrappedDetector& udet,m_unwrappedDetectors)
    {
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
    QMap<Mantid::Geometry::ComponentID,QRectF>::iterator assRect = m_assemblies.find(ass->getComponentID());
    if (assRect != m_assemblies.end())
    {
      zoom(*assRect);
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
      QRgb pixel = m_pickImage->pixel(x,y);
      int ind = getDetectorIndex((unsigned char)qRed(pixel),(unsigned char)qGreen(pixel),(unsigned char)qBlue(pixel));
      if (ind >= 0)
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
      QRgb pixel = m_pickImage->pixel(x,y);
      int ind = getDetectorIndex((unsigned char)qRed(pixel),(unsigned char)qGreen(pixel),(unsigned char)qBlue(pixel));
      if (ind >= 0)
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
  for(int i = 0; i < m_unwrappedDetectors.size(); ++i)
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
  for(int i = 0; i < m_unwrappedDetectors.size(); ++i)
  {
    const UnwrappedDetector& udet = m_unwrappedDetectors[i];
    if (! udet.detector ) continue;
    if (m_maskShapes.isMasked(udet.u, udet.v))
    {
      dets.append(udet.detector->getID());
    }
  }
}

void UnwrappedSurface::findAndCorrectUGap()
{
  double period = uPeriod();
  if (period == 0.0) return;
  const int nbins = 1000;
  std::vector<bool> ubins(nbins);
  double bin_width = fabs(m_u_max - m_u_min) / (nbins - 1);
  if (bin_width == 0.0)
  {
    QMessageBox::warning(NULL, tr("MantidPLot - Instrument view error"), tr("Failed to build unwrapped surface"));
    m_u_min = 0.0;
    m_u_max = 1.0;
    return;
  }

  QList<UnwrappedDetector>::const_iterator ud = m_unwrappedDetectors.begin();
  for(;ud != m_unwrappedDetectors.end(); ++ud)
  {
    if (! ud->detector ) continue;
    double u = ud->u;
    int i = int((u - m_u_min) / bin_width);
    ubins[i] = true;
  }

  int iFrom = 0; // marks gap start
  int iTo   = 0; // marks gap end
  int i0 = 0;
  bool inGap = false; 
  for(int i = 0;i < int(ubins.size())-1;++i)
  {
    if (!ubins[i])
    {
      if (!inGap)
      {
        i0 = i;
      }
      inGap = true;
    }
    else
    {
      if (inGap && iTo - iFrom < i - i0)
      {
        iFrom = i0; // first bin in the gap
        iTo   = i;  // first bin after the gap
      }
      inGap = false;
    }
  }

  double uFrom = m_u_min + iFrom * bin_width;
  double uTo   = m_u_min + iTo   * bin_width;
  if (uTo - uFrom > period - (m_u_max - m_u_min))
  {
    double du = m_u_max - uTo;
    m_u_max = uFrom + du;
    QList<UnwrappedDetector>::iterator ud = m_unwrappedDetectors.begin();
    for(;ud != m_unwrappedDetectors.end(); ++ud)
    {
      if (! ud->detector ) continue;
      double& u = ud->u;
      u += du;
      if (u > m_u_max)
      {
        u -= period;
      }
    }
    //m_u_min += du;
  }
}

void UnwrappedSurface::changeColorMap()
{
  for(int i = 0; i < m_unwrappedDetectors.size(); ++i)
  {
    UnwrappedDetector& udet = m_unwrappedDetectors[i];
    if (! udet.detector ) continue;
    const boost::shared_ptr<const Mantid::Geometry::IDetector> det = udet.detector;
    unsigned char color[3];
    m_instrActor->getColor(det->getID()).getUB3(&color[0]);
    udet.color[0] = color[0];
    udet.color[1] = color[1];
    udet.color[2] = color[2];
  }
}

void UnwrappedSurface::mousePressEventMove(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton)
  {
    m_leftButtonDown = true;
    startSelection(e->x(),e->y());
  }
}

void UnwrappedSurface::mouseMoveEventMove(QMouseEvent* e)
{
  if (m_leftButtonDown)
  {
    moveSelection(e->x(),e->y());
  }
}

void UnwrappedSurface::mouseReleaseEventMove(QMouseEvent* e)
{
  if (m_leftButtonDown)
  {
    if (!m_pickImage) // we are in normal mode
    {
      if (!m_selectRect.isNull())
      {
        zoom();
      }
    }
    endSelection(e->x(),e->y());
    m_leftButtonDown = false;
  }
  else if (e->button() == Qt::RightButton)
  {
    unzoom();
  }
}

void UnwrappedSurface::wheelEventMove(QWheelEvent*)
{
}

QString UnwrappedSurface::getInfoText()const
{
  if (m_interactionMode == PickMode)
  {
    return getPickInfoText();
  }
  QString text = "Left mouse click and drag to zoom in.\nRight mouse click to zoom out.";
  return text;
}

QRectF UnwrappedSurface::getSurfaceBounds()const
{
  return QRectF(m_viewRect.left(),m_viewRect.bottom(),m_viewRect.width(),-m_viewRect.height());
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
  PeakOverlay* po = new PeakOverlay(pws);
  po->setPrecision(m_peakLabelPrecision);
  po->setShowRowsFlag(m_showPeakRow);
  m_peakShapes.append(po);
  m_startPeakShapes = true;
}

/**
 * Create the peak labels from the peaks set by setPeaksWorkspace. The method is called from the draw(...) method
 * @param window :: The screen window rectangle in pixels.
 */
void UnwrappedSurface::createPeakShapes(const QRect& window)const
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  PeakOverlay& peakShapes = *m_peakShapes.last();
  PeakMarker2D::Style style = m_peakShapes.first()->getNextDefaultStyle();
  peakShapes.setWindow(getSurfaceBounds(),window);
  int nPeaks = peakShapes.getNumberPeaks();
  for(int i = 0; i < nPeaks; ++i)
  {
    Mantid::API::IPeak& peak = peakShapes.getPeak(i);
    int detID = peak.getDetectorID();
    foreach(UnwrappedDetector udet,m_unwrappedDetectors)
    {
      Mantid::Geometry::IDetector_const_sptr det = udet.detector;
      if (! det ) continue;
      if (det->getID() != detID) continue;
      PeakMarker2D* r = new PeakMarker2D(peakShapes.realToUntransformed(QPointF(udet.u,udet.v)),style);
      r->setPeak(peak,i);
      peakShapes.addMarker(r);
    }
  }
  peakShapes.deselectAll();
  m_startPeakShapes = false;
  QApplication::restoreOverrideCursor();
}

/**
 * Toggle between flipped and straight view.
 */
void UnwrappedSurface::setFlippedView(bool on)
{
  m_flippedView = on;
  qreal left = m_viewRect.left();
  qreal right = m_viewRect.right();
  m_viewRect.setLeft(right);
  m_viewRect.setRight(left);
}
