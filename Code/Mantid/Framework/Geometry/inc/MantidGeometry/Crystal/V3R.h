#ifndef MANTID_GEOMETRY_V3R_H_
#define MANTID_GEOMETRY_V3R_H_

#include "MantidGeometry/DllConfig.h"

#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"

#include <boost/rational.hpp>

namespace Mantid {
namespace Geometry {

/** V3R :

    In crystallography, many operations use rational numbers like 1/2, 1/4
    or 2/3. Floating point numbers can approximate these, but calculations
    involving these approximations are never exact.

    V3R is a vector with three rational components, implemented using
    boost::rational<int>. This way, crystallographic calculations involving
    fractional vectors may be carried out exactly (for example symmetry
    operations with a translational component).

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 26/09/2014

    Copyright © 2014 PSI-MSS

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
typedef boost::rational<int> RationalNumber;

class MANTID_GEOMETRY_DLL V3R {
public:
  V3R();
  V3R(const RationalNumber &x, const RationalNumber &y,
      const RationalNumber &z);
  V3R(const std::vector<int> &vector);

  V3R(const V3R &other);
  V3R &operator=(const V3R &other);

  ~V3R();

  const RationalNumber &x() const;
  void setX(const RationalNumber &newX);

  const RationalNumber &y() const;
  void setY(const RationalNumber &newY);

  const RationalNumber &z() const;
  void setZ(const RationalNumber &newZ);

  RationalNumber &operator[](size_t index);
  const RationalNumber &operator[](size_t index) const;

  // Operations with other vectors of rational numbers
  V3R operator+(const V3R &other) const;
  V3R &operator+=(const V3R &other);

  V3R operator-() const;
  V3R operator-(const V3R &other) const;
  V3R &operator-=(const V3R &other);

  // Operations with integers
  V3R operator+(int other) const;
  V3R &operator+=(int other);

  V3R operator-(int other) const;
  V3R &operator-=(int other);

  V3R operator*(int other) const;
  V3R &operator*=(int other);

  V3R operator/(int other) const;
  V3R &operator/=(int other);

  // Operations with rational numbers
  V3R operator+(const RationalNumber &other) const;
  V3R &operator+=(const RationalNumber &other);

  V3R operator-(const RationalNumber &other) const;
  V3R &operator-=(const RationalNumber &other);

  V3R operator*(const RationalNumber &other) const;
  V3R &operator*=(const RationalNumber &other);

  V3R operator/(const RationalNumber &other) const;
  V3R &operator/=(const RationalNumber &other);

  // Operations with V3D
  operator Kernel::V3D() const;
  Kernel::V3D operator+(const Kernel::V3D &other) const;
  Kernel::V3D operator-(const Kernel::V3D &other) const;

  // Comparison operators
  bool operator==(const V3R &other) const;
  bool operator!=(const V3R &other) const;
  bool operator<(const V3R &other) const;

  bool operator==(int other) const;
  bool operator!=(int other) const;

  V3R getPositiveVector() const;

protected:
  RationalNumber m_x;
  RationalNumber m_y;
  RationalNumber m_z;
};

MANTID_GEOMETRY_DLL V3R operator*(const Kernel::IntMatrix &lhs, const V3R &rhs);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_V3R_H_ */
