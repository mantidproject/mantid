#include "MantidBeamline/ComponentInfo.h"
#include <boost/make_shared.hpp>
#include <numeric>
#include <utility>
#include <vector>

namespace Mantid {
namespace Beamline {

ComponentInfo::ComponentInfo()
    : m_detectorIndices(boost::make_shared<std::vector<size_t>>(0)),
      m_ranges(
          boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(0)),
      m_size(0) {}

ComponentInfo::ComponentInfo(
    const std::vector<std::pair<size_t, size_t>> &ranges,
    const size_t detectorsSize)
    : m_detectorIndices(boost::make_shared<std::vector<size_t>>(detectorsSize)),
      m_ranges(boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          ranges)),
      m_size(detectorsSize + ranges.size()) {
  // fill 0-n
  std::iota(m_detectorIndices->begin(), m_detectorIndices->end(), 0);
}

std::vector<size_t>
ComponentInfo::detectorIndices(const size_t componentIndex) const {
  if (componentIndex < m_detectorIndices->size()) {
    // This is a single detector. Just return the corresponding index.
    return std::vector<size_t>{(*m_detectorIndices)[componentIndex]};
  }
  // Calculate index into our ranges (non-detector) component items.
  const auto rangesIndex = componentIndex - m_detectorIndices->size();
  const auto range = m_ranges->at(rangesIndex);
  // Extract as a block
  return std::vector<size_t>(m_detectorIndices->begin() + range.first,
                             m_detectorIndices->begin() + range.second);
}

size_t ComponentInfo::size() const { return m_size; }

} // namespace Beamline
} // namespace Mantid
