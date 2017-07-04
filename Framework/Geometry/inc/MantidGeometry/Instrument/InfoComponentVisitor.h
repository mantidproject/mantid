#ifndef MANTID_GEOMETRY_INFOCOMPONENTVISITOR_H_
#define MANTID_GEOMETRY_INFOCOMPONENTVISITOR_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include <cstddef>
#include <utility>
#include <vector>
#include <unordered_map>
#include <boost/shared_ptr.hpp>
#include <Eigen/Geometry>

namespace Mantid {
using detid_t = int32_t;
namespace Geometry {
class IComponent;
class ICompAssembly;
class IDetector;
class ParameterMap;
}
namespace Beamline {
class ComponentInfo;
}

namespace Geometry {

/** InfoComponentVisitor : Visitor for components with access to Info wrapping
  features.

  This visitor ensures only minimal changes are required to any of the
  IComponent/Instrument1.0 hierachy in order to fully process it. It also
  eliminates the need for any dynamic casting. Note that InfoComponentVisitor
  provides accessors for the client to extract visited information such as
  ComponentIDs.

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
class MANTID_GEOMETRY_DLL InfoComponentVisitor
    : public Mantid::Geometry::ComponentVisitor {
private:
  /// Detectors components always specified first
  boost::shared_ptr<std::vector<Mantid::Geometry::IComponent *>> m_componentIds;

  /// Detector indexes sorted by assembly
  boost::shared_ptr<std::vector<size_t>> m_assemblySortedDetectorIndices;

  /// Component indexes sorted by assembly
  boost::shared_ptr<std::vector<size_t>> m_assemblySortedComponentIndices;

  /// Index of the parent component
  boost::shared_ptr<std::vector<size_t>> m_parentComponentIndices;

  /// Only Assemblies and other NON-detectors yield detector ranges
  boost::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_detectorRanges;

  /// Component ranges.
  boost::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_componentRanges;

  /// Component ID -> Component Index map
  boost::shared_ptr<std::unordered_map<Mantid::Geometry::IComponent *, size_t>>
      m_componentIdToIndexMap;

  /// Counter for dropped detectors
  size_t m_droppedDetectors = 0;

  /// Detector ID -> index mappings
  boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
      m_detectorIdToIndexMap;

  /// Detector indices
  boost::shared_ptr<std::vector<detid_t>> m_orderedDetectorIds;

  /// Positions
  boost::shared_ptr<std::vector<Eigen::Vector3d>> m_positions;

  /// Rotations
  boost::shared_ptr<std::vector<Eigen::Quaterniond>> m_rotations;

  /// Parameter map to purge.
  Mantid::Geometry::ParameterMap &m_pmap;

  /// Source id to look for
  Mantid::Geometry::IComponent *m_sourceId;

  /// Sample id to look for
  Mantid::Geometry::IComponent *m_sampleId;

  /// Source index to set
  int64_t m_sourceIndex = -1;

  /// Sample index to set
  int64_t m_sampleIndex = -1;

  void markAsSourceOrSample(Mantid::Geometry::IComponent *componentId,
                            const size_t componentIndex);

public:
  InfoComponentVisitor(std::vector<detid_t> orderedDetectorIds,
                       ParameterMap &pmap,
                       Mantid::Geometry::IComponent *source = nullptr,
                       Mantid::Geometry::IComponent *sample = nullptr);

  virtual size_t registerComponentAssembly(
      const Mantid::Geometry::ICompAssembly &assembly) override;

  virtual size_t registerGenericComponent(
      const Mantid::Geometry::IComponent &component) override;
  virtual size_t
  registerDetector(const Mantid::Geometry::IDetector &detector) override;

  boost::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>>
  componentIds() const;

  boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
  componentDetectorRanges() const;

  boost::shared_ptr<const std::vector<std::pair<size_t, size_t>>>
  componentChildComponentRanges() const;

  boost::shared_ptr<const std::vector<size_t>>
  assemblySortedDetectorIndices() const;

  boost::shared_ptr<const std::vector<size_t>>
  assemblySortedComponentIndices() const;

  boost::shared_ptr<const std::vector<size_t>> parentComponentIndices() const;

  boost::shared_ptr<
      const std::unordered_map<Mantid::Geometry::IComponent *, size_t>>
  componentIdToIndexMap() const;

  boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
  detectorIdToIndexMap() const;
  size_t size() const;

  bool isEmpty() const;

  std::unique_ptr<Beamline::ComponentInfo> componentInfo() const;

  boost::shared_ptr<std::vector<detid_t>> detectorIds() const;

  boost::shared_ptr<std::vector<Eigen::Vector3d>> positions() const;

  boost::shared_ptr<std::vector<Eigen::Quaterniond>> rotations() const;

  int64_t sample() const;

  int64_t source() const;
};
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INFOCOMPONENTVISITOR_H_ */
