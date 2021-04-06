// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
template <class T> class Scalable {
public:
  /// Scales each element in the container by the factor given by scale.
  T &operator*=(const double scale) & {
    auto &derived = static_cast<T &>(*this);
    std::for_each(derived.begin(), derived.end(), [=](double &value) { value *= scale; });
    return derived;
  }

  /// Divides each element in the container by denominator.
  T &operator/=(const double denominator) & { return (*this) *= 1.0 / denominator; }

protected:
  ~Scalable() = default;
};

/// Scales each element in lhs by the factor given by rhs.
template <class T, class = typename std::enable_if<std::is_base_of<Scalable<T>, T>::value>::type>
inline T operator*(T lhs, const double rhs) {
  return lhs *= rhs;
}

/// Divides each element in lhs by rhs.
template <class T, class = typename std::enable_if<std::is_base_of<Scalable<T>, T>::value>::type>
inline T operator/(T lhs, const double rhs) {
  return lhs /= rhs;
}

/// Scales each element in rhs by the factor given by lhs.
template <class T, class = typename std::enable_if<std::is_base_of<Scalable<T>, T>::value>::type>
inline T operator*(const double lhs, T rhs) {
  return rhs *= lhs;
}

} // namespace detail
} // namespace HistogramData
} // namespace Mantid
