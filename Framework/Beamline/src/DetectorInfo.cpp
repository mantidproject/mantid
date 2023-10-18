// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidBeamline/DetectorInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidKernel/make_cow.h"

#include <algorithm>
#include <exception>

namespace Mantid::Beamline {

DetectorInfo::DetectorInfo(std::vector<Eigen::Vector3d> positions,
                           std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>> rotations)
    : m_isMonitor(Kernel::make_cow<std::vector<bool>>(positions.size())),
      m_isMasked(Kernel::make_cow<std::vector<bool>>(positions.size())),
      m_positions(Kernel::make_cow<std::vector<Eigen::Vector3d>>(std::move(positions))),
      m_rotations(Kernel::make_cow<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>(
          std::move(rotations))) {
  if (m_positions->size() != m_rotations->size())
    throw std::runtime_error("DetectorInfo: Position and rotations vectors "
                             "must have identical size");
}

DetectorInfo::DetectorInfo(std::vector<Eigen::Vector3d> positions,
                           std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>> rotations,
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
  if (!(m_isMonitor == other.m_isMonitor) && (*m_isMonitor != *other.m_isMonitor))
    return false;
  if (!(m_isMasked == other.m_isMasked) && (*m_isMasked != *other.m_isMasked))
    return false;

  // Scanning related fields. Not testing m_indexMap since
  // those just are internally derived from m_indices.
  if (this->hasComponentInfo() && (this->scanIntervals() != other.scanIntervals()))
    return false;

  // Positions: Absolute difference matter, so comparison is not relative.
  // Changes below 1 nm = 1e-9 m are allowed.
  if (!(m_positions == other.m_positions) &&
      !std::equal(m_positions->begin(), m_positions->end(), other.m_positions->begin(),
                  [](const Eigen::Vector3d &a, const Eigen::Vector3d &b) { return (a - b).norm() < 1e-9; }))
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
      !std::equal(m_rotations->begin(), m_rotations->end(), other.m_rotations->begin(),
                  [imag_norm_max](const Eigen::Quaterniond &a, const Eigen::Quaterniond &b) {
                    return (a * b.conjugate()).vec().norm() < imag_norm_max;
                  }))
    return false;
  return true;
}

/// Returns true if the detector with given detector index is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const {
  if (index >= m_isMonitor->size()) {
    throw std::runtime_error("Invalid monitor index");
  }
  // No check for time dependence since monitor flags are not time dependent.
  return (*m_isMonitor)[index];
}

/** Returns true if the detector with given index is a monitor.
 *
 * The time component of the index is ignored since a detector is a monitor
 * either for *all* times or for *none*. */
bool DetectorInfo::isMonitor(const std::pair<size_t, size_t> &index) const {
  if (index.first >= m_isMonitor->size()) {
    throw std::runtime_error("Invalid monitor index");
  }
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
bool DetectorInfo::isMasked(const std::pair<size_t, size_t> &index) const { return (*m_isMasked)[linearIndex(index)]; }

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
void DetectorInfo::setMasked(const std::pair<size_t, size_t> &index, bool masked) {
  m_isMasked.access()[linearIndex(index)] = masked;
}

/// Returns the scan count of the detector, reading it from m_componentInfo
size_t DetectorInfo::scanCount() const { return m_componentInfo->scanCount(); }

/** Returns the scan interval of the detector with given index.
 *
 * The interval start and end values would typically correspond to nanoseconds
 * since 1990, as in Types::Core::DateAndTime. */
const std::vector<std::pair<int64_t, int64_t>> DetectorInfo::scanIntervals() const {
  return m_componentInfo->scanIntervals();
}

namespace {
void failMerge(const std::string &what) { throw std::runtime_error(std::string("Cannot merge DetectorInfo: ") + what); }
} // namespace

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
 * ignored, i.e., no time index is added.
 *
 * The `merge()` operation is private in `DetectorInfo`, and only
 * accessible through `ComponentInfo` (via a `friend` declaration)
 * because we need to avoid merging `DetectorInfo` without merging
 * `ComponentInfo`, since that would effectively let us create a non-sync
 * scan. Otherwise we cannot provide scanning-related methods in
 * `ComponentInfo` without component index.
 *
 * The `scanIntervals` are stored in `ComponentInfo` only, to make sure all
 * components have the same ones. The `bool` vector `merge` dictating whether
 * a given interval should be merged or not was built by `ComponentInfo` and
 * passed on to this function.
 */
void DetectorInfo::merge(const DetectorInfo &other, const std::vector<bool> &merge) {
  checkSizes(other);
  for (size_t timeIndex = 0; timeIndex < other.scanCount(); ++timeIndex) {
    if (!merge[timeIndex])
      continue;
    auto &isMaskedVec = m_isMasked.access();
    auto &positions = m_positions.access();
    auto &rotations = m_rotations.access();
    const size_t indexStart = other.linearIndex({0, timeIndex});
    size_t indexEnd = indexStart + size();
    isMaskedVec.insert(isMaskedVec.end(), other.m_isMasked->begin() + indexStart, other.m_isMasked->begin() + indexEnd);
    positions.insert(positions.end(), other.m_positions->begin() + indexStart, other.m_positions->begin() + indexEnd);
    rotations.insert(rotations.end(), other.m_rotations->begin() + indexStart, other.m_rotations->begin() + indexEnd);
  }
}

void DetectorInfo::setComponentInfo(ComponentInfo *componentInfo) { m_componentInfo = componentInfo; }

bool DetectorInfo::hasComponentInfo() const { return m_componentInfo != nullptr; }

double DetectorInfo::l1() const {
  // TODO Not scan safe yet for scanning ComponentInfo
  if (!hasComponentInfo()) {
    throw std::runtime_error("DetectorInfo has no valid ComponentInfo thus cannot determine l1");
  }
  return m_componentInfo->l1();
}

const Eigen::Vector3d &DetectorInfo::sourcePosition() const {
  // TODO Not scan safe yet for scanning ComponentInfo
  if (!hasComponentInfo()) {
    throw std::runtime_error("DetectorInfo has no valid ComponentInfo thus "
                             "cannot determine sourcePosition");
  }
  return m_componentInfo->sourcePosition();
}

const Eigen::Vector3d &DetectorInfo::samplePosition() const {
  // TODO Not scan safe yet for scanning ComponentInfo
  if (!hasComponentInfo()) {
    throw std::runtime_error("DetectorInfo has no valid ComponentInfo thus "
                             "cannot determine samplePosition");
  }
  return m_componentInfo->samplePosition();
}

void DetectorInfo::checkSizes(const DetectorInfo &other) const {
  if (size() != other.size())
    failMerge("size mismatch");
  if (!(m_isMonitor == other.m_isMonitor) && (*m_isMonitor != *other.m_isMonitor))
    failMerge("monitor flags mismatch");
  // TODO If we make masking time-independent we need to check masking here.
}

} // namespace Mantid::Beamline
