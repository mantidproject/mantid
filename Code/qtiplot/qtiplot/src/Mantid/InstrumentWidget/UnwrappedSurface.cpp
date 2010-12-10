#include "UnwrappedSurface.h"
#include "GLColor.h"
#include "GL3DWidget.h"
#include "OpenGLError.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/IInstrument.h"

#include <QRgb>
#include <QSet>

#include <cfloat>

UnwrappedDetector::UnwrappedDetector(const unsigned char* c,
                     boost::shared_ptr<const Mantid::Geometry::IDetector> det
                     ):
detector(det)
{
  color[0] = *c;
  color[1] = *(c+1);
  color[2] = *(c+2);
}


double UnwrappedSurface::m_tolerance = 0.00001;

UnwrappedSurface::UnwrappedSurface(const InstrumentActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis):
    m_instrActor(rootActor),
    m_pos(origin),
    m_zaxis(axis),
    m_u_min(DBL_MAX),
    m_u_max(-DBL_MAX),
    m_v_min(DBL_MAX),
    m_v_max(-DBL_MAX),
    m_unwrappedImage(NULL),
    m_pickImage(NULL),
    m_unwrappedViewChanged(true),
    m_unwrappedView(),
    m_selectRect()
{
}

UnwrappedSurface::~UnwrappedSurface()
{
  //std::cerr<<"UnwrappedSurface deleted\n";
  if (m_unwrappedImage)
  {
    delete m_unwrappedImage;
  }
  if (m_pickImage)
  {
    delete m_pickImage;
  }
}

void UnwrappedSurface::init()
{
  // the actor calls this->callback for each detector
  m_instrActor->detectorCallback(this);
  double du = fabs(m_u_max - m_u_min) * 0.05;
  double dv = fabs(m_v_max - m_v_min) * 0.05;
  m_u_min -= du;
  m_u_max += du;
  m_v_min -= dv;
  m_v_max += dv;
  m_unwrappedView = QRectF(QPointF(m_u_min,m_v_max),
                           QPointF(m_u_max,m_v_min));
}

void UnwrappedSurface::clear()
{
  if (m_unwrappedImage)
  {
    delete m_unwrappedImage;
    m_unwrappedImage = NULL;
  }
  if (m_pickImage)
  {
    delete m_pickImage;
    m_pickImage = NULL;
  }
  m_unwrappedViewChanged = true;
  m_unwrappedDetectors.clear();
  m_unwrappedView = QRectF();
  m_selectRect = QRect();
  m_zoomStack.clear();
  m_u_min =  DBL_MAX;
  m_u_max = -DBL_MAX;
  m_v_min =  DBL_MAX;
  m_v_max = -DBL_MAX;
}

