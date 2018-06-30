#ifndef MANTID_HISTOGRAMDATA_MULTIPLIABLE_H_
#define MANTID_HISTOGRAMDATA_MULTIPLIABLE_H_

#include "MantidHistogramData/DllConfig.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <type_traits>

namespace Mantid {
namespace HistogramData {
namespace detail {

/** Multipliable

  This class is an implementation detail of class like HistogramData::Counts and
  HistogramData::HistogramY. By inheriting from it, a type becomes multipliable,
  i.e., an object can be multiplied with another objects of the same type.

  @author Simon Heybrock
  @date 2016

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
template <class T> class Multipliable {
public:
  /// Element-wise multiplication of this and other.
  T &operator*=(const T &other) & {
    auto &derived = static_cast<T &>(*this);
    checkLengths(derived, other);
    std::transform(derived.cbegin(), derived.cend(), other.begin(),
                   derived.begin(), std::multiplies<double>());
    return derived;
  }

  /// Element-wise division of this and other.
  T &operator/=(const T &other) & {
    auto &derived = static_cast<T &>(*this);
    checkLengths(derived, other);
    std::transform(derived.cbegin(), derived.cend(), other.begin(),
                   derived.begin(), std::divides<double>());
    return derived;
  }

  /// Element-wise multiplication of lhs and rhs.
  T operator*(T rhs) const {
    auto &derived = static_cast<const T &>(*this);
    return rhs *= derived;
  }

  /// Element-wise division of lhs and rhs.
  T operator/(const T &rhs) const {
    auto &derived = static_cast<const T &>(*this);
    T out(derived);
    return out /= rhs;
  }

protected:
  ~Multipliable() = default;

private:
  void checkLengths(const T &v1, const T &v2) {
    if (v1.size() != v2.size())
      throw std::runtime_error("Cannot multiply vectors, lengths must match");
  }
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_MULTIPLIABLE_H_ */
