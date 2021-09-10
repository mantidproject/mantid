// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidHistogramData/Slice.h"

namespace Mantid {
namespace HistogramData {

/// Returns a slice of histogram between given begin and end indices.
Histogram slice(const Histogram &histogram, const size_t begin, const size_t end) {
  if (begin > end)
    throw std::out_of_range("Histogram slice: begin must not be greater than end");
  if (end > histogram.size())
    throw std::out_of_range("Histogram slice: end may not be larger than the histogram size");
  auto sliced(histogram);
  if (begin == 0 && end == histogram.size())
    return sliced;
  sliced.resize(end - begin);
  if (end - begin == 0)
    return sliced;

  auto xEnd = histogram.xMode() == Histogram::XMode::Points ? end : end + 1;
  sliced.mutableX().assign(histogram.x().begin() + begin, histogram.x().begin() + xEnd);
  if (sliced.sharedY())
    sliced.mutableY().assign(histogram.y().begin() + begin, histogram.y().begin() + end);
  if (sliced.sharedE())
    sliced.mutableE().assign(histogram.e().begin() + begin, histogram.e().begin() + end);
  if (sliced.sharedDx())
    sliced.mutableDx().assign(histogram.dx().begin() + begin, histogram.dx().begin() + end);
  return sliced;
}

} // namespace HistogramData
} // namespace Mantid
