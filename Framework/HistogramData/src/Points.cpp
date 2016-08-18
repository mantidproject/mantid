#include "MantidHistogramData/Points.h"
#include "MantidHistogramData/BinEdges.h"

namespace Mantid {
namespace HistogramData {

/// Constructs Points from BinEdges, where each point is a bin center.
Points::Points(const BinEdges &edges) {
  if (!edges)
    return;
  if (edges.size() == 1)
    throw std::logic_error("Points: Cannot construct from BinEdges of size 1");
  if (edges.size() == 0) {
    m_data = Kernel::make_cow<HistogramX>(0);
    return;
  }

  std::vector<double> data(edges.size() - 1);
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = (0.5 * (edges[i] + edges[i + 1]));
  }
  m_data = Kernel::make_cow<HistogramX>(std::move(data));
}

} // namespace HistogramData
} // namespace Mantid
