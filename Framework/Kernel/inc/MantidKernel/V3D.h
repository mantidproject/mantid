// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Tolerance.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numeric>
#include <sstream>
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
class MANTID_KERNEL_DLL V3D final {
public:
  constexpr V3D() noexcept : m_pt({{0., 0., 0.}}) {}
  constexpr V3D(double xx, double yy, double zz) noexcept : m_pt({{xx, yy, zz}}) {}

  /// Convenience method for sorting list of V3D objects based on magnitude
  static bool compareMagnitude(const Kernel::V3D &v1, const Kernel::V3D &v2);

  // explicit conversion into vector
  operator std::vector<double>() const { return std::vector<double>(m_pt.cbegin(), m_pt.cend()); }

  /**
    Number of components in V3D
     @return 3
  */
  std::size_t size() const noexcept { return m_pt.size(); }

  /**
    Addtion operator
     @param v :: Vector to add
     @return *this+v;
  */
  constexpr V3D operator+(const V3D &v) const noexcept {
    return V3D(m_pt[0] + v.m_pt[0], m_pt[1] + v.m_pt[1], m_pt[2] + v.m_pt[2]);
  }

  /**
    Subtraction operator
    @param v :: Vector to sub.
    @return *this-v;
  */
  constexpr V3D operator-(const V3D &v) const noexcept {
    return V3D(m_pt[0] - v.m_pt[0], m_pt[1] - v.m_pt[1], m_pt[2] - v.m_pt[2]);
  }

  /**
    Inner product
    @param v :: Vector to sub.
    @return *this * v;
  */
  constexpr V3D operator*(const V3D &v) const noexcept {
    return V3D(m_pt[0] * v.m_pt[0], m_pt[1] * v.m_pt[1], m_pt[2] * v.m_pt[2]);
  }

  /**
    Inner division
    @param v :: Vector to divide
    @return *this * v;
  */
  constexpr V3D operator/(const V3D &v) const noexcept {
    return V3D(m_pt[0] / v.m_pt[0], m_pt[1] / v.m_pt[1], m_pt[2] / v.m_pt[2]);
  }

  /**
    Self-Addition operator
    @param v :: Vector to add.
    @return *this+=v;
  */
  V3D &operator+=(const V3D &v) noexcept {
    for (size_t i = 0; i < m_pt.size(); ++i) {
      m_pt[i] += v.m_pt[i];
    }
    return *this;
  }

  /**
    Self-Subtraction operator
    @param v :: Vector to sub.
    @return *this-v;
  */
  V3D &operator-=(const V3D &v) noexcept {
    for (size_t i = 0; i < m_pt.size(); ++i) {
      m_pt[i] -= v.m_pt[i];
    }
    return *this;
  }

  /**
    Self-Inner product
    @param v :: Vector to multiply
    @return *this*=v;
  */
  V3D &operator*=(const V3D &v) noexcept {
    for (size_t i = 0; i < m_pt.size(); ++i) {
      m_pt[i] *= v.m_pt[i];
    }
    return *this;
  }

  /**
    Self-Inner division
    @param v :: Vector to divide
    @return *this*=v;
  */
  V3D &operator/=(const V3D &v) noexcept {
    for (size_t i = 0; i < m_pt.size(); ++i) {
      m_pt[i] /= v.m_pt[i];
    }
    return *this;
  }

  /**
    Scalar product
    @param D :: value to scale
    @return this * D
   */
  constexpr V3D operator*(const double D) const noexcept { return V3D(m_pt[0] * D, m_pt[1] * D, m_pt[2] * D); }

  /**
    Scalar divsion
    @param D :: value to scale
    @return this / D
  */
  constexpr V3D operator/(const double D) const noexcept { return V3D(m_pt[0] / D, m_pt[1] / D, m_pt[2] / D); }

  /**
    Scalar product
    @param D :: value to scale
    @return this *= D
  */
  V3D &operator*=(const double D) noexcept {
    std::for_each(m_pt.begin(), m_pt.end(), [D](auto &pt) { pt *= D; });
    return *this;
  }

  /**
    Scalar division
    @param D :: value to scale
    @return this /= D
    \todo ADD TOLERANCE
  */
  V3D &operator/=(const double D) noexcept {
    std::for_each(m_pt.begin(), m_pt.end(), [D](auto &pt) { pt /= D; });
    return *this;
  }

  /**
    Negation
   * @return a vector with same magnitude but in opposite direction
   */
  constexpr V3D operator-() const noexcept { return V3D(-m_pt[0], -m_pt[1], -m_pt[2]); }

  /**
    Equals operator with tolerance factor
    @param v :: V3D for comparison
    @return true if the items are equal
  */
  bool operator==(const V3D &v) const noexcept {
    return !(std::abs(m_pt[0] - v.m_pt[0]) > Tolerance || std::abs(m_pt[1] - v.m_pt[1]) > Tolerance ||
             std::abs(m_pt[2] - v.m_pt[2]) > Tolerance);
  }

  /** Not equals operator with tolerance factor.
   *  @param other :: The V3D to compare against
   *  @returns True if the vectors are different
   */
  bool operator!=(const V3D &other) const noexcept { return !(this->operator==(other)); }

  /**
    compare
    @return true if V is greater
   */
  constexpr bool operator<(const V3D &V) const noexcept {
    if (m_pt[0] != V.m_pt[0])
      return m_pt[0] < V.m_pt[0];
    if (m_pt[1] != V.m_pt[1])
      return m_pt[1] < V.m_pt[1];
    return m_pt[2] < V.m_pt[2];
  }

  /// Comparison operator greater than.
  constexpr bool operator>(const V3D &rhs) const noexcept { return rhs < *this; }

