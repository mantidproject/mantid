// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

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
  */

using RationalNumber = boost::rational<int>;

class MANTID_GEOMETRY_DLL V3R {
public:
  V3R();
  V3R(const RationalNumber &x, const RationalNumber &y, const RationalNumber &z);
  V3R(const std::vector<int> &vector);

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

  // std::vector<double> operator
  operator std::vector<double>() const;

protected:
  RationalNumber m_x;
  RationalNumber m_y;
  RationalNumber m_z;
};

/// Performs a matrix multiplication v' = M * v, throws
/// Kernel::Exception::MisMatch<size_t> if M does not have exactly 3 columns.
template <typename T> V3R operator*(const Kernel::Matrix<T> &lhs, const V3R &rhs) {
  size_t rows = lhs.numRows();
  size_t cols = lhs.numCols();

  if (cols != 3) {
    throw Kernel::Exception::MisMatch<size_t>(cols, 3, "operator*(IntMatrix, V3R)");
  }

  V3R result;
  for (size_t r = 0; r < rows; ++r) {
    for (size_t c = 0; c < cols; ++c) {
      result[r] += static_cast<typename RationalNumber::int_type>(lhs[r][c]) * rhs[c];
    }
  }

  return result;
}
} // namespace Geometry
} // namespace Mantid
