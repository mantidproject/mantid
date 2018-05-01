#ifndef MANTID_BEAMLINE_COMPONENTINFO_H_
#define MANTID_BEAMLINE_COMPONENTINFO_H_

#include "MantidBeamline/ComponentType.h"
#include "MantidBeamline/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include <Eigen/Geometry>
#include <Eigen/StdVector>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include <cstddef>
#include <string>
#include <utility>

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
  boost::shared_ptr<std::vector<std::vector<size_t>>> m_children;
  Mantid::Kernel::cow_ptr<std::vector<Eigen::Vector3d>> m_positions;
  Mantid::Kernel::cow_ptr<std::vector<
      Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>
      m_rotations;
  Mantid::Kernel::cow_ptr<std::vector<Eigen::Vector3d>> m_scaleFactors;
  Mantid::Kernel::cow_ptr<std::vector<ComponentType>> m_componentType;
  boost::shared_ptr<const std::vector<std::string>> m_names;

  const size_t m_size = 0;
  const int64_t m_sourceIndex = -1;
  const int64_t m_sampleIndex = -1;
  DetectorInfo *m_detectorInfo; // Geometry::DetectorInfo is the owner.
  size_t m_scanCounts = 1;
  Kernel::cow_ptr<std::vector<std::pair<int64_t, int64_t>>> m_scanIntervals{
      nullptr};
  /// For (component index, time index) -> linear index conversions
  Kernel::cow_ptr<std::vector<std::vector<size_t>>> m_indexMap{nullptr};
  /// For linear index -> (detector index, time index) conversions
  Kernel::cow_ptr<std::vector<std::pair<size_t, size_t>>> m_indices{nullptr};
  void failIfDetectorInfoScanning() const;
  size_t linearIndex(const std::pair<size_t, size_t> &index) const;
  void initScanIntervals();
  void checkNoTimeDependence() const;
  std::vector<bool> buildMergeIndicesSync(const ComponentInfo &other) const;
  void checkSizes(const ComponentInfo &other) const;
  void initIndices();
  void checkIdenticalIntervals(const ComponentInfo &other,
                               const size_t linearIndexOther,
                               const size_t linearIndexThis) const;
  void checkSpecialIndices(size_t componentIndex) const;
  size_t nonDetectorSize() const;
  /// Copy constructor is private because of the way DetectorInfo stored
  ComponentInfo(const ComponentInfo &) = default;

public:
  ComponentInfo();
  ComponentInfo(
      boost::shared_ptr<const std::vector<size_t>>
          assemblySortedDetectorIndices,
      boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
          detectorRanges,
      boost::shared_ptr<const std::vector<size_t>>
          assemblySortedComponentIndices,
      boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
          componentRanges,
      boost::shared_ptr<const std::vector<size_t>> parentIndices,
      boost::shared_ptr<std::vector<std::vector<size_t>>> children,
      boost::shared_ptr<std::vector<Eigen::Vector3d>> positions,
      boost::shared_ptr<std::vector<
          Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>>
          rotations,
      boost::shared_ptr<std::vector<Eigen::Vector3d>> scaleFactors,
      boost::shared_ptr<std::vector<ComponentType>> componentType,
      boost::shared_ptr<const std::vector<std::string>> names,
      int64_t sourceIndex, int64_t sampleIndex);
  /// Copy assignment not permitted because of the way DetectorInfo stored
  ComponentInfo &operator=(const ComponentInfo &other) = delete;
  /// Clone method
  std::unique_ptr<ComponentInfo> cloneWithoutDetectorInfo() const;
  std::vector<size_t> detectorsInSubtree(const size_t componentIndex) const;
  std::vector<size_t> componentsInSubtree(const size_t componentIndex) const;
  const std::vector<size_t> &children(const size_t componentIndex) const;
  size_t size() const;
  size_t numberOfDetectorsInSubtree(const size_t componentIndex) const;
  bool isDetector(const size_t componentIndex) const {
    return componentIndex < m_assemblySortedDetectorIndices->size();
  }
  size_t compOffsetIndex(const size_t componentIndex) const {
    return componentIndex - m_assemblySortedDetectorIndices->size();
  }

  Eigen::Vector3d position(const size_t componentIndex) const;
  Eigen::Vector3d position(const std::pair<size_t, size_t> &index) const;
  Eigen::Quaterniond rotation(const size_t componentIndex) const;
  Eigen::Quaterniond rotation(const std::pair<size_t, size_t> &index) const;
  Eigen::Vector3d relativePosition(const size_t componentIndex) const;
  Eigen::Quaterniond relativeRotation(const size_t componentIndex) const;
  void setPosition(const size_t componentIndex,
                   const Eigen::Vector3d &newPosition);
  void setPosition(const std::pair<size_t, size_t> index,
                   const Eigen::Vector3d &newPosition);
  void setRotation(const size_t componentIndex,
                   const Eigen::Quaterniond &newRotation);
  void setRotation(const std::pair<size_t, size_t> index,
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
  const std::string &name(const size_t componentIndex) const;
  size_t indexOfAny(const std::string &name) const;
  Eigen::Vector3d scaleFactor(const size_t componentIndex) const;
  void setScaleFactor(const size_t componentIndex,
                      const Eigen::Vector3d &scaleFactor);
  ComponentType componentType(const size_t componentIndex) const;

  size_t scanCount(const size_t index) const;
  size_t scanSize() const;
  bool isScanning() const;
  std::pair<int64_t, int64_t>
  scanInterval(const std::pair<size_t, size_t> &index) const;
  void setScanInterval(const std::pair<int64_t, int64_t> &interval);
  void merge(const ComponentInfo &other);

  class Range {
  private:
    const std::vector<size_t>::const_iterator m_begin;
    const std::vector<size_t>::const_iterator m_end;

  public:
    Range(std::vector<size_t>::const_iterator &&begin,
          std::vector<size_t>::const_iterator &&end)
        : m_begin(std::move(begin)), m_end(std::move(end)) {}
    bool empty() const { return m_begin == m_end; }
    auto begin() const -> decltype(m_begin) { return m_begin; }
    auto end() const -> decltype(m_end) { return m_end; }
    boost::reverse_iterator<std::vector<size_t>::const_iterator>
    rbegin() const {
      return boost::make_reverse_iterator(m_end);
    }
    boost::reverse_iterator<std::vector<size_t>::const_iterator> rend() const {
      return boost::make_reverse_iterator(m_begin);
    }
  };

  Range detectorRangeInSubtree(const size_t index) const;
  Range componentRangeInSubtree(const size_t index) const;

private:
  void doSetPosition(const std::pair<size_t, size_t> &index,
                     const Eigen::Vector3d &newPosition,
                     const ComponentInfo::Range &detectorRange);
  void doSetRotation(const std::pair<size_t, size_t> &index,
                     const Eigen::Quaterniond &newRotation,
                     const ComponentInfo::Range &detectorRange);
};
} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_COMPONENTINFO_H_ */
