#ifndef MANTID_GEOMETRY_ISHKL_H_
#define MANTID_GEOMETRY_ISHKL_H_

#include <array>
#include <functional>
#include <cmath>
#include <iostream>

#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {

/** IsHKL :  TODO: DESCRIPTION

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
  typedef typename std::array<NumericType, 3> storage_type;

  typedef typename storage_type::iterator iterator;
  typedef typename storage_type::const_iterator const_iterator;

  static constexpr NumericType comparison_tolerance =
      std::numeric_limits<NumericType>::epsilon();

  IsHKL() : m_data() {}
  IsHKL(const Derived &other) : m_data(other.m_data) {}
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
    return std::equal(cbegin(), cend(), other.cbegin(),
                      [](const NumericType &lhs, const NumericType &rhs) {
                        return std::abs(lhs - rhs) <=
                               Derived::comparison_tolerance;
                      });
  }

  bool operator!=(const Derived &other) const { return !(operator==(other)); }

  bool operator<(const Derived &other) const {
    auto mismatch = std::mismatch(
        cbegin(), cend(), other.cbegin(),
        [](const NumericType &lhs, const NumericType &rhs) {
          return std::abs(lhs - rhs) <= Derived::comparison_tolerance;
        });

    return mismatch.first != cend() && *(mismatch.first) < *(mismatch.second);
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
          return std::abs(element - scalar) > Derived::comparison_tolerance;
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

private:
  std::array<NumericType, 3> m_data;
};

class ProHKL : public IsHKL<double, ProHKL> {
public:
  using IsHKL<double, ProHKL>::IsHKL;

  explicit ProHKL(const Kernel::V3D &hkl) : ProHKL(hkl.X(), hkl.Y(), hkl.Z()) {}
};

class IntegerHKL : public IsHKL<int, IntegerHKL> {
public:
  using IsHKL<int, IntegerHKL>::IsHKL;

  explicit IntegerHKL(const Kernel::V3D &hkl)
      : IntegerHKL(static_cast<int>(std::round(hkl.X())),
                   static_cast<int>(std::round(hkl.Y())),
                   static_cast<int>(std::round(hkl.Z()))) {}

  explicit IntegerHKL(const ProHKL &hkl)
      : IntegerHKL(static_cast<int>(std::round(hkl.h())),
                   static_cast<int>(std::round(hkl.k())),
                   static_cast<int>(std::round(hkl.l()))) {}
};

class FractionalHKL : public IsHKL<double, FractionalHKL> {
public:
  using IsHKL<double, FractionalHKL>::IsHKL;

  explicit FractionalHKL(const Kernel::V3D &hkl)
      : FractionalHKL(hkl.X(), hkl.Y(), hkl.Z()) {}

  explicit FractionalHKL(const ProHKL &hkl)
      : FractionalHKL(hkl.h(), hkl.k(), hkl.l()) {}

  explicit FractionalHKL(const IntegerHKL &hkl)
      : FractionalHKL(static_cast<double>(hkl.h()),
                      static_cast<double>(hkl.k()),
                      static_cast<double>(hkl.l())) {}
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_ISHKL_H_ */