  /**
    Sets the vector position from a triplet of doubles x,y,z
    @param xx :: The X coordinate
    @param yy :: The Y coordinate
    @param zz :: The Z coordinate
  */
  void operator()(const double xx, const double yy, const double zz) noexcept { m_pt = {{xx, yy, zz}}; }

  // Access
  // Setting x, y and z values
  void spherical(const double R, const double theta, const double phi) noexcept;
  void spherical_rad(const double R, const double polar, const double azimuth) noexcept;
  void azimuth_polar_SNS(const double R, const double azimuth, const double polar) noexcept;
  /**
    Set is x position
    @param xx :: The X coordinate
  */
  void setX(const double xx) noexcept { m_pt[0] = xx; }

  /**
    Set is y position
    @param yy :: The Y coordinate
  */
  void setY(const double yy) noexcept { m_pt[1] = yy; }

  /**
    Set is z position
    @param zz :: The Z coordinate
  */
  void setZ(const double zz) noexcept { m_pt[2] = zz; }

  constexpr double X() const noexcept { return m_pt[0]; } ///< Get x
  constexpr double Y() const noexcept { return m_pt[1]; } ///< Get y
  constexpr double Z() const noexcept { return m_pt[2]; } ///< Get z

  /**
    Returns the axis value based in the index provided
    @param index :: 0=x, 1=y, 2=z
    @return a double value of the requested axis
  */
  constexpr double operator[](const size_t index) const noexcept {
    assert(index < m_pt.size());
    return m_pt[index];
  }

  /**
    Returns the axis value based in the index provided
    @param index :: 0=x, 1=y, 2=z
    @return a double value of the requested axis
  */
  double &operator[](const size_t index) noexcept {
    assert(index < m_pt.size());
    return m_pt[index];
  }

  void getSpherical(double &R, double &theta, double &phi) const noexcept;

  void rotate(const Matrix<double> &) noexcept;

  void round() noexcept;
  /// Make a normalized vector (return norm value)
  double normalize();
  double norm() const noexcept { return sqrt(norm2()); }
  /// Vector length squared
  constexpr double norm2() const noexcept { return m_pt[0] * m_pt[0] + m_pt[1] * m_pt[1] + m_pt[2] * m_pt[2]; }
  /// transform vector into form, used to describe directions in
  /// crystallogaphical coodinate system
  double toMillerIndexes(double eps = 1.e-3);
  /**
    Calculates the cross product. Returns (this * v).
    @param v :: The second vector to include in the calculation
    @return The cross product of the two vectors (this * v)
  */
  constexpr double scalar_prod(const V3D &v) const noexcept {
    return m_pt[0] * v.m_pt[0] + m_pt[1] * v.m_pt[1] + m_pt[2] * v.m_pt[2];
  }
  /// Cross product (this * argument)
  constexpr V3D cross_prod(const V3D &v) const noexcept {
    return V3D(m_pt[1] * v.m_pt[2] - m_pt[2] * v.m_pt[1], m_pt[2] * v.m_pt[0] - m_pt[0] * v.m_pt[2],
               m_pt[0] * v.m_pt[1] - m_pt[1] * v.m_pt[0]);
  }
  /**
    Calculates the distance between two vectors
    @param v :: The second vector to include in the calculation
    @return The distance between the two vectors
  */
  double distance(const V3D &v) const noexcept { return (*this - v).norm(); }
  /// Zenith (theta) angle between this and another vector
  double zenith(const V3D &) const noexcept;
  /// Angle between this and another vector
  double angle(const V3D &) const;
  /// cos(Angle) between this and another vector
  double cosAngle(const V3D &) const;
  /// Direction angles
  V3D directionAngles(bool inDegrees = true) const;
  /// Maximum absolute integer value
  int maxCoeff() const;
  /// Absolute value
  V3D absoluteValue() const;
  /// Calculates the error in hkl
  double hklError() const;

  // Make 2 vectors into 3 orthogonal vectors
  static std::vector<V3D> makeVectorsOrthogonal(const std::vector<V3D> &vectors);

  // Send to a stream
  void printSelf(std::ostream &) const;
  void readPrinted(std::istream &);
  void read(std::istream &);
  void write(std::ostream &) const;
  std::string toString() const;
  void fromString(const std::string &str);

  /// Calculate the volume of a cube X*Y*Z
  double volume() const noexcept { return std::abs(m_pt[0] * m_pt[1] * m_pt[2]); }
  /// rebase to new basis vector
  int reBase(const V3D &, const V3D &, const V3D &) noexcept;
  /// Determine if there is a master direction
  int masterDir(const double Tol = 1e-3) const noexcept;
  /// Determine if the point is null
  bool nullVector(const double tolerance = 1e-3) const noexcept;
  bool unitVector(const double tolerance = Kernel::Tolerance) const noexcept;
  bool coLinear(const V3D &, const V3D &) const noexcept;

  void saveNexus(::NeXus::File *file, const std::string &name) const;
  void loadNexus(::NeXus::File *file, const std::string &name);

  static bool isnan(V3D vec) { return (std::isnan(vec[0]) || std::isnan(vec[1]) || std::isnan(vec[2])); }

private:
  std::array<double, 3> m_pt;
};

// Overload operator <<
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const V3D &);
MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, V3D &);

/** Normalizes a V3D.
 * @param v a vector to normalize.
 * @return a vector with norm 1 parallel to v
 * @throw std::runtime_error if v is a null vector.
 */
inline MANTID_KERNEL_DLL V3D normalize(V3D v) {
  v.normalize();
  return v;
}

} // Namespace Kernel
} // Namespace Mantid
