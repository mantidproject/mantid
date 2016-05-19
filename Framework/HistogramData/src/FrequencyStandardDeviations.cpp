#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"

namespace Mantid {
namespace HistogramData {

FrequencyStandardDeviations::FrequencyStandardDeviations(
    const CountStandardDeviations &counts, const BinEdges &edges)
    : FrequencyStandardDeviations(CountStandardDeviations(counts), edges) {}

FrequencyStandardDeviations::FrequencyStandardDeviations(
    CountStandardDeviations &&counts, const BinEdges &edges) {
  if (!counts)
    return;
  if (!edges)
    throw std::logic_error("FrequencyStandardDeviations: Cannot construct from "
                           "CountStandardDeviations -- BinEdges are NULL.");
  if ((counts.size() + 1) != edges.size())
    if (counts.size() != 0 || edges.size() != 0)
      throw std::logic_error("FrequencyStandardDeviations: Cannot construct "
                             "from CountStandardDeviations -- BinEdges size "
                             "does not match.");
  // Cannot move counts private data since it is of different type.
  m_data = counts.cowData();
  counts = Kernel::cow_ptr<HistogramE>(nullptr);
  auto &data = m_data.access();
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] /= edges[i + 1] - edges[i];
  }
}

} // namespace HistogramData
} // namespace Mantid
