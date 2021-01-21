// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/VarianceVectorOf.h"

namespace Mantid {
namespace HistogramData {

class PointStandardDeviations;

/** PointVariances

  Container for the variances of the points in a histogram. A copy-on-write
  mechanism saves memory and makes copying cheap. The implementation is based on
  detail::VarianceVectorOf, which provides conversion from the corresponding
  standard deviation type, PointStandardDeviations.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL PointVariances
    : public detail::VarianceVectorOf<PointVariances, HistogramDx, PointStandardDeviations> {
public:
  using VarianceVectorOf<PointVariances, HistogramDx, PointStandardDeviations>::VarianceVectorOf;
  using VarianceVectorOf<PointVariances, HistogramDx, PointStandardDeviations>::operator=;
  /// Default constructor, creates a NULL object.
  PointVariances() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  PointVariances(const PointVariances &) = default;
  /// Move constructor.
  PointVariances(PointVariances &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  PointVariances &operator=(const PointVariances &) & = default;
  /// Move assignment.
  PointVariances &operator=(PointVariances &&) & = default;
};

} // namespace HistogramData
} // namespace Mantid