void UnwrappedSurface::callback(boost::shared_ptr<const Mantid::Geometry::IDetector> det,const DetectorCallbackData& data)
{
  if (det->isMonitor()) return;
  unsigned char color[3];
  data.color.getUB3(&color[0]);
  // first detector defines the cylinder's x axis
  if (m_xaxis.nullVector())
  {
    Mantid::Geometry::V3D pos = det->getPos() - m_pos;
    double z = pos.scalar_prod(m_zaxis);
    m_xaxis = pos - m_zaxis * z;
    m_xaxis.normalize();
    m_yaxis = m_zaxis.cross_prod(m_xaxis);
  }
  UnwrappedDetector udet(&color[0],det);
  this->calcUV(udet);
  if (udet.u < m_u_min) m_u_min = udet.u;
  if (udet.u > m_u_max) m_u_max = udet.u;
  if (udet.v < m_v_min) m_v_min = udet.v;
  if (udet.v > m_v_max) m_v_max = udet.v;
  m_unwrappedDetectors.append(udet);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
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

void UnwrappedSurface::draw(GL3DWidget *widget)
{
  if (widget->getInteractionMode() == GL3DWidget::MoveMode)
  {
    drawSurface(widget,false);
    if (m_pickImage)
    {
      delete m_pickImage;
      m_pickImage = NULL;
    }
  }
  else
  {
    bool changed = m_unwrappedViewChanged;
    drawSurface(widget,true);
    m_unwrappedViewChanged = changed;
    drawSurface(widget,false);
  }
}

/**
  * Draw the unwrapped instrument onto the screen
  * @param widget The widget to draw it on.
  */
void UnwrappedSurface::drawSurface(GL3DWidget *widget,bool picking)
{
//  std::cerr<<"drawing to:\n";
//  std::cerr<<m_unwrappedView.left()<<','<<m_unwrappedView.top()<<' '
//      <<m_unwrappedView.width()<<','<<m_unwrappedView.height()<<'\n'<<'\n';
  int vwidth,vheight;
  widget->getViewport(vwidth,vheight);
  const double dw = fabs(m_unwrappedView.width() / vwidth);
  const double dh = fabs(m_unwrappedView.height()/ vheight);

  QImage **image = picking ? &m_pickImage : &m_unwrappedImage;

  if (!*image || (*image)->width() != vwidth || (*image)->height() != vheight)
  {
    m_unwrappedViewChanged = true;
  }

  if (m_unwrappedViewChanged)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, vwidth, vheight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(m_unwrappedView.left(),m_unwrappedView.right(),
            m_unwrappedView.bottom(),m_unwrappedView.top(),
            -10,10);
    glMatrixMode(GL_MODELVIEW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLfloat oldLineWidth;
    glGetFloatv(GL_LINE_WIDTH,&oldLineWidth);
    glLineWidth(1.0f);

    glLoadIdentity();

    for(int i=0;i<m_unwrappedDetectors.size();++i)
    {
      UnwrappedDetector& udet = m_unwrappedDetectors[i];

      if (!m_unwrappedView.contains(udet.u,udet.v)) continue;

      setColor(i,picking);

      int iw = udet.width / dw;
      int ih = udet.height / dh;
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

        Mantid::Geometry::Quat rot;
        this->calcRot(udet,rot);
        double deg,ax0,ax1,ax2;
        rot.getAngleAxis(deg,ax0,ax1,ax2);
        glRotated(deg,ax0,ax1,ax2);

        Mantid::Geometry::V3D scaleFactor = udet.detector->getScaleFactor();
        glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);

        udet.detector->shape()->draw();

        glPopMatrix();

      }

    }

    glLineWidth(oldLineWidth);

    if (*image)
    {
      delete (*image);
    }
    (*image) = new QImage(widget->grabFrameBuffer());
//    if (picking)
//    {
//      (*image)->save("C:\\Documents and Settings\\hqs74821\\Desktop\\tmp\\test\\Unwrapped.png");
//    }
    if (!picking)
    {
      widget->swapBuffers();
    }
    m_unwrappedViewChanged = false;
  }
  else if (!picking)
  {
    QPainter painter(widget);
    painter.drawImage(0,0,**image);
    // draw the selection rectangle
    if (!m_selectRect.isNull())
    {
      painter.setPen(Qt::blue);
      //painter.setCompositionMode(QPainter::CompositionMode_Xor);
      painter.drawRect(m_selectRect);
    }
    painter.end();
  }
  OpenGLError::check("UnwrappedSurface::drawSurface");
}

void UnwrappedSurface::calcSize(UnwrappedDetector& udet,const Mantid::Geometry::V3D& X,
              const Mantid::Geometry::V3D& Y)
{
  Mantid::Geometry::Quat R;
  this->calcRot(udet,R);

  Mantid::Geometry::BoundingBox bbox = udet.detector->shape()->getBoundingBox();
  Mantid::Geometry::V3D scale = udet.detector->getScaleFactor();

//  udet.minPoint = bbox.minPoint();
//  udet.maxPoint = bbox.maxPoint();

  Mantid::Geometry::V3D size = bbox.maxPoint() - bbox.minPoint();
  size *= scale;
  Mantid::Geometry::V3D s1(size);
  Mantid::Geometry::V3D s2 = size + Mantid::Geometry::V3D(-size.X(),0,0) - Mantid::Geometry::V3D(size.X(),0,0);
  Mantid::Geometry::V3D s3 = size + Mantid::Geometry::V3D(0,-size.Y(),0) - Mantid::Geometry::V3D(0,size.Y(),0);
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

}

/**
  * Find a rotation from one orthonormal basis set (Xfrom,Yfrom,Zfrom) to
  * another orthonormal basis set (Xto,Yto,Zto). Both sets must be right-handed
  * (or same-handed, I didn't check). The method doesn't check the sets for orthogonality
  * or normality. The result is a rotation quaternion such that:
  *   R.rotate(Xfrom) == Xto
  *   R.rotate(Yfrom) == Yto
  *   R.rotate(Zfrom) == Zto
  * @param Xfrom The X axis of the original basis set
  * @param Yfrom The Y axis of the original basis set
  * @param Zfrom The Z axis of the original basis set
  * @param Xto The X axis of the final basis set
  * @param Yto The Y axis of the final basis set
  * @param Zto The Z axis of the final basis set
  * @param R The output rotation as a quaternion
  * @param out Debug printout flag
  */
