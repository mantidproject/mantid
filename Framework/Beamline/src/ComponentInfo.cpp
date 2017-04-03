#include "MantidBeamline/ComponentInfo.h"
#include <boost/make_shared.hpp>
#include <numeric>
#include <utility>
#include <vector>

namespace Mantid {
namespace Beamline {

ComponentInfo::ComponentInfo()
    : m_assemblySortedDetectorIndices(
          boost::make_shared<std::vector<size_t>>(0)),
      m_size(0) {}

ComponentInfo::ComponentInfo(
    const std::vector<size_t> &assemblySortedDetectorIndices,
    const std::vector<std::pair<size_t, size_t>> &ranges)
    : m_assemblySortedDetectorIndices(boost::make_shared<std::vector<size_t>>(
          assemblySortedDetectorIndices)),
      m_ranges(boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          ranges)),
      m_size(m_assemblySortedDetectorIndices->size() + ranges.size()) {}

std::vector<size_t>
ComponentInfo::detectorIndices(const size_t componentIndex) const {
  if (componentIndex < m_assemblySortedDetectorIndices->size()) {
    /* This is a single detector. Just return the corresponding index.
     * detectorIndex == componentIndex
     */
    return std::vector<size_t>{componentIndex};
  }
  // Calculate index into our ranges (non-detector) component items.
  const auto rangesIndex =
      componentIndex - m_assemblySortedDetectorIndices->size();
  const auto range = (*m_ranges)[rangesIndex];
  // Extract as a block
  return std::vector<size_t>(
      m_assemblySortedDetectorIndices->begin() + range.first,
      m_assemblySortedDetectorIndices->begin() + range.second);
}

size_t ComponentInfo::size() const { return m_size; }

} // namespace Beamline
} // namespace Mantid
