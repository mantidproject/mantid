#ifndef MANTID_GEOMETRY_INSTRUMENTVISITOR_H_
#define MANTID_GEOMETRY_INSTRUMENTVISITOR_H_

#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include <Eigen/Geometry>
#include <Eigen/StdVector>
#include <boost/shared_ptr.hpp>
#include <cstddef>
#include <unordered_map>
#include <utility>

namespace Mantid {
using detid_t = int32_t;
namespace Beamline {
class ComponentInfo;
class DetectorInfo;
} // namespace Beamline
namespace Geometry {
class ComponentInfo;
class DetectorInfo;
class ICompAssembly;
class IComponent;
class IDetector;
class IObjComponent;
class Instrument;
class IObject;
class ParameterMap;
class RectangularDetector;
class ObjCompAssembly;

/** InstrumentVisitor : Visitor for components with access to Info wrapping
  features.

  This visitor ensures only minimal changes are required to any of the
  IComponent/Instrument1.0 hierachy in order to fully process it. It also
  eliminates the need for any dynamic casting. Note that InstrumentVisitor
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
class MANTID_GEOMETRY_DLL InstrumentVisitor
    : public Mantid::Geometry::ComponentVisitor {
private:
  /// Detector indices
  boost::shared_ptr<std::vector<detid_t>> m_orderedDetectorIds;

  /// Detectors components always specified first
  boost::shared_ptr<std::vector<Mantid::Geometry::IComponent *>> m_componentIds;

  /// Detector indexes sorted by assembly
  boost::shared_ptr<std::vector<size_t>> m_assemblySortedDetectorIndices;

  /// Component indexes sorted by assembly
  boost::shared_ptr<std::vector<size_t>> m_assemblySortedComponentIndices;

  /// Index of the parent component
  boost::shared_ptr<std::vector<size_t>> m_parentComponentIndices;

  /// Stores instrument tree structure by storing children of all Components
  boost::shared_ptr<std::vector<std::vector<size_t>>> m_children;

  /// Only Assemblies and other NON-detectors yield detector ranges
  boost::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_detectorRanges;

  /// Component ranges.
  boost::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_componentRanges;

  /// Component ID -> Component Index map
  boost::shared_ptr<std::unordered_map<Mantid::Geometry::IComponent *, size_t>>
      m_componentIdToIndexMap;

  /// Detector ID -> index mappings
  boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
      m_detectorIdToIndexMap;

  /// Positions for non-detectors
  boost::shared_ptr<std::vector<Eigen::Vector3d>> m_positions;

  /// Positions for detectors
  boost::shared_ptr<std::vector<Eigen::Vector3d>> m_detectorPositions;

  /// Rotations for non-detectors
  boost::shared_ptr<std::vector<Eigen::Quaterniond,
                                Eigen::aligned_allocator<Eigen::Quaterniond>>>
      m_rotations;

  /// Rotations for detectors
  boost::shared_ptr<std::vector<Eigen::Quaterniond,
                                Eigen::aligned_allocator<Eigen::Quaterniond>>>
      m_detectorRotations;

  /// Monitor indexes for detectors
  boost::shared_ptr<std::vector<size_t>> m_monitorIndices;

  /// Instrument to build around
  boost::shared_ptr<const Mantid::Geometry::Instrument> m_instrument;

  /// Parameter map to purge.
  Mantid::Geometry::ParameterMap *m_pmap;

  /// Source id to look for
  Mantid::Geometry::IComponent *m_sourceId;

  /// Sample id to look for
  Mantid::Geometry::IComponent *m_sampleId;

  /// Source index to set
  int64_t m_sourceIndex = -1;

  /// Sample index to set
  int64_t m_sampleIndex = -1;

  /// Null shared (empty shape)
  boost::shared_ptr<const Mantid::Geometry::IObject> m_nullShape;

  /// Shapes stored in fly-weight fashion
  boost::shared_ptr<
      std::vector<boost::shared_ptr<const Mantid::Geometry::IObject>>> m_shapes;

  /// Scale factors
  boost::shared_ptr<std::vector<Eigen::Vector3d>> m_scaleFactors;

  /// Structured bank flag
  boost::shared_ptr<std::vector<Beamline::ComponentType>> m_componentType;

  /// Component names
  boost::shared_ptr<std::vector<std::string>> m_names;

  void markAsSourceOrSample(Mantid::Geometry::IComponent *componentId,
                            const size_t componentIndex);

  std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
  makeWrappers() const;

  /// Extract the common aspects relevant to all component types
  size_t commonRegistration(const Mantid::Geometry::IComponent &component);

public:
  InstrumentVisitor(boost::shared_ptr<const Instrument> instrument);

  void walkInstrument();

  virtual size_t registerComponentAssembly(
      const Mantid::Geometry::ICompAssembly &assembly) override;

  virtual size_t registerGenericComponent(
      const Mantid::Geometry::IComponent &component) override;

  virtual size_t registerInfiniteComponent(
      const Mantid::Geometry::IComponent &component) override;

  virtual size_t registerGenericObjComponent(
      const Mantid::Geometry::IObjComponent &objComponent) override;

  virtual size_t
  registerGridBank(const Mantid::Geometry::ICompAssembly &bank) override;

  virtual size_t
  registerRectangularBank(const Mantid::Geometry::ICompAssembly &bank) override;

  virtual size_t
  registerInfiniteObjComponent(const IObjComponent &objComponent) override;

  virtual size_t
  registerStructuredBank(const Mantid::Geometry::ICompAssembly &bank) override;

  virtual size_t
  registerDetector(const Mantid::Geometry::IDetector &detector) override;

  virtual size_t
  registerObjComponentAssembly(const ObjCompAssembly &obj) override;

  boost::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>>
  componentIds() const;

  boost::shared_ptr<
      const std::unordered_map<Mantid::Geometry::IComponent *, size_t>>
  componentIdToIndexMap() const;

  boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
  detectorIdToIndexMap() const;
  size_t size() const;

  bool isEmpty() const;

  std::unique_ptr<Beamline::ComponentInfo> componentInfo() const;
  std::unique_ptr<Beamline::DetectorInfo> detectorInfo() const;

  boost::shared_ptr<std::vector<detid_t>> detectorIds() const;

  static std::pair<std::unique_ptr<ComponentInfo>,
                   std::unique_ptr<DetectorInfo>>
  makeWrappers(const Instrument &instrument, ParameterMap *pmap = nullptr);
};
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INSTRUMENTVISITOR_H_ */
