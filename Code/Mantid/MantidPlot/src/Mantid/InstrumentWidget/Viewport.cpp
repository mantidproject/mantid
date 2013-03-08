#include "Viewport.h"
#include <math.h>
#include <iostream>
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidKernel/V3D.h"
#include "OpenGLError.h"

/**
 * Initialize with defaults.
 */
Viewport::Viewport(int w, int h, ProjectionType type):
m_projectionType( type ),
m_width(w), m_height(h),
m_left(-1), m_right(1), m_bottom(-1), m_top(1), m_near(-1), m_far(1),
m_rotationspeed(180.0 / M_PI),
m_zoomFactor(1.0),
m_xTrans(0.0),
m_yTrans(0.0)
{
  m_quaternion.GLMatrix( &m_rotationmatrix[0] );
}

/**
 * Resize the viewport.
 * @param w :: New viewport width in pixels.
 * @param h :: New viewport height in pixels.
 */
void Viewport::resize(int w, int h)
{
    m_width = w;
    m_height = h;
}

/**
 * Get the size of the viewport in screen pixels.
 * @param w :: Buffer to accept the viewport width value.
 * @param h :: Buffer to accept the viewport height value.
 */
void Viewport::getViewport(int& w, int& h) const
{
    w = m_width;
    h = m_height;
}

/**
 * This will set the projection to Ortho
 *
 * @param l :: left side of the Ortho projection (xmin)
 * @param r :: right side of the Ortho projection (xmax)
 * @param b :: bottom side of the Ortho projection (ymin)
 * @param t :: top side of the Ortho projection (ymax)
 * @param nearz :: near side of the Ortho Projection (zmin)
 * @param farz :: far side of the Ortho Projection (zmax)
 */
void Viewport::setProjection(double l, double r, double b, double t, double nearz, double farz, Viewport::ProjectionType type)
{
	m_projectionType = type;
	m_left = l;
	m_right = r;
	if (m_left > m_right) std::swap(m_left, m_right);
	m_bottom = b;
	m_top = t;
	if (m_bottom > m_top) std::swap(m_bottom, m_top);
  m_near = nearz;
  m_far = farz;
  
}

/**
 * Return XY plane bounds corrected for the aspect ratio.
 */
void Viewport::correctForAspectRatio(double& xmin, double& xmax, double& ymin, double& ymax)const
{
  xmin = m_left;
  xmax = m_right;
  ymin = m_bottom;
  ymax = m_top;
  // check if the scene is going to be stretched anlong x or y axes
  // and correct the extent to make it loook normal
  double xSize = m_right - m_left;
  double ySize = m_top - m_bottom;
  double r = ySize * m_width / ( xSize * m_height );
  if ( r < 1.0 )
  {
    // ySize is too small
    ySize /= r;
    ymin = (m_bottom + m_top - ySize) / 2;
    ymax = ymin + ySize;
  }
  else
  {
    // xSize is too small
    xSize *= r;
    xmin = (m_left + m_right - xSize) / 2;
    xmax = xmin + xSize;
  }
}

Viewport::ProjectionType Viewport::getProjectionType()const
{
	return m_projectionType;
}

void Viewport::getInstantProjection(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax)const
{
	xmin = m_left;
	xmax = m_right;
	ymin = m_bottom;
	ymax = m_top;
	zmin = m_near;
	zmax = m_far;
}


//void GLViewport::setZoomFactor(double val)
//{
//	mZoomFactor=val;
//	issueGL();
//}
//
//double GLViewport::getZoomFactor()
//{
//	return mZoomFactor;
//}
//
//void GLViewport::setTranslation(double xval,double yval)
//{
//	mXTrans=xval;
//	mYTrans=yval;
//	issueGL();
//}
//void GLViewport::getTranslation(double& xval,double& yval)
//{
//	xval=mXTrans;
//	yval=mYTrans;
//}

/** 
 * Issue the OpenGL commands that define the viewport and projection. 
 */
