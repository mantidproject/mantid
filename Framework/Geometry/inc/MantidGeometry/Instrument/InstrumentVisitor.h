// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include <Eigen/Geometry>
#include <Eigen/StdVector>
#include <cstddef>
#include <memory>
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
*/
class MANTID_GEOMETRY_DLL InstrumentVisitor : public Mantid::Geometry::ComponentVisitor {
private:
  /// Detector indices
  std::shared_ptr<std::vector<detid_t>> m_orderedDetectorIds;

  /// Detectors components always specified first
  std::shared_ptr<std::vector<Mantid::Geometry::IComponent *>> m_componentIds;

  /// Detector indexes sorted by assembly
  std::shared_ptr<std::vector<size_t>> m_assemblySortedDetectorIndices;

  /// Component indexes sorted by assembly
  std::shared_ptr<std::vector<size_t>> m_assemblySortedComponentIndices;

  /// Index of the parent component
  std::shared_ptr<std::vector<size_t>> m_parentComponentIndices;

  /// Stores instrument tree structure by storing children of all Components
  std::shared_ptr<std::vector<std::vector<size_t>>> m_children;

  /// Only Assemblies and other NON-detectors yield detector ranges
  std::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_detectorRanges;

  /// Component ranges.
  std::shared_ptr<std::vector<std::pair<size_t, size_t>>> m_componentRanges;

  /// Component ID -> Component Index map
  std::shared_ptr<std::unordered_map<Mantid::Geometry::IComponent const *, size_t>> m_componentIdToIndexMap;

  /// Detector ID -> index mappings
  std::shared_ptr<const std::unordered_map<detid_t, size_t>> m_detectorIdToIndexMap;

  /// Positions for non-detectors
  std::shared_ptr<std::vector<Eigen::Vector3d>> m_positions;

  /// Positions for detectors
  std::shared_ptr<std::vector<Eigen::Vector3d>> m_detectorPositions;

  /// Rotations for non-detectors
  std::shared_ptr<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>> m_rotations;

  /// Rotations for detectors
  std::shared_ptr<std::vector<Eigen::Quaterniond, Eigen::aligned_allocator<Eigen::Quaterniond>>> m_detectorRotations;

  /// Monitor indexes for detectors
  std::shared_ptr<std::vector<size_t>> m_monitorIndices;

  /// Instrument to build around
  std::shared_ptr<const Mantid::Geometry::Instrument> m_instrument;

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
  std::shared_ptr<const Mantid::Geometry::IObject> m_nullShape;

  /// Shapes stored in fly-weight fashion
  std::shared_ptr<std::vector<std::shared_ptr<const Mantid::Geometry::IObject>>> m_shapes;

  /// Scale factors
  std::shared_ptr<std::vector<Eigen::Vector3d>> m_scaleFactors;

  /// Structured bank flag
  std::shared_ptr<std::vector<Beamline::ComponentType>> m_componentType;

  /// Component names
  std::shared_ptr<std::vector<std::string>> m_names;

  void markAsSourceOrSample(Mantid::Geometry::IComponent *componentId, const size_t componentIndex);

  std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>> makeWrappers() const;

  /// Extract the common aspects relevant to all component types
  size_t commonRegistration(const Mantid::Geometry::IComponent &component);

public:
  InstrumentVisitor(std::shared_ptr<const Instrument> instrument);

  void walkInstrument();

  virtual size_t registerComponentAssembly(const Mantid::Geometry::ICompAssembly &assembly) override;

  virtual size_t registerGenericComponent(const Mantid::Geometry::IComponent &component) override;

  virtual size_t registerInfiniteComponent(const Mantid::Geometry::IComponent &component) override;

  virtual size_t registerGenericObjComponent(const Mantid::Geometry::IObjComponent &objComponent) override;

  virtual size_t registerGridBank(const Mantid::Geometry::ICompAssembly &bank) override;

  virtual size_t registerRectangularBank(const Mantid::Geometry::ICompAssembly &bank) override;

  virtual size_t registerInfiniteObjComponent(const IObjComponent &objComponent) override;

  virtual size_t registerStructuredBank(const Mantid::Geometry::ICompAssembly &bank) override;

  virtual size_t registerDetector(const Mantid::Geometry::IDetector &detector) override;

  virtual size_t registerObjComponentAssembly(const ObjCompAssembly &obj) override;

  std::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>> componentIds() const;

  std::shared_ptr<const std::unordered_map<Mantid::Geometry::IComponent const *, size_t>> componentIdToIndexMap() const;

  std::shared_ptr<const std::unordered_map<detid_t, size_t>> detectorIdToIndexMap() const;
  size_t size() const;

  bool isEmpty() const;

  std::unique_ptr<Beamline::ComponentInfo> componentInfo() const;
  std::unique_ptr<Beamline::DetectorInfo> detectorInfo() const;

  std::shared_ptr<std::vector<detid_t>> detectorIds() const;

  static std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
  makeWrappers(const Instrument &instrument, ParameterMap *pmap = nullptr);
};
} // namespace Geometry
} // namespace Mantid
