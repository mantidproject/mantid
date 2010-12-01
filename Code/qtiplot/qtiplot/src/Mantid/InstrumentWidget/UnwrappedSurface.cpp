#include "UnwrappedSurface.h"
#include "GLColor.h"
#include "GL3DWidget.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"

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

UnwrappedSurface::UnwrappedSurface(const GLActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis):
    m_rootActor(rootActor),
    m_pos(origin),
    m_zaxis(axis),
    m_u_min(DBL_MAX),
    m_u_max(-DBL_MAX),
    m_v_min(DBL_MAX),
    m_v_max(-DBL_MAX),
    m_unwrappedImage(NULL),
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
}

void UnwrappedSurface::init()
{
  // the actor calls this->callback for each detector
  m_rootActor->detectorCallback(this);
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
}

/**
  * Draw the unwrapped instrument onto the screen
  * @param widget The widget to draw it on.
  */
void UnwrappedSurface::draw(GL3DWidget *widget)
{
  int vwidth,vheight;
  widget->getViewport(vwidth,vheight);
  const double dw = fabs(m_unwrappedView.width() / vwidth);
  const double dh = fabs(m_unwrappedView.height()/ vheight);

  if (!m_unwrappedImage || m_unwrappedImage->width() != vwidth || m_unwrappedImage->height() != vheight)
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

      glColor3ubv(&udet.color[0]);

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

    if (m_unwrappedImage)
    {
      delete m_unwrappedImage;
    }
    m_unwrappedImage = new QImage(widget->grabFrameBuffer());
    //m_unwrappedImage->save("C:\\Documents and Settings\\hqs74821\\Desktop\\tmp\\test\\Unwrapped.png");
    m_unwrappedViewChanged = false;
    widget->swapBuffers();
  }
  else
  {
    QPainter painter(widget);
    painter.drawImage(0,0,*m_unwrappedImage);
    // draw the selection rectangle
    if (!m_selectRect.isNull())
    {
      painter.setPen(Qt::blue);
      //painter.setCompositionMode(QPainter::CompositionMode_Xor);
      painter.drawRect(m_selectRect);
    }
    painter.end();
  }
}

void UnwrappedSurface::calcSize(UnwrappedDetector& udet,const Mantid::Geometry::V3D& X,
              const Mantid::Geometry::V3D& Y)
{
  Mantid::Geometry::Quat R;
  this->calcRot(udet,R);

  Mantid::Geometry::BoundingBox bbox = udet.detector->shape()->getBoundingBox();
  Mantid::Geometry::V3D scale = udet.detector->getScaleFactor();

  udet.minPoint = bbox.minPoint();
  udet.maxPoint = bbox.maxPoint();

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


void UnwrappedSurface::startUnwrappedSelection(int x,int y)
{
  m_selectRect.setRect(x,y,1,1);
}

void UnwrappedSurface::moveUnwrappedSelection(int x,int y)
{
  m_selectRect.setBottomRight(QPoint(x,y));
}

void UnwrappedSurface::endUnwrappedSelection(int x,int y)
{
  if (!m_selectRect.isNull())
  {
    zoomUnwrapped();
    m_selectRect = QRect();
  }
}

void UnwrappedSurface::zoomUnwrapped()
{
  if (!m_unwrappedImage) return;
  double x_min = double(m_selectRect.left())/m_unwrappedImage->width();
  double x_size = double(m_selectRect.width())/m_unwrappedImage->width();
  double y_min = double(/*m_selectRect.height() - */m_selectRect.top())/m_unwrappedImage->height();
  double y_size = double(m_selectRect.height())/m_unwrappedImage->height();

  x_min = m_unwrappedView.left() + x_min * m_unwrappedView.width();
  x_size = x_size * m_unwrappedView.width();
  y_min = m_unwrappedView.top() + y_min * m_unwrappedView.height();
  y_size = y_size * m_unwrappedView.height();

  m_zoomStack.push(m_unwrappedView);
  m_unwrappedView.setRect(x_min,y_min,x_size,y_size);
  m_unwrappedViewChanged = true;
}

void UnwrappedSurface::unzoomUnwrapped()
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
