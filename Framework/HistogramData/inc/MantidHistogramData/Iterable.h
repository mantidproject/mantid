// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"

namespace Mantid {
namespace HistogramData {
namespace detail {

/** Iterable

  This class is an implementation detail of class like HistogramData::BinEdges
  and HistogramData::Points. By inheriting from it, a type becomes iterable,
  i.e., provides index and iterator based access.
*/
template <class T> class Iterable {
public:
  /** Returns a const reference to the element at specified location pos. No
   * bounds checking is performed.
   *
   * Note: There is no non-const version of this since it requires expensive cow
   * access. */
  const double &operator[](size_t pos) const { return static_cast<const T *>(this)->data()[pos]; }

  std::vector<double>::iterator begin() { return static_cast<T *>(this)->mutableData().begin(); }

  std::vector<double>::iterator end() { return static_cast<T *>(this)->mutableData().end(); }

  std::vector<double>::const_iterator begin() const { return static_cast<const T *>(this)->data().begin(); }

  std::vector<double>::const_iterator end() const { return static_cast<const T *>(this)->data().end(); }

  std::vector<double>::const_iterator cbegin() const { return static_cast<const T *>(this)->data().cbegin(); }

  std::vector<double>::const_iterator cend() const { return static_cast<const T *>(this)->data().cend(); }

  /** Returns a const reference to the first element.
   *
   * Note: There is no non-const version of this since it requires expensive cow
   * access. */
  const double &front() const { return static_cast<const T *>(this)->data().front(); }

  /** Returns a const reference to the last element.
   *
   * Note: There is no non-const version of this since it requires expensive cow
   * access. */
  const double &back() const { return static_cast<const T *>(this)->data().back(); }

  // expose typedefs for the iterator types in the underlying container
  using iterator = std::vector<double>::iterator;
  using const_iterator = std::vector<double>::const_iterator;

protected:
  ~Iterable() = default;
};

template <class T> auto begin(Iterable<T> &container) -> decltype(container.begin()) { return container.begin(); }

template <class T> auto end(Iterable<T> &container) -> decltype(container.end()) { return container.end(); }

template <class T> auto begin(const Iterable<T> &container) -> decltype(container.begin()) { return container.begin(); }

template <class T> auto end(const Iterable<T> &container) -> decltype(container.end()) { return container.end(); }

template <class T> auto cbegin(const Iterable<T> &container) -> decltype(container.cbegin()) {
  return container.cbegin();
}

template <class T> auto cend(const Iterable<T> &container) -> decltype(container.cend()) { return container.cend(); }

} // namespace detail
} // namespace HistogramData
} // namespace Mantid