void Viewport::applyProjection() const
{
  glViewport(0, 0, m_width, m_height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  OpenGLError::check("GLViewport::issueGL()");

  double xmin, xmax, ymin, ymax;
  correctForAspectRatio(xmin, xmax, ymin, ymax);

  if(m_projectionType == Viewport::PERSPECTIVE)
  {
    glFrustum( xmin, xmax, ymin, ymax, m_near, m_far );

    if (OpenGLError::hasError("GLViewport::issueGL()"))
    {
      OpenGLError::log() << "Arguments to glFrustum:\n";
      OpenGLError::log() 
                         << xmin << ' ' << xmax << '\n'
                         << ymin << ' ' << ymax << '\n'
                         << m_near << ' ' << m_far << "\n\n";
    }
  }
  else
  {
    glOrtho(xmin, xmax, ymin, ymax, m_near, m_far );

    if (OpenGLError::hasError("GLViewport::issueGL()"))
    {
      OpenGLError::log() << "Arguments to glOrtho:\n";
      OpenGLError::log() 
                         << xmin << ' ' << xmax << '\n'
                         << ymin << ' ' << ymax << '\n'
                         << m_near << ' ' << m_far << "\n\n";
    }
  }
  // Reset the rendering options just in case
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}


/**
 * A point on the screen is projected onto a sphere with center at the rotation point
 * @param a :: The x screen coordinate in pixels
 * @param b :: The y screen coordinate in pixels
 * @param point :: The projection point on the sphere in model coordinates 
 */
void Viewport::projectOnSphere(int a,int b,Mantid::Kernel::V3D& point) const
{
  // z initiaised to zero if out of the sphere
  double z = 0;
  double x = static_cast<double>( (2.0 * a - m_width) / m_width );
  double y = static_cast<double>( (m_height - 2.0 * b) / m_height );
  double norm = x * x + y * y;
  if (norm > 1.0) // The point is inside the sphere
  {
    norm = sqrt( norm );
    x /= norm;
    y /= norm;
  }
  else // The point is outside the sphere, so project to nearest point on circle
    z = sqrt( 1.0 - norm );
  // Set-up point
  point( x, y, z);
}

void Viewport::applyRotation() const
{
  // Rotate with respect to the centre
  glMultMatrixd( m_rotationmatrix );
  // Zoom
  glScaled( m_zoomFactor, m_zoomFactor, m_zoomFactor );

  OpenGLError::check("GLTrackball::IssueRotation()");
}

/// Clear all transforamtions (rotation, translation. scaling)
void Viewport::reset()
{
  m_quaternion.init();
}

void Viewport::setViewToXPositive()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(1.0,0.0,0.0));
  m_quaternion=tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

void Viewport::setViewToYPositive()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(0.0,1.0,0.0));
  m_quaternion=tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

void Viewport::setViewToZPositive()
{
  reset();
  m_quaternion.init();
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

void Viewport::setViewToXNegative()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(-1.0,0.0,0.0));
  m_quaternion=tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

void Viewport::setViewToYNegative()
{
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0,0.0,1.0),Mantid::Kernel::V3D(0.0,-1.0,0.0));
  m_quaternion=tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

void Viewport::setViewToZNegative()
{
  reset();
  Mantid::Kernel::Quat tempy(180.0,Mantid::Kernel::V3D(0.0,1.0,0.0));
  m_quaternion=tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

void Viewport::initZoomFrom( int a, int b )
{
  if (a <= 0 || b <= 0) return;
  if( a >= m_width || b >= m_height ) return;
  double z = 0;
  double x = static_cast<double>( m_width - a );
  double y = static_cast<double>( b- m_height );
  m_lastpoint( x, y, z);
}

void Viewport::generateZoomTo(int a, int b)
{
  if ( a >= m_width || b >= m_height || a <= 0 || b <= 0 ) return;
  double y = static_cast<double>( b - m_height );
  if ( y == 0 ) y = m_lastpoint[1];
  double diff = m_lastpoint[1] / y;
  m_zoomFactor /= diff;
}

/**
 * Generate zooming factor using mouse wheel
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 * @param d :: The mouse wheel delta
 */
void Viewport::wheelZoom( int a, int b, int d)
{
  // Unused but can be used in the future
  (void)a;
  (void)b;
  double diff = 1.0 + 12.0 / d;
  m_zoomFactor *= diff;
}

/**
 * Start a trackball rotation from here.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::initRotationFrom(int a,int b)
{
  projectOnSphere(a,b,m_lastpoint);
}

/**
 * Generate the rotation matrix to rotate to this point.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::generateRotationTo(int a,int b)
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

/**
 * Initialize scene translation at a point on the screen
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::initTranslateFrom(int a,int b)
{
  generateTranslationPoint(a, b, m_lastpoint);
}

/**
 * Generate scene translation such that a point of the last initTranslateFrom
 * moved to the new position pointed by the mouse.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::generateTranslationTo(int a, int b)
{
  Mantid::Kernel::V3D newpoint;
  generateTranslationPoint(a, b, newpoint);
  // This is now the difference
  newpoint -= m_lastpoint;
  m_xTrans += newpoint[0];
  m_yTrans += newpoint[1];
}

void Viewport::generateTranslationPoint(int a, int b, Mantid::Kernel::V3D& point)const
{
  double x,y,z=0.0;
  double xmin,xmax,ymin,ymax,zmin,zmax;
  zmin = m_near;
  zmax = m_far;
  correctForAspectRatio(xmin,xmax,ymin,ymax);
  x=static_cast<double>((xmin+((xmax-xmin)*((double)a/(double)m_width))));
  y=static_cast<double>((ymin+((ymax-ymin)*(m_height-b)/m_height)));
  double factor=m_zoomFactor;
  x*=factor;
  y*=factor;
  // Assign new values to point
  point(x,y,z);
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << a << ' ' << xmin << ' '<< xmax << std::endl;
  std::cerr << m_width << ' ' << factor << std::endl;
  std::cerr << point << std::endl;
}


