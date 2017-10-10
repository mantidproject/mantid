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
#include <stack>
#include <iterator>

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

/**
 * Calculates the absolute bounding box for the leaf item at index
 *
 * @param index : Component index
 * @param reference : Optional reference for coordinate system for non-axis
 *aligned bounding boxes
 * @return Absolute bounding box.
 */
BoundingBox
ComponentInfo::componentBoundingBox(const size_t index,
                                    const BoundingBox *reference) const {
  // Check that we have a valid shape here
  if (!hasShape(index)) {
    return BoundingBox(); // Return null bounding box
  }
  const auto &s = this->shape(index);
  BoundingBox absoluteBB = s.getBoundingBox();

  // modify in place for speed
  const Eigen::Vector3d scaleFactor = m_componentInfo->scaleFactor(index);
  // Scale
  absoluteBB.xMin() *= scaleFactor[0];
  absoluteBB.xMax() *= scaleFactor[0];
  absoluteBB.yMin() *= scaleFactor[1];
  absoluteBB.yMax() *= scaleFactor[1];
  absoluteBB.zMin() *= scaleFactor[2];
  absoluteBB.zMax() *= scaleFactor[2];
  // Rotate
  (this->rotation(index))
      .rotateBB(absoluteBB.xMin(), absoluteBB.yMin(), absoluteBB.zMin(),
                absoluteBB.xMax(), absoluteBB.yMax(), absoluteBB.zMax());

  // Shift
  const Eigen::Vector3d localPos = m_componentInfo->position(index);
  absoluteBB.xMin() += localPos[0];
  absoluteBB.xMax() += localPos[0];
  absoluteBB.yMin() += localPos[1];
  absoluteBB.yMax() += localPos[1];
  absoluteBB.zMin() += localPos[2];
  absoluteBB.zMax() += localPos[2];

  if (reference && !reference->isAxisAligned()) { // copy coordinate system

    std::vector<Kernel::V3D> coordSystem;
    coordSystem.assign(reference->getCoordSystem().begin(),
                       reference->getCoordSystem().end());

    // realign to reference coordinate system
    absoluteBB.realign(&coordSystem);
  }
  return absoluteBB;
}

/**
 * Compute the bounding box for the component with componentIndex taking into
 *account
 * all sub components.
 *
 * @param componentIndex : Component index to get the bounding box for
 * @param reference : Optional reference for coordinate system for non-axis
 *aligned bounding boxes
 * @return Absolute bounding box
 */
BoundingBox ComponentInfo::boundingBox(const size_t componentIndex,
                                       const BoundingBox *reference) const {
  if (isDetector(componentIndex)) {
    return componentBoundingBox(componentIndex, reference);
  }
  BoundingBox absoluteBB;
  auto rangeComp = m_componentInfo->componentRangeInSubtree(componentIndex);
  std::stack<std::pair<size_t, size_t>> detExclusions{};
  auto compIterator = rangeComp.rbegin();
  while (compIterator != rangeComp.rend()) {
    const size_t index = *compIterator;
    if (hasSource() && index == source()) {
      ++compIterator;
    } else if (isStructuredBank(index)) {
      auto innerRangeComp = m_componentInfo->componentRangeInSubtree(index);
      // nSubComponents, subtract off self hence -1. nSubComponents = number of
      // horizontal columns.
      auto nSubComponents = innerRangeComp.end() - innerRangeComp.begin() - 1;
      auto innerRangeDet = m_componentInfo->detectorRangeInSubtree(index);
      auto nSubDetectors =
          std::distance(innerRangeDet.begin(), innerRangeDet.end());
      auto nY = nSubDetectors / nSubComponents;
      size_t bottomLeft = *innerRangeDet.begin();
      size_t topRight = bottomLeft + nSubDetectors - 1;
      size_t topLeft = bottomLeft + (nY - 1);
      size_t bottomRight = topRight - (nY - 1);

      absoluteBB.grow(componentBoundingBox(bottomLeft, reference));
      absoluteBB.grow(componentBoundingBox(topRight, reference));
      absoluteBB.grow(componentBoundingBox(topLeft, reference));
      absoluteBB.grow(componentBoundingBox(bottomRight, reference));

      // Get bounding box for rectangular bank.
      // Record detector ranges to skip
      // Skip all sub components.
      detExclusions.emplace(std::make_pair(bottomLeft, topRight));
      compIterator = innerRangeComp.rend();
    } else {
      absoluteBB.grow(componentBoundingBox(index, reference));
      ++compIterator;
    }
  }

  // Now deal with bounding boxes for detectors
  auto rangeDet = m_componentInfo->detectorRangeInSubtree(componentIndex);
  auto detIterator = rangeDet.begin();
  auto *exclusion = detExclusions.empty() ? nullptr : &detExclusions.top();
  while (detIterator != rangeDet.end()) {

    // Handle detectors in exclusion ranges
    if (exclusion && (*detIterator) >= exclusion->first &&
        (*detIterator) <= exclusion->second) {
      detIterator += (exclusion->second - exclusion->first +
                      1); // Jump the iterator forward
      detExclusions.pop();
      exclusion = detExclusions.empty() ? nullptr : &detExclusions.top();
    } else if (detIterator != rangeDet.end()) {
      absoluteBB.grow(componentBoundingBox(*detIterator, reference));
      ++detIterator;
    }
  }
  return absoluteBB;
}

bool ComponentInfo::isStructuredBank(const size_t componentIndex) const {
  return m_componentInfo->isStructuredBank(componentIndex);
}

} // namespace Geometry
} // namespace Mantid
