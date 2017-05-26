#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
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
    boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
        detectorRanges,
    boost::shared_ptr<const std::vector<size_t>> assemblySortedComponentIndices,
    boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
        componentRanges,
    boost::shared_ptr<std::vector<Eigen::Vector3d>> positions,
    boost::shared_ptr<std::vector<Eigen::Quaterniond>> rotations,
    boost::shared_ptr<DetectorInfo> detectorInfo)
    : m_assemblySortedDetectorIndices(std::move(assemblySortedDetectorIndices)),
      m_assemblySortedComponentIndices(
          std::move(assemblySortedComponentIndices)),
      m_detectorRanges(std::move(detectorRanges)),
      m_componentRanges(std::move(componentRanges)),
      m_positions(std::move(positions)), m_rotations(std::move(rotations)),
      m_size(m_assemblySortedDetectorIndices->size() +
             m_detectorRanges->size()),
      m_detectorInfo(detectorInfo) {
  if (m_rotations->size() != m_positions->size()) {
        throw std::invalid_argument("ComponentInfo should have been provided same "
                                    "number of postions and rotations");
      }
      if (m_rotations->size() != m_detectorRanges->size()) {
        throw std::invalid_argument(
            "ComponentInfo should have as many positions "
            "and rotations as assembly sorted detector component "
            "ranges");
      }
      if (m_detectorInfo->size() != m_assemblySortedDetectorIndices->size()) {
        throw std::invalid_argument("ComponentInfo must have detector indices "
                                    "input of same size as size of DetectorInfo");
      }
      if (m_rotations->size() != m_componentRanges->size()) {
        throw std::invalid_argument(
            "ComponentInfo should have as many positions "
            "and rotations as assembly sorted component "
            "ranges");
      }
      if (m_assemblySortedComponentIndices->size() != m_size) {
        throw std::invalid_argument("ComponentInfo must have component indices "
                                    "input of same size as the sum of "
                                    "non-detector and detector components");
      }
}

bool ComponentInfo::isDetector(const size_t componentIndex) const {
  return componentIndex < m_assemblySortedDetectorIndices->size();
}

std::vector<size_t>
ComponentInfo::detectorIndices(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    /* This is a single detector. Just return the corresponding index.
     * detectorIndex == componentIndex
     */
    return std::vector<size_t>{componentIndex};
  }
  // Calculate index into our ranges (non-detector) component items.
  const auto rangesIndex =
      componentIndex - m_assemblySortedDetectorIndices->size();
  const auto range = (*m_detectorRanges)[rangesIndex];
  // Extract as a block
  return std::vector<size_t>(
      m_assemblySortedDetectorIndices->begin() + range.first,
      m_assemblySortedDetectorIndices->begin() + range.second);
}

std::vector<size_t>
ComponentInfo::componentIndices(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    /* This is a single detector. Just return the corresponding index.
     * detectorIndex == componentIndex. Never any sub-components for detectors.
     */
    return std::vector<size_t>{componentIndex};
  }
  // Calculate index into our ranges (non-detector) component items.
  const auto rangesIndex =
      componentIndex - m_assemblySortedDetectorIndices->size();
  const auto range = (*m_componentRanges)[rangesIndex];

  // Extract as a block
  return std::vector<size_t>(
      m_assemblySortedComponentIndices->begin() + range.first,
      m_assemblySortedComponentIndices->begin() + range.second);
}

size_t ComponentInfo::size() const { return m_size; }

bool ComponentInfo::operator==(const ComponentInfo &other) const {
  return m_size == other.m_size && m_detectorRanges == other.m_detectorRanges &&
         *m_assemblySortedDetectorIndices ==
             *other.m_assemblySortedDetectorIndices;
}
bool ComponentInfo::operator!=(const ComponentInfo &other) const {
  return !(this->operator==(other));
}

Eigen::Vector3d ComponentInfo::position(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    return m_detectorInfo->position(componentIndex);
  }
  const auto rangesIndex =
      componentIndex - m_assemblySortedDetectorIndices->size();
  return (*m_positions)[rangesIndex];
}

Eigen::Quaterniond ComponentInfo::rotation(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    return m_detectorInfo->rotation(componentIndex);
  }
  const auto rangesIndex =
      componentIndex - m_assemblySortedDetectorIndices->size();
  return (*m_rotations)[rangesIndex];
}

void ComponentInfo::setPosition(const size_t componentIndex,
                                const Eigen::Vector3d &newPosition) {

  const Eigen::Vector3d offset = newPosition - position(componentIndex);
  const auto subTreeIndexes = this->componentIndices(componentIndex);
  for(auto &childCompIndex : subTreeIndexes){
      if(isDetector(childCompIndex)){
          m_detectorInfo->setPosition(childCompIndex, m_detectorInfo->position(childCompIndex)+offset);
      } else {
          const size_t positionIndex = childCompIndex - m_assemblySortedDetectorIndices->size();
          m_positions.access()[positionIndex] += offset;
      }
  }
  if(isDetector(componentIndex)){
          m_detectorInfo->setPosition(componentIndex, m_detectorInfo->position(componentIndex)+offset);
  } else {
          const size_t positionIndex = componentIndex - m_assemblySortedDetectorIndices->size();
          m_positions.access()[positionIndex] += offset;
  }
}

void ComponentInfo::setRotation(const size_t componentIndex,
                                const Eigen::Quaterniond &rotation) {
  throw std::runtime_error("Not yet implemented");
}
} // namespace Beamline
} // namespace Mantid
