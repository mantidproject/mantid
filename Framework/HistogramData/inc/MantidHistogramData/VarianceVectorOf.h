// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/VectorOf.h"

namespace Mantid {
namespace HistogramData {
namespace detail {

/** VarianceVectorOf

  This class is an implementation detail of classes like
  HistogramData::CountVariances and HistogramData::FrequencyVariances. It
  extends VectorOf with conversions (construction and assignment) from objects
  holding standard deviations.

  The first template parameter is the type of the inheriting class. This is the
  CRTP (curiously recurring template pattern).

  The second template parameter is the type of the object to store.

  The third template parameter is the type of the associated standard deviation
  object.

  @author Simon Heybrock
  @date 2016
*/
template <class T, class CowType, class Sigmas>
class VarianceVectorOf : public VectorOf<T, CowType>, public Iterable<T> {
public:
  using VectorOf<T, CowType>::VectorOf;
  using VectorOf<T, CowType>::operator=;

  VarianceVectorOf() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  VarianceVectorOf(const VarianceVectorOf &) = default;
  VarianceVectorOf(VarianceVectorOf &&) = default;
  VarianceVectorOf &operator=(const VarianceVectorOf &) & = default;
  VarianceVectorOf &operator=(VarianceVectorOf &&) & = default;

  /// Copy construct from sigmas, taking the square of each sigma value.
  VarianceVectorOf(const Sigmas &sigmas);
  /// Move construct from sigmas, taking the square of each sigma value.
  VarianceVectorOf(Sigmas &&sigmas);
  /// Copy assignment from sigmas, taking the square of each sigma value.
  VarianceVectorOf &operator=(const Sigmas &sigmas) &;
  /// Move assignment from sigmas, taking the square of each sigma value.
  VarianceVectorOf &operator=(Sigmas &&sigmas) &;

protected:
  // This is used as base class only, cannot delete polymorphically, so
  // destructor is protected.
  ~VarianceVectorOf() = default;
};

template <class T, class CowType, class Sigmas>
VarianceVectorOf<T, CowType, Sigmas>::VarianceVectorOf(const Sigmas &sigmas) {
  if (!sigmas)
    return;
  auto &derived = static_cast<T &>(*this);
  derived.operator=(sigmas.cowData());
  std::transform(derived.cbegin(), derived.cend(), derived.begin(), [](const double &x) { return x * x; });
}

template <class T, class CowType, class Sigmas>
VarianceVectorOf<T, CowType, Sigmas>::VarianceVectorOf(Sigmas &&sigmas) {
  if (!sigmas)
    return;
  auto &derived = static_cast<T &>(*this);
  derived.operator=(sigmas.cowData());
  // Data is now shared between sigmas and derived. We want to avoid triggering
  // a copy by the iterator access below, so we have to set sigmas to null. We
  // cannot directly null the cow_ptr in sigmas, since it is of a different type
  // and we do not have access to its private members.
  sigmas = Kernel::cow_ptr<CowType>(nullptr);
  std::transform(derived.cbegin(), derived.cend(), derived.begin(), [](const double &x) { return x * x; });
}

template <class T, class CowType, class Sigmas>
VarianceVectorOf<T, CowType, Sigmas> &VarianceVectorOf<T, CowType, Sigmas>::operator=(const Sigmas &sigmas) & {
  VarianceVectorOf<T, CowType, Sigmas> tmp(sigmas);
  auto &derived = static_cast<T &>(*this);
  derived.operator=(tmp.cowData());
  return *this;
}

template <class T, class CowType, class Sigmas>
VarianceVectorOf<T, CowType, Sigmas> &VarianceVectorOf<T, CowType, Sigmas>::operator=(Sigmas &&sigmas) & {
  VarianceVectorOf<T, CowType, Sigmas> tmp(std::move(sigmas));
  auto &derived = static_cast<T &>(*this);
  derived.operator=(tmp.cowData());
  return *this;
}

} // namespace detail
} // namespace HistogramData
} // namespace Mantid
