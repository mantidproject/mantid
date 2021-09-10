// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/Multipliable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"

namespace Mantid {
namespace HistogramData {
class Counts;
class Frequencies;
class HistogramY;
namespace detail {
template <class Counts, class HistogramY> class VectorOf;
template <class Frequencies, class HistogramY> class VectorOf;
} // namespace detail

/** HistogramY

  This class holds y-data of a histogram. The y-data can be counts or
  frequencies. To prevent breaking the histogram, the length of HistogramY
  cannot be changed via the public interface.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL HistogramY : public detail::FixedLengthVector<HistogramY>,
                                            public detail::Addable<HistogramY>,
                                            public detail::Offsetable<HistogramY>,
                                            public detail::Multipliable<HistogramY>,
                                            public detail::Scalable<HistogramY> {
public:
  using detail::FixedLengthVector<HistogramY>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramY>::operator=;
  HistogramY() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  HistogramY(const HistogramY &) = default;
  HistogramY(HistogramY &&) = default;
  HistogramY &operator=(const HistogramY &) & = default;
  HistogramY &operator=(HistogramY &&) & = default;
  // Multiple inheritance causes ambiguous overload, bring operators into scope.
  using detail::Addable<HistogramY>::operator+;
  using detail::Addable<HistogramY>::operator+=;
  using detail::Addable<HistogramY>::operator-;
  using detail::Addable<HistogramY>::operator-=;
  using detail::Offsetable<HistogramY>::operator+;
  using detail::Offsetable<HistogramY>::operator+=;
  using detail::Offsetable<HistogramY>::operator-;
  using detail::Offsetable<HistogramY>::operator-=;
  using detail::Multipliable<HistogramY>::operator*=;
  using detail::Multipliable<HistogramY>::operator/=;
  using detail::Scalable<HistogramY>::operator*=;
  using detail::Scalable<HistogramY>::operator/=;

  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<Counts, HistogramY>;
  friend class detail::VectorOf<Frequencies, HistogramY>;
};

} // namespace HistogramData
} // namespace Mantid
