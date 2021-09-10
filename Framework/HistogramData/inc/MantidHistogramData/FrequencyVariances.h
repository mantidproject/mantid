// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/VarianceVectorOf.h"

namespace Mantid {
namespace HistogramData {

class BinEdges;
class CountVariances;
class FrequencyStandardDeviations;

/** FrequencyVariances

  Container for the variances of the frequencies in a histogram. A copy-on-write
  mechanism saves memory and makes copying cheap. The implementation is based on
  detail::VarianceVectorOf, which provides conversion from the corresponding
  standard deviation type, FrequencyStandardDeviations.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL FrequencyVariances
    : public detail::VarianceVectorOf<FrequencyVariances, HistogramE, FrequencyStandardDeviations> {
public:
  using VarianceVectorOf<FrequencyVariances, HistogramE, FrequencyStandardDeviations>::VarianceVectorOf;
  using VarianceVectorOf<FrequencyVariances, HistogramE, FrequencyStandardDeviations>::operator=;
  /// Default constructor, creates a NULL object.
  FrequencyVariances() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  FrequencyVariances(const FrequencyVariances &) = default;
  /// Move constructor.
  FrequencyVariances(FrequencyVariances &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  FrequencyVariances &operator=(const FrequencyVariances &) & = default;
  /// Move assignment.
  FrequencyVariances &operator=(FrequencyVariances &&) & = default;

  FrequencyVariances(const CountVariances &counts, const BinEdges &edges);
  FrequencyVariances(CountVariances &&counts, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid
