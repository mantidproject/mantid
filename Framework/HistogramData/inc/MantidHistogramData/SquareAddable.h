#ifndef MANTID_HISTOGRAMDATA_SQUAREADDABLE_H_
#define MANTID_HISTOGRAMDATA_SQUAREADDABLE_H_

#include "MantidHistogramData/DllConfig.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace Mantid {
namespace HistogramData {
namespace detail {

/** SquareAddable

  This class is an implementation detail of class like
  HistogramData::CountStandardDeviations. By inheriting from it, a type becomes
  square-addable, i.e., an object can be added to another objects of the same
  type by taking the square-root of the sum of the squares.

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
template <class T> class SquareAddable {
public:
  /// Element-wise addition of elements in container and other.
  T &operator+=(const T &other) & {
    auto &derived = static_cast<T &>(*this);
    std::transform(derived.cbegin(), derived.cend(), other.begin(),
                   derived.begin(),
                   [](const double &lhs, const double &rhs)
                       -> double { return std::sqrt(lhs * lhs + rhs * rhs); });
    return derived;
  }

  /// Element-wise addition of elements in lhs and rhs.
  inline T operator+(T other) const {
    auto &derived = static_cast<const T &>(*this);
    return other += derived;
  }

protected:
  ~SquareAddable() = default;
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_SQUAREADDABLE_H_ */
