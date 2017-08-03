#ifndef MANTID_BEAMLINE_COMPONENTINFO_H_
#define MANTID_BEAMLINE_COMPONENTINFO_H_

#include "MantidBeamline/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <utility>
#include <cstddef>
#include <Eigen/Geometry>

namespace Mantid {
namespace Beamline {
class DetectorInfo;
/** ComponentInfo : Provides a component centric view on to the instrument.
  Indexes
  are per component.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_BEAMLINE_DLL ComponentInfo {

private:
  boost::shared_ptr<const std::vector<size_t>> m_assemblySortedDetectorIndices;
  /// Contains only indices of non-detector components
  boost::shared_ptr<const std::vector<size_t>> m_assemblySortedComponentIndices;
  /// Ranges of component ids that are contiguous blocks of detectors.
  boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
      m_detectorRanges;
  /// Ranges of component ids that are contiguous blocks of components.
  boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
      m_componentRanges;
  boost::shared_ptr<const std::vector<size_t>> m_parentIndices;
  Mantid::Kernel::cow_ptr<std::vector<Eigen::Vector3d>> m_positions;
  Mantid::Kernel::cow_ptr<std::vector<Eigen::Quaterniond>> m_rotations;
  const size_t m_size = 0;
  const int64_t m_sourceIndex = -1;
  const int64_t m_sampleIndex = -1;
  DetectorInfo *m_detectorInfo; // ExperimentInfo is the owner.

  void failIfScanning() const;

public:
  ComponentInfo();
  ComponentInfo(boost::shared_ptr<const std::vector<size_t>>
                    assemblySortedDetectorIndices,
                boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
                    detectorRanges,
                boost::shared_ptr<const std::vector<size_t>>
                    assemblySortedComponentIndices,
                boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
                    componentRanges,
                boost::shared_ptr<const std::vector<size_t>> parentIndices,
                boost::shared_ptr<std::vector<Eigen::Vector3d>> positions,
                boost::shared_ptr<std::vector<Eigen::Quaterniond>> rotations,
                int64_t sourceIndex, int64_t sampleIndex);

  std::vector<size_t> detectorsInSubtree(const size_t componentIndex) const;
  std::vector<size_t> componentsInSubtree(const size_t componentIndex) const;
  size_t size() const;
  bool isDetector(const size_t componentIndex) const {
    return componentIndex < m_assemblySortedDetectorIndices->size();
  }
  size_t compOffsetIndex(const size_t componentIndex) const {
    return componentIndex - m_assemblySortedDetectorIndices->size();
  }

  Eigen::Vector3d position(const size_t componentIndex) const;
  Eigen::Quaterniond rotation(const size_t componentIndex) const;
  Eigen::Vector3d relativePosition(const size_t componentIndex) const;
  Eigen::Quaterniond relativeRotation(const size_t componentIndex) const;
  void setPosition(const size_t componentIndex,
                   const Eigen::Vector3d &newPosition);
  void setRotation(const size_t componentIndex,
                   const Eigen::Quaterniond &newRotation);

  size_t parent(const size_t componentIndex) const;
  bool hasParent(const size_t componentIndex) const;
  bool hasDetectorInfo() const;
  void setDetectorInfo(DetectorInfo *detectorInfo);
  bool hasSource() const;
  bool hasSample() const;
  Eigen::Vector3d sourcePosition() const;
  Eigen::Vector3d samplePosition() const;
  size_t source() const;
  size_t sample() const;
  size_t root() const;
  double l1() const;
};
} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_COMPONENTINFO_H_ */
