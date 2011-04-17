#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>

#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Tolerance.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Geometry
{

/// Constructor [Null]
V3D::V3D():x(0),y(0),z(0)
{}

/// Value constructor
V3D::V3D(const double xx, const double yy, const double zz) :
  x(xx),y(yy),z(zz)
{}

/// Copy constructor
V3D::V3D(const V3D& v):x(v.x),y(v.y),z(v.z)
{}

/**
  Sets the vector position based on spherical coordinates

  @param R :: The R value (distance)
  @param theta :: The theta value (in degrees) = the polar angle away from the +Z axis.
  @param phi :: The phi value (in degrees) = the azimuthal angle, where 0 points along +X and rotates counter-clockwise in the XY plane
*/
void V3D::spherical(const double& R, const double& theta, const double& phi)
{
const double deg2rad=M_PI/180.0;
z=R*cos(theta*deg2rad);
const double ct=sin(theta*deg2rad);
x=R*ct*cos(phi*deg2rad);
y=R*ct*sin(phi*deg2rad);

// Setting this way can lead to very small values of x & y that should really be zero.
// This can cause confusion for the atan2 function used in getSpherical.
if (std::abs(x) < Tolerance) x = 0.0;
if (std::abs(y) < Tolerance) y = 0.0;
}

/**
  Sets the vector position based on spherical coordinates, in radians

  @param R :: The R value (distance)
  @param polar :: the polar angle (in radians) away from the +Z axis.
  @param azimuth :: the azimuthal angle (in radians), where 0 points along +X and rotates counter-clockwise in the XY plane
*/
void V3D::spherical_rad(const double& R, const double& polar, const double& azimuth)
{
z=R*cos(polar);
const double ct=R*sin(polar);
x=ct*cos(azimuth);
y=ct*sin(azimuth);

// Setting this way can lead to very small values of x & y that should really be zero.
// This can cause confusion for the atan2 function used in getSpherical.
if (std::abs(x) < Tolerance) x = 0.0;
if (std::abs(y) < Tolerance) y = 0.0;
}


/**
  Sets the vector position based on azimuth and polar angle, in RADIANS, in the SNS instrument coordinate system,
    where +Z = beam direction, +Y = vertical.

  @param R :: The R value (distance)
  @param azimuth :: The azimuthal angle (in Radians)
  @param polar :: The polar value (in Radians)
*/

void V3D::azimuth_polar_SNS(const double& R, const double& azimuth, const double& polar)
{
  y=R*cos(polar);
  const double ct=R*sin(polar);
  x=ct*cos(azimuth);
  z=ct*sin(azimuth);

  // Setting this way can lead to very small values of x & y that should really be zero.
  // This can cause confusion for the atan2 function used in getSpherical.
  if (std::abs(x) < Tolerance) x = 0.0;
  if (std::abs(y) < Tolerance) y = 0.0;
  if (std::abs(z) < Tolerance) z = 0.0;
}


  /**
    Assignment operator
    @param rhs :: V3D to copy
    @return *this
  */
V3D& V3D::operator=(const V3D& rhs)
{
  x = rhs.x;
  y = rhs.y;
  z = rhs.z;
  return *this;
}

/**
  Constructor from a pointer.
  requires that the point is assigned after this has
  been allocated since vPtr[x] may throw.
*/
V3D::V3D(const double* vPtr)

{
  if (vPtr)
    {
      x=vPtr[0];
      y=vPtr[1];
      z=vPtr[2];
    }
}

  /// Destructor
V3D::~V3D()
{}

  /**
    Addtion operator
     @param v :: Vector to add
     @return *this+v;
  */
V3D
V3D::operator+(const V3D& v) const
{
  V3D out(*this);
  out+=v;
  return out;
}

  /**
    Subtraction operator
    @param v :: Vector to sub.
    @return *this-v;
  */
V3D
V3D::operator-(const V3D& v) const
{
  V3D out(*this);
  out-=v;
  return out;
}

  /**
    Inner product
    @param v :: Vector to sub.
    @return *this * v;
  */
V3D
V3D::operator*(const V3D& v) const
{
  V3D out(*this);
  out*=v;
  return out;
}

  /**
    Inner division
    @param v :: Vector to divide
    @return *this * v;
  */
V3D
V3D::operator/(const V3D& v) const
{
  V3D out(*this);
  out/=v;
  return out;
}

  /**
    Self-Addition operator
    @param v :: Vector to add.
    @return *this+=v;
  */
V3D&
V3D::operator+=(const V3D& v)
{
  x+=v.x;
  y+=v.y;
  z+=v.z;
  return *this;
}

  /**
    Self-Subtraction operator
    @param v :: Vector to sub.
    @return *this-v;
  */
V3D&
V3D::operator-=(const V3D& v)
{
  x-=v.x;
  y-=v.y;
  z-=v.z;
  return *this;
}

  /**
    Self-Inner product
    @param v :: Vector to multiply
    @return *this*=v;
  */
V3D&
V3D::operator*=(const V3D& v)
{
  x*=v.x;
  y*=v.y;
  z*=v.z;
  return *this;
}

  /**
    Self-Inner division
    @param v :: Vector to divide
    @return *this*=v;
  */
V3D&
V3D::operator/=(const V3D& v)
{
  x/=v.x;
  y/=v.y;
  z/=v.z;
  return *this;
}

  /**
    Scalar product
    @param D :: value to scale
    @return this * D
   */
V3D
V3D::operator*(const double D) const
{
  V3D out(*this);
  out*=D;
  return out;
}

  /**
    Scalar divsion
    @param D :: value to scale
    @return this / D
  */
V3D
V3D::operator/(const double D) const
{
  V3D out(*this);
  out/=D;
  return out;
}

  /**
    Scalar product
    @param D :: value to scale
    @return this *= D
  */
V3D&
V3D::operator*=(const double D)
{
  x*=D;
  y*=D;
  z*=D;
  return *this;
}

  /**
    Scalar division
    @param D :: value to scale
    @return this /= D
    \todo ADD TOLERANCE
  */
V3D&
V3D::operator/=(const double D)
{
  if (D!=0.0)
    {
      x/=D;
      y/=D;
      z/=D;
    }
  return *this;
}

  /**
    Equals operator with tolerance factor
    @param v :: V3D for comparison
    @return true if the items are equal
  */
bool
V3D::operator==(const V3D& v) const
{
  using namespace std;
  return (fabs(x-v.x)>Tolerance ||
    fabs(y-v.y)>Tolerance ||
    fabs(z-v.z)>Tolerance)  ?
    false : true;
}

/** Not equals operator with tolerance factor.
 *  @param other :: The V3D to compare against
 *  @returns True if the vectors are different
 */
bool V3D::operator!=(const V3D& other) const
{
  return !(this->operator==(other));
}

  /**
    compare
    @return true if V is greater
   */
bool
V3D::operator<(const V3D& V) const
{
  if (x!=V.x)
    return x<V.x;
  if (y!=V.y)
    return y<V.y;
  return z<V.z;
}

  /**
    Sets the vector position from a triplet of doubles x,y,z
    @param xx :: The X coordinate
    @param yy :: The Y coordinate
    @param zz :: The Z coordinate
  */
void
V3D::operator()(const double xx, const double yy, const double zz)
{
  x=xx;
  y=yy;
  z=zz;
  return;
}

  /**
    Set is x position
    @param xx :: The X coordinate
  */
void V3D::setX(const double xx)
{
  x=xx;
}

  /**
    Set is y position
    @param yy :: The Y coordinate
  */
void V3D::setY(const double yy)
{
  y=yy;
}

  /**
    Set is z position
    @param zz :: The Z coordinate
  */
void V3D::setZ(const double zz)
{
  z=zz;
}

  /**
    Returns the axis value based in the index provided
    @param Index :: 0=x, 1=y, 2=z
    @return a double value of the requested axis
  */
const double&
V3D::operator[](const int Index) const
{
  switch (Index)
    {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    default:
      throw Kernel::Exception::IndexError(Index,2,"V3D::operator[] range error");
    }
}

  /**
    Returns the axis value based in the index provided
    @param Index :: 0=x, 1=y, 2=z
    @return a double value of the requested axis
  */
double&
V3D::operator[](const int Index)
{
  switch (Index)
    {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    default:
      throw Kernel::Exception::IndexError(Index,2,"V3D::operator[] range error");
    }
}

/** Return the vector's position in spherical coordinates
 *  @param R ::     Returns the radial distance
 *  @param theta :: Returns the theta angle in degrees
 *  @param phi ::   Returns the phi (azimuthal) angle in degrees
 */
void V3D::getSpherical(double& R, double& theta, double& phi) const
{
  const double rad2deg = 180.0/M_PI;
  R = norm();
  theta = 0.0;
  if ( R ) theta = acos(z/R) * rad2deg;
  phi = atan2(y,x) * rad2deg;
  return;
}

  /**
    Vector length
    @return vec.length()
  */
double
V3D::norm() const
{
  return sqrt(x*x+y*y+z*z);
}

  /**
    Vector length without the sqrt
    @return vec.length()
  */
double
V3D::norm2() const
{
	return (x*x+y*y+z*z);
}

  /**
    Normalises the vector and
    then returns the scalar value of the vector
    @return Norm
  */
double
V3D::normalize()
{
  const double ND(norm());
  this->operator/=(ND);
  return ND;
}

  /**
    Calculates the scalar product
    @param V :: The second vector to include in the calculation
    @return The scalar product of the two vectors
  */
double
V3D::scalar_prod(const V3D& V) const
{
  return (x*V.x+y*V.y+z*V.z);
}

  /**
    Calculates the cross product. Returns (this * v).
    @param v :: The second vector to include in the calculation
    @return The cross product of the two vectors (this * v)
  */
V3D
V3D::cross_prod(const V3D& v) const
{
  return V3D(y*v.z-z*v.y, z*v.x-x*v.z,x*v.y-y*v.x);
}

  /**
    Calculates the distance between two vectors
    @param v :: The second vector to include in the calculation
    @return The distance between the two vectors
  */
double
V3D::distance(const V3D& v) const
{
  V3D dif(*this);
  dif-=v;
  return dif.norm();
}

/** Calculates the zenith angle (theta) of this vector with respect to another
 *  @param v :: The other vector
 *  @return The azimuthal angle in radians (0 < theta < pi)
 */
double V3D::zenith(const V3D& v) const
{
  double R = distance(v);
  double zOffset = z - v.z;
  if ( R )
  {
    return acos( zOffset / R );
  }
  else
  {
    return 0.0;
  }
}

/** Calculates the angle between this and another vector.
 *
 *  @param v :: The other vector
 *  @return The angle between the vectors in radians (0 < theta < pi)
 */
double V3D::angle(const V3D& v) const
{
  return acos( this->scalar_prod(v) / (this->norm() * v.norm()) );
}

int
V3D::reBase(const V3D& A,const V3D&B,const V3D& C)
  /**
     Re-express this point components of A,B,C.
     Assuming that A,B,C are form an basis set (which
     does not have to be othonormal.
     @param A :: Unit vector in basis
     @param B :: Unit vector in basis
     @param C :: Unit vector in basis
     @retval -1 :: The points do not form a basis set.
     @retval 0  :: Vec3D has successfully been re-expressed.
  */
{
  Matrix<double> T(3,3);
  for(int i=0;i<3;i++)
    {
      T[i][0]=A[i];
      T[i][1]=B[i];
      T[i][2]=C[i];
    }
  const double det=T.Invert();
  if (fabs(det)<1e-13)       // failed
    return -1;
  rotate(T);
  return 0;
}

void
V3D::rotate(const Geometry::Matrix<double>& A)
  /**
    Rotate a point by a matrix
    @param A :: Rotation matrix (needs to be >3x3)
  */
{
  Matrix<double> Pv(3,1);
  Pv[0][0]=x;
  Pv[1][0]=y;
  Pv[2][0]=z;
  Matrix<double> Po=A*Pv;
  x=Po[0][0];
  y=Po[1][0];
  z=Po[2][0];
  return;
}

/**
  Determines if this,B,C are collinear
  @param Bv :: Vector to test
  @param Cv :: Vector to test
  @return false is no colinear and true if they are (within Ptolerance)
*/
bool
V3D::coLinear(const V3D& Bv,const V3D& Cv) const
{
  const V3D& Av=*this;
  const V3D Tmp((Bv-Av).cross_prod(Cv-Av));
  return (Tmp.norm()>Tolerance) ? false : true;
}

bool
V3D::nullVector(const double Tol) const
  /**
    Checks the size of the vector
    @param Tol :: size of the biggest zero vector allowed.
    @retval 1 : the vector squared components
    magnitude are less than Tol
    @retval 0 :: Vector bigger than Tol
  */
{
  using namespace std;
  if (fabs(x)>Tol)
    return false;
  if (fabs(y)>Tol)
    return false;
  if (fabs(z)>Tol)
    return false;

  // Getting to this point means a null vector
  return true;
}

int
V3D::masterDir(const double Tol) const
  /**
     Calculates the index of the primary direction (if there is one)
     @param Tol :: Tolerance accepted
     @retval range -3,-2,-1 1,2,3  if the vector
     is orientaged within Tol on the x,y,z direction (the sign
     indecates the direction to the +ve side )
     @retval 0 :: No master direction
  */
{
  // Calc max dist
  double max=x*x;
  double other=max;
  double u2=y*y;
  int idx=(x>0) ? 1 : -1;
  if (u2>max)
    {
      max=u2;
      idx=(y>0) ? 2 : -2;
    }
  other+=u2;
  u2=z*z;
  if (u2>max)
    {
      max=u2;
      idx=(z>0) ? 3 : -3;
    }
  other+=u2;
  other-=max;
  if ((other/max)>Tol)    //doesn't have master direction
    {
      return 0;
    }
  return idx;
}

/**
  Read data from a stream.
  \todo Check Error handling
  @param IX :: Input Stream
*/
void
V3D::read(std::istream& IX)
{
  IX>>x>>y>>z;
  return;
}

void
V3D::write(std::ostream& OX) const
  /**
    Write out the point values
    @param OX :: Output stream
  */
{
  OX<<x<<" "<<y<<" "<<z;
  return;
}

  /**
    Prints a text representation of itself in format "[x,y,z]"
    @param os :: the Stream to output to
  */
void
V3D::printSelf(std::ostream& os) const
{
  os << "[" << x << "," << y << "," << z << "]";
  return;
}

/**
  Read data from a stream in the format returned by printSelf ("[x,y,z]").
  @param IX :: Input Stream
  @throw std::runtime_error if the input is of wrong format
*/
void
V3D::readPrinted(std::istream& IX)
{
    std::string in;
    std::getline(IX,in);
    size_t i = in.find_first_of('[');
    if (i == std::string::npos) throw std::runtime_error("Wrong format for V3D input: "+in);
    size_t j = in.find_last_of(']');
    if (j == std::string::npos || j < i + 6) throw std::runtime_error("Wrong format for V3D input: "+in);

    size_t c1 = in.find_first_of(',');
    size_t c2 = in.find_first_of(',',c1+1);
    if (c1 == std::string::npos || c2 == std::string::npos) throw std::runtime_error("Wrong format for V3D input: ["+in+"]");

    x = atof(in.substr(i+1,c1-i-1).c_str());
    y = atof(in.substr(c1+1,c2-c1-1).c_str());
    z = atof(in.substr(c2+1,j-c2-1).c_str());

    return;
}

  /**
    Prints a text representation of itself
    @param os :: the Stream to output to
    @param v :: the vector to output
    @return the output stream
    */
std::ostream&
operator<<(std::ostream& os, const V3D& v)
{
  v.printSelf(os);
  return os;
}

std::istream&
operator>>(std::istream& IX,V3D& A)
  /**
    Calls Vec3D method write to output class
    @param IX :: Input Stream
    @param A :: Vec3D to write
    @return Current state of stream
  */
{
  A.read(IX);
  return IX;
}

} // Namespace Geometry
} // Namespace Mantid
