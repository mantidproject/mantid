#include "MantidHistogram/BinEdges.h"
#include "MantidHistogram/Points.h"

namespace Mantid {
namespace Histogram {

BinEdges::BinEdges(const Points &points) {
  if (!points)
    return;
  m_data = Kernel::make_cow<std::vector<double>>();
  const size_t numPoints = points.size();
  const size_t numEdges = numPoints + 1;

  if (numPoints < 1)
    return;
  auto &data = m_data.access();
  data.resize(numEdges);

  if (numPoints == 1) {
    data[0] = points[0] - 0.5;
    data[numPoints] = points[0] + 0.5;
    return;
  }

  // Handle the front and back points outside
  for (size_t i = 0; i < numPoints - 1; ++i) {
    data[i + 1] = 0.5 * (points[i + 1] + points[i]);
  }
  // Now deal with the end points
  data[0] = points[0] - (data[1] - points[0]);
  data[numPoints] =
      points[numPoints - 1] + (points[numPoints - 1] - data[numEdges - 2]);
}

} // namespace Histogram
} // namespace Mantid
