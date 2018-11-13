// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_V3D_H_
#define MANTID_KERNEL_V3D_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Tolerance.h"
#include <cmath>
#include <iosfwd>
#include <vector>

namespace NeXus {
class File;
}

namespace Mantid {
namespace Kernel {
template <class T> class Matrix;
/** @class V3D V3D.h Kernel\V3D.h

Class for 3D vectors.

@author Laurent C Chapon, ISIS, RAL
@date 09/10/2007
*/
class MANTID_KERNEL_DLL V3D {
public:
  V3D();
  V3D(const double, const double, const double);

  /// Convenience method for sorting list of V3D objects based on magnitude
  static bool CompareMagnitude(const Kernel::V3D &v1, const Kernel::V3D &v2);

  // explicit conversion into vector
  operator std::vector<double>() const {
    std::vector<double> tmp(3);
    tmp[0] = x;
    tmp[1] = y;
    tmp[2] = z;
    return tmp;
  }

  /**
    Addtion operator
     @param v :: Vector to add
     @return *this+v;
  */
  V3D operator+(const V3D &v) const {
    V3D out(*this);
    out += v;
    return out;
  }

  /**
    Subtraction operator
    @param v :: Vector to sub.
    @return *this-v;
  */
  V3D operator-(const V3D &v) const {
    V3D out(*this);
    out -= v;
    return out;
  }

  /**
    Inner product
    @param v :: Vector to sub.
    @return *this * v;
  */
  V3D operator*(const V3D &v) const {
    V3D out(*this);
    out *= v;
    return out;
  }

  /**
    Inner division
    @param v :: Vector to divide
    @return *this * v;
  */
  V3D operator/(const V3D &v) const {
    V3D out(*this);
    out /= v;
    return out;
  }

  /**
    Self-Addition operator
    @param v :: Vector to add.
    @return *this+=v;
  */
  V3D &operator+=(const V3D &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }

  /**
    Self-Subtraction operator
    @param v :: Vector to sub.
    @return *this-v;
  */
  V3D &operator-=(const V3D &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }

  /**
    Self-Inner product
    @param v :: Vector to multiply
    @return *this*=v;
  */
  V3D &operator*=(const V3D &v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
  }

  /**
    Self-Inner division
    @param v :: Vector to divide
    @return *this*=v;
  */
  V3D &operator/=(const V3D &v) {
    x /= v.x;
    y /= v.y;
    z /= v.z;
    return *this;
  }

  /**
    Scalar product
    @param D :: value to scale
    @return this * D
   */
  V3D operator*(const double D) const {
    V3D out(*this);
    out *= D;
    return out;
  }

  /**
    Scalar divsion
    @param D :: value to scale
    @return this / D
  */
  V3D operator/(const double D) const {
    V3D out(*this);
    out /= D;
    return out;
  }

  /**
    Scalar product
    @param D :: value to scale
    @return this *= D
  */
  V3D &operator*=(const double D) {
    x *= D;
    y *= D;
    z *= D;
    return *this;
  }

  /**
    Scalar division
    @param D :: value to scale
    @return this /= D
    \todo ADD TOLERANCE
  */
  V3D &operator/=(const double D) {
    if (D != 0.0) {
      x /= D;
      y /= D;
      z /= D;
    }
    return *this;
  }

  /**
    Negation
   * @return a vector with same magnitude but in opposite direction
   */
  V3D operator-() const { return V3D(-x, -y, -z); }

  /**
    Equals operator with tolerance factor
    @param v :: V3D for comparison
    @return true if the items are equal
  */
  bool operator==(const V3D &v) const {
    using namespace std;
    return !(fabs(x - v.x) > Tolerance || fabs(y - v.y) > Tolerance ||
             fabs(z - v.z) > Tolerance);
  }

  /** Not equals operator with tolerance factor.
   *  @param other :: The V3D to compare against
   *  @returns True if the vectors are different
   */
  bool operator!=(const V3D &other) const { return !(this->operator==(other)); }

  /**
    compare
    @return true if V is greater
   */
  bool operator<(const V3D &V) const {
    if (x != V.x)
      return x < V.x;
    if (y != V.y)
      return y < V.y;
    return z < V.z;
  }