void UnwrappedSurface::BasisRotation(const Mantid::Geometry::V3D& Xfrom,
                const Mantid::Geometry::V3D& Yfrom,
                const Mantid::Geometry::V3D& Zfrom,
                const Mantid::Geometry::V3D& Xto,
                const Mantid::Geometry::V3D& Yto,
                const Mantid::Geometry::V3D& Zto,
                Mantid::Geometry::Quat& R,
                bool out
                )
{
  // Find transformation from (X,Y,Z) to (XX,YY,ZZ)
  // R = R1*R2*R3, where R1, R2, and R3 are Euler rotations

//  std::cerr<<"RCRotation-----------------------------\n";
//  std::cerr<<"From "<<Xfrom<<Yfrom<<Zfrom<<'\n';
//  std::cerr<<"To   "<<Xto<<Yto<<Zto<<'\n';

  double sZ = Zfrom.scalar_prod(Zto);
  if (fabs(sZ - 1) < m_tolerance) // vectors the same
  {
    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < m_tolerance)
    {
      R = Mantid::Geometry::Quat();
    }
    else if (fabs(sX + 1) < m_tolerance)
    {
      R = Mantid::Geometry::Quat(180,Zfrom);
    }
    else
    {
      R = Mantid::Geometry::Quat(Xfrom,Xto);
    }
  }
  else if(fabs(sZ + 1) < m_tolerance) // rotated by 180 degrees
  {
    if (fabs(Xfrom.scalar_prod(Xto)-1) < m_tolerance)
    {
      R = Mantid::Geometry::Quat(180.,Xfrom);
    }
    else if (fabs(Yfrom.scalar_prod(Yto)-1) < m_tolerance)
    {
      R = Mantid::Geometry::Quat(180.,Yfrom);
    }
    else
    {
      R = Mantid::Geometry::Quat(180.,Xto)*Mantid::Geometry::Quat(Xfrom,Xto);
    }
  }
  else
  {
    // Rotation R1 of system (X,Y,Z) around Z by alpha
    Mantid::Geometry::V3D X1;
    Mantid::Geometry::Quat R1;

    X1 = Zfrom.cross_prod(Zto);
    X1.normalize();

    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < m_tolerance)
    {
      R = Mantid::Geometry::Quat(Zfrom,Zto);
      return;
    }

    sX = Xfrom.scalar_prod(X1);
    if (fabs(sX - 1) < m_tolerance)
    {
      R1 = Mantid::Geometry::Quat();
    }
    else if (fabs(sX + 1) < m_tolerance) // 180 degree rotation
    {
      R1 = Mantid::Geometry::Quat(180.,Zfrom);
    }
    else
    {
      R1 = Mantid::Geometry::Quat(Xfrom,X1);
    }
    if (out)
    std::cerr<<"R1="<<R1<<'\n';

    // Rotation R2 around X1 by beta
    Mantid::Geometry::Quat R2(Zfrom,Zto); // vectors are different
    if (out)
    std::cerr<<"R2="<<R2<<'\n';

    // Rotation R3 around ZZ by gamma
    Mantid::Geometry::Quat R3;
    sX = Xto.scalar_prod(X1);
    if (fabs(sX - 1) < m_tolerance)
    {
      R3 = Mantid::Geometry::Quat();
    }
    else if (fabs(sX + 1) < m_tolerance) // 180 degree rotation
    {
      R3 = Mantid::Geometry::Quat(180.,Zto);
    }
    else
    {
      R3 = Mantid::Geometry::Quat(X1,Xto);
    }
    if (out)
    std::cerr<<"R3="<<R3<<'\n';

    // Combined rotation
    R = R3*R2*R1;
  }
}


void UnwrappedSurface::startSelection(int x,int y)
{
  m_selectRect.setRect(x,y,1,1);
}

void UnwrappedSurface::moveSelection(int x,int y)
{
  m_selectRect.setBottomRight(QPoint(x,y));
}

void UnwrappedSurface::endSelection(int x,int y)
{
  if (m_pickImage) // we are in picking mode
  {
    showPickedDetector();
  }
  else
  {
    if (!m_selectRect.isNull())
    {
      zoom();
    }
  }
  m_selectRect = QRect();
}

void UnwrappedSurface::zoom()
{
  if (!m_unwrappedImage) return;
  m_zoomStack.push(m_unwrappedView);
  m_unwrappedView = selectionRectUV();
  m_unwrappedViewChanged = true;
}

