#include <ostream>
#include <stdexcept>
#include <cmath> 
#include <vector>

#include "V3D.h"

namespace Mantid
{
namespace Geometry
{

  /// The default precision 1e-7
const double precision(1e-7);   

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

  /*!
    Sets the vector position based on spherical coordinates
    \param R :: The R value
    \param theta :: The theta value
    \param phi :: The phi value
  */
void V3D::spherical(double R, double theta, double phi)
{
	double deg2rad=M_PI/180.0;
	z=R*sin(theta*deg2rad);
	double ct=cos(theta*deg2rad);
	x=R*ct*cos(phi*deg2rad);
	y=R*ct*sin(phi*deg2rad);
}

  /*!
    Assignment operator
    \param v :: V3D to copy 
    \return *this
  */
V3D& 
V3D::operator=(const V3D& A)
{
  if (this!=&A)
    {
      x=A.x;
      y=A.y;
      z=A.z;
    }
  return *this;
}

/*!
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

  /*
    Addtion operator
     \param v :: Vector to add
     \return *this+v;
  */
V3D 
V3D::operator+(const V3D& v) const
{
  V3D out(*this);
  out+=v;
  return out;
}

  /*
    Subtraction operator
    \param v :: Vector to sub.
    \return *this-v;
  */
V3D 
V3D::operator-(const V3D& v) const
{
  V3D out(*this);
  out-=v;
  return out;
}

  /*
    Inner product
    \param v :: Vector to sub.
    \return *this * v;
  */
V3D 
V3D::operator*(const V3D& v) const
{
  V3D out(*this);
  out*=v;
  return out;
}

  /*
    Inner division
    \param v :: Vector to divide
    \return *this * v;
  */
V3D 
V3D::operator/(const V3D& v) const
{
  V3D out(*this);
  out/=v;
  return out;
}

  /*
    Self-Addition operator
    \param v :: Vector to add.
    \return *this+=v;
  */
V3D& 
V3D::operator+=(const V3D& v) 
{
  x+=v.x;
  y+=v.y;
  z+=v.z;
  return *this;
}

  /*
    Self-Subtraction operator
    \param v :: Vector to sub.
    \return *this-v;
  */
V3D& 
V3D::operator-=(const V3D& v) 
{
  x-=v.x;
  y-=v.y;
  z-=v.z;
  return *this;
}

  /*
    Self-Inner product
    \param v :: Vector to multiply
    \return *this*=v;
  */
V3D& 
V3D::operator*=(const V3D& v) 
{
  x*=v.x;
  y*=v.y;
  z*=v.z;
  return *this;
}

  /*
    Self-Inner division
    \param v :: Vector to divide
    \return *this*=v;
  */
V3D& 
V3D::operator/=(const V3D& v) 
{
  x/=v.x;
  y/=v.y;
  z/=v.z;
  return *this;
}

  /*!
    Scalar product
    \param D :: value to scale
    \return this * D
   */
V3D 
V3D::operator*(const double D) const
{
  V3D out(*this);
  out*=D;
  return out;
}

  /*!
    Scalar divsion
    \param D :: value to scale
    \return this / D
  */
V3D 
V3D::operator/(const double D) const
{
  V3D out(*this);
  out/=D;
  return out;
}

  /*!
    Scalar product
    \param D :: value to scale
    \return this *= D
  */
V3D& 
V3D::operator*=(const double D)
{
  x*=D;
  y*=D;
  z*=D;
  return *this;
}

  /*!
    Scalar division
    \param D :: value to scale
    \return this /= D
    \todo ADD PRECISION
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

  /*!
    Equals operator with tolerance factor
    \param v :: V3D for comparison
  */
bool 
V3D::operator==(const V3D& v) const
{
  return (fabs(x-v.x)>precision ||
	  fabs(y-v.y)>precision ||
	  fabs(z-v.z)>precision)  ?
    false : true;
}

  /*!
    \todo ADD PRCESSION
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

  /*!
    Sets the vector position from a triplet of doubles x,y,z
    \param xx The X coordinate
    \param yy The Y coordinate
    \param zz The Z coordinate
  */
void 
V3D::operator()(const double xx, const double yy, const double zz)
{
  x=xx;
  y=yy;
  z=zz;
  return;
}

  /*!
    Returns the axis value based in the index provided
    \param Index 0=x, 1=y, 2=z
    \returns a double value of the requested axis
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
      throw std::runtime_error("V3D::operator[] range error");
    }
}

  /*!
    Returns the axis value based in the index provided
    \param Index 0=x, 1=y, 2=z
    \returns a double value of the requested axis
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
      throw std::runtime_error("V3D::operator[] range error");
    }
}

  /*!
    Vector length
    \return vec.length()
  */
double 
V3D::norm() const
{
  return sqrt(x*x+y*y+z*z);
}

  /*!
    Vector length without the sqrt
    \return vec.length()
  */
double 
V3D::norm2() const
{
	return (x*x+y*y+z*z);
}

  /*!
    Normalises the vector and 
    then returns the scalar value of the vector
    \return Norm
  */
double
V3D::normalize()
{
  const double ND(norm());
  this->operator/=(ND);
  return ND;
}

  /*!
    Calculates the scalar product
    \param V The second vector to include in the calculation
    \return The scalar product of the two vectors
  */
double 
V3D::scalar_prod(const V3D& V) const
{
  return (x*V.x+y*V.y+z*V.z);
}

  /*!
    Calculates the cross product
    \param V The second vector to include in the calculation
    \return The cross product of the two vectors
  */
V3D 
V3D::cross_prod(const V3D& v) const
{
  V3D out;
  out.x=y*v.z-z*v.y;
  out.y=z*v.x-x*v.z;
  out.z=x*v.y-y*v.x;
  return out;
}

  /*!
    Calculates the distance between two vectors
    \param V The second vector to include in the calculation
    \return The distance between the two vectors
  */
double 
V3D::distance(const V3D& v) const
{
  V3D dif(*this);
  dif-=v;
  return dif.norm();
}

  /*!
    Prints a text representation of itself
    \param os the Stream to output to
  */
void 
V3D::printSelf(std::ostream& os) const
{
  os << "[" << x << "," << y << "," << z << "]";
  return;
}

  /*!
    Prints a text representation of itself
    \param os the Stream to output to
    \param v the vector to output
    \returns the output stream
    */
std::ostream& 
operator<<(std::ostream& os, const V3D& v)
{
  v.printSelf(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid
