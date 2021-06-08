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

/** Offsetable

  This class is an implementation detail of class like HistogramData::BinEdges
  and HistogramData::HistogramX. By inheriting from it, a type becomes
  offsetable, i.e., a scalar can be added to it.
*/
template <class T> class Offsetable {
public:
  /// Offsets each element in the container by offset.
  T &operator+=(const double offset) & {
    auto &derived = static_cast<T &>(*this);
    std::for_each(derived.begin(), derived.end(), [=](double &value) { value += offset; });
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
