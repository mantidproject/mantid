#include <ostream>
#include <stdexcept>
#include <cmath> 
#include <vector>


// THIS IS VERY ERROR PRONE :: USE "V3D.h" and -I flag.
#include "../inc/V3D.h"

namespace Mantid
{
namespace Geometry
{

const double precision(1e-7);   

V3D::V3D():x(0),y(0),z(0)
  /// Constructor [Null]
{}

V3D::V3D(const double xx, const double yy, const double zz) :
  x(xx),y(yy),z(zz)
  /// Value constructor
{}

V3D::V3D(const V3D& v):x(v.x),y(v.y),z(v.z)
  /// Copy constructor
{}

V3D& 
V3D::operator=(const V3D& A)
  /*!
    Assignment operator
    \param v :: V3D to copy 
    \return *this
  */
{
  if (this!=&A)
    {
      x=A.x;
      y=A.y;
      z=A.z;
    }
  return *this;
}

V3D::V3D(const double* vPtr)  
  /*!
    Constructor from a pointer.
    requires that the point is assigned after this has
    been allocated since vPtr[x] may throw.
  */
{
  if (vPtr)
    {
      x=vPtr[0];
      y=vPtr[1];
      z=vPtr[2];
    }
}

V3D::~V3D()
  /// Destructor
{}

V3D 
V3D::operator+(const V3D& v) const
  /*
    Addtion operator
     \param v :: Vector to add
     \return *this+v;
  */
{
  V3D out(*this);
  out+=v;
  return out;
}

V3D 
V3D::operator-(const V3D& v) const
  /*
    Subtraction operator
    \param v :: Vector to sub.
    \return *this-v;
  */
{
  V3D out(*this);
  out-=v;
  return out;
}

V3D 
V3D::operator*(const V3D& v) const
  /*
    Inner product
    \param v :: Vector to sub.
    \return *this * v;
  */
{
  V3D out(*this);
  out*=v;
  return out;
}

V3D 
V3D::operator/(const V3D& v) const
  /*
    Inner division
    \param v :: Vector to divide
    \return *this * v;
  */
{
  V3D out(*this);
  out/=v;
  return out;
}

V3D& 
V3D::operator+=(const V3D& v) 
  /*
    Self-Addition operator
    \param v :: Vector to add.
    \return *this+=v;
  */
{
  x+=v.x;
  y+=v.y;
  z+=v.z;
  return *this;
}

V3D& 
V3D::operator-=(const V3D& v) 
  /*
    Self-Subtraction operator
    \param v :: Vector to sub.
    \return *this-v;
  */
{
  x-=v.x;
  y-=v.y;
  z-=v.z;
  return *this;
}

V3D& 
V3D::operator*=(const V3D& v) 
  /*
    Self-Inner product
    \param v :: Vector to multiply
    \return *this*=v;
  */
{
  x*=v.x;
  y*=v.y;
  z*=v.z;
  return *this;
}

V3D& 
V3D::operator/=(const V3D& v) 
  /*
    Self-Inner division
    \param v :: Vector to divide
    \return *this*=v;
  */
{
  x/=v.x;
  y/=v.y;
  z/=v.z;
  return *this;
}

V3D 
V3D::operator*(const double D) const
  /*!
    Scalar product
    \param D :: value to scale
    \return this * D
   */
{
  V3D out(*this);
  out*=D;
  return out;
}

V3D 
V3D::operator/(const double D) const
  /*!
    Scalar divsion
    \param D :: value to scale
    \return this / D
  */
{
  V3D out(*this);
  out/=D;
  return out;
}

V3D& 
V3D::operator*=(const double D)
  /*!
    Scalar product
    \param D :: value to scale
    \return this *= D
  */
{
  x*=D;
  y*=D;
  z*=D;
  return *this;
}

V3D& 
V3D::operator/=(const double D) 
  /*!
    Scalar division
    \param D :: value to scale
    \return this /= D
    \todo ADD PRECISION
  */
{
  if (D!=0.0)
    {
      x/=D;
      y/=D;
      z/=D;
    }
  return *this;
}

bool 
V3D::operator==(const V3D& v) const
  /*!
    Equals operator with tolerance factor
    \param v :: V3D for comparison
  */
{
  return (fabs(x-v.x)>precision ||
	  fabs(y-v.y)>precision ||
	  fabs(z-v.z)>precision)  ?
    false : true;
}

bool 
V3D::operator<(const V3D& V) const
  /*!
    \tood ADD PRCESSION
   */
{
  if (x!=V.x)
    return x<V.x;
  if (y!=V.y)
    return y<V.y;
  return z<V.z;
}

void 
V3D::operator()(const double xx, const double yy, const double zz)
{
  x=xx;
  y=yy;
  z=zz;
  return;
}

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

double 
V3D::norm() const
  /*!
    Vector length
    \return vec.length()
  */
{
  return sqrt(x*x+y*y+z*z);
}

double 
V3D::norm2() const
{
	return (x*x+y*y+z*z);
}

double
V3D::normalize()
  /*!
    Normalises the vector and 
    then returns the scalar value of the vector
    \return Norm
  */
{
  const double ND(norm());
  this->operator/=(ND);
  return ND;
}

double 
V3D::scalar_prod(const V3D& V) const
{
  return (x*V.x+y*V.y+z*V.z);
}

V3D 
V3D::cross_prod(const V3D& v) const
{
  V3D out;
  out.x=y*v.z-z*v.y;
  out.y=z*v.x-x*v.z;
  out.z=x*v.y-y*v.x;
  return out;
}

double 
V3D::distance(const V3D& v) const
{
  V3D dif(*this);
  dif-=v;
  return dif.norm();
}

void 
V3D::printSelf(std::ostream& os) const
{
  os << "[" << x << "," << y << "," << z << "]";
  return;
}

std::ostream& 
operator<<(std::ostream& os, const V3D& v)
{
  v.printSelf(os);
  return os;
}

} // Namespace Geometry
} // Namespace Mantid
