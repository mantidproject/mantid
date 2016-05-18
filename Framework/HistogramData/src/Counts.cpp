#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Frequencies.h"

namespace Mantid {
namespace HistogramData {

Counts::Counts(const Frequencies &frequencies, const BinEdges &edges)
    : Counts(Frequencies(frequencies), edges) {}

Counts::Counts(Frequencies &&frequencies, const BinEdges &edges) {
  if (!frequencies)
    return;
  if (!edges)
    throw std::logic_error("Counts: Cannot construct from Frequencies -- BinEdges are NULL.");
  if ((frequencies.size() + 1) != edges.size())
    if (frequencies.size() != 0 || edges.size() != 0)
      throw std::logic_error("Counts: Cannot construct from Frequencies -- "
                             "BinEdges size does not match.");
  m_data = std::move(frequencies.m_data);
  auto &data = m_data.access();
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] *= (edges[i + 1] - edges[i]);
  }
}

} // namespace HistogramData
} // namespace Mantid
