#ifndef MANTID_HISTOGRAMDATA_ITERABLE_H_
#define MANTID_HISTOGRAMDATA_ITERABLE_H_

#include "MantidHistogramData/DllConfig.h"

namespace Mantid {
namespace HistogramData {
namespace detail {

/** Iterable

  This class is an implementation detail of class like HistogramData::BinEdges
  and HistogramData::Points. By inheriting from it, a type becomes iterable,
  i.e., provides index and iterator based access.

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
  /** Returns a const reference to the element at specified location pos. No
   * bounds checking is performed.
   *
   * Note: There is no non-const version of this since it requires expensive cow
   * access. */
  const double &operator[](size_t pos) const {
    return static_cast<const T *>(this)->data()[pos];
  }

  std::vector<double>::iterator begin() {
    return static_cast<T *>(this)->mutableData().begin();
  }

  std::vector<double>::iterator end() {
    return static_cast<T *>(this)->mutableData().end();
  }

  std::vector<double>::const_iterator begin() const {
    return static_cast<const T *>(this)->data().begin();
  }

  std::vector<double>::const_iterator end() const {
    return static_cast<const T *>(this)->data().end();
  }

  std::vector<double>::const_iterator cbegin() const {
    return static_cast<const T *>(this)->data().cbegin();
  }

  std::vector<double>::const_iterator cend() const {
    return static_cast<const T *>(this)->data().cend();
  }

  /** Returns a const reference to the first element.
   *
   * Note: There is no non-const version of this since it requires expensive cow
   * access. */
  const double &front() const {
    return static_cast<const T *>(this)->data().front();
  }

  /** Returns a const reference to the last element.
   *
   * Note: There is no non-const version of this since it requires expensive cow
   * access. */
  const double &back() const {
    return static_cast<const T *>(this)->data().back();
  }

  // expose typedefs for the iterator types in the underlying container
  typedef std::vector<double>::iterator iterator;
  typedef std::vector<double>::const_iterator const_iterator;

protected:
  ~Iterable() = default;
};

template <class T>
auto begin(Iterable<T> &container) -> decltype(container.begin()) {
  return container.begin();
}

template <class T>
auto end(Iterable<T> &container) -> decltype(container.end()) {
  return container.end();
}

template <class T>
auto begin(const Iterable<T> &container) -> decltype(container.begin()) {
  return container.begin();
}

template <class T>
auto end(const Iterable<T> &container) -> decltype(container.end()) {
  return container.end();
}

template <class T>
auto cbegin(const Iterable<T> &container) -> decltype(container.cbegin()) {
  return container.cbegin();
}

template <class T>
auto cend(const Iterable<T> &container) -> decltype(container.cend()) {
  return container.cend();
}

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_ITERABLE_H_ */
