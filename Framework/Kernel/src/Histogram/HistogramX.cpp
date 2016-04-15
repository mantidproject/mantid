#include "MantidKernel/Histogram/HistogramX.h"

namespace Mantid {
namespace Kernel {

HistogramX::HistogramX(const Points &points)
    : HistogramData<HistogramX>(points.cowData()), m_xMode(XMode::Points) {}

HistogramX::HistogramX(const BinEdges &binEdges)
    : HistogramData<HistogramX>(binEdges.cowData()), m_xMode(XMode::BinEdges) {
  if (size() == 1)
    throw std::logic_error("HistogramX: BinEdges size cannot be 1");
}

} // namespace Kernel
} // namespace Mantid
