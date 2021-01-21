// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/StandardDeviationVectorOf.h"

namespace Mantid {
namespace HistogramData {

class PointVariances;

/** PointStandardDeviations

  Container for the standard deviations of the points in a histogram. A
  copy-on-write mechanism saves memory and makes copying cheap. The
  implementation is based on detail::StandardDeviationVectorOf, which provides
  conversion from the corresponding variance type, PointVariances.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL PointStandardDeviations
    : public detail::StandardDeviationVectorOf<PointStandardDeviations, HistogramDx, PointVariances> {
public:
  using StandardDeviationVectorOf<PointStandardDeviations, HistogramDx, PointVariances>::StandardDeviationVectorOf;
  using StandardDeviationVectorOf<PointStandardDeviations, HistogramDx, PointVariances>::operator=;
  /// Default constructor, creates a NULL object.
  PointStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  PointStandardDeviations(const PointStandardDeviations &) = default;
  /// Move constructor.
  PointStandardDeviations(PointStandardDeviations &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  PointStandardDeviations &operator=(const PointStandardDeviations &) & = default;
  /// Move assignment.
  PointStandardDeviations &operator=(PointStandardDeviations &&) & = default;
};

} // namespace HistogramData
} // namespace Mantid
