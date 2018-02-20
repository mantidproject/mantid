#include "MantidBeamline/DetectorInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidKernel/make_cow.h"

#include <algorithm>

namespace Mantid {
namespace Beamline {

DetectorInfo::DetectorInfo(
    std::vector<Eigen::Vector3d> positions,
    std::vector<Eigen::Quaterniond,
                Eigen::aligned_allocator<Eigen::Quaterniond>> rotations)
    : m_isMonitor(Kernel::make_cow<std::vector<bool>>(positions.size())),
      m_isMasked(Kernel::make_cow<std::vector<bool>>(positions.size())),
      m_positions(
          Kernel::make_cow<std::vector<Eigen::Vector3d>>(std::move(positions))),
      m_rotations(Kernel::make_cow<std::vector<
          Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>(
          std::move(rotations))) {
  if (m_positions->size() != m_rotations->size())
    throw std::runtime_error("DetectorInfo: Position and rotations vectors "
                             "must have identical size");
}

DetectorInfo::DetectorInfo(
    std::vector<Eigen::Vector3d> positions,
    std::vector<Eigen::Quaterniond,
                Eigen::aligned_allocator<Eigen::Quaterniond>> rotations,
    const std::vector<size_t> &monitorIndices)
    : DetectorInfo(std::move(positions), std::move(rotations)) {
  for (const auto i : monitorIndices)
    m_isMonitor.access().at(i) = true;
}

/** Returns true if the content of this is equivalent to the content of other.
 *
 * Here "equivalent" implies equality of all member, except for positions and
 * rotations, which are treated specially:
 * - Positions that differ by less than 1 nm = 1e-9 m are considered equivalent.
 * - Rotations that imply relative position changes of less than 1 nm = 1e-9 m
 *   with a rotation center that is 1000 m away are considered equivalent.
 * Note that in both cases the actual limit may be lower, but it is guaranteed
 * that any LARGER differences are NOT considered equivalent. */
bool DetectorInfo::isEquivalent(const DetectorInfo &other) const {
  if (this == &other)
    return true;
  // Same number of detectors
  if (size() != other.size())
    return false;
  if (size() == 0)
    return true;
  if (!(m_isMonitor == other.m_isMonitor) &&
      (*m_isMonitor != *other.m_isMonitor))
    return false;
  if (!(m_isMasked == other.m_isMasked) && (*m_isMasked != *other.m_isMasked))
    return false;

  // Scanning related fields. Not testing m_scanCounts and m_indexMap since
  // those just are internally derived from m_indices.
  if (m_scanIntervals && other.m_scanIntervals &&
      !(m_scanIntervals == other.m_scanIntervals) &&
      (*m_scanIntervals != *other.m_scanIntervals))
    return false;
  if (m_indices && other.m_indices && !(m_indices == other.m_indices) &&
      (*m_indices != *other.m_indices))
    return false;

  // Positions: Absolute difference matter, so comparison is not relative.
  // Changes below 1 nm = 1e-9 m are allowed.
  if (!(m_positions == other.m_positions) &&
      !std::equal(m_positions->begin(), m_positions->end(),
                  other.m_positions->begin(),
                  [](const Eigen::Vector3d &a, const Eigen::Vector3d &b) {
                    return (a - b).norm() < 1e-9;
                  }))
    return false;
  // At a distance of L = 1000 m (a reasonable upper limit for instrument sizes)
  // from the rotation center we want a difference of less than d = 1 nm = 1e-9
  // m). We have, using small angle approximation,
  // d \approx theta * L.
  // The norm of the imaginary part of the quaternion can give the angle:
  // x^2 + y^2 + z^2 = (sin(theta/2))^2.
  constexpr double d_max = 1e-9;
  constexpr double L = 1000.0;
  constexpr double safety_factor = 2.0;
  const double imag_norm_max = sin(d_max / (2.0 * L * safety_factor));
  if (!(m_rotations == other.m_rotations) &&
      !std::equal(m_rotations->begin(), m_rotations->end(),
                  other.m_rotations->begin(),
                  [imag_norm_max](const Eigen::Quaterniond &a,
                                  const Eigen::Quaterniond &b) {
                    return (a * b.conjugate()).vec().norm() < imag_norm_max;
                  }))
    return false;
  return true;
}

/** Returns the number of sum of the scan intervals for every detector in the
 *instrument.
 *
 * If a detector is moving, i.e., has more than one associated position, every
 *position is counted. */
size_t DetectorInfo::scanSize() const {
  if (!m_positions)
    return 0;
  return m_positions->size();
}

/**
 * Returns true if all of the detectors all have the same scan interval. Will
 * return false if DetectorInfo is not scanning.
 */
bool DetectorInfo::isSyncScan() const { return isScanning() && m_isSyncScan; }

/// Returns true if the detector with given detector index is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const {
  // No check for time dependence since monitor flags are not time dependent.
  return (*m_isMonitor)[index];
}

/** Returns true if the detector with given index is a monitor.
 *
 * The time component of the index is ignored since a detector is a monitor
 * either for *all* times or for *none*. */
bool DetectorInfo::isMonitor(const std::pair<size_t, size_t> &index) const {
  // Monitors are not time dependent, ignore time component of index.
  return (*m_isMonitor)[index.first];
}

/** Returns true if the detector with given detector index is masked.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
bool DetectorInfo::isMasked(const size_t index) const {
  checkNoTimeDependence();
  return (*m_isMasked)[index];
}

/// Returns true if the detector with given index is masked.
bool DetectorInfo::isMasked(const std::pair<size_t, size_t> &index) const {
  return (*m_isMasked)[linearIndex(index)];
}

/** Set the mask flag of the detector with given detector index. Not thread
 * safe.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
void DetectorInfo::setMasked(const size_t index, bool masked) {
  checkNoTimeDependence();
  m_isMasked.access()[index] = masked;
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const std::pair<size_t, size_t> &index,
                             bool masked) {
  m_isMasked.access()[linearIndex(index)] = masked;
}

/// Returns the scan count of the detector with given detector index.
size_t DetectorInfo::scanCount(const size_t index) const {
  if (!m_scanCounts)
    return 1;
  if (m_isSyncScan)
    return (*m_scanCounts)[0];
  return (*m_scanCounts)[index];
}

/** Returns the scan interval of the detector with given index.
 *
 * The interval start and end values would typically correspond to nanoseconds
 * since 1990, as in Types::Core::DateAndTime. */
std::pair<int64_t, int64_t>
DetectorInfo::scanInterval(const std::pair<size_t, size_t> &index) const {
  if (!m_scanIntervals)
    return {0, 0};
  if (m_isSyncScan)
    return (*m_scanIntervals)[index.second];
  return (*m_scanIntervals)[linearIndex(index)];
}

namespace {
void checkScanInterval(const std::pair<int64_t, int64_t> &interval) {
  if (interval.first >= interval.second)
    throw std::runtime_error(
        "DetectorInfo: cannot set scan interval with start >= end");
}
}

/** Set the scan interval of the detector with given detector index.
 *
 * The interval start and end values would typically correspond to nanoseconds
 * since 1990, as in Types::Core::DateAndTime. Note that it is currently not
 *possible
 * to modify scan intervals for a DetectorInfo with time-dependent detectors,
 * i.e., time intervals must be set with this method before merging individual
 * scans. */
void DetectorInfo::setScanInterval(
    const size_t index, const std::pair<int64_t, int64_t> &interval) {
  // Time intervals must be set up before adding time sensitive
  // positions/rotations hence check below.
  checkNoTimeDependence();
  checkScanInterval(interval);
  if (!m_scanIntervals)
    initScanIntervals();
  if (m_isSyncScan)
    throw std::runtime_error("DetectorInfo has been initialized with a "
                             "synchonous scan, cannot set scan interval for "
                             "individual detector.");
  m_scanIntervals.access()[index] = interval;
}

/** Set the scan interval for all detectors.
 *
 * Prefer this over setting intervals for individual detectors since it enables
 * internal performance optimization. See also overload for other details. */
void DetectorInfo::setScanInterval(
    const std::pair<int64_t, int64_t> &interval) {
  checkNoTimeDependence();
  checkScanInterval(interval);
  if (!m_scanIntervals) {
    m_scanIntervals =
        Kernel::make_cow<std::vector<std::pair<int64_t, int64_t>>>(
            1, std::pair<int64_t, int64_t>{0, 1});
  }
  if (!m_isSyncScan) {
    throw std::runtime_error(
        "DetectorInfo has been initialized with a "
        "asynchonous scan, cannot set synchronous scan interval.");
  }
  m_scanIntervals.access()[0] = interval;
}

namespace {
void failMerge(const std::string &what) {
  throw std::runtime_error(std::string("Cannot merge DetectorInfo: ") + what);
}

std::pair<size_t, size_t>
getIndex(const Kernel::cow_ptr<std::vector<std::pair<size_t, size_t>>> &indices,
         const size_t index) {
  if (!indices)
    return {index, 0};
  return (*indices)[index];
}
}

/** Merges the contents of other into this.
 *
 * Scan intervals in both other and this must be set. Intervals must be
 * identical or non-overlapping. If they are identical all other parameters (for
 * that index) must match.
 *
 * Time indices in `this` are preserved. Time indices added from `other` are
 * incremented by the scan count of that detector in `this`. The relative order
 * of time indices added from `other` is preserved. If the interval for a time
 * index in `other` is identical to a corresponding interval in `this`, it is
 * ignored, i.e., no time index is added. */
void DetectorInfo::merge(const DetectorInfo &other) {
  if (!m_scanCounts)
    initScanCounts();
  if (m_isSyncScan) {
    const auto &merge = buildMergeSyncScanIndices(other);
    for (size_t timeIndex = 0; timeIndex < other.m_scanIntervals->size();
         ++timeIndex) {
      if (!merge[timeIndex])
        continue;
      auto &scanIntervals = m_scanIntervals.access();
      auto &isMasked = m_isMasked.access();
      auto &positions = m_positions.access();
      auto &rotations = m_rotations.access();
      m_scanCounts.access()[0]++;
      scanIntervals.push_back((*other.m_scanIntervals)[timeIndex]);
      const size_t indexStart = other.linearIndex({0, timeIndex});
      size_t indexEnd = indexStart + size();
      isMasked.insert(isMasked.end(), other.m_isMasked->begin() + indexStart,
                      other.m_isMasked->begin() + indexEnd);
      positions.insert(positions.end(), other.m_positions->begin() + indexStart,
                       other.m_positions->begin() + indexEnd);
      rotations.insert(rotations.end(), other.m_rotations->begin() + indexStart,
                       other.m_rotations->begin() + indexEnd);
    }
    return;
  }
  const auto &merge = buildMergeIndices(other);
  if (!m_indexMap)
    initIndices();
  // Temporary to accumulate scan counts (need original for index offset).
  auto scanCounts(m_scanCounts);
  for (size_t linearIndex = 0; linearIndex < other.m_positions->size();
       ++linearIndex) {
    if (!merge[linearIndex])
      continue;
    auto newIndex = getIndex(other.m_indices, linearIndex);
    const size_t detIndex = newIndex.first;
    newIndex.second += scanCount(detIndex);
    scanCounts.access()[detIndex]++;
    m_indexMap.access()[detIndex].push_back((*m_indices).size());
    m_indices.access().push_back(newIndex);
    m_isMasked.access().push_back((*other.m_isMasked)[linearIndex]);
    m_positions.access().push_back((*other.m_positions)[linearIndex]);
    m_rotations.access().push_back((*other.m_rotations)[linearIndex]);
    m_scanIntervals.access().push_back((*other.m_scanIntervals)[linearIndex]);
  }
  m_scanCounts = std::move(scanCounts);
}

void DetectorInfo::setComponentInfo(ComponentInfo *componentInfo) {
  m_componentInfo = componentInfo;
}

bool DetectorInfo::hasComponentInfo() const {
  return m_componentInfo != nullptr;
}

double DetectorInfo::l1() const {
  // TODO Not scan safe yet for scanning ComponentInfo
  if (!hasComponentInfo()) {
    throw std::runtime_error(
        "DetectorInfo has no valid ComponentInfo thus cannot determine l1");
  }
  return m_componentInfo->l1();
}

Eigen::Vector3d DetectorInfo::sourcePosition() const {
  // TODO Not scan safe yet for scanning ComponentInfo
  if (!hasComponentInfo()) {
    throw std::runtime_error("DetectorInfo has no valid ComponentInfo thus "
                             "cannot determine sourcePosition");
  }
  return m_componentInfo->sourcePosition();
}

Eigen::Vector3d DetectorInfo::samplePosition() const {
  // TODO Not scan safe yet for scanning ComponentInfo
  if (!hasComponentInfo()) {
    throw std::runtime_error("DetectorInfo has no valid ComponentInfo thus "
                             "cannot determine samplePosition");
  }
  return m_componentInfo->samplePosition();
}

void DetectorInfo::initScanCounts() {
  checkNoTimeDependence();
  if (m_isSyncScan)
    m_scanCounts = Kernel::make_cow<std::vector<size_t>>(1, 1);
  else
    m_scanCounts = Kernel::make_cow<std::vector<size_t>>(size(), 1);
}

void DetectorInfo::initScanIntervals() {
  checkNoTimeDependence();
  m_isSyncScan = false;
  m_scanIntervals = Kernel::make_cow<std::vector<std::pair<int64_t, int64_t>>>(
      size(), std::pair<int64_t, int64_t>{0, 1});
}

void DetectorInfo::initIndices() {
  checkNoTimeDependence();
  m_indexMap = Kernel::make_cow<std::vector<std::vector<size_t>>>();
  m_indices = Kernel::make_cow<std::vector<std::pair<size_t, size_t>>>();
  auto &indexMap = m_indexMap.access();
  auto &indices = m_indices.access();
  indexMap.reserve(size());
  indices.reserve(size());
  // No time dependence, so both the detector index and the linear index are i.
  for (size_t i = 0; i < size(); ++i) {
    indexMap.emplace_back(1, i);
    indices.emplace_back(i, 0);
  }
}

// Indices returned here are the list of linear indexes not to merge
std::vector<bool>
DetectorInfo::buildMergeIndices(const DetectorInfo &other) const {
  checkSizes(other);
  std::vector<bool> merge(other.m_positions->size(), true);

  for (size_t linearIndex1 = 0; linearIndex1 < other.m_positions->size();
       ++linearIndex1) {
    const size_t detIndex = getIndex(other.m_indices, linearIndex1).first;
    const auto &interval1 = (*other.m_scanIntervals)[linearIndex1];
    for (size_t timeIndex = 0; timeIndex < scanCount(detIndex); ++timeIndex) {
      const auto linearIndex2 = linearIndex({detIndex, timeIndex});
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

// Indices returned here are the list of time indexes not to merge
std::vector<bool>
DetectorInfo::buildMergeSyncScanIndices(const DetectorInfo &other) const {
  checkSizes(other);
  std::vector<bool> merge(other.m_scanIntervals->size(), true);

  for (size_t t1 = 0; t1 < other.m_scanIntervals->size(); ++t1) {
    for (size_t t2 = 0; t2 < m_scanIntervals->size(); ++t2) {
      const auto &interval1 = (*other.m_scanIntervals)[t1];
      const auto &interval2 = (*m_scanIntervals)[t2];
      if (interval1 == interval2) {
        for (size_t detIndex = 0; detIndex < size(); ++detIndex) {
          const size_t linearIndex1 = other.linearIndex({detIndex, t1});
          const size_t linearIndex2 = linearIndex({detIndex, t2});
          checkIdenticalIntervals(other, linearIndex1, linearIndex2);
        }
        merge[t1] = false;
      } else if ((interval1.first < interval2.second) &&
                 (interval1.second > interval2.first)) {
        failMerge("sync scan intervals overlap but not identical");
      }
    }
  }
  return merge;
}

void DetectorInfo::checkSizes(const DetectorInfo &other) const {
  if (size() != other.size())
    failMerge("size mismatch");
  if (!m_scanIntervals || !other.m_scanIntervals)
    failMerge("scan intervals not defined");
  if (m_isSyncScan != other.m_isSyncScan)
    failMerge("both or none of the scans must be synchronous");
  if (!(m_isMonitor == other.m_isMonitor) &&
      (*m_isMonitor != *other.m_isMonitor))
    failMerge("monitor flags mismatch");
  // TODO If we make masking time-independent we need to check masking here.
}

void DetectorInfo::checkIdenticalIntervals(const DetectorInfo &other,
                                           const size_t linearIndexOther,
                                           const size_t linearIndexThis) const {
  if ((*m_isMasked)[linearIndexThis] != (*other.m_isMasked)[linearIndexOther])
    failMerge("matching scan interval but mask flags differ");
  if ((*m_positions)[linearIndexThis] != (*other.m_positions)[linearIndexOther])
    failMerge("matching scan interval but positions differ");
  if ((*m_rotations)[linearIndexThis].coeffs() !=
      (*other.m_rotations)[linearIndexOther].coeffs())
    failMerge("matching scan interval but rotations differ");
}

} // namespace Beamline
} // namespace Mantid
