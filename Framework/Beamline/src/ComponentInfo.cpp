#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/make_cow.h"
#include <boost/make_shared.hpp>
#include <numeric>
#include <utility>
#include <vector>

namespace Mantid {
namespace Beamline {

namespace {
std::pair<size_t, size_t>
getIndex(const Kernel::cow_ptr<std::vector<std::pair<size_t, size_t>>> &indices,
         const size_t index) {
  if (!indices)
    return {index, 0};
  return (*indices)[index];
}
void failMerge(const std::string &what) {
  throw std::runtime_error(std::string("Cannot merge ComponentInfo: ") + what);
}

namespace {
void checkScanInterval(const std::pair<int64_t, int64_t> &interval) {
  if (interval.first >= interval.second)
    throw std::runtime_error(
        "ComponentInfo: cannot set scan interval with start >= end");
}
}
}

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
    boost::shared_ptr<std::vector<bool>> isStructuredBank, int64_t sourceIndex,
    int64_t sampleIndex)
    : m_assemblySortedDetectorIndices(std::move(assemblySortedDetectorIndices)),
      m_assemblySortedComponentIndices(
          std::move(assemblySortedComponentIndices)),
      m_detectorRanges(std::move(detectorRanges)),
      m_componentRanges(std::move(componentRanges)),
      m_parentIndices(std::move(parentIndices)),
      m_positions(std::move(positions)), m_rotations(std::move(rotations)),
      m_scaleFactors(std::move(scaleFactors)),
      m_isStructuredBank(std::move(isStructuredBank)),
      m_size(m_assemblySortedDetectorIndices->size() +
             m_detectorRanges->size()),
      m_sourceIndex(sourceIndex), m_sampleIndex(sampleIndex),
      m_detectorInfo(nullptr) {
  if (m_rotations->size() != m_positions->size()) {
    throw std::invalid_argument("ComponentInfo should have been provided same "
                                "number of postions and rotations");
  }
  if (m_rotations->size() != m_positions->size()) {
    throw std::invalid_argument("ComponentInfo should have as many positions "
                                "as rotations ranges");
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
  if (m_isStructuredBank->size() != m_componentRanges->size()) {
    throw std::invalid_argument("ComponentInfo should be provided same number "
                                "of rectangular bank flags as number of "
                                "non-detector components");
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
  checkNoTimeDependence();
  if (isDetector(componentIndex)) {
    return m_detectorInfo->position(componentIndex);
  }
  const auto rangesIndex = compOffsetIndex(componentIndex);
  return (*m_positions)[rangesIndex];
}

Eigen::Vector3d
ComponentInfo::position(const std::pair<size_t, size_t> &index) const {

  const auto componentIndex = index.first;
  if (isDetector(componentIndex)) {
    // Time index info must be same between detector and component infos!
    return m_detectorInfo->position(index);
  }
  const auto rangesIndex = compOffsetIndex(componentIndex);
  return (*m_positions)[linearIndex({rangesIndex, index.second})];
}

Eigen::Quaterniond ComponentInfo::rotation(const size_t componentIndex) const {
  checkNoTimeDependence();
  if (isDetector(componentIndex)) {
    return m_detectorInfo->rotation(componentIndex);
  }
  const auto rangesIndex = compOffsetIndex(componentIndex);
  return (*m_rotations)[rangesIndex];
}

Eigen::Quaterniond
ComponentInfo::rotation(const std::pair<size_t, size_t> &index) const {
  const auto componentIndex = index.first;
  if (isDetector(componentIndex)) {
    // Time index info must be same between detector and component infos!
    return m_detectorInfo->rotation(index);
  }
  const auto rangesIndex = compOffsetIndex(componentIndex);
  return (*m_rotations)[linearIndex({rangesIndex, index.second})];
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
  checkNoTimeDependence();
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
  checkNoTimeDependence();
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
  checkNoTimeDependence();
  if (isDetector(componentIndex))
    return m_detectorInfo->setPosition(componentIndex, newPosition);

  const Eigen::Vector3d offset = newPosition - position(componentIndex);

  // Optimization: Not using detectorsInSubtree and componentsInSubtree to avoid
  // memory allocations.
  // Optimization: Split loop over detectors and other components.
  const auto range = detectorRangeInSubtree(componentIndex);
  if (!range.empty())
    failIfDetectorInfoScanning();
  // Optimization: After failIfDetectorInfoScanning() we know that the time
  // index is always
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

void ComponentInfo::setPosition(const std::pair<size_t, size_t> index,
                                const Eigen::Vector3d &newPosition) {

  auto componentIndex = index.first;
  auto timeIndex = index.second;
  if (isDetector(componentIndex)) {
    return m_detectorInfo->setPosition(index, newPosition);
  }
  const Eigen::Vector3d offset = newPosition - position(index);

  // Optimization: Not using detectorsInSubtree and componentsInSubtree to avoid
  // memory allocations.
  // Optimization: Split loop over detectors and other components.
  const auto range = detectorRangeInSubtree(componentIndex);
  for (const auto &subIndex : range) {
    m_detectorInfo->setPosition(
        {subIndex, timeIndex},
        m_detectorInfo->position({subIndex, timeIndex}) + offset);
  }

  for (const auto &index : componentRangeInSubtree(componentIndex)) {
    size_t offsetIndex = compOffsetIndex(index);
    m_positions.access()[offsetIndex] += offset;
  }

  m_positions.access()[linearIndex(index)] = newPosition;
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
  checkNoTimeDependence();
  if (isDetector(componentIndex))
    return m_detectorInfo->setRotation(componentIndex, newRotation);

  const Eigen::Vector3d compPos = position(componentIndex);
  const Eigen::Quaterniond currentRotInv = rotation(componentIndex).inverse();
  const Eigen::Quaterniond rotDelta =
      (newRotation * currentRotInv).normalized();
  auto transform = Eigen::Matrix3d(rotDelta);

  const auto range = detectorRangeInSubtree(componentIndex);
  if (!range.empty())
    failIfDetectorInfoScanning();
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

void ComponentInfo::setRotation(const std::pair<size_t, size_t> index,
                                const Eigen::Quaterniond &newRotation) {
  const auto componentIndex = index.first;
  const auto timeIndex = index.second;
  if (isDetector(componentIndex))
    return m_detectorInfo->setRotation(index, newRotation);

  const Eigen::Vector3d compPos = position(index);
  const Eigen::Quaterniond currentRotInv = rotation(index).inverse();
  const Eigen::Quaterniond rotDelta =
      (newRotation * currentRotInv).normalized();
  auto transform = Eigen::Matrix3d(rotDelta);

  const auto range = detectorRangeInSubtree(componentIndex);
  for (const auto &subDetIndex : range) {
    auto newPos =
        transform *
            (m_detectorInfo->position({subDetIndex, timeIndex}) - compPos) +
        compPos;
    auto newRot = rotDelta * m_detectorInfo->rotation({subDetIndex, timeIndex});
    m_detectorInfo->setPosition({subDetIndex, timeIndex}, newPos);
    m_detectorInfo->setRotation({subDetIndex, timeIndex}, newRot);
  }

  for (const auto &subCompIndex : componentRangeInSubtree(componentIndex)) {
    auto newPos =
        transform * (position({subCompIndex, timeIndex}) - compPos) + compPos;
    auto newRot = rotDelta * rotation({subCompIndex, timeIndex});
    const size_t childCompIndexOffset = compOffsetIndex(subCompIndex);
    m_positions.access()[linearIndex({childCompIndexOffset, timeIndex})] =
        newPos;
    m_rotations.access()[linearIndex({childCompIndexOffset, timeIndex})] =
        newRot.normalized();
  }
}

void ComponentInfo::failIfDetectorInfoScanning() const {
  if (m_detectorInfo->isScanning()) {
    throw std::runtime_error(
        "Cannot move or rotate parent component containing "
        "detectors since the beamline has "
        "time-dependent (moving) detectors.");
  }
}

size_t
ComponentInfo::linearIndex(const std::pair<size_t, size_t> &index) const {
  // The most common case are beamlines with static components. In that case the
  // time index is always 0 and we avoid expensive map lookups. Linear indices
  // are ordered such that the first block contains everything for time index 0
  // so even in the time dependent case no translation is necessary.
  if (index.second == 0)
    return index.first;
  // Calculate the linear index without a lookup
  if (m_isSyncScan) {
    const size_t nNonDetectorComponents = m_detectorRanges->size();
    return index.first + nNonDetectorComponents * index.second;
  }
  return (*m_indexMap)[index.first][index.second];
}

void ComponentInfo::initIndices() {
  checkNoTimeDependence();
  m_indexMap = Kernel::make_cow<std::vector<std::vector<size_t>>>();
  m_indices = Kernel::make_cow<std::vector<std::pair<size_t, size_t>>>();
  auto &indexMap = m_indexMap.access();
  auto &indices = m_indices.access();
  indexMap.reserve(m_detectorRanges->size());
  indices.reserve(m_detectorRanges->size());
  // No time dependence, so both the component index and the linear index are i.
  for (size_t i = 0; i < m_detectorRanges->size(); ++i) {
    indexMap.emplace_back(1, i);
    indices.emplace_back(i, 0);
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
  // Getting position with time index to bypass scanning check. Sources are not
  // scanned.
  return position({static_cast<size_t>(m_sourceIndex), 0});
}

Eigen::Vector3d ComponentInfo::samplePosition() const {
  if (!hasSample()) {
    throw std::runtime_error("Sample component has not been specified");
  }
  // Getting position with time index to bypass scanning check. Samples are not
  // scanned.
  return position({static_cast<size_t>(m_sampleIndex), 0});
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

bool ComponentInfo::isStructuredBank(const size_t componentIndex) const {
  const auto rangesIndex = compOffsetIndex(componentIndex);
  return !isDetector(componentIndex) && (*m_isStructuredBank)[rangesIndex];
}

/**
 * Get the number of scans for component index
 * @param index : Component Index
 * @return Number of scans for component index
 */
size_t ComponentInfo::scanCount(const size_t index) const {
  if (m_detectorInfo && isDetector(index))
    return m_detectorInfo->scanCount(index);
  if (!m_scanCounts) {
    return 1;
  }
  if (m_isSyncScan) {
    return (*m_scanCounts)[0];
  }
  return (*m_scanCounts)[compOffsetIndex(index)];
}

size_t ComponentInfo::scanSize() const {
  const auto detectorScanSize = m_detectorInfo ? m_detectorInfo->scanSize() : 0;
  if (!m_positions)
    return 0 + detectorScanSize;
  return m_positions->size() + detectorScanSize;
}

bool ComponentInfo::isScanning() const {
  if (m_detectorInfo && m_detectorInfo->isScanning())
    return true;
  else if (!m_positions || !m_componentRanges)
    return false;
  else
    // TODO what if you have a scan that causes no position change, such as a
    // rotation. Not scanning???
    return m_componentRanges->size() != m_positions->size();
}

/// Throws if this has time-dependent data.
void ComponentInfo::checkNoTimeDependence() const {
  if (isScanning())
    throw std::runtime_error(
        "ComponentInfo accessed without time index but the "
        "beamline has time-dependent (moving) components.");
}

/**
 * Retrieve the scan interval for a component at a time index
 * @param index component index, time index pair
 * @return offset interval times since epoch
 */
std::pair<int64_t, int64_t>
ComponentInfo::scanInterval(const std::pair<size_t, size_t> &index) const {
  if (m_detectorInfo && isDetector(index.first))
    return m_detectorInfo->scanInterval(index);
  if (!m_scanIntervals)
    return {0, 0};
  if (m_isSyncScan)
    return (*m_scanIntervals)[index.second];
  return (*m_scanIntervals)[linearIndex(
      {compOffsetIndex(index.first), index.second})];
}

/**
 * Set the scan interval using nanosecond offsets from 00:00:00 01/01/1990 epoch
 *
 * This MUST be called before any merging has happened, therefore index is the
 *time-independent
 * component index.
 *
 * @param index : component to which to add the scan interval
 * component
 * @param interval : Time interval for scan point
 */
void ComponentInfo::setScanInterval(
    const size_t index, const std::pair<int64_t, int64_t> &interval) {
  // Enforces setting scan intervals BEFORE time indexed positions and rotations
  checkNoTimeDependence();
  checkScanInterval(interval);
  if (m_scanIntervals && m_isSyncScan) {
    throw std::runtime_error("ComponentInfo has been initialized with a "
                             "synchonous scan, cannot set scan interval for "
                             "individual component.");
  }
  if (!m_scanIntervals) {
    m_isSyncScan = false;
    initScanIntervals();
  }
  for (auto subIndex : this->componentsInSubtree(index)) {
    if (m_detectorInfo && isDetector(subIndex)) {
      m_detectorInfo->setScanInterval(subIndex, interval);
    } else {
      m_scanIntervals.access()[compOffsetIndex(subIndex)] = interval;
    }
  }
}

void ComponentInfo::setScanInterval(
    const std::pair<int64_t, int64_t> &interval) {
  // Enforces setting scan intervals BEFORE time indexed positions and rotations
  checkNoTimeDependence();
  checkScanInterval(interval);
  if (m_scanIntervals && !m_isSyncScan) {
    throw std::runtime_error(
        "ComponentInfo has been initialized with a "
        "asynchonous scan, cannot set synchronous scan interval.");
  }
  if (!m_scanIntervals) {
    m_isSyncScan = true;
    initScanIntervals();
  }
  m_scanIntervals.access()[0] = interval;
  if (m_detectorInfo) {
    m_detectorInfo->setScanInterval(interval);
  }
}

/**
Merges the contents of other ComponentInfo into this. The assumption is that
this has no time dependence prior to this operation.
 *
 * Scan intervals in both other and this must be set. Intervals must be
 * identical or non-overlapping. If they are identical all other parameters (for
 * that index) must match.
 *
 * Time indices in `this` are preserved. Time indices added from `other` are
 * incremented by the scan count of that detector in `this`. The relative order
 * of time indices added from `other` is preserved. If the interval for a time
 * index in `other` is identical to a corresponding interval in `this`, it is
 * ignored, i.e., no time index is added.
**/
void ComponentInfo::merge(const ComponentInfo &other) {
  if (!m_scanCounts)
    this->initScanCounts();
  const auto toMerge = buildMergeIndices(other);
  if (m_isSyncScan)
    throw std::runtime_error(
        "Sync scans not currently supported for ComponentInfo");
  initIndices();
  auto scanCounts(m_scanCounts);
  for (size_t linearIndex = 0; linearIndex < other.m_positions->size();
       ++linearIndex) {
    if (!toMerge[linearIndex])
      continue;
    auto newIndex = getIndex(other.m_indices, linearIndex);
    const size_t compIndex = newIndex.first;
    newIndex.second += scanCount(compIndex);
    scanCounts.access()[compIndex]++;
    m_indexMap.access()[compIndex].push_back((*m_indices).size());
    m_indices.access().push_back(newIndex);
    m_positions.access().push_back((*other.m_positions)[linearIndex]);
    m_rotations.access().push_back((*other.m_rotations)[linearIndex]);
    m_scanIntervals.access().push_back((*other.m_scanIntervals)[linearIndex]);
  }
  m_scanCounts = std::move(scanCounts);
  m_detectorInfo->merge(*other.m_detectorInfo);
}

/**
 *
 * @param other : ComponentInfo
 * @return vector of linear indices to merge (true to merge)
 */
std::vector<bool>
ComponentInfo::buildMergeIndices(const ComponentInfo &other) const {
  checkSizes(other);

  size_t mergeSize = other.m_positions->size();
  std::vector<bool> merge(mergeSize, true);

  for (size_t linearIndex1 = 0; linearIndex1 < other.m_positions->size();
       ++linearIndex1) {
    const size_t compIndex = getIndex(other.m_indices, linearIndex1).first;
    const auto &interval1 = (*other.m_scanIntervals)[linearIndex1];
    for (size_t timeIndex = 0; timeIndex < (*m_scanCounts)[compIndex];
         ++timeIndex) {
      const auto linearIndex2 = linearIndex({compIndex, timeIndex});
      const auto &interval2 = (*m_scanIntervals)[linearIndex2];
      if (interval1 == interval2) {
        checkIdenticalIntervals(other, linearIndex1, linearIndex2);
        merge[linearIndex1] = false;
      } else if ((interval1.first < interval2.second) &&
                 (interval1.second > interval2.first)) {
        failMerge("scan intervals overlap but not identical");
      }
    }
  }
  return merge;
}

void ComponentInfo::checkSizes(const ComponentInfo &other) const {
  if (size() != other.size())
    failMerge("size mismatch");
  if (!m_scanIntervals || !other.m_scanIntervals)
    failMerge("scan intervals not defined");
  if (m_isSyncScan != other.m_isSyncScan)
    failMerge("both or none of the scans must be synchronous");
}

void ComponentInfo::checkIdenticalIntervals(
    const ComponentInfo &other, const size_t linearIndexOther,
    const size_t linearIndexThis) const {
  if ((*m_positions)[linearIndexThis] != (*other.m_positions)[linearIndexOther])
    failMerge("matching scan interval but positions differ");
  if ((*m_rotations)[linearIndexThis].coeffs() !=
      (*other.m_rotations)[linearIndexOther].coeffs())
    failMerge("matching scan interval but rotations differ");
}

void ComponentInfo::initScanCounts() {
  checkNoTimeDependence();
  if (m_isSyncScan)
    m_scanCounts = Kernel::make_cow<std::vector<size_t>>(1, 1);
  else {
    const size_t componentsSize = this->m_detectorRanges->size();
    m_scanCounts = Kernel::make_cow<std::vector<size_t>>(componentsSize, 1);
  }
}

void ComponentInfo::initScanIntervals() {
  checkNoTimeDependence();
  if (m_isSyncScan) {
    m_scanIntervals =
        Kernel::make_cow<std::vector<std::pair<int64_t, int64_t>>>(
            1, std::pair<int64_t, int64_t>{0, 1});
  } else {
    const size_t componentsSize = this->m_detectorRanges->size();
    m_scanIntervals =
        Kernel::make_cow<std::vector<std::pair<int64_t, int64_t>>>(
            componentsSize, std::pair<int64_t, int64_t>{0, 1});
  }
}

} // namespace Beamline
} // namespace Mantid
