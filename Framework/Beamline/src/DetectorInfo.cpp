#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/make_cow.h"

#include <algorithm>

namespace Mantid {
namespace Beamline {

DetectorInfo::DetectorInfo(std::vector<Eigen::Vector3d> positions,
                           std::vector<Eigen::Quaterniond> rotations)
    : m_isMonitor(Kernel::make_cow<std::vector<bool>>(positions.size())),
      m_isMasked(Kernel::make_cow<std::vector<bool>>(positions.size())),
      m_positions(
          Kernel::make_cow<std::vector<Eigen::Vector3d>>(std::move(positions))),
      m_rotations(Kernel::make_cow<std::vector<Eigen::Quaterniond>>(
          std::move(rotations))) {
  if (m_positions->size() != m_rotations->size())
    throw std::runtime_error("DetectorInfo: Position and rotations vectors "
                             "must have identical size");
}

DetectorInfo::DetectorInfo(std::vector<Eigen::Vector3d> positions,
                           std::vector<Eigen::Quaterniond> rotations,
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
 * Note that in both cases the actual limit may be lower, but it is guarenteed
 * that any LARGER differences are NOT considered equivalent. */
bool DetectorInfo::isEquivalent(const DetectorInfo &other) const {
  if (this == &other)
    return true;
  if (size() != other.size())
    return false;
  if (size() == 0)
    return true;
  if (!(m_isMonitor == other.m_isMonitor) &&
      (*m_isMonitor != *other.m_isMonitor))
    return false;
  if (!(m_isMasked == other.m_isMasked) && (*m_isMasked != *other.m_isMasked))
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
  if (!(m_rotations == other.m_rotations) &&
      !std::equal(m_rotations->begin(), m_rotations->end(),
                  other.m_rotations->begin(),
                  [](const Eigen::Quaterniond &a, const Eigen::Quaterniond &b) {
                    constexpr double d_max = 1e-9;
                    constexpr double L = 1000.0;
                    constexpr double safety_factor = 2.0;
                    constexpr double imag_norm_max =
                        sin(d_max / (2.0 * L * safety_factor));
                    return (a * b.conjugate()).vec().norm() < imag_norm_max;
                  }))
    return false;
  return true;
}

/// Returns the size of the DetectorInfo, i.e., the number of detectors in the
/// instrument.
size_t DetectorInfo::size() const {
  if (!m_isMasked)
    return 0;
  return m_isMasked->size();
}

/// Returns true if the detector is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const {
  return (*m_isMonitor)[index];
}

/// Returns true if the detector is masked.
bool DetectorInfo::isMasked(const size_t index) const {
  return (*m_isMasked)[index];
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const size_t index, bool masked) {
  m_isMasked.access()[index] = masked;
}

/// Returns the position of the detector with given index.
Eigen::Vector3d DetectorInfo::position(const size_t index) const {
  return (*m_positions)[index];
}

/// Returns the rotation of the detector with given index.
Eigen::Quaterniond DetectorInfo::rotation(const size_t index) const {
  return (*m_rotations)[index];
}

/// Set the position of the detector with given index.
void DetectorInfo::setPosition(const size_t index,
                               const Eigen::Vector3d &position) {
  m_positions.access()[index] = position;
}

/// Set the rotation of the detector with given index.
void DetectorInfo::setRotation(const size_t index,
                               const Eigen::Quaterniond &rotation) {
  m_rotations.access()[index] = rotation.normalized();
}

} // namespace Beamline
} // namespace Mantid
