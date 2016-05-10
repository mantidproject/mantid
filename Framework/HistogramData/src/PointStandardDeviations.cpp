#include "MantidHistogramData/PointStandardDeviations.h"
#include "MantidHistogramData/BinEdgeStandardDeviations.h"

namespace Mantid {
namespace HistogramData {

PointStandardDeviations::PointStandardDeviations(
    const BinEdgeStandardDeviations &edges) {
  if (!edges)
    return;
  if (edges.size() == 1)
    throw std::logic_error("PointStandardDeviations: Cannot construct from "
                           "BinEdgeStandardDeviations of size 1");
  if (edges.size() == 0) {
    m_data = Kernel::make_cow<HistogramDx>(0);
    return;
  }
  m_data = Kernel::make_cow<HistogramDx>(edges.size() - 1);
  auto &data = m_data.access();
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = (0.5 * (edges[i] + edges[i + 1]));
  }
}

} // namespace HistogramData
} // namespace Mantid
