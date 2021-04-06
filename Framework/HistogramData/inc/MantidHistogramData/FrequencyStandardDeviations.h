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
class CountStandardDeviations;
class FrequencyVariances;

/** FrequencyStandardDeviations

  Container for the standard deviations of the frequencies in a histogram. A
  copy-on-write mechanism saves memory and makes copying cheap. The
  implementation is based on detail::StandardDeviationVectorOf, which provides
  conversion from the corresponding variance type, FrequencyVariances.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL FrequencyStandardDeviations
    : public detail::StandardDeviationVectorOf<FrequencyStandardDeviations, HistogramE, FrequencyVariances> {
public:
  using StandardDeviationVectorOf<FrequencyStandardDeviations, HistogramE,
                                  FrequencyVariances>::StandardDeviationVectorOf;
  using StandardDeviationVectorOf<FrequencyStandardDeviations, HistogramE, FrequencyVariances>::operator=;
  /// Default constructor, creates a NULL object.
  FrequencyStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  FrequencyStandardDeviations(const FrequencyStandardDeviations &) = default;
  /// Move constructor.
  FrequencyStandardDeviations(FrequencyStandardDeviations &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  FrequencyStandardDeviations &operator=(const FrequencyStandardDeviations &) & = default;
  /// Move assignment.
  FrequencyStandardDeviations &operator=(FrequencyStandardDeviations &&) & = default;

  FrequencyStandardDeviations(const CountStandardDeviations &counts, const BinEdges &edges);
  FrequencyStandardDeviations(CountStandardDeviations &&counts, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid
