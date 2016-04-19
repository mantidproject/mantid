#include "MantidHistogramData/HistogramX.h"

namespace Mantid {
namespace HistogramData {

  /*
HistogramX::HistogramX(const Points &points)
    : detail::FixedLengthVector<HistogramX>(points.cowData()),
      m_xMode(XMode::Points) {}

HistogramX::HistogramX(const BinEdges &binEdges)
    : detail::FixedLengthVector<HistogramX>(binEdges.cowData()),
      m_xMode(XMode::BinEdges) {
  if (size() == 1)
    throw std::logic_error("HistogramX: BinEdges size cannot be 1");
}

HistogramX &HistogramX::operator=(const HistogramX &rhs) {
  if (rhs.xMode() == XMode::Points) {
    checkSize(rhs.points());
  } else {
    checkSize(rhs.binEdges());
  }
  m_xMode = rhs.m_xMode;
  m_data = rhs.m_data;
  return *this;
}

Points HistogramX::points() const {
  if (xMode() == XMode::BinEdges)
    return Points(BinEdges(m_data));
  else
    return Points(m_data);
}

BinEdges HistogramX::binEdges() const {
  if (xMode() == XMode::Points)
    return BinEdges(Points(m_data));
  else
    return BinEdges(m_data);
}

HistogramX::XMode HistogramX::xMode() const noexcept { return m_xMode; }

void HistogramX::checkSize(const Points &points) const {
  size_t target = size();
  // 0 edges -> 0 points, otherwise points are 1 less than edges.
  if (xMode() == XMode::BinEdges && target > 0)
    target--;
  if (target != points.size())
    throw std::logic_error("HistogramX: size mismatch of Points\n");
}

void HistogramX::checkSize(const BinEdges &binEdges) const {
  size_t target = size();
  // 0 points -> 0 edges, otherwise edges are 1 more than points.
  if (xMode() == XMode::Points && target > 0)
    target++;
  if (target != binEdges.size())
    throw std::logic_error("HistogramX: size mismatch of BinEdges\n");
}
*/

} // namespace HistogramData
} // namespace Mantid
