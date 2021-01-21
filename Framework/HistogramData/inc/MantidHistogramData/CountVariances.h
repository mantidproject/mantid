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
class CountStandardDeviations;
class FrequencyVariances;

/** CountVariances

  Container for the variances of the counts in a histogram. A copy-on-write
  mechanism saves memory and makes copying cheap. The implementation is based on
  detail::VarianceVectorOf, which provides conversion from the corresponding
  standard deviation type, CountStandardDeviations.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL CountVariances
    : public detail::VarianceVectorOf<CountVariances, HistogramE, CountStandardDeviations> {
public:
  using VarianceVectorOf<CountVariances, HistogramE, CountStandardDeviations>::VarianceVectorOf;
  using VarianceVectorOf<CountVariances, HistogramE, CountStandardDeviations>::operator=;
  /// Default constructor, creates a NULL object.
  CountVariances() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  CountVariances(const CountVariances &) = default;
  /// Move constructor.
  CountVariances(CountVariances &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  CountVariances &operator=(const CountVariances &) & = default;
  /// Move assignment.
  CountVariances &operator=(CountVariances &&) & = default;

  CountVariances(const FrequencyVariances &frequencies, const BinEdges &edges);
  CountVariances(FrequencyVariances &&frequencies, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid
