#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace HistogramData {

Histogram::XMode getHistogramXMode(size_t xLength, size_t yLength) {
  if (xLength == yLength)
    return Histogram::XMode::Points;
  if (xLength == (yLength + 1))
    return Histogram::XMode::BinEdges;
  throw std::logic_error("getHistogramXMode(): x-y size mismatch, cannot "
                         "determine Histogram::XMode.");
}

void Histogram::checkSize(const Points &points) const {
  size_t target = m_x->size();
  // 0 edges -> 0 points, otherwise points are 1 less than edges.
  if (xMode() == XMode::BinEdges && target > 0)
    target--;
  if (target != points.size())
    throw std::logic_error("Histogram: size mismatch of Points\n");
}

void Histogram::checkSize(const BinEdges &binEdges) const {
  size_t target = m_x->size();
  // 0 points -> 0 edges, otherwise edges are 1 more than points.
  if (xMode() == XMode::Points && target > 0)
    target++;
  if (target != binEdges.size())
    throw std::logic_error("Histogram: size mismatch of BinEdges\n");
}

} // namespace HistogramData
} // namespace Mantid
