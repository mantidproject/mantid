#include "GLTrackball.h"
#include "Viewport.h"
#include "OpenGLError.h"

#include "MantidGeometry/Rendering/OpenGL_Headers.h"

#include <cmath>

GLTrackball::GLTrackball(Viewport* viewport):m_viewport(viewport)
{
  reset();
  // Rotation speed defines as such is equal 1 in relative units,
  // i.e. the trackball will follow exactly the displacement of the mouse
  // on the sceen. The factor 180/M_PI is simply rad to deg conversion. THis
  // prevent recalculation of this factor every time a generateRotationTo call is issued.
  m_rotationspeed = 180 / M_PI;
  m_modelCenter = Mantid::Kernel::V3D(0.0, 0.0, 0.0);
  m_hasOffset = false;
}

void GLTrackball::initRotationFrom(int a,int b)
{
  projectOnSphere(a,b,m_lastpoint);
}

void GLTrackball::generateRotationTo(int a,int b)
{
  Mantid::Kernel::V3D newpoint;
  projectOnSphere( a, b, newpoint );
  Mantid::Kernel::V3D diff( m_lastpoint );
  // Difference between old point and new point
  diff -= newpoint;
  // Angle is given in degrees as the dot product of the two vectors
  double angle = m_rotationspeed * newpoint.angle( m_lastpoint );
  diff = m_lastpoint.cross_prod( newpoint );
  // Create a quaternion from the angle and vector direction
  Mantid::Kernel::Quat temp( angle, diff );
  // Left multiply
  temp *= m_quaternion;
  // Assignment of _quaternion
  m_quaternion( temp );
  // Get the corresponding OpenGL rotation matrix
  m_quaternion.GLMatrix( &m_rotationmatrix[0] );
}

void GLTrackball::initTranslateFrom(int a,int b)
{
  generateTranslationPoint(a, b, m_lastpoint);
}

void GLTrackball::generateTranslationTo(int a, int b)
{
  Mantid::Kernel::V3D newpoint;
  generateTranslationPoint(a, b, newpoint);
  // This is now the difference
  newpoint -= m_lastpoint;
  double x, y;
  m_viewport->getTranslation( x, y );
  m_viewport->setTranslation( x + newpoint[0], y + newpoint[1] );
}

void GLTrackball::initZoomFrom( int a, int b )
{
  if (a<=0 || b<=0)
    return;
  double x,y,z=0;
  int _viewport_w, _viewport_h;
  _viewport->getViewport(&_viewport_w,&_viewport_h);
  if(a>=_viewport_w || b>=_viewport_h)
    return;
  x=static_cast<double>((_viewport_w-a));
  y=static_cast<double>((b-_viewport_h));
  m_lastpoint(x,y,z);
}

void GLTrackball::generateZoomTo(int a, int b)
{
  double y=0;
  int _viewport_w, _viewport_h;
  _viewport->getViewport(&_viewport_w,&_viewport_h);
  if(a>=_viewport_w || b>=_viewport_h||a <= 0||b<=0)return;
  y=static_cast<double>((b-_viewport_h));
  if(y==0) y=m_lastpoint[1];
  double diff= m_lastpoint[1]/y ;
  diff*=_viewport->getZoomFactor();
  _viewport->setZoomFactor(diff);
}


void GLTrackball::IssueRotation() const
{
  if (m_viewport)
  {
    // Translate if offset is defined
    if (m_hasOffset)
    {
      glTranslated( m_modelCenter[0], m_modelCenter[1], m_modelCenter[2] );
    }
    // Rotate with respect to the centre
    glMultMatrixd( m_rotationmatrix );
    // Translate back
    if ( m_hasOffset )
      glTranslated( - m_modelCenter[0], - m_modelCenter[1], - m_modelCenter[2] );
  }

  OpenGLError::check("GLTrackball::IssueRotation()");
  return;
}

