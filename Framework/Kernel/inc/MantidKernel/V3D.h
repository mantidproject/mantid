#ifndef MANTID_KERNEL_V3D_H_
#define MANTID_KERNEL_V3D_H_

#include <cmath>
#include <cfloat>
#include <complex>
#include <vector>
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace Kernel {
/** @class V3D V3D.h Kernel\V3D.h

Class for 3D vectors.

@author Laurent C Chapon, ISIS, RAL
@date 09/10/2007

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL V3D {
public:
  V3D();
  V3D(const V3D & /*v*/);
  V3D &operator=(const V3D & /*rhs*/);
  V3D(const double /*xx*/, const double /*yy*/, const double /*zz*/);
  ~V3D();

  // Arithemetic operators overloaded
  V3D operator+(const V3D &v) const;
  V3D &operator+=(const V3D &v);

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

  V3D operator-(const V3D &v) const;
  V3D &operator-=(const V3D &v);
  // Inner product
  V3D operator*(const V3D &v) const;
  V3D &operator*=(const V3D &v);
  // Inner division
  V3D operator/(const V3D &v) const;
  V3D &operator/=(const V3D &v);
  // Scale
  V3D operator*(const double D) const;
  V3D &operator*=(const double D);
  V3D operator/(const double D) const;
  V3D &operator/=(const double D);
  // Simple Comparison
  bool operator==(const V3D & /*v*/) const;
  bool operator!=(const V3D & /*other*/) const;
  bool operator<(const V3D & /*V*/) const;
  bool operator>(const V3D &rhs) const;

  // Access
  // Setting x, y and z values
  void operator()(const double xx, const double yy, const double zz);
  void spherical(const double &R, const double &theta, const double &phi);
  void spherical_rad(const double &R, const double &polar,
                     const double &azimuth);
  void azimuth_polar_SNS(const double &R, const double &azimuth,
                         const double &polar);
  void setX(const double xx);
  void setY(const double yy);
  void setZ(const double zz);

  const double &X() const { return x; } ///< Get x
  const double &Y() const { return y; } ///< Get y
  const double &Z() const { return z; } ///< Get z

  const double &operator[](const size_t Index) const;
  double &operator[](const size_t Index);

  void getSpherical(double &R, double &theta, double &phi) const;

  //      void rotate(const V3D&,const V3D&,const double);
  void rotate(const Matrix<double> & /*A*/);

  void round();

  /// Make a normalized vector (return norm value)
  double normalize(); // Vec3D::makeUnit
  double norm() const;
  double norm2() const;
  /// transform vector into form, used to describe directions in
  /// crystallogaphical coodinate system
  double toMillerIndexes(double eps = 1.e-3);
  /// Scalar product
  double scalar_prod(const V3D & /*V*/) const;
  /// Cross product
  V3D cross_prod(const V3D & /*v*/) const;
  /// Distance (R) between two points defined as vectors
  double distance(const V3D & /*v*/) const;
  /// Zenith (theta) angle between this and another vector
  double zenith(const V3D & /*v*/) const;
  /// Angle between this and another vector
  double angle(const V3D & /*v*/) const;
  /// Direction angles
  V3D directionAngles(bool inDegrees = true) const;

  // Make 2 vectors into 3 orthogonal vectors
  static std::vector<V3D> makeVectorsOrthogonal(std::vector<V3D> &vectors);

  // Send to a stream
  void printSelf(std::ostream & /*os*/) const;
  void readPrinted(std::istream & /*IX*/);
  void read(std::istream & /*IX*/);
  void write(std::ostream & /*OX*/) const;
  std::string toString() const;
  void fromString(const std::string &str);

  double volume() const {
    return fabs(x * y * z);
  } ///< Calculate the volume of a cube X*Y*Z

  int reBase(const V3D & /*A*/, const V3D & /*B*/,
             const V3D & /*C*/); ///<rebase to new basis vector
  int masterDir(const double Tol =
                    1e-3) const; ///< Determine if there is a master direction
  bool
  nullVector(const double Tol = 1e-3) const; ///< Determine if the point is null
  bool coLinear(const V3D & /*Bv*/, const V3D & /*Cv*/) const;

  void saveNexus(::NeXus::File *file, const std::string &name) const;
  void loadNexus(::NeXus::File *file, const std::string &name);

private:
  double x; ///< X value [unitless]
  double y; ///< Y value [unitless]
  double z; ///< Z value [unitless]
};

// Overload operator <<
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream & /*os*/, const V3D & /*v*/);
MANTID_KERNEL_DLL std::istream &operator>>(std::istream & /*IX*/, V3D & /*A*/);

} // Namespace Kernel
} // Namespace Mantid

#endif /*MANTID_KERNEL_V3D_H_*/
