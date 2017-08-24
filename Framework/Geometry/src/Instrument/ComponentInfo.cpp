#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/IComponent.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/make_unique.h"
#include <exception>
#include <string>
#include <Eigen/Geometry>

namespace Mantid {
namespace Geometry {

namespace {
/**
 * Rotate point by inverse of rotation held at componentIndex
 */
const Eigen::Vector3d undoRotation(const Eigen::Vector3d &point,
                                   const Beamline::ComponentInfo &compInfo,
                                   const size_t componentIndex) {
  auto unRotateTransform =
      Eigen::Affine3d(compInfo.rotation(componentIndex).inverse());
  return unRotateTransform * point;
}
/**
 * Put the point into the frame of the shape.
 * 1. Subtract component position (puts component pos at origin, same as shape
 * coordinate system).
 * 2. Apply inverse rotation of component to point. Unrotates the component into
 * shape coordinate frame, with observer reorientated.
 */
const Kernel::V3D toShapeFrame(const Kernel::V3D &point,
                               const Beamline::ComponentInfo &compInfo,
                               const size_t componentIndex) {
  return Kernel::toV3D(undoRotation(Kernel::toVector3d(point) -
                                        compInfo.position(componentIndex),
                                    compInfo, componentIndex));
}
}

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
      m_compIDToIndex(other.m_compIDToIndex), m_shapes(other.m_shapes) {}

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

bool ComponentInfo::hasShape(const size_t componentIndex) const {
  return (*m_shapes)[componentIndex].get() != nullptr;
}

Kernel::V3D ComponentInfo::sourcePosition() const {
  return Kernel::toV3D(m_componentInfo->sourcePosition());
}

Kernel::V3D ComponentInfo::samplePosition() const {
  return Kernel::toV3D(m_componentInfo->samplePosition());
}

bool ComponentInfo::hasSource() const { return m_componentInfo->hasSource(); }

bool ComponentInfo::hasSample() const { return m_componentInfo->hasSample(); }

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

Kernel::V3D ComponentInfo::scaleFactor(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->scaleFactor(componentIndex));
}

void ComponentInfo::setScaleFactor(const size_t componentIndex,
                                   const Kernel::V3D &scaleFactor) {
  m_componentInfo->setScaleFactor(componentIndex,
                                  Kernel::toVector3d(scaleFactor));
}

double ComponentInfo::solidAngle(const size_t componentIndex,
                                 const Kernel::V3D &observer) const {
  if (!hasShape(componentIndex))
    throw Kernel::Exception::NullPointerException("ComponentInfo::solidAngle",
                                                  "shape");
  // This is the observer position in the shape's coordinate system.
  const Kernel::V3D relativeObserver =
      toShapeFrame(observer, *m_componentInfo, componentIndex);
  const Kernel::V3D scaleFactor = this->scaleFactor(componentIndex);
  if ((scaleFactor - Kernel::V3D(1.0, 1.0, 1.0)).norm() < 1e-12)
    return shape(componentIndex).solidAngle(relativeObserver);
  else {
    // This function will scale the object shape when calculating the solid
    // angle.
    return shape(componentIndex).solidAngle(relativeObserver, scaleFactor);
  }
}

void ComponentInfo::doGetBoundingBox(const size_t index,
                                     BoundingBox &absoluteBB) const {
  // Check that we have a valid shape here
  if (!hasShape(index)) {
    return; // This index will not contribute to bounding box
  }
  const auto &s = this->shape(index);
  const BoundingBox &shapeBox = s.getBoundingBox();
  std::vector<Kernel::V3D> coordSystem;
  if (!absoluteBB.isAxisAligned()) { // copy coordinate system (it is better

    coordSystem.assign(absoluteBB.getCoordSystem().begin(),
                       absoluteBB.getCoordSystem().end());
  }
  auto currentBB = BoundingBox(shapeBox);
  // modify in place for speed
  const Eigen::Vector3d scaleFactor = m_componentInfo->scaleFactor(index);
  // Scale
  currentBB.xMin() *= scaleFactor[0];
  currentBB.xMax() *= scaleFactor[0];
  currentBB.yMin() *= scaleFactor[1];
  currentBB.yMax() *= scaleFactor[1];
  currentBB.zMin() *= scaleFactor[2];
  currentBB.zMax() *= scaleFactor[2];
  // Rotate
  (this->rotation(index))
      .rotateBB(currentBB.xMin(), currentBB.yMin(), currentBB.zMin(),
                currentBB.xMax(), currentBB.yMax(), currentBB.zMax());

  // Shift
  const Eigen::Vector3d localPos = m_componentInfo->position(index);
  currentBB.xMin() += localPos[0];
  currentBB.xMax() += localPos[0];
  currentBB.yMin() += localPos[1];
  currentBB.yMax() += localPos[1];
  currentBB.zMin() += localPos[2];
  currentBB.zMax() += localPos[2];

  if (!coordSystem.empty()) {
    currentBB.realign(&coordSystem);
  }
  absoluteBB.grow(currentBB);
}

void ComponentInfo::getBoundingBox(const size_t componentIndex,
                                   BoundingBox &absoluteBB) const {

  if (isDetector(componentIndex)) {
    doGetBoundingBox(componentIndex, absoluteBB);
    return;
  }
  auto rangeComp = m_componentInfo->componentRangeInSubtree(componentIndex);
  std::vector<std::pair<size_t, size_t>> detExclusions{};
  for (auto it = rangeComp.begin(); it != rangeComp.end(); ++it) {

    const size_t index = *it;
    if (hasSource() && index == source()) {
      continue;
    }
    if (isRectangularBank(index)) {

      auto innerRangeComp = m_componentInfo->componentRangeInSubtree(index);
      auto nSubComponents = innerRangeComp.end() - innerRangeComp.begin() - 1;
      auto innerRangeDet = m_componentInfo->detectorRangeInSubtree(index);
      auto nSubDetectors = innerRangeDet.end() - innerRangeDet.begin();
      auto nY = nSubDetectors / nSubComponents;
      auto nX = nSubComponents;
      size_t corner1 = *innerRangeDet.begin();
      size_t corner2 = *(innerRangeDet.end() - 1);
      size_t corner3 = *(innerRangeDet.begin() + nY);
      size_t corner4 = *(innerRangeDet.end() - 1 - nX);

      doGetBoundingBox(corner1, absoluteBB);
      doGetBoundingBox(corner2, absoluteBB);
      doGetBoundingBox(corner3, absoluteBB);
      doGetBoundingBox(corner4, absoluteBB);

      // Get bounding box for rectangular detector
      // Skip all sub components.
      detExclusions.emplace_back(std::make_pair(corner1, corner2));
      // it = innerRangeComp.end();
    } else {
      doGetBoundingBox(index, absoluteBB);
    }
  }

  // Now deal with bounding boxes for detectors
  auto rangeDet = m_componentInfo->detectorRangeInSubtree(componentIndex);
  auto it = rangeDet.begin();
  while (it != rangeDet.end()) {
    for (auto exc = detExclusions.begin(); exc != detExclusions.end(); ++exc) {
      if ((*it) >= exc->first && (*it) <= exc->second) {
        it += (exc->second - exc->first + 1); // Jump the iterator forward
      }
    }
    if (it != rangeDet.end()) {
      doGetBoundingBox(*it, absoluteBB);
      ++it;
    }
  }
}

bool ComponentInfo::isRectangularBank(const size_t componentIndex) const {
  return m_componentInfo->isRectangularBank(componentIndex);
}

} // namespace Geometry
} // namespace Mantid
