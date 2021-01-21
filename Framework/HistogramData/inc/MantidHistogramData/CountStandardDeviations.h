// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/StandardDeviationVectorOf.h"

namespace Mantid {
namespace HistogramData {

class BinEdges;
class CountVariances;
class FrequencyStandardDeviations;

/** CountStandardDeviations

  Container for the standard deviations of the counts in a histogram. A
  copy-on-write mechanism saves memory and makes copying cheap. The
  implementation is based on detail::StandardDeviationVectorOf, which provides
  conversion from the corresponding variance type, CountVariances.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL CountStandardDeviations
    : public detail::StandardDeviationVectorOf<CountStandardDeviations, HistogramE, CountVariances> {
public:
  using StandardDeviationVectorOf<CountStandardDeviations, HistogramE, CountVariances>::StandardDeviationVectorOf;
  using StandardDeviationVectorOf<CountStandardDeviations, HistogramE, CountVariances>::operator=;
  /// Default constructor, creates a NULL object.
  CountStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  CountStandardDeviations(const CountStandardDeviations &) = default;
  /// Move constructor.
  CountStandardDeviations(CountStandardDeviations &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  CountStandardDeviations &operator=(const CountStandardDeviations &) & = default;
  /// Move assignment.
  CountStandardDeviations &operator=(CountStandardDeviations &&) & = default;

  CountStandardDeviations(const FrequencyStandardDeviations &frequencies, const BinEdges &edges);
  CountStandardDeviations(FrequencyStandardDeviations &&frequencies, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid
