#include "MantidHistogramData/FrequencyVariances.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"

namespace Mantid {
namespace HistogramData {

/// Constructs FrequencVariances from CountVariances and bin width based on
/// BinEdges.
FrequencyVariances::FrequencyVariances(const CountVariances &counts,
                                       const BinEdges &edges)
    : FrequencyVariances(CountVariances(counts), edges) {}

/// Move-constructs FrequencyVariances from CountVariances and bin width based
/// on BinEdges.
FrequencyVariances::FrequencyVariances(CountVariances &&counts,
                                       const BinEdges &edges) {
  if (!counts)
    return;
  if (!edges)
    throw std::logic_error("FrequencyVariances: Cannot construct from "
                           "CountVariances -- BinEdges are NULL.");
  if ((counts.size() + 1) != edges.size())
    if (!counts.empty() || !edges.empty())
      throw std::logic_error("FrequencyVariances: Cannot construct from "
                             "CountVariances -- BinEdges size does not match.");
  // Cannot move counts private data since it is of different type.
  m_data = counts.cowData();
  counts = Kernel::cow_ptr<HistogramE>(nullptr);
  auto &data = m_data.access();
  for (size_t i = 0; i < data.size(); ++i) {
    const auto width = edges[i + 1] - edges[i];
    data[i] /= width * width;
  }
}

} // namespace HistogramData
} // namespace Mantid
