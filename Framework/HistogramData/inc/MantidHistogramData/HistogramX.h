// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class Points;
class HistogramX;
namespace detail {
template <class BinEdges, class HistogramX> class VectorOf;
template <class Points, class HistogramX> class VectorOf;
} // namespace detail

/** HistogramX

  This class holds x-data of a histogram. The x-data can be bin edges or points.
  HistogramX is used to directly reference data in a Histogram when it needs to
  be modified without a specific need for bin edges or points. To prevent
  breaking the histogram, the length of HistogramX cannot be changed via the
  public interface.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL HistogramX : public detail::FixedLengthVector<HistogramX>,
                                            public detail::Offsetable<HistogramX>,
                                            public detail::Scalable<HistogramX> {
public:
  using detail::FixedLengthVector<HistogramX>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramX>::operator=;
  HistogramX() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  HistogramX(const HistogramX &) = default;
  HistogramX(HistogramX &&) = default;
  HistogramX &operator=(const HistogramX &) & = default;
  HistogramX &operator=(HistogramX &&) & = default;
  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<BinEdges, HistogramX>;
  friend class detail::VectorOf<Points, HistogramX>;
};

} // namespace HistogramData
} // namespace Mantid
