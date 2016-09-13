#ifndef MANTID_HISTOGRAMDATA_OFFSETABLE_H_
#define MANTID_HISTOGRAMDATA_OFFSETABLE_H_

#include "MantidHistogramData/DllConfig.h"

#include <algorithm>
#include <type_traits>

namespace Mantid {
namespace HistogramData {
namespace detail {

/** Offsetable

  This class is an implementation detail of class like HistogramData::BinEdges
  and HistogramData::HistogramX. By inheriting from it, a type becomes
  offsetable, i.e., a scalar can be added to it.

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
template <class T> class Offsetable {
public:
  /// Offsets each element in the container by offset.
  T &operator+=(const double offset) & {
    auto &derived = static_cast<T &>(*this);
    std::for_each(derived.begin(), derived.end(),
                  [=](double &value) { value += offset; });
    return derived;
  }

  /// Subtracts offset from each element in the container.
  T &operator-=(const double offset) & { return (*this) += -offset; }

  /// Offsets each element in lhs by rhs.
  T operator+(const double rhs) const {
    auto &derived = static_cast<const T &>(*this);
    T out(derived);
    return out += rhs;
  }

  /// Subtracts rhs from each element in lhs.
  T operator-(const double rhs) const {
    auto &derived = static_cast<const T &>(*this);
    T out(derived);
    return out += -rhs;
  }

protected:
  ~Offsetable() = default;
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_OFFSETABLE_H_ */
