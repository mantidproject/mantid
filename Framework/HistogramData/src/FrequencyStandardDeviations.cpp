#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/FrequencyVariances.h"

namespace Mantid {
namespace HistogramData {

/// Constructs FrequencStandardDeviations from CountStandardDeviations and bin
/// width based on BinEdges.
FrequencyStandardDeviations::FrequencyStandardDeviations(
    const CountStandardDeviations &counts, const BinEdges &edges)
    : FrequencyStandardDeviations(CountStandardDeviations(counts), edges) {}

/// Move-constructs FrequencyStandardDeviations from CountStandardDeviations and
/// bin width based on BinEdges.
FrequencyStandardDeviations::FrequencyStandardDeviations(
    CountStandardDeviations &&counts, const BinEdges &edges) {
  if (!counts)
    return;
  if (!edges)
    throw std::logic_error("FrequencyStandardDeviations: Cannot construct from "
                           "CountStandardDeviations -- BinEdges are NULL.");
  if ((counts.size() + 1) != edges.size())
    if (!counts.empty() || !edges.empty())
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
