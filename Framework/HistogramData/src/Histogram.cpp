#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace HistogramData {

/** Obtain a valid XMode for a Histogram based on x and y data size.

  Throws if there is no valid XMode, i.e., of x-length is neither equal to or 1
  larger than y-length. */
Histogram::XMode getHistogramXMode(size_t xLength, size_t yLength) {
  if (xLength == yLength)
    return Histogram::XMode::Points;
  if (xLength == (yLength + 1))
    return Histogram::XMode::BinEdges;
  throw std::logic_error("getHistogramXMode(): x-y size mismatch, cannot "
                         "determine Histogram::XMode.");
}

/** Returns the bin edges of the Histogram.

  If the histogram stores points, the bin edges are computed based on them.
  Otherwise the returned BinEdges' internal pointer references the same data as
  the Histogram, i.e., there is little overhead. */
BinEdges Histogram::binEdges() const {
  if (xMode() == XMode::BinEdges)
    return BinEdges(m_x);
  else
    return BinEdges(Points(m_x));
}

/// Returns the standard deviations of the bin edges of the Histogram.
BinEdgeStandardDeviations Histogram::binEdgeStandardDeviations() const {
  if (xMode() == XMode::BinEdges)
    return BinEdgeStandardDeviations(m_dx);
  else
    return BinEdgeStandardDeviations(PointStandardDeviations(m_dx));
}

/** Returns the points (or bin centers) of the Histogram.

  If the histogram stores bin edges, the points are computed based on them.
  Otherwise the returned Points's internal pointer references the same data as
  the Histogram, i.e., there is little overhead. */
Points Histogram::points() const {
  if (xMode() == XMode::BinEdges)
    return Points(BinEdges(m_x));
  else
    return Points(m_x);
}

/// Returns the standard deviations of the points (or bin centers) of the Histogram.
PointStandardDeviations Histogram::pointStandardDeviations() const {
  if (xMode() == XMode::BinEdges)
    return PointStandardDeviations(BinEdgeStandardDeviations(m_dx));
  else
    return PointStandardDeviations(m_dx);
}

/** Sets the internal x-data pointer of the Histogram.

  Throws if the size does not match the current size. */
void Histogram::setSharedX(const Kernel::cow_ptr<HistogramX> &X) & {
  // TODO Check size only if we have y-data.
  // TODO but if size changes, also need to invalidate m_dx!
  if (m_x->size() != X->size())
    throw std::logic_error("Histogram::setSharedX: size mismatch\n");
  m_x = X;
}

/** Sets the internal dx-data pointer of the Histogram.

  Throws if the size does not match the current size. */
void Histogram::setSharedDx(const Kernel::cow_ptr<HistogramDx> &Dx) & {
  // Setting a NULL Dx is fine, this disables x errors.
  // Note that we compare with m_x -- m_dx might be NULL.
  if (Dx && m_x->size() != Dx->size())
    throw std::logic_error("Histogram::setSharedDx: size mismatch\n");
  m_dx = Dx;
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
