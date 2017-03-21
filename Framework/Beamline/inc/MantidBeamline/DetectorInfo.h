#ifndef MANTID_BEAMLINE_DETECTORINFO_H_
#define MANTID_BEAMLINE_DETECTORINFO_H_

#include "MantidBeamline/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

#include "Eigen/Geometry"

namespace Mantid {
namespace Beamline {

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

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_BEAMLINE_DLL DetectorInfo {
public:
  DetectorInfo() = default;
  DetectorInfo(std::vector<Eigen::Vector3d> positions,
               std::vector<Eigen::Quaterniond> rotations);
  DetectorInfo(std::vector<Eigen::Vector3d> positions,
               std::vector<Eigen::Quaterniond> rotations,
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

  size_t scanCount(const size_t index) const;
  std::pair<int64_t, int64_t>
  scanInterval(const std::pair<size_t, size_t> &index) const;
  void setScanInterval(const size_t index,
                       const std::pair<int64_t, int64_t> &interval);

  void merge(const DetectorInfo &other);

private:
  size_t linearIndex(const std::pair<size_t, size_t> &index) const;
  void checkNoTimeDependence() const;
  void initScanCounts();
  void initScanIntervals();
  void initIndices();
  std::vector<bool> buildMergeIndices(const DetectorInfo &other) const;

  Kernel::cow_ptr<std::vector<bool>> m_isMonitor{nullptr};
  Kernel::cow_ptr<std::vector<bool>> m_isMasked{nullptr};
  Kernel::cow_ptr<std::vector<Eigen::Vector3d>> m_positions{nullptr};
  Kernel::cow_ptr<std::vector<Eigen::Quaterniond>> m_rotations{nullptr};

  Kernel::cow_ptr<std::vector<size_t>> m_scanCounts{nullptr};
  Kernel::cow_ptr<std::vector<std::pair<int64_t, int64_t>>> m_scanIntervals{
      nullptr};
  Kernel::cow_ptr<std::vector<std::vector<size_t>>> m_indexMap{nullptr};
  Kernel::cow_ptr<std::vector<std::pair<size_t, size_t>>> m_indices{nullptr};
};

/** Returns the position of the detector with given detector index.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
inline Eigen::Vector3d DetectorInfo::position(const size_t index) const {
  checkNoTimeDependence();
  return (*m_positions)[index];
}

/** Returns the rotation of the detector with given detector index.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
inline Eigen::Quaterniond DetectorInfo::rotation(const size_t index) const {
  checkNoTimeDependence();
  return (*m_rotations)[index];
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

/** Set the rotation of the detector with given detector index.
 *
 * Convenience method for beamlines with static (non-moving) detectors.
 * Throws if there are time-dependent detectors. */
inline void DetectorInfo::setRotation(const size_t index,
                                      const Eigen::Quaterniond &rotation) {
  checkNoTimeDependence();
  m_rotations.access()[index] = rotation.normalized();
}

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_DETECTORINFO_H_ */
