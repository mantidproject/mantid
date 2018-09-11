#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/FrequencyVariances.h"

namespace Mantid {
namespace HistogramData {

/// Constructs CountVariances from FrequencyVariances and bin width based on
/// BinEdges.
CountVariances::CountVariances(const FrequencyVariances &frequencies,
                               const BinEdges &edges)
    : CountVariances(FrequencyVariances(frequencies), edges) {}

/// Move-constructs CountVariances from FrequencyVariances and bin width based
/// on BinEdges.
CountVariances::CountVariances(FrequencyVariances &&frequencies,
                               const BinEdges &edges) {
  if (!frequencies)
    return;
  if (!edges)
    throw std::logic_error("CountVariances: Cannot construct from "
                           "FrequencyVariances -- BinEdges are NULL.");
  if ((frequencies.size() + 1) != edges.size())
    if (!frequencies.empty() || !edges.empty())
      throw std::logic_error("CountVariances: Cannot construct from "
                             "FrequencyVariances -- BinEdges size does not "
                             "match.");
  // Cannot move frequencies private data since it is of different type.
  m_data = frequencies.cowData();
  frequencies = Kernel::cow_ptr<HistogramE>(nullptr);
  auto &data = m_data.access();
  for (size_t i = 0; i < data.size(); ++i) {
    const auto width = edges[i + 1] - edges[i];
    data[i] *= width * width;
  }
}

} // namespace HistogramData
} // namespace Mantid