  /// Comparison operator greater than.
  bool operator>(const V3D &rhs) const { return rhs < *this; }

  /**
    Sets the vector position from a triplet of doubles x,y,z
    @param xx :: The X coordinate
    @param yy :: The Y coordinate
    @param zz :: The Z coordinate
  */
  void operator()(const double xx, const double yy, const double zz) {
    x = xx;
    y = yy;
    z = zz;
  }

  // Access
  // Setting x, y and z values
  void spherical(const double &R, const double &theta, const double &phi);
  void spherical_rad(const double &R, const double &polar,
                     const double &azimuth);
  void azimuth_polar_SNS(const double &R, const double &azimuth,
                         const double &polar);
  /**
    Set is x position
    @param xx :: The X coordinate
  */
  void setX(const double xx) { x = xx; }

  /**
    Set is y position
    @param yy :: The Y coordinate
  */
  void setY(const double yy) { y = yy; }

  /**
    Set is z position
    @param zz :: The Z coordinate
  */
  void setZ(const double zz) { z = zz; }

  const double &X() const { return x; } ///< Get x
  const double &Y() const { return y; } ///< Get y
  const double &Z() const { return z; } ///< Get z

  /**
    Returns the axis value based in the index provided
    @param Index :: 0=x, 1=y, 2=z
    @return a double value of the requested axis
  */
  const double &operator[](const size_t Index) const {
    switch (Index) {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      throw Kernel::Exception::IndexError(Index, 2, "operator[] range error");
    }
  }

  /**
    Returns the axis value based in the index provided
    @param Index :: 0=x, 1=y, 2=z
    @return a double value of the requested axis
  */
  double &operator[](const size_t Index) {
    switch (Index) {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      throw Kernel::Exception::IndexError(Index, 2, "operator[] range error");
    }
  }

  void getSpherical(double &R, double &theta, double &phi) const;

  void rotate(const Matrix<double> &);

  void round();

  /// Make a normalized vector (return norm value)
  double normalize(); // Vec3D::makeUnit
  double norm() const;
  double norm2() const;
  /// transform vector into form, used to describe directions in
  /// crystallogaphical coodinate system
  double toMillerIndexes(double eps = 1.e-3);
  /// Scalar product
  double scalar_prod(const V3D &) const;
  /// Cross product (this * argument)
  V3D cross_prod(const V3D &) const;
  /// Distance (R) between two points defined as vectors
  double distance(const V3D &) const;
  /// Zenith (theta) angle between this and another vector
  double zenith(const V3D &) const;
  /// Angle between this and another vector
  double angle(const V3D &) const;
  /// Direction angles
  V3D directionAngles(bool inDegrees = true) const;
  /// Maximum absolute integer value
  int maxCoeff();
  /// Absolute value
  V3D absoluteValue() const;
  /// Calculates the error in hkl
  double hklError() const;

  // Make 2 vectors into 3 orthogonal vectors
  static std::vector<V3D> makeVectorsOrthogonal(std::vector<V3D> &vectors);

  // Send to a stream
  void printSelf(std::ostream &) const;
  void readPrinted(std::istream &);
  void read(std::istream &);
  void write(std::ostream &) const;
  std::string toString() const;
  void fromString(const std::string &str);

  double volume() const {
    return fabs(x * y * z);
  } ///< Calculate the volume of a cube X*Y*Z

  int reBase(const V3D &, const V3D &,
             const V3D &); ///< rebase to new basis vector
  int masterDir(const double Tol =
                    1e-3) const; ///< Determine if there is a master direction
  bool
  nullVector(const double Tol = 1e-3) const; ///< Determine if the point is null
  bool coLinear(const V3D &, const V3D &) const;

  void saveNexus(::NeXus::File *file, const std::string &name) const;
  void loadNexus(::NeXus::File *file, const std::string &name);

private:
  double x; ///< X value [unitless]
  double y; ///< Y value [unitless]
  double z; ///< Z value [unitless]
};

// Overload operator <<
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const V3D &);
MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, V3D &);

} // Namespace Kernel
} // Namespace Mantid

#endif /*MANTID_KERNEL_V3D_H_*/
