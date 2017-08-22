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
      m_size(0), m_detectorInfo(nullptr) {}

ComponentInfo::ComponentInfo(
    boost::shared_ptr<const std::vector<size_t>> assemblySortedDetectorIndices,
    boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
        detectorRanges,
    boost::shared_ptr<const std::vector<size_t>> assemblySortedComponentIndices,
    boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
        componentRanges,
    boost::shared_ptr<const std::vector<size_t>> parentIndices,
    boost::shared_ptr<std::vector<Eigen::Vector3d>> positions,
    boost::shared_ptr<std::vector<Eigen::Quaterniond>> rotations,
    boost::shared_ptr<std::vector<Eigen::Vector3d>> scaleFactors,
    int64_t sourceIndex, int64_t sampleIndex)
    : m_assemblySortedDetectorIndices(std::move(assemblySortedDetectorIndices)),
      m_assemblySortedComponentIndices(
          std::move(assemblySortedComponentIndices)),
      m_detectorRanges(std::move(detectorRanges)),
      m_componentRanges(std::move(componentRanges)),
      m_parentIndices(std::move(parentIndices)),
      m_positions(std::move(positions)), m_rotations(std::move(rotations)),
      m_scaleFactors(std::move(scaleFactors)),
      m_size(m_assemblySortedDetectorIndices->size() +
             m_detectorRanges->size()),
      m_sourceIndex(sourceIndex), m_sampleIndex(sampleIndex) {
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
  if (m_rotations->size() != m_componentRanges->size()) {
    throw std::invalid_argument("ComponentInfo should have as many positions "
                                "and rotations as assembly sorted component "
                                "ranges");
  }
  if (m_assemblySortedComponentIndices->size() +
          m_assemblySortedDetectorIndices->size() !=
      m_size) {
    throw std::invalid_argument("ComponentInfo must have component indices "
                                "input of same size as the sum of "
                                "non-detector and detector components");
  }
  if (m_scaleFactors->size() != m_size) {
    throw std::invalid_argument(
        "ComponentInfo should have been provided same "
        "number of scale factors as number of components");
  }
}

std::vector<size_t>
ComponentInfo::detectorsInSubtree(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    /* This is a single detector. Just return the corresponding index.
     * detectorIndex == componentIndex
     */
    return std::vector<size_t>{componentIndex};
  }
  // Calculate index into our ranges (non-detector) component items.
  const auto rangesIndex = compOffsetIndex(componentIndex);
  const auto range = (*m_detectorRanges)[rangesIndex];
  // Extract as a block
  return std::vector<size_t>(
      m_assemblySortedDetectorIndices->begin() + range.first,
      m_assemblySortedDetectorIndices->begin() + range.second);
}

std::vector<size_t>
ComponentInfo::componentsInSubtree(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    /* This is a single detector. Just return the corresponding index.
     * detectorIndex == componentIndex. Never any sub-components for detectors.
     */
    return std::vector<size_t>{componentIndex};
  }
  // Calculate index into our ranges (non-detector) component items.
  const auto rangesIndex = compOffsetIndex(componentIndex);
  const auto detRange = (*m_detectorRanges)[rangesIndex];
  const auto compRange = (*m_componentRanges)[rangesIndex];

  // Extract as a block
  std::vector<size_t> indices(
      m_assemblySortedDetectorIndices->begin() + detRange.first,
      m_assemblySortedDetectorIndices->begin() + detRange.second);
  indices.insert(indices.end(),
                 m_assemblySortedComponentIndices->begin() + compRange.first,
                 m_assemblySortedComponentIndices->begin() + compRange.second);
  return indices;
}

size_t ComponentInfo::size() const { return m_size; }

Eigen::Vector3d ComponentInfo::position(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    return m_detectorInfo->position(componentIndex);
  }
  const auto rangesIndex = compOffsetIndex(componentIndex);
  return (*m_positions)[rangesIndex];
}

Eigen::Quaterniond ComponentInfo::rotation(const size_t componentIndex) const {
  if (isDetector(componentIndex)) {
    return m_detectorInfo->rotation(componentIndex);
  }
  const auto rangesIndex = compOffsetIndex(componentIndex);
  return (*m_rotations)[rangesIndex];
}

