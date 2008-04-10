#ifndef Geometry_Vec3D_h
#define Geometry_Vec3D_h

#include "MantidKernel/System.h"
#include "../../inc/Matrix.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{

namespace Geometry 
{

/*!
  \class Vec3D
  \brief Basic 3D point class
  \version 1.1
  \date August 2004
  \author S. Ansell

  Simple Vec3D function based on x,y,z and providing
  simple dot and cross products
 */

class DLLExport Vec3D
{
 protected:
  
  double x;        ///< X-Coordinates
  double y;        ///< Y-Coordinates
  double z;        ///< Z-Coordinates
  
  virtual void rotate(const Vec3D&,const double); 

 public:
  
  Vec3D();
  Vec3D(const double,const double,const double);
  Vec3D(const double*);
  Vec3D(const Vec3D&);
  virtual ~Vec3D();

  double X() const { return x; }   ///< Accessor function (X)
  double Y() const { return y; }   ///< Accessor function (Y)
  double Z() const { return z; }   ///< Accessor function (Z)

  Vec3D& operator=(const Vec3D&);
  Vec3D operator()(const double,const double,const double) const;
  double& operator[](const int);
  const double operator[](const int) const;
  template<typename T> Vec3D operator()(const Matrix<T>&) const;  ///< Convert matrix to a point (3x1 or 1x3)

  Vec3D& operator*=(const Vec3D&);
  template<typename T> Vec3D& operator*=(const Matrix<T>&);
  Vec3D& operator*=(const double);
  Vec3D& operator/=(const double);
  Vec3D& operator+=(const Vec3D&);
  Vec3D& operator-=(const Vec3D&);

  Vec3D operator*(const Vec3D&) const;
  template<typename T> Vec3D operator*(const Matrix<T>&) const;

  Vec3D operator*(const double) const;     // Scale factor
  Vec3D operator/(const double) const;     // Scale factor
  Vec3D operator+(const Vec3D&) const;
  Vec3D operator-(const Vec3D&) const;
  Vec3D operator-() const;
 
  bool operator==(const Vec3D&) const;
  virtual void rotate(const Vec3D&,const Vec3D&,const double);

  double Distance(const Vec3D&) const;    ///< Calculate scale distance
  double makeUnit();                      ///< Convert into unit vector
  double volume() const { return fabs(x*y*z); }      ///< Calculate the volmue of a cube X*Y*Z

  double dotProd(const Vec3D&) const;
  double abs() const;    
  template<typename T> void rotate(const Matrix<T>&); 
  int reBase(const Vec3D&,const Vec3D&,const Vec3D&);   ///<rebase to new basis vector
  int masterDir(const double=1e-3) const;               ///< Determine if there is a master direction
  int nullVector(const double=1e-3) const;              ///< Determine if the point is null
  int coLinear(const Vec3D&,const Vec3D&) const;
  
  void read(std::istream&);
  void write(std::ostream&) const;
};


DLLExport std::ostream& operator<<(std::ostream&,const Vec3D&);
DLLExport std::istream& operator>>(std::istream&,Vec3D&);

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif

