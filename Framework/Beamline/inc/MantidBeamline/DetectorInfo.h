// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_BEAMLINE_DETECTORINFO_H_
#define MANTID_BEAMLINE_DETECTORINFO_H_

#include "MantidBeamline/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

#include "Eigen/Geometry"
#include "Eigen/StdVector"

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
               std::vector<Eigen::Quaterniond,
                           Eigen::aligned_allocator<Eigen::Quaterniond>>
                   rotations);
  DetectorInfo(std::vector<Eigen::Vector3d> positions,
               std::vector<Eigen::Quaterniond,
                           Eigen::aligned_allocator<Eigen::Quaterniond>>
                   rotations,
               const std::vector<size_t> &monitorIndices);

  bool isEquivalent(const DetectorInfo &other) const;

  size_t size() const;
  bool isScanning() const;

  bool isMonitor(const size_t index) const;
  bool isMonitor(const std::pair<size_t, size_t> &index) const;
  bool isMasked(const size_t index) const;
  bool isMasked(const std::pair<size_t, size_t> &index) const;
  void setMasked(const size_t index, bool masked);
  void setMasked(const std::pair<size_t, size_t> &index, bool masked);
  bool hasMaskedDetectors() const;
  Eigen::Vector3d position(const size_t index) const;
  Eigen::Vector3d position(const std::pair<size_t, size_t> &index) const;
  Eigen::Quaterniond rotation(const size_t index) const;
  Eigen::Quaterniond rotation(const std::pair<size_t, size_t> &index) const;
  void setPosition(const size_t index, const Eigen::Vector3d &position);
  void setPosition(const std::pair<size_t, size_t> &index,
                   const Eigen::Vector3d &position);
  void setRotation(const size_t index, const Eigen::Quaterniond &rotation);
  void setRotation(const std::pair<size_t, size_t> &index,
                   const Eigen::Quaterniond &rotation);

  size_t scanCount() const;
  const std::vector<std::pair<int64_t, int64_t>> scanIntervals() const;

  void setComponentInfo(ComponentInfo *componentInfo);
  bool hasComponentInfo() const;
  double l1() const;
  Eigen::Vector3d sourcePosition() const;
  Eigen::Vector3d samplePosition() const;

  /** The `merge()` operation was made private in `DetectorInfo`, and only
   * accessible through `ComponentInfo` (via this `friend` declaration)
   * because we need to avoid merging `DetectorInfo` without merging
   * `ComponentInfo`, since that would effectively let us create a non-sync
   * scan. Otherwise we cannot provide scanning-related methods in
   * `ComponentInfo` without component index.
   */
  friend class ComponentInfo;

private:
  size_t linearIndex(const std::pair<size_t, size_t> &index) const;
  void checkNoTimeDependence() const;
  void checkSizes(const DetectorInfo &other) const;
  void merge(const DetectorInfo &other, const std::vector<bool> &merge);

  Kernel::cow_ptr<std::vector<bool>> m_isMonitor{nullptr};
  Kernel::cow_ptr<std::vector<bool>> m_isMasked{nullptr};
  Kernel::cow_ptr<std::vector<Eigen::Vector3d>> m_positions{nullptr};
  Kernel::cow_ptr<std::vector<Eigen::Quaterniond,
                              Eigen::aligned_allocator<Eigen::Quaterniond>>>
      m_rotations{nullptr};

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

/// Returns true if the beamline has scanning detectors.
inline bool DetectorInfo::isScanning() const {
  if (!m_positions)
    return false;
  return size() != m_positions->size();
}

/** Returns the position of the detector with given detector index.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
inline Eigen::Vector3d DetectorInfo::position(const size_t index) const {
  checkNoTimeDependence();
  return (*m_positions)[index];
}

/// Returns the position of the detector with given index.
inline Eigen::Vector3d
DetectorInfo::position(const std::pair<size_t, size_t> &index) const {
  return (*m_positions)[linearIndex(index)];
}

/** Returns the rotation of the detector with given detector index.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
inline Eigen::Quaterniond DetectorInfo::rotation(const size_t index) const {
  checkNoTimeDependence();
  return (*m_rotations)[index];
}

/// Returns the rotation of the detector with given index.
inline Eigen::Quaterniond
DetectorInfo::rotation(const std::pair<size_t, size_t> &index) const {
  return (*m_rotations)[linearIndex(index)];
}

/** Set the position of the detector with given detector index.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
inline void DetectorInfo::setPosition(const size_t index,
                                      const Eigen::Vector3d &position) {
  checkNoTimeDependence();
  m_positions.access()[index] = position;
}

/// Set the position of the detector with given index.
inline void DetectorInfo::setPosition(const std::pair<size_t, size_t> &index,
                                      const Eigen::Vector3d &position) {
  m_positions.access()[linearIndex(index)] = position;
}

/** Set the rotation of the detector with given detector index.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
inline void DetectorInfo::setRotation(const size_t index,
                                      const Eigen::Quaterniond &rotation) {
  checkNoTimeDependence();
  m_rotations.access()[index] = rotation.normalized();
}

/// Set the rotation of the detector with given index.
inline void DetectorInfo::setRotation(const std::pair<size_t, size_t> &index,
                                      const Eigen::Quaterniond &rotation) {
  m_rotations.access()[linearIndex(index)] = rotation.normalized();
}

/// Throws if this has time-dependent data.
inline void DetectorInfo::checkNoTimeDependence() const {
  if (isScanning())
    throw std::runtime_error("DetectorInfo accessed without time index but the "
                             "beamline has time-dependent (moving) detectors.");
}

/// Returns the linear index for a pair of detector index and time index.
inline size_t
DetectorInfo::linearIndex(const std::pair<size_t, size_t> &index) const {
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
  return std::any_of(m_isMasked->cbegin(), m_isMasked->cend(),
                     [](const auto flag) { return flag; });
}

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_DETECTORINFO_H_ */