/**
 * Extract the position of a component relative to it's parent
 *
 * The parent rotatation is unwound prior to establishing the offset. This means
 *that
 * recorded relative positions are independent of changes in rotation.
 *
 * BEWARE that this method does not account for scaling factors as found in
 *RectangularDetectors
 * see Instrument::makeLegacyParmeterMap for correct handling of those cases.
 *
 * BEWARE of peformance on repeated calls to this method as the transformation
 *has to
 * be established every time, independent of whether the parent remains the
 *same.
 *
 * @param componentIndex
 * @return
 */
Eigen::Vector3d
ComponentInfo::relativePosition(const size_t componentIndex) const {
  size_t parentIndex = parent(componentIndex);
  if (parentIndex == componentIndex) {
    return position(componentIndex);
  } else {
    const auto parentPos = position(parentIndex);
    auto transformation = Eigen::Affine3d(
        rotation(parentIndex).conjugate()); // Inverse parent rotation
    transformation.translate(-parentPos);
    return transformation * position(componentIndex);
  }
}

/**
 * Extract the rotation of a component relative to it's parent
 * @param componentIndex
 * @return
 */
Eigen::Quaterniond
ComponentInfo::relativeRotation(const size_t componentIndex) const {
  size_t parentIndex = parent(componentIndex);
  if (parentIndex == componentIndex) {
    return rotation(componentIndex);
  } else {
    return rotation(parentIndex).inverse() * rotation(componentIndex);
  }
}

/**
 * Sets the rotation for a component described by target component index
 *
 * This will propagate and apply the derived position offsets to all known
 *sub-components
 *
 * @param componentIndex : Component index to update at
 * @param newPosition : Absolute position to set
 */
void ComponentInfo::setPosition(const size_t componentIndex,
                                const Eigen::Vector3d &newPosition) {
  // This method is performance critical for some client code. Optimizations are
  // explained below.
  if (isDetector(componentIndex))
    return m_detectorInfo->setPosition(componentIndex, newPosition);

  const Eigen::Vector3d offset = newPosition - position(componentIndex);

  // Optimization: Not using detectorsInSubtree and componentsInSubtree to avoid
  // memory allocations.
  // Optimization: Split loop over detectors and other components.
  const auto range = detectorRangeInSubtree(componentIndex);
  if (!range.empty())
    failIfScanning();
  // Optimization: After failIfScanning() we know that the time index is always
  // 0 so we use faster access with index pair instead of only detector index.
  for (const auto &index : range) {
    m_detectorInfo->setPosition({index, 0},
                                m_detectorInfo->position({index, 0}) + offset);
  }

  for (const auto &index : componentRangeInSubtree(componentIndex)) {
    size_t offsetIndex = compOffsetIndex(index);
    m_positions.access()[offsetIndex] += offset;
  }
}

/**
 * Sets the rotation for a component described by target component index.
 *
 * This will propagate and apply the derived rotation to all known
 *sub-components
 * This will also update derived positions for target component and all
 *sub-components
 *
 * @param componentIndex : Component index to update at
 * @param newRotation : Absolute rotation to set
 */
void ComponentInfo::setRotation(const size_t componentIndex,
                                const Eigen::Quaterniond &newRotation) {
  // This method is performance critical for some client code. Optimizations are
  // as in setRotation.
  if (isDetector(componentIndex))
    return m_detectorInfo->setRotation(componentIndex, newRotation);

  const Eigen::Vector3d compPos = position(componentIndex);
  const Eigen::Quaterniond currentRotInv = rotation(componentIndex).inverse();
  const Eigen::Quaterniond rotDelta =
      (newRotation * currentRotInv).normalized();
  auto transform = Eigen::Matrix3d(rotDelta);

  const auto range = detectorRangeInSubtree(componentIndex);
  if (!range.empty())
    failIfScanning();
  for (const auto &index : range) {
    auto newPos =
        transform * (m_detectorInfo->position({index, 0}) - compPos) + compPos;
    auto newRot = rotDelta * m_detectorInfo->rotation({index, 0});
    m_detectorInfo->setPosition({index, 0}, newPos);
    m_detectorInfo->setRotation({index, 0}, newRot);
  }

  for (const auto &index : componentRangeInSubtree(componentIndex)) {
    auto newPos = transform * (position(index) - compPos) + compPos;
    auto newRot = rotDelta * rotation(index);
    const size_t childCompIndexOffset = compOffsetIndex(index);
    m_positions.access()[childCompIndexOffset] = newPos;
    m_rotations.access()[childCompIndexOffset] = newRot.normalized();
  }
}

void ComponentInfo::failIfScanning() const {
  if (m_detectorInfo->isScanning()) {
    throw std::runtime_error(
        "Cannot move or rotate parent component containing "
        "detectors since the beamline has "
        "time-dependent (moving) detectors.");
  }
}

size_t ComponentInfo::parent(const size_t componentIndex) const {
  return (*m_parentIndices)[componentIndex];
}

bool ComponentInfo::hasParent(const size_t componentIndex) const {
  return parent(componentIndex) != componentIndex;
}

bool ComponentInfo::hasDetectorInfo() const {
  return m_detectorInfo != nullptr;
}

void ComponentInfo::setDetectorInfo(DetectorInfo *detectorInfo) {
  if (detectorInfo->size() != m_assemblySortedDetectorIndices->size()) {
    throw std::invalid_argument("ComponentInfo must have detector indices "
                                "input of same size as size of DetectorInfo");
  }
  m_detectorInfo = detectorInfo;
}

bool ComponentInfo::hasSource() const { return m_sourceIndex >= 0; }

bool ComponentInfo::hasSample() const { return m_sampleIndex >= 0; }

Eigen::Vector3d ComponentInfo::sourcePosition() const {
  if (!hasSource()) {
    throw std::runtime_error("Source component has not been specified");
  }
  return position(static_cast<size_t>(m_sourceIndex));
}

Eigen::Vector3d ComponentInfo::samplePosition() const {
  if (!hasSample()) {
    throw std::runtime_error("Sample component has not been specified");
  }
  return position(static_cast<size_t>(m_sampleIndex));
}

size_t ComponentInfo::source() const {
  if (!hasSource()) {
    throw std::runtime_error("Source component has not been specified");
  }
  return static_cast<size_t>(m_sourceIndex);
}

size_t ComponentInfo::sample() const {
  if (!hasSample()) {
    throw std::runtime_error("Sample component has not been specified");
  }
  return static_cast<size_t>(m_sampleIndex);
}

size_t ComponentInfo::root() const {
  return m_size - 1; // Root would always be the last index.
}

double ComponentInfo::l1() const {
  return (sourcePosition() - samplePosition()).norm();
}

/// Returns a Range containing all detectors in the subtree specified by index.
ComponentInfo::Range
ComponentInfo::detectorRangeInSubtree(const size_t index) const {
  const auto rangesIndex = compOffsetIndex(index);
  const auto range = (*m_detectorRanges)[rangesIndex];
  return {m_assemblySortedDetectorIndices->begin() + range.first,
          m_assemblySortedDetectorIndices->begin() + range.second};
}

/// Returns a Range containing all non-detectors in the subtree specified by
/// index.
ComponentInfo::Range
ComponentInfo::componentRangeInSubtree(const size_t index) const {
  const auto rangesIndex = compOffsetIndex(index);
  const auto range = (*m_componentRanges)[rangesIndex];
  return {m_assemblySortedComponentIndices->begin() + range.first,
          m_assemblySortedComponentIndices->begin() + range.second};
}
Eigen::Vector3d ComponentInfo::scaleFactor(const size_t componentIndex) const {
  return (*m_scaleFactors)[componentIndex];
}

void ComponentInfo::setScaleFactor(const size_t componentIndex,
                                   const Eigen::Vector3d &scaleFactor) {
  m_scaleFactors.access()[componentIndex] = scaleFactor;
}

} // namespace Beamline
} // namespace Mantid
