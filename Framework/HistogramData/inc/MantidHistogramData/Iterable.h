#ifndef MANTID_HISTOGRAMDATA_ITERABLE_H_
#define MANTID_HISTOGRAMDATA_ITERABLE_H_

#include "MantidHistogramData/DllConfig.h"

namespace Mantid {
namespace HistogramData {
namespace detail {

/** Iterable : TODO: DESCRIPTION

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
template <class T> class Iterable {
public:
  // Note: There is no non-const version of this since it requires expensive cow
  // access.
  const double &operator[](size_t pos) const {
    return static_cast<const T *>(this)->constData()[pos];
  }

protected:
  ~Iterable() = default;
};

template <class T>
auto begin(Iterable<T> &container)
    -> decltype(static_cast<T *>(&container)->data().begin()) {
  return static_cast<T *>(&container)->data().begin();
}

template <class T>
auto end(Iterable<T> &container)
    -> decltype(static_cast<T *>(&container)->data().end()) {
  return static_cast<T *>(&container)->data().end();
}

template <class T>
auto begin(const Iterable<T> &container)
    -> decltype(static_cast<const T *>(&container)->data().begin()) {
  return static_cast<const T *>(&container)->data().begin();
}

template <class T>
auto end(const Iterable<T> &container)
    -> decltype(static_cast<const T *>(&container)->data().end()) {
  return static_cast<const T *>(&container)->data().end();
}

template <class T>
auto cbegin(const Iterable<T> &container)
    -> decltype(static_cast<const T *>(&container)->data().cbegin()) {
  return static_cast<const T *>(&container)->data().cbegin();
}

template <class T>
auto cend(const Iterable<T> &container)
    -> decltype(static_cast<const T *>(&container)->data().cend()) {
  return static_cast<const T *>(&container)->data().cend();
}

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_ITERABLE_H_ */
