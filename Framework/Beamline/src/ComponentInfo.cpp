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
    boost::shared_ptr<const std::vector<size_t>> assemblySortedDetectorIndices,
    boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>> ranges)
    : m_assemblySortedDetectorIndices(std::move(assemblySortedDetectorIndices)),
      m_ranges(std::move(ranges)),
      m_size(m_assemblySortedDetectorIndices->size() + m_ranges->size()) {}

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

bool ComponentInfo::operator==(const ComponentInfo &other) const {
  return m_size == other.m_size && m_ranges == other.m_ranges &&
         *m_assemblySortedDetectorIndices ==
             *other.m_assemblySortedDetectorIndices;
}
bool ComponentInfo::operator!=(const ComponentInfo &other) const {
  return !(this->operator==(other));
}
} // namespace Beamline
} // namespace Mantid
