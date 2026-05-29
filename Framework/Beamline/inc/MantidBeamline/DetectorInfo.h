// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidBeamline/DllConfig.h"
#include "MantidBeamline/VirtualBankSegment.h"
#include "MantidKernel/cow_ptr.h"

#include "Eigen/Geometry"
#include "Eigen/StdVector"

#include <vector>

namespace Mantid {
namespace Beamline {

class ComponentInfo;
/** Beamline::DetectorInfo provides easy access to commonly used parameters of
  individual detectors (pixels) in a beamline, such as mask and monitor flags,
  positions, L2, and 2-theta.

  Currently only a limited subset of functionality is implemented in
  Beamline::DetectorInfo. The remainder is available in API::DetectorInfo which
  acts as a wrapper around the old instrument implementation. API::DetectorInfo
  will be removed once all functionality has been moved to
  Beamline::DetectorInfo. For the time being, API::DetectorInfo will forward
  calls to Beamline::DetectorInfo when applicable.

  The reason for having both DetectorInfo classes in parallel is:
  - We need to be able to move around the DetectorInfo object including data it
    contains such as a vector of mask flags. This is relevant for the interface
    of ExperimentInfo, when replacing the ParameterMap or when setting a new
    instrument.
  - API::DetectorInfo contains a caching mechanism and is frequently flushed
    upon modification of the instrument and is thus hard to handle outside the
    context of its owning workspace.
  Splitting DetectorInfo into two classes seemed to be the safest and easiest
  solution to this.


  @author Simon Heybrock
  @date 2016
*/
class MANTID_BEAMLINE_DLL DetectorInfo {
public:
  DetectorInfo() = default;
  DetectorInfo(std::vector<Eigen::Vector3d> positions,
               std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>> rotations);
  DetectorInfo(std::vector<Eigen::Vector3d> positions,
               std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>> rotations,
               const std::vector<size_t> &monitorIndices);
  /// Constructor for instruments with virtual (PixelAssembly) banks.
  /// @param compactPositions  positions for real (non-virtual) detectors only
  /// @param compactRotations  rotations for real (non-virtual) detectors only
  /// @param monitorIndices    logical indices of monitor detectors
  /// @param virtualBanks      sorted virtual-bank segments (ascending firstIndex)
  DetectorInfo(std::vector<Eigen::Vector3d> compactPositions,
               std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>> compactRotations,
               const std::vector<size_t> &monitorIndices, std::vector<VirtualBankSegment> virtualBanks);

  bool isEquivalent(const DetectorInfo &other) const;

  size_t size() const;
  size_t getMemorySize() const;
  bool isScanning() const;

  bool isMonitor(const size_t index) const;
  bool isMonitor(const std::pair<size_t, size_t> &index) const;
  bool isMasked(const size_t index) const;
  bool isMasked(const std::pair<size_t, size_t> &index) const;
  void setMasked(const size_t index, bool masked);
  void setMasked(const std::pair<size_t, size_t> &index, bool masked);
  bool hasMaskedDetectors() const;
  // For virtual-bank instruments position/rotation are computed analytically and
  // cannot return a reference into a flat array — these overloads return by value.
  Eigen::Vector3d position(const size_t index) const;
  const Eigen::Vector3d &position(const std::pair<size_t, size_t> &index) const;
  Eigen::Quaterniond rotation(const size_t index) const;
  const Eigen::Quaterniond &rotation(const std::pair<size_t, size_t> &index) const;
  void setPosition(const size_t index, const Eigen::Vector3d &position);
  void setPosition(const std::pair<size_t, size_t> &index, const Eigen::Vector3d &position);
  void setRotation(const size_t index, const Eigen::Quaterniond &rotation);
  void setRotation(const std::pair<size_t, size_t> &index, const Eigen::Quaterniond &rotation);

  size_t scanCount() const;
  const std::vector<std::pair<int64_t, int64_t>> scanIntervals() const;

  void setComponentInfo(ComponentInfo *componentInfo);
  bool hasComponentInfo() const;
  double l1() const;
  const Eigen::Vector3d &sourcePosition() const;
  const Eigen::Vector3d &samplePosition() const;

  /// Whether this instrument has any PixelAssembly (virtual) banks.
  bool hasVirtualBanks() const noexcept { return !m_virtualBanks.empty(); }

  /// Returns the VirtualBankSegment whose ID range contains @p detId,
  /// or nullptr if @p detId is not a virtual pixel in any bank.
  const VirtualBankSegment *findVirtualSegmentByDetId(int32_t detId) const noexcept;

