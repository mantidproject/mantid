#include "MantidHistogramData/BinEdgeStandardDeviations.h"
#include "MantidHistogramData/BinEdgeVariances.h"
#include "MantidHistogramData/PointVariances.h"

namespace Mantid {
namespace HistogramData {

BinEdgeVariances::BinEdgeVariances(const PointVariances &points) {
  if (!points)
    return;
  const size_t numPoints = points.size();
  size_t numEdges = numPoints + 1;
  if (numPoints == 0)
    numEdges = 0;
  m_data = Kernel::make_cow<HistogramDx>(numEdges);
  if (numPoints == 0)
    return;

  auto &data = m_data.access();

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

} // namespace HistogramData
} // namespace Mantid