/**
  * Zooms to the specified area. The previous zoom stack is cleared.
  */
void UnwrappedSurface::zoom(const QRectF& area)
{
  if (!m_zoomStack.isEmpty())
  {
    m_unwrappedView = m_zoomStack.first();
    m_zoomStack.clear();
  }
  m_zoomStack.push(m_unwrappedView);

  double left = area.left();
  double top  = area.top();
  double width = area.width();
  double height = area.height();

  if (width * m_unwrappedView.width() < 0)
  {
    left += width;
    width = -width;
  }
  if (height * m_unwrappedView.height() < 0)
  {
    top += height;
    height = -height;
  }
//  std::cerr<<"New area:\n";
//  std::cerr<<left<<','<<top<<' '<<width<<','<<height<<'\n'<<'\n';
  m_unwrappedView = QRectF(left,top,width,height);
  m_unwrappedViewChanged = true;
}

void UnwrappedSurface::unzoom()
{
  if (!m_zoomStack.isEmpty())
  {
    m_unwrappedView = m_zoomStack.pop();
    m_unwrappedViewChanged = true;
  }
}

void UnwrappedSurface::updateView()
{
  m_unwrappedViewChanged = true;
}

void UnwrappedSurface::updateDetectors()
{
  clear();
  init();
}

QRect UnwrappedSurface::selectionRect()const
{
  if (m_selectRect.width() == 0 || m_selectRect.height() == 0) return QRect();

  int x_min  = m_selectRect.left();
  int x_size = m_selectRect.width();
  int y_min  = m_selectRect.top();
  int y_size = m_selectRect.height();

  if (x_size < 0)
  {
    x_min += x_size;
    x_size = abs(x_size);
  }

  if (y_size < 0)
  {
    y_min += y_size;
    y_size = abs(y_size);
  }

  return QRect(x_min,y_min,x_size,y_size);
}

QRectF UnwrappedSurface::selectionRectUV()const
{
  if (m_selectRect.width() == 0 || m_selectRect.height() == 0) return QRectF();

  double x_min  = double(m_selectRect.left())/m_unwrappedImage->width();
  double x_size = double(m_selectRect.width())/m_unwrappedImage->width();
  double y_min  = double(m_selectRect.top())/m_unwrappedImage->height();
  double y_size = double(m_selectRect.height())/m_unwrappedImage->height();

  if (x_size < 0)
  {
    x_min += x_size;
    x_size = fabs(x_size);
  }

  if (y_size < 0)
  {
    y_min += y_size;
    y_size = fabs(y_size);
  }

  x_min = m_unwrappedView.left() + x_min * m_unwrappedView.width();
  x_size = x_size * m_unwrappedView.width();
  y_min = m_unwrappedView.top() + y_min * m_unwrappedView.height();
  y_size = y_size * m_unwrappedView.height();

  return QRectF(x_min,y_min,x_size,y_size);
}

void UnwrappedSurface::setColor(int index,bool picking)
{
  if (picking)
  {
    unsigned int id = (unsigned int)(index+1);
    unsigned char r = (unsigned char)(id % 256); id /= 256;
    unsigned char g = (unsigned char)(id % 256); id /= 256;
    unsigned char b = (unsigned char)(id % 256); id /= 256;
    if (id != 0)
    {
      throw std::overflow_error("Picking color overflow with detector id "+
                                QString::number(m_unwrappedDetectors[index].detector->getID()).toStdString());
    }
    glColor3ub(r,g,b);
  }
  else
  {
    glColor3ubv(&m_unwrappedDetectors[index].color[0]);
  }
}

int UnwrappedSurface::getDetectorID(unsigned char r,unsigned char g,unsigned char b)const
{
  unsigned int id = b;
  id *= 256;
  id += g;
  id *= 256;
  id += r;
  if (id == 0)
  {
    return -1;
  }
  else
  {
    return m_unwrappedDetectors[id-1].detector->getID();
  }
}

void UnwrappedSurface::showPickedDetector()
{
  if (m_selectRect.isNull())
  {
    std::cerr<<"nothing selected\n";
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
      int detID = getDetectorID(qRed(pixel),qGreen(pixel),qBlue(pixel));
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
  boost::shared_ptr<const Mantid::Geometry::IInstrument> instr = m_instrActor->getInstrument();
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
      if (udet.detector->getID() == detID)
      {
        QRectF area(udet.u - udet.width,udet.v - udet.height,udet.width*2,udet.height*2);
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

