// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"

namespace Mantid {
namespace HistogramData {
class Histogram;
class PointVariances;
class PointStandardDeviations;
class HistogramDx;
namespace detail {
template <class PointVariances, class HistogramDx> class VectorOf;
template <class PointStandardDeviations, class HistogramDx> class VectorOf;
} // namespace detail

/** HistogramDx
 */
class MANTID_HISTOGRAMDATA_DLL HistogramDx : public detail::FixedLengthVector<HistogramDx> {
public:
  using detail::FixedLengthVector<HistogramDx>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramDx>::operator=;
  HistogramDx() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  HistogramDx(const HistogramDx &) = default;
  HistogramDx(HistogramDx &&) = default;
  HistogramDx &operator=(const HistogramDx &) & = default;
  HistogramDx &operator=(HistogramDx &&) & = default;
  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<PointVariances, HistogramDx>;
  friend class detail::VectorOf<PointStandardDeviations, HistogramDx>;
};

} // namespace HistogramData
} // namespace Mantid
