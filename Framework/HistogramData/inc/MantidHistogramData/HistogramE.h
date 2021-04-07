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
#include "MantidHistogramData/Scalable.h"

namespace Mantid {
namespace HistogramData {
class CountStandardDeviations;
class CountVariances;
class FrequencyStandardDeviations;
class FrequencyVariances;
class HistogramE;
namespace detail {
template <class CountStandardDeviations, class HistogramE> class VectorOf;
template <class CountVariances, class HistogramE> class VectorOf;
template <class FrequencyStandardDeviations, class HistogramE> class VectorOf;
template <class FrequencyVariances, class HistogramE> class VectorOf;
} // namespace detail

/** HistogramE

  This class holds e-data of a histogram. The e-data can be variances or
  standard deviations of counts or frequencies. To prevent breaking the
  histogram, the length of HistogramE cannot be changed via the public
  interface.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL HistogramE : public detail::FixedLengthVector<HistogramE>,
                                            public detail::Addable<HistogramE>,
                                            public detail::Multipliable<HistogramE>,
                                            public detail::Scalable<HistogramE> {
public:
  using detail::FixedLengthVector<HistogramE>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramE>::operator=;
  HistogramE() = default;
  // The copy and move constructor and assignment are not captured properly
  // by
  // the using declaration above, so we need them here explicitly.
  HistogramE(const HistogramE &) = default;
  HistogramE(HistogramE &&) = default;
  HistogramE &operator=(const HistogramE &) & = default;
  HistogramE &operator=(HistogramE &&) & = default;
  // Multiple inheritance causes ambiguous overload, bring operators into scope.
  using detail::Multipliable<HistogramE>::operator*=;
  using detail::Multipliable<HistogramE>::operator/=;
  using detail::Scalable<HistogramE>::operator*=;
  using detail::Scalable<HistogramE>::operator/=;
  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<CountStandardDeviations, HistogramE>;
  friend class detail::VectorOf<CountVariances, HistogramE>;
  friend class detail::VectorOf<FrequencyStandardDeviations, HistogramE>;
  friend class detail::VectorOf<FrequencyVariances, HistogramE>;
};

} // namespace HistogramData
} // namespace Mantid
