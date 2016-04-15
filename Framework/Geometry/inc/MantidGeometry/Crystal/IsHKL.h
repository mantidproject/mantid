#ifndef MANTID_GEOMETRY_ISHKL_H_
#define MANTID_GEOMETRY_ISHKL_H_

#include <array>
#include <functional>
#include <cmath>
#include <iostream>

#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Geometry {

/** IsHKL : TODO: DESCRIPTION

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename NumericType, typename Derived> class IsHKL {
public:
  typedef typename std::array<NumericType, 3>::iterator iterator;
  typedef typename std::array<NumericType, 3>::const_iterator const_iterator;

  static constexpr NumericType comparison_tolerance =
      std::numeric_limits<NumericType>::epsilon();

  IsHKL() : m_data() {}
  IsHKL(const IsHKL<NumericType, Derived> &other) : m_data(other.m_data) {}
  IsHKL(const NumericType &h, const NumericType &k, const NumericType &l)
      : m_data({{h, k, l}}) {}

  const NumericType &h() const { return m_data[0]; }
  void setH(const NumericType &h) { m_data[0] = h; }

  const NumericType &k() const { return m_data[1]; }
  void setK(const NumericType &k) { m_data[1] = k; }

  const NumericType &l() const { return m_data[2]; }
  void setL(const NumericType &l) { m_data[2] = l; }

  iterator begin() { return m_data.begin(); }
  iterator end() { return m_data.end(); }

  const_iterator cbegin() const { return m_data.cbegin(); }
  const_iterator cend() const { return m_data.cend(); }

  Derived &operator+=(const Derived &other) {
    std::transform(cbegin(), cend(), other.cbegin(), begin(),
                   std::plus<NumericType>());
    return static_cast<Derived &>(*this);
  }

  Derived &operator-=(const Derived &other) {
    std::transform(cbegin(), cend(), other.cbegin(), begin(),
                   std::minus<NumericType>());
    return static_cast<Derived &>(*this);
  }

  bool operator==(const Derived &other) const {
    return (static_cast<const Derived &>(*this) - other).isZero();
  }

  bool operator!=(const Derived &other) const { return !(operator==(other)); }

  bool operator<(const Derived &other) const {
    return (static_cast<const Derived &>(*this) - other)
        .isLessThan(static_cast<NumericType>(0));
  }

  bool operator>(const Derived &other) const {
    return other < static_cast<const Derived &>(this);
  }

  bool isZero() const {
    return std::all_of(cbegin(), cend(), [](const NumericType &element) {
      return std::fabs(element) <= Derived::comparison_tolerance;
    });
  }

  bool isLessThan(const NumericType &scalar) const {
    auto match =
        std::find_if(cbegin(), cend(), [scalar](const NumericType &element) {
          return std::fabs(element - scalar) > Derived::comparison_tolerance;
        });

    return match != cend() && *match < scalar;
  }

  friend Derived operator+(Derived lhs, const Derived &rhs) {
    return lhs += rhs;
  }

  friend Derived operator-(Derived lhs, const Derived &rhs) {
    return lhs -= rhs;
  }

  friend Derived operator*(const Kernel::Matrix<NumericType> &M,
                           const Derived &rhs) {
    if (M.numCols() != 3 && M.numRows() != 3) {
      throw std::invalid_argument("Matrix must be 3x3.");
    }

    return Derived(M[0][0] * rhs.h() + M[0][1] * rhs.k() + M[0][2] * rhs.l(),
                   M[1][0] * rhs.h() + M[1][1] * rhs.k() + M[1][2] * rhs.l(),
                   M[2][0] * rhs.h() + M[2][1] * rhs.k() + M[2][2] * rhs.l());
  }

protected:
  ~IsHKL() {}

  std::array<NumericType, 3> m_data;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_ISHKL_H_ */
