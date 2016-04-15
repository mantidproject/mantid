#include "MantidHistogram/Points.h"
#include "MantidHistogram/BinEdges.h"

namespace Mantid {
namespace Histogram {

Points::Points(const BinEdges &edges) {
  if (!edges)
    return;
  if (edges.size() == 1)
    throw std::logic_error("Points: Cannot construct from BinEdges of size 1");
  m_data = Kernel::make_cow<std::vector<double>>();
  if (edges.size() < 2)
    return;
  auto &data = m_data.access();
  data.reserve(edges.size() - 1);
  for (auto it = cbegin(edges) + 1; it < cend(edges); ++it) {
    data.emplace_back(0.5 * (*it + *(it - 1)));
  }
}

} // namespace Histogram
} // namespace Mantid