  /** The `merge()` operation was made private in `DetectorInfo`, and only
   * accessible through `ComponentInfo` (via this `friend` declaration)
   * because we need to avoid merging `DetectorInfo` without merging
   * `ComponentInfo`, since that would effectively let us create a non-sync
   * scan. Otherwise we cannot provide scanning-related methods in
   * `ComponentInfo` without component index.
   */
  friend class ComponentInfo;

  /// Returns the segment owning @p index, or nullptr if not virtual.
  const VirtualBankSegment *findVirtualSegment(size_t index) const noexcept;
  /// Maps a logical detector index to its compact array offset, skipping virtual pixel ranges.
  /// Only valid for non-virtual indices.
  size_t compactDetectorIndex(size_t index) const noexcept;

private:
  size_t linearIndex(const std::pair<size_t, size_t> &index) const;
  void checkNoTimeDependence() const;
  void checkSizes(const DetectorInfo &other) const;
  void merge(const DetectorInfo &other, const std::vector<bool> &merge);

  Kernel::cow_ptr<std::vector<bool>> m_isMonitor{nullptr};
  Kernel::cow_ptr<std::vector<bool>> m_isMasked{nullptr};
  /// Compact position array: one entry per real (non-virtual) detector, in
  /// the same order as logical indices with virtual pixel gaps removed.
  Kernel::cow_ptr<std::vector<Eigen::Vector3d>> m_positions{nullptr};
  Kernel::cow_ptr<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>> m_rotations{nullptr};

  /// Sorted (ascending firstIndex) list of virtual-bank segments.
  /// Empty for instruments that have no PixelAssembly banks.
  std::vector<VirtualBankSegment> m_virtualBanks;

  ComponentInfo *m_componentInfo = nullptr; // Geometry::ComponentInfo owner
};

/** Returns the number of detectors in the instrument.
 *
 * If a detector is moving, i.e., has more than one associated position, it is
 * nevertheless only counted as a single detector. */
inline size_t DetectorInfo::size() const {
  if (!m_isMonitor)
    return 0;
  return m_isMonitor->size();
}

/// Returns true if the beamline has scanning (time-dependent) detectors.
/// Virtual-bank (PixelAssembly) instruments are NOT scanning: their compact
/// position arrays are smaller than size() but hold no time-dependent data.
inline bool DetectorInfo::isScanning() const {
  if (!m_positions)
    return false;
  // Scanning instruments hold N * T positions (T scan points, T > 1).
  // Compact virtual-bank instruments hold N_real < N_total positions.
  // Only the scanning case has *more* positions than total detectors.
  return m_positions->size() > size();
}

/// Returns the position of the detector with given index (pair overload for scanning).
inline const Eigen::Vector3d &DetectorInfo::position(const std::pair<size_t, size_t> &index) const {
  return (*m_positions)[linearIndex(index)];
}

/// Returns the rotation of the detector with given index (pair overload for scanning).
inline const Eigen::Quaterniond &DetectorInfo::rotation(const std::pair<size_t, size_t> &index) const {
  return (*m_rotations)[linearIndex(index)];
}

/// Set the position of the detector with given index (scanning pair overload).
inline void DetectorInfo::setPosition(const std::pair<size_t, size_t> &index, const Eigen::Vector3d &position) {
  m_positions.access()[linearIndex(index)] = position;
}

/// Set the rotation of the detector with given index (scanning pair overload).
inline void DetectorInfo::setRotation(const std::pair<size_t, size_t> &index, const Eigen::Quaterniond &rotation) {
  m_rotations.access()[linearIndex(index)] = rotation.normalized();
}

/// Throws if this has time-dependent data.
inline void DetectorInfo::checkNoTimeDependence() const {
  if (isScanning())
    throw std::runtime_error("DetectorInfo accessed without time index but the "
                             "beamline has time-dependent (moving) detectors.");
}

/// Returns the linear index for a pair of detector index and time index.
inline size_t DetectorInfo::linearIndex(const std::pair<size_t, size_t> &index) const {
  // The most common case are beamlines with static detectors. In that case the
  // time index is always 0 and we avoid expensive map lookups. Linear indices
  // are ordered such that the first block contains everything for time index 0
  // so even in the time dependent case no translation is necessary.
  if (index.second == 0)
    return index.first;
  else
    return index.first + size() * index.second;
}

/// Returns if there are masked detectors
inline bool DetectorInfo::hasMaskedDetectors() const {
  return std::any_of(m_isMasked->cbegin(), m_isMasked->cend(), [](const auto flag) { return flag; });
}

} // namespace Beamline
} // namespace Mantid
