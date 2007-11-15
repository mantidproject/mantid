#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#include "Logger.h"
#include "AuxException.h"
#include "Matrix.h"
#include "Vec3D.h"

namespace Mantid 
{

const double PTolerance(1e-8);   ///< Tolerance for a point

std::ostream& 
Geometry::operator<<(std::ostream& OX,const Geometry::Vec3D& A)
  /*!
    Calls Vec3D method write to output class
    \param of :: Output stream
    \param A :: Vec3D to write
    \return Current state of stream
  */
{
  A.write(OX);
  return OX;
}

std::istream& 
Geometry::operator>>(std::istream& IX,Geometry::Vec3D& A)
  /*!
    Calls Vec3D method write to output class
    \param of :: Output stream
    \param A :: Vec3D to write
    \return Current state of stream
  */
{
  A.read(IX);
  return IX;
}

namespace Geometry
{


Vec3D::Vec3D():
  x(0.0),y(0.0),z(0.0)
  /*!
    Origin Constructor
  */
{}

Vec3D::Vec3D(const double X,const double Y,const double Z) :
  x(X),y(Y),z(Z)
  /*!
    Constructor at Positon
    \param X :: x-coord
    \param Y :: y-coord
    \param Z :: z-coord
  */
{}

Vec3D::Vec3D(const double* xyz) :
  x(xyz[0]),y(xyz[1]),z(xyz[2])
  /*!
    Constructor from array
    \param xyz :: 3 Vec3D array
  */
{}

Vec3D::Vec3D(const Vec3D& A) :
  x(A.x),y(A.y),z(A.z)
  /*!
    Copy constructor
    \param A :: Vec3D to copy
  */
{}

Vec3D&
Vec3D::operator=(const Vec3D& A)
  /*!
    Assignment operator
    \param A :: Vec3D to copy
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

Vec3D::~Vec3D()
  /*!
    Standard Destructor
  */
{}

Vec3D
Vec3D::operator()(const double a,const double b,const double c) const
  /*!
    Operator() casts a set of double to a Vec3D
    Could be static ?
    \param a :: x-cord
    \param b :: y-cord
    \param c :: z-cord
    \return New point
  */
{
  Vec3D A(a,b,c);
  return A;
}

template<typename T>
Vec3D
Vec3D::operator()(const Matrix<T>& A) const
  /*!
    Operator() casts Matrix to a point
    The matrix needs to be 3x1 or 1x3
    \param A :: Matrix Item to be case
    \return Vec3D of matrix
  */
{
  std::pair<int,int> Asize=A.size();
  if (Asize.first*Asize.second<1)
    return Vec3D(0,0,0);
  Vec3D Out;
  if (Asize.first>Asize.second) 
    {
      for(int i=0;i<3;i++)
	Out[i]=(i<Asize.first) ? A[i][0] : 0.0;
    }
  else
    {
      for(int i=0;i<3;i++)
	Out[i]=(i<Asize.second) ? A[0][i] : 0.0;
    }
  return Out;
}

double&
Vec3D::operator[](const int A)
  /*!
    Operator [] isolates component based on 
    index A 
    \param A :: Index Item
    \throw IndexError :: A out of range
    \return reference to coordinate
  */
{
  switch (A)
    {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      throw ColErr::IndexError(A,2,"Vec3D::operator[]");
    }
  return z;
}

const double
Vec3D::operator[](const int A) const
  /*!
    Operator [] isolates component based on 
    index A
    \param A :: Index number (0-2) 
    \return value of the coordinate (z on error)
  */
{
  switch (A)
    {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      throw ColErr::IndexError(A,2,"Vec3D::operator[] const");
    }
  return z;
}

int
Vec3D::operator==(const Vec3D& A) const
  /*!
    Equality operator within tolerance
    \param A :: Vec3D to compare
    \return A==this
  */
{
  return (&A==this || Distance(A)<=PTolerance) ? 1 : 0;
}


Vec3D
Vec3D::operator*(const Vec3D& A) const
  /*!
    Cross product of this*A
    \param A :: Vec3D to take cross product from
    \returns cross product
  */  
{
  Vec3D X;
  X.x=y*A.z-z*A.y;
  X.y=z*A.x-x*A.z;
  X.z=x*A.y-y*A.x;
  return X;
}

template<typename T>
Vec3D
Vec3D::operator*(const Matrix<T>& A) const
  /*!
    Impliments a rotation 
    \param A :: Matrix to rotate by
    \returns Vec3D rotated by Matrix
  */
  
{
  Vec3D X(*this);
  X.rotate(A);
  return X;
}

Vec3D
Vec3D::operator*(const double V) const
  /*!
    Simple multiplication of this/Value 
    \param V :: scalar to multiply each component 
    \return Vec3D scaled by value
  */
{
  Vec3D X(*this);
  X*=V;
  return X;
}


Vec3D
Vec3D::operator/(const double V) const
  /*!
    Simple division of this/V 
    \param V :: Value to divide by.
    \returns Vec3D / Value
  */
{
  Vec3D X(*this);
  X/=V;
  return X;
}

Vec3D
Vec3D::operator+(const Vec3D& A) const
  /*! 
    Simple Vec3D addition
    \param A :: Vec3D to add
    \returns Vec3D + A
  */
{
  Vec3D X(*this);
  X+=A;
  return X;
}

Vec3D
Vec3D::operator-(const Vec3D& A) const
  /*!
    Simple Vec3D subtraction
    \param A : Vec3D to subtract
    \returns Vec3D - A
  */
{
  Vec3D X(*this);
  X-=A;
  return X;
}

Vec3D
Vec3D::operator-() const
  /*!
    Simple negation of the Vec3D
    \return -Vec3D
  */
{
  Vec3D X(*this);
  X *= -1.0;
  return X;
}

Vec3D&
Vec3D::operator*=(const Vec3D& A)
  /*!
    Cross produce of this*A
    \param A :: Vec3D 
    \return this X A
  */
{
  *this = this->operator*(A);
  return *this;
}

template<typename T> 
Vec3D&
Vec3D::operator*=(const Matrix<T>& A)
  /*!
    Rotate this by matrix A
    \param A :: Rotation Matrix (3x3)
    \return this after Rot
  */
{
  rotate(A);
  return *this;
}

Vec3D&
Vec3D::operator*=(const double V)
  /*!
    Vec3D multication by a value
    \param V :: Multiplication value
    \return *this * V
  */
{
  x*=V;
  y*=V;
  z*=V;
  return *this;
}

Vec3D&
Vec3D::operator/=(const double V)
  /*!
    Vec3D division by a value 
    (contains simple test for zero)
    \param V :: Value operator
    \return *this/V
  */
{
  if (V!=0.0)
    {
      x/=V;
      y/=V;
      z/=V;
    }
  return *this;
}

Vec3D&
Vec3D::operator+=(const Vec3D& A)
  /*!
    Vec3D self addition 
    \param A :: Vec3D to add
    \return *this + A
  */
{
  x+=A.x;
  y+=A.y;
  z+=A.z;
  return *this;
}

Vec3D&
Vec3D::operator-=(const Vec3D& A)
  /*!
    Vec3D self subtraction 
    \param A :: Vec3D to subtract
    \return (this - A)
  */
{
  x-=A.x;
  y-=A.y;
  z-=A.z;
  return *this;
}

int 
Vec3D::masterDir(const double Tol) const
  /*! 
     Calculates the index of the primary direction (if there is one)
     \param Tol :: Tolerance accepted
     \retval range -3,-2,-1 1,2,3  if the vector
     is orientaged within Tol on the x,y,z direction (the sign
     indecates the direction to the +ve side )
     \retval 0 :: No master direction
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

double
Vec3D::Distance(const Vec3D& A) const
  /*! 
    Determine the distance bwtween points
    \param A :: Vec3D 
    \return :: the distance between A and this 
  */
{
  return sqrt((A.x-x)*(A.x-x)+
	      (A.y-y)*(A.y-y)+
	      (A.z-z)*(A.z-z));
}

double
Vec3D::makeUnit()
  /*!
    Make the vector a unit vector 
    \return :: the old magnitude 
  */
{
  const double Sz(abs());
  if (Sz>PTolerance)
    {
      x/=Sz;
      y/=Sz;
      z/=Sz;
    }
  return Sz;
}

double
Vec3D::dotProd(const Vec3D& A) const
  /*!
    Calculate the dot product.
    \param A :: vector to take product from
    \returns this.A
  */
{
  return A.x*x+A.y*y+A.z*z;
}

int 
Vec3D::nullVector(const double Tol) const
  /*! 
    Checks the size of the vector
    \param Tol :: size of the biggest zero vector allowed.
    \retval 1 : the vector squared components
    magnitude are less than Tol 
    \retval 0 :: Vector bigger than Tol
  */
{
  return ((x*x+y*y+z*z)>Tol) ? 0 :1;
}


double
Vec3D::abs() const
  /*!
    Calculate the magnatude of the point
    \returns \f$ | this | \f$ 
  */
{
  return sqrt(x*x+y*y+z*z);
}

template<typename T>
void
Vec3D::rotate(const Matrix<T>& A)
  /*!
    Rotate a point by a matrix 
    \param A :: Rotation matrix (needs to be 3x3)
  */
{
  Matrix<T> Pv(3,1);
  Pv[0][0]=x;
  Pv[1][0]=y;
  Pv[2][0]=z;
  Matrix<T> Po=A*Pv;
  x=Po[0][0];
  y=Po[1][0];
  z=Po[2][0];
  return;
}

void
Vec3D::rotate(const Vec3D& Origin,const Vec3D& Axis,const double theta)
  /*!
    Executes an arbitory rotation about an Axis, Origin and 
    for an angle
    \param Origin :: Origin point to do rotation
    \param Axis :: Axis value to rotate around [needs to be unit]
    \param Theta :: Angle in radians
  */
{
   x -= Origin.x;
   y -= Origin.y;
   z -= Origin.z;
   this->rotate(Axis,theta);
   x += Origin.x;
   y += Origin.y;
   z += Origin.z;
   return;
}

void
Vec3D::rotate(const Vec3D& Axis,const double theta)
  /*!
    Executes an arbitory rotation about an Axis and 
    for an angle
    \param Axis :: Axis value to rotate around [needs to be unit]
    \param Theta :: Angle in radians
  */
{
   const double costheta = cos(theta);
   const double sintheta = sin(theta);
   Vec3D Q;       // output point
   Q.x += (costheta + (1 - costheta) * Axis.x * Axis.x) * x;
   Q.x += ((1 - costheta) * Axis.x * Axis.y - Axis.z * sintheta) * y;
   Q.x += ((1 - costheta) * Axis.x * Axis.z + Axis.y * sintheta) * z;

   Q.y += ((1 - costheta) * Axis.x * Axis.y + Axis.z * sintheta) * x;
   Q.y += (costheta + (1 - costheta) * Axis.y * Axis.y) * y;
   Q.y += ((1 - costheta) * Axis.y * Axis.z - Axis.x * sintheta) * z;

   Q.z += ((1 - costheta) * Axis.x * Axis.z - Axis.y * sintheta) * x;
   Q.z += ((1 - costheta) * Axis.y * Axis.z + Axis.x * sintheta) * y;
   Q.z += (costheta + (1 - costheta) * Axis.z * Axis.z) * z;

   x=Q.x;
   y=Q.y;
   z=Q.z;
   return;
}


int
Vec3D::reBase(const Vec3D& A,const Vec3D&B,const Vec3D& C) 
  /*! 
     Re-express this point components of A,B,C.
     Assuming that A,B,C are form an basis set (which
     does not have to be othonormal.
     \param A :: Unit vector in basis
     \param B :: Unit vector in basis
     \param C :: Unit vector in basis
     \retval -1 :: The points do not form a basis set.
     \retval 0  :: Vec3D has successfully been re-expressed.
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

int
Vec3D::coLinear(const Vec3D& Bv,const Vec3D& Cv) const
  /*!
    Determines if this,B,C are collinear (returns 1 if true)
    \param Bv :: Vector to test
    \param Cv :: Vector to test
    \returns 0 is no colinear and 1 if they are (within Ptolerance)
  */
{
  const Vec3D& Av=*this;
  const Vec3D Tmp((Bv-Av)*(Cv-Av));
  return (Tmp.abs()>PTolerance) ? 0 : 1;
}

void
Vec3D::read(std::istream& IX)
  /*!
    Read data from a stream.
    \todo Check Error handling 
    \param OX :: Output stream
  */
{
  IX>>x>>y>>z;
  return;
}

void
Vec3D::write(std::ostream& OX) const
  /*!
    Write out the point values
    \param OX :: Output stream
  */
{
  OX<<x<<" "<<y<<" "<<z;
  return;
}

}  // NAMESPACE Geometry


/// \cond TEMPLATE

template Geometry::Vec3D& 
Geometry::Vec3D::operator*=(const Geometry::Matrix<double>&);
template Geometry::Vec3D 
Geometry::Vec3D::operator*(const Geometry::Matrix<double>&) const;

/// \endcond TEMPLATE

}  // NAMESPACE Mantid
