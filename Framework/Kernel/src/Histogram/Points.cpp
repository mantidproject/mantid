#include "MantidKernel/Histogram/Points.h"
#include "MantidKernel/Histogram/BinEdges.h"

namespace Mantid {
namespace Kernel {

Points::Points(const BinEdges &edges) {
  if (!edges)
    return;
  m_data = make_cow<std::vector<double>>();
  if (edges.size() < 2)
    return;
  auto &data = m_data.access();
  data.reserve(edges.size() - 1);
  for (auto it = cbegin(edges) + 1; it < cend(edges); ++it) {
    data.emplace_back(0.5 * (*it + *(it - 1)));
  }
}

} // namespace Kernel
} // namespace Mantid
