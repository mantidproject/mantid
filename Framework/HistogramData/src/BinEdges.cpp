#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Points.h"

namespace Mantid {
namespace HistogramData {

/// Constructs BinEdges from points, approximating each bin edges as mid-point
/// between two points.
BinEdges::BinEdges(const Points &points) {
  if (!points)
    return;
  const size_t numPoints = points.size();
  size_t numEdges = numPoints + 1;
  if (numPoints == 0) {
    m_data = Kernel::make_cow<HistogramX>(numPoints);
    return;
  }

  if (numPoints == 1) {
    auto data = {points[0] - 0.5, points[0] + 0.5};
    m_data = Kernel::make_cow<HistogramX>(data);
    return;
  }

  std::vector<double> data(numEdges);
  // Handle the front and back points outside
  for (size_t i = 0; i < numPoints - 1; ++i) {
    data[i + 1] = 0.5 * (points[i + 1] + points[i]);
  }
  // Now deal with the end points
  data[0] = points[0] - (data[1] - points[0]);
  data[numPoints] =
      points[numPoints - 1] + (points[numPoints - 1] - data[numEdges - 2]);
  m_data = Kernel::make_cow<HistogramX>(std::move(data));
}

} // namespace HistogramData
} // namespace Mantid
