#ifndef MANTID_HISTOGRAMDATA_SCALABLE_H_
#define MANTID_HISTOGRAMDATA_SCALABLE_H_

#include "MantidHistogramData/DllConfig.h"

#include <algorithm>
#include <type_traits>

namespace Mantid {
namespace HistogramData {
namespace detail {

/** Scalable

  This class is an implementation detail of class like HistogramData::BinEdges
  and HistogramData::HistogramX. By inheriting from it, a type becomes scalable,
  i.e., can be multiplied by a scalar.

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
template <class T> class Scalable {
public:
  /// Scales each element in the container by the factor given by scale.
  T &operator*=(const double scale) & {
    auto &derived = static_cast<T &>(*this);
    std::for_each(derived.begin(), derived.end(),
                  [=](double &value) { value *= scale; });
    return derived;
  }

  /// Divides each element in the container by denominator.
  T &operator/=(const double denominator) & {
    return (*this) *= 1.0 / denominator;
  }

protected:
  ~Scalable() = default;
};

/// Scales each element in lhs by the factor given by rhs.
template <class T, class = typename std::enable_if<
                       std::is_base_of<Scalable<T>, T>::value>::type>
inline T operator*(T lhs, const double rhs) {
  return lhs *= rhs;
}

/// Divides each element in lhs by rhs.
template <class T, class = typename std::enable_if<
                       std::is_base_of<Scalable<T>, T>::value>::type>
inline T operator/(T lhs, const double rhs) {
  return lhs /= rhs;
}

/// Scales each element in rhs by the factor given by lhs.
template <class T, class = typename std::enable_if<
                       std::is_base_of<Scalable<T>, T>::value>::type>
inline T operator*(const double lhs, T rhs) {
  return rhs *= lhs;
}

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_SCALABLE_H_ */
