#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/make_unique.h"
#include <exception>
#include <string>

namespace Mantid {
namespace Geometry {

/**
 * Constructor.
 * @param componentInfo : Internal Beamline ComponentInfo
 * @param componentIds : ComponentIDs ordered by component
 * @param componentIdToIndexMap : ID -> index translation map
 * @param shapes : Shapes for each component
 * */
ComponentInfo::ComponentInfo(
    std::unique_ptr<Beamline::ComponentInfo> componentInfo,
    boost::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>>
        componentIds,
    boost::shared_ptr<const std::unordered_map<Geometry::IComponent *, size_t>>
        componentIdToIndexMap,
    boost::shared_ptr<std::vector<boost::shared_ptr<const Geometry::Object>>>
        shapes)
    : m_componentInfo(std::move(componentInfo)),
      m_componentIds(std::move(componentIds)),
      m_compIDToIndex(std::move(componentIdToIndexMap)),
      m_shapes(std::move(shapes)) {

  if (m_componentIds->size() != m_compIDToIndex->size()) {
    throw std::invalid_argument("Inconsistent ID and Mapping input containers "
                                "for Geometry::ComponentInfo");
  }
  if (m_componentIds->size() != m_componentInfo->size()) {
    throw std::invalid_argument("Inconsistent ID and base "
                                "Beamline::ComponentInfo sizes for "
                                "Geometry::ComponentInfo");
  }
}

/** Copy constructor. Use with EXTREME CARE.
 *
 * Public copy should not be used since proper links between DetectorInfo and
 * ComponentInfo must be set up. */
ComponentInfo::ComponentInfo(const ComponentInfo &other)
    : m_componentInfo(
          Kernel::make_unique<Beamline::ComponentInfo>(*other.m_componentInfo)),
      m_componentIds(other.m_componentIds),
      m_compIDToIndex(other.m_compIDToIndex) {}

// Defined as default in source for forward declaration with std::unique_ptr.
ComponentInfo::~ComponentInfo() = default;

std::vector<size_t>
ComponentInfo::detectorsInSubtree(size_t componentIndex) const {
  return m_componentInfo->detectorsInSubtree(componentIndex);
}

std::vector<size_t>
ComponentInfo::componentsInSubtree(size_t componentIndex) const {
  return m_componentInfo->componentsInSubtree(componentIndex);
}

size_t ComponentInfo::size() const { return m_componentInfo->size(); }

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const {
  return m_compIDToIndex->at(id);
}

bool ComponentInfo::isDetector(const size_t componentIndex) const {
  return m_componentInfo->isDetector(componentIndex);
}

Kernel::V3D ComponentInfo::position(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->position(componentIndex));
}

Kernel::Quat ComponentInfo::rotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo->rotation(componentIndex));
}

Kernel::V3D ComponentInfo::relativePosition(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->relativePosition(componentIndex));
}

Kernel::Quat
ComponentInfo::relativeRotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo->relativeRotation(componentIndex));
}

size_t ComponentInfo::parent(const size_t componentIndex) const {
  return m_componentInfo->parent(componentIndex);
}

bool ComponentInfo::hasParent(const size_t componentIndex) const {
  return m_componentInfo->hasParent(componentIndex);
}

Kernel::V3D ComponentInfo::sourcePosition() const {
  return Kernel::toV3D(m_componentInfo->sourcePosition());
}

Kernel::V3D ComponentInfo::samplePosition() const {
  return Kernel::toV3D(m_componentInfo->samplePosition());
}

size_t ComponentInfo::source() const { return m_componentInfo->source(); }

size_t ComponentInfo::sample() const { return m_componentInfo->sample(); }

double ComponentInfo::l1() const { return m_componentInfo->l1(); }

size_t ComponentInfo::root() { return m_componentInfo->root(); }

void ComponentInfo::setPosition(const size_t componentIndex,
                                const Kernel::V3D &newPosition) {
  m_componentInfo->setPosition(componentIndex, Kernel::toVector3d(newPosition));
}

void ComponentInfo::setRotation(const size_t componentIndex,
                                const Kernel::Quat &newRotation) {
  m_componentInfo->setRotation(componentIndex,
                               Kernel::toQuaterniond(newRotation));
}

const Object &ComponentInfo::shape(const size_t componentIndex) const {
  return *(*m_shapes)[componentIndex];
}

} // namespace Geometry
} // namespace Mantid
