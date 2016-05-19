#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"

namespace Mantid {
namespace HistogramData {

CountStandardDeviations::CountStandardDeviations(
    const FrequencyStandardDeviations &frequencies, const BinEdges &edges)
    : CountStandardDeviations(FrequencyStandardDeviations(frequencies), edges) {
}

CountStandardDeviations::CountStandardDeviations(
    FrequencyStandardDeviations &&frequencies, const BinEdges &edges) {
  if (!frequencies)
    return;
  if (!edges)
    throw std::logic_error("CountStandardDeviations: Cannot construct from "
                           "FrequencyStandardDeviations -- BinEdges are NULL.");
  if ((frequencies.size() + 1) != edges.size())
    if (frequencies.size() != 0 || edges.size() != 0)
      throw std::logic_error("CountStandardDeviations: Cannot construct from "
                             "FrequencyStandardDeviations -- BinEdges size "
                             "does not "
                             "match.");
  // Cannot move frequencies private data since it is of different type.
  m_data = frequencies.cowData();
  frequencies = Kernel::cow_ptr<HistogramE>(nullptr);
  auto &data = m_data.access();
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] *= edges[i + 1] - edges[i];
  }
}

} // namespace HistogramData
} // namespace Mantid
