#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace HistogramData {

Histogram::XMode getHistogramXMode(size_t xLength, size_t yLength) {
  if (xLength == yLength)
    return Histogram::XMode::Points;
  if (xLength == (yLength + 1))
    return Histogram::XMode::BinEdges;
  return Histogram::XMode::Uninitialized;
}

} // namespace HistogramData
} // namespace Mantid