void GLTrackball::setModelCenter(const Mantid::Kernel::V3D& center)
{
  m_modelCenter = center;
  if ( m_modelCenter.nullVector() )
    m_hasOffset=false;
  else
    m_hasOffset=true;
}

Mantid::Kernel::V3D GLTrackball::getModelCenter() const
{
  return m_modelCenter;
}

void GLTrackball::projectOnSphere(int a,int b,Mantid::Kernel::V3D& point)
{
  // z initiaised to zero if out of the sphere
  double x,y,z=0;
  int _viewport_w, _viewport_h;
  m_viewport->getViewport(_viewport_w, _viewport_h);
  x=static_cast<double>((2.0*a-_viewport_w)/_viewport_w);
  y=static_cast<double>((_viewport_h-2.0*b)/_viewport_h);
  double norm=x*x+y*y;
  if (norm>1.0) // The point is inside the sphere
  {
    norm=sqrt(norm);
    x/=norm;
    y/=norm;
  }
  else // The point is outside the sphere, so project to nearest point on circle
    z=sqrt(1.0-norm);
  // Set-up point
  point(x,y,z);
}

void GLTrackball::generateTranslationPoint(int a,int b,Mantid::Kernel::V3D& point)
{
  double x,y,z=0.0;
  int _viewport_w, _viewport_h;
  double xmin,xmax,ymin,ymax,zmin,zmax;
  _viewport->getViewport(&_viewport_w,&_viewport_h);
  _viewport->getProjection(xmin,xmax,ymin,ymax,zmin,zmax);
  x=static_cast<double>((xmin+((xmax-xmin)*((double)a/(double)_viewport_w))));
  y=static_cast<double>((ymin+((ymax-ymin)*(_viewport_h-b)/_viewport_h)));
  double factor=_viewport->getZoomFactor();
  x*=factor;
  y*=factor;
  // Assign new values to point
  point(x,y,z);
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << a << ' ' << xmin << ' '<< xmax << std::endl;
  std::cerr << _viewport_w << ' ' << factor << std::endl;
  std::cerr << point << std::endl;
}

void GLTrackball::setRotationSpeed(double r)
{
  // Rotation speed needs to contains conversion to degrees.
  //
  if ( r > 0 ) m_rotationspeed = r * 180.0 / M_PI;
}

void GLTrackball::reset()
{
  //Reset rotation,scale and translation
  m_quaternion.init();
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  m_viewport->setTranslation(0.0,0.0);
  m_viewport->setZoomFactor(1.0);
}

void GLTrackball::setViewToXPositive()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(1.0,0.0,0.0));
  _quaternion=tempy;
  _quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToYPositive()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(0.0,1.0,0.0));
  _quaternion=tempy;
  _quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToZPositive()
{
  reset();
  _quaternion.init();
  _quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToXNegative()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(-1.0,0.0,0.0));
  _quaternion=tempy;
  _quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToYNegative()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(0.0,-1.0,0.0));
  _quaternion=tempy;
  _quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToZNegative()
{
  reset();
  Mantid::Kernel::Quat tempy(180.0,Mantid::Kernel::V3D(0.0,1.0,0.0));
  _quaternion=tempy;
  _quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::rotateBoundingBox(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax)
{
  // remove offset
  xmin-=_modelCenter[0];ymin-=_modelCenter[1];zmin-=_modelCenter[2];
  xmax-=_modelCenter[0];ymax-=_modelCenter[1];zmax-=_modelCenter[2];
  // Get the new bounding box
  _quaternion.rotateBB(xmin,ymin,zmin,xmax,ymax,zmax);
  // Re-apply offset
  xmin+=_modelCenter[0];ymin+=_modelCenter[1];zmin+=_modelCenter[2];
  xmax+=_modelCenter[0];ymax+=_modelCenter[1];zmax+=_modelCenter[2];
  return;
}

void GLTrackball::setRotation(const Mantid::Kernel::Quat& quat)
{
  _quaternion=quat;
  _quaternion.GLMatrix(&_rotationmatrix[0]);
}
