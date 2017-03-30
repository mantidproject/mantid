#include "MantidBeamline/ComponentInfo.h"
#include <boost/make_shared.hpp>
#include <numeric>
#include <utility>
#include <vector>

namespace Mantid {
namespace Beamline {

ComponentInfo::ComponentInfo(
    const std::vector<size_t> &assemblySortedDetectorIndices,
    const std::vector<std::pair<size_t, size_t>> &ranges,
    boost::shared_ptr<std::vector<Eigen::Vector3d>> positions,
    boost::shared_ptr<std::vector<Eigen::Quaterniond>> rotations)
    : m_assemblySortedDetectorIndices(boost::make_shared<std::vector<size_t>>(
          assemblySortedDetectorIndices)),
      m_ranges(boost::make_shared<const std::vector<std::pair<size_t, size_t>>>(
          ranges)),
      m_positions(positions), m_rotations(rotations),
      m_size(m_assemblySortedDetectorIndices->size() + ranges.size()) {

  if (m_rotations->size() != m_positions->size()) {
    throw std::invalid_argument("ComponentInfo should have been provided same "
                                "number of postions and rotations");
  }
  if (m_rotations->size() != m_ranges->size()) {
    throw std::invalid_argument("ComponentInfo should have as many positions "
                                "and rotations as non-detector component "
                                "ranges");
  }
}

bool ComponentInfo::isDetectorDomain(const size_t componentIndex) const {
  return componentIndex < m_assemblySortedDetectorIndices->size();
}

std::vector<size_t>
ComponentInfo::detectorIndices(const size_t componentIndex) const {
  if (isDetectorDomain(componentIndex)) {
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

Eigen::Vector3d ComponentInfo::position(const size_t componentIndex) const {
  if (isDetectorDomain(componentIndex)) {
    throw std::runtime_error("No detector info yet");
  }
  const auto rangesIndex =
      componentIndex - m_assemblySortedDetectorIndices->size();
  return (*m_positions)[rangesIndex];
}

Eigen::Quaterniond ComponentInfo::rotation(const size_t componentIndex) const {
  if (isDetectorDomain(componentIndex)) {
    throw std::runtime_error("No detector info yet");
  }
  const auto rangesIndex =
      componentIndex - m_assemblySortedDetectorIndices->size();
  return (*m_rotations)[rangesIndex];
}

} // namespace Beamline
} // namespace Mantid
