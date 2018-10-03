// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_ADDABLE_H_
#define MANTID_HISTOGRAMDATA_ADDABLE_H_

#include "MantidHistogramData/DllConfig.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <type_traits>

namespace Mantid {
namespace HistogramData {
namespace detail {

/** Addable

  This class is an implementation detail of class like HistogramData::Counts and
  HistogramData::HistogramY. By inheriting from it, a type becomes addable,
  i.e., an object can be added to another objects of the same type.

  @author Simon Heybrock
  @date 2016
*/
template <class T> class Addable {
public:
  /// Element-wise addition of this and other.
  T &operator+=(const T &other) & {
    auto &derived = static_cast<T &>(*this);
    checkLengths(derived, other);
    std::transform(derived.cbegin(), derived.cend(), other.begin(),
                   derived.begin(), std::plus<double>());
    return derived;
  }

  /// Element-wise subtraction of this and other.
  T &operator-=(const T &other) & {
    auto &derived = static_cast<T &>(*this);
    checkLengths(derived, other);
    std::transform(derived.cbegin(), derived.cend(), other.begin(),
                   derived.begin(), std::minus<double>());
    return derived;
  }

  /// Element-wise addition of lhs and rhs.
  T operator+(T rhs) const {
    auto &derived = static_cast<const T &>(*this);
    return rhs += derived;
  }

  /// Element-wise subtraction of lhs and rhs.
  T operator-(const T &rhs) const {
    auto &derived = static_cast<const T &>(*this);
    T out(derived);
    return out -= rhs;
  }

protected:
  ~Addable() = default;

private:
  void checkLengths(const T &v1, const T &v2) {
    if (v1.size() != v2.size())
      throw std::runtime_error("Cannot add vectors, lengths must match");
  }
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_ADDABLE_H_ */
