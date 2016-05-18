#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Frequencies.h"

namespace Mantid {
namespace HistogramData {

Frequencies::Frequencies(const Counts &counts, const BinEdges &edges)
    : Frequencies(Counts(counts), edges) {}

Frequencies::Frequencies(Counts &&counts, const BinEdges &edges) {
  if (!counts)
    return;
  if (!edges)
    throw std::logic_error(
        "Frequencies: Cannot construct from Counts -- BinEdges are NULL.");
  if ((counts.size() + 1) != edges.size())
    if (counts.size() != 0 || edges.size() != 0)
      throw std::logic_error("Frequencies: Cannot construct from Counts -- "
                             "BinEdges size does not match.");
  m_data = std::move(counts.m_data);
  auto &data = m_data.access();
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] /= (edges[i + 1] - edges[i]);
  }
}

} // namespace HistogramData
} // namespace Mantid
