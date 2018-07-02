#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/make_unique.h"
#include <Eigen/Geometry>
#include <exception>
#include <iterator>
#include <string>

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

} // namespace

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
    boost::shared_ptr<std::vector<boost::shared_ptr<const Geometry::IObject>>>
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

/**
 * Clone current instance but not the DetectorInfo non-owned parts
 * @return unique pointer wrapped deep copy of ComponentInfo
 */
std::unique_ptr<Geometry::ComponentInfo>
ComponentInfo::cloneWithoutDetectorInfo() const {

  return std::unique_ptr<Geometry::ComponentInfo>(
      new Geometry::ComponentInfo(*this));
}

/** Copy constructor. Use with EXTREME CARE.
 *
 * Should not be public since proper links between DetectorInfo and
 * ComponentInfo must be set up. */
ComponentInfo::ComponentInfo(const ComponentInfo &other)
    : m_componentInfo(other.m_componentInfo->cloneWithoutDetectorInfo()),
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

const std::vector<size_t> &
ComponentInfo::children(size_t componentIndex) const {
  return m_componentInfo->children(componentIndex);
}

size_t ComponentInfo::size() const { return m_componentInfo->size(); }

ComponentInfo::QuadrilateralComponent
ComponentInfo::quadrilateralComponent(const size_t componentIndex) const {
  auto type = componentType(componentIndex);
  if (!(type == Beamline::ComponentType::Structured ||
        type == Beamline::ComponentType::Rectangular))
    throw std::runtime_error("ComponentType is not Structured or Rectangular "
                             "in ComponentInfo::quadrilateralComponent.");

  QuadrilateralComponent corners;
  auto innerRangeComp =
      m_componentInfo->componentRangeInSubtree(componentIndex);
  // nSubComponents, subtract off self hence -1. nSubComponents = number of
  // horizontal columns.
  corners.nX = innerRangeComp.end() - innerRangeComp.begin() - 1;
  auto innerRangeDet = m_componentInfo->detectorRangeInSubtree(componentIndex);
  auto nSubDetectors =
      std::distance(innerRangeDet.begin(), innerRangeDet.end());
  corners.nY = nSubDetectors / corners.nX;

  corners.bottomLeft = *innerRangeDet.begin();
  corners.topRight = corners.bottomLeft + nSubDetectors - 1;
  corners.topLeft = corners.bottomLeft + (corners.nY - 1);
  corners.bottomRight = corners.topRight - (corners.nY - 1);
  return corners;
}

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const {
  return m_compIDToIndex->at(id);
}

size_t ComponentInfo::indexOfAny(const std::string &name) const {
  return m_componentInfo->indexOfAny(name);
}

bool ComponentInfo::isDetector(const size_t componentIndex) const {
  return m_componentInfo->isDetector(componentIndex);
}

bool ComponentInfo::hasValidShape(const size_t componentIndex) const {
  const auto *shape = (*m_shapes)[componentIndex].get();
  return shape != nullptr && shape->hasValidShape();
}

Kernel::V3D ComponentInfo::position(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->position(componentIndex));
}

Kernel::V3D
ComponentInfo::position(const std::pair<size_t, size_t> index) const {
  return Kernel::toV3D(m_componentInfo->position(index));
}

Kernel::Quat ComponentInfo::rotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo->rotation(componentIndex));
}

Kernel::Quat
ComponentInfo::rotation(const std::pair<size_t, size_t> index) const {
  return Kernel::toQuat(m_componentInfo->rotation(index));
}

Kernel::V3D ComponentInfo::relativePosition(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->relativePosition(componentIndex));
}

Kernel::Quat
ComponentInfo::relativeRotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo->relativeRotation(componentIndex));
}

void ComponentInfo::setPosition(const std::pair<size_t, size_t> index,
                                const Kernel::V3D &newPosition) {
  m_componentInfo->setPosition(index, Kernel::toVector3d(newPosition));
}

void ComponentInfo::setRotation(const std::pair<size_t, size_t> index,
                                const Kernel::Quat &newRotation) {
  m_componentInfo->setRotation(index, Kernel::toQuaterniond(newRotation));
}

size_t ComponentInfo::parent(const size_t componentIndex) const {
  return m_componentInfo->parent(componentIndex);
}

bool ComponentInfo::hasParent(const size_t componentIndex) const {
  return m_componentInfo->hasParent(componentIndex);
}

bool ComponentInfo::hasDetectorInfo() const {
  return m_componentInfo->hasDetectorInfo();
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

size_t ComponentInfo::root() const { return m_componentInfo->root(); }

void ComponentInfo::setPosition(const size_t componentIndex,
                                const Kernel::V3D &newPosition) {
  m_componentInfo->setPosition(componentIndex, Kernel::toVector3d(newPosition));
}

void ComponentInfo::setRotation(const size_t componentIndex,
                                const Kernel::Quat &newRotation) {
  m_componentInfo->setRotation(componentIndex,
                               Kernel::toQuaterniond(newRotation));
}

const IObject &ComponentInfo::shape(const size_t componentIndex) const {
  return *(*m_shapes)[componentIndex];
}

Kernel::V3D ComponentInfo::scaleFactor(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->scaleFactor(componentIndex));
}

const std::string &ComponentInfo::name(const size_t componentIndex) const {
  return m_componentInfo->name(componentIndex);
}

void ComponentInfo::setScaleFactor(const size_t componentIndex,
                                   const Kernel::V3D &scaleFactor) {
  m_componentInfo->setScaleFactor(componentIndex,
                                  Kernel::toVector3d(scaleFactor));
}

double ComponentInfo::solidAngle(const size_t componentIndex,
                                 const Kernel::V3D &observer) const {
  if (!hasValidShape(componentIndex))
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
 * Grow the bounding box on the basis that the component described by index is a
 * regular grid in a trapezoid, thus the bounding box can be fully described by
 * just the 4 vertex detectors within.
 *
 * Find the vertexes
 * Grow the bounding box by each vetex
 * Record detectors that form part of this bank that are enclosed and therefore
 * already inclusive of the external bounding box Based on the number of
 * sub-components, which are also fully enclosed, skip the component iterator
 * forward so that those will not need to be evaluated.
 *
 * @param index : Index of the component to get the bounding box for
 * @param reference : Reference bounding box (optional)
 * @param mutableBB : Output bounding box. This will be grown.
 * @param mutableDetExclusions : Output detector exclusions to append to. These
 * are ranges of detector indices that we do NOT need to consider for future
 * bounding box calculations for detectors.
 * @param mutableIterator : Iterator to the next non-detector component.
 */
template <typename IteratorT>
void ComponentInfo::growBoundingBoxAsRectuangularBank(
    size_t index, const Geometry::BoundingBox *reference,
    Geometry::BoundingBox &mutableBB,
    std::map<size_t, size_t> &mutableDetExclusions,
    IteratorT &mutableIterator) const {

  auto panel = quadrilateralComponent(index);
  mutableBB.grow(componentBoundingBox(panel.bottomLeft, reference));
  mutableBB.grow(componentBoundingBox(panel.topRight, reference));
  mutableBB.grow(componentBoundingBox(panel.topLeft, reference));
  mutableBB.grow(componentBoundingBox(panel.bottomRight, reference));

  // Get bounding box for rectangular bank.
  // Record detector ranges to skip
  mutableDetExclusions.insert(std::make_pair(panel.bottomLeft, panel.topRight));
  // Skip all sub components.
  const auto innerRangeComp = m_componentInfo->componentRangeInSubtree(index);
  mutableIterator = innerRangeComp.rend();
}

/**
 * Grow the bounding box on the basis that the component described by index
 * has an outline, which describes the full bounding box of all components and
 * detectors held within.
 *
 * @param index : Index of the component to get the bounding box for
 * @param reference : Reference bounding box (optional)
 * @param mutableBB : Output bounding box. This will be grown.
 * @param mutableDetExclusions : Output detector exclusions to append to. These
 * are ranges of detector indices that we do NOT need to consider for future
 * bounding box calculations for detectors.
 * @param mutableIterator : Iterator to the next non-detector component.
 */
template <typename IteratorT>
void ComponentInfo::growBoundingBoxAsOutline(
    size_t index, const BoundingBox *reference, BoundingBox &mutableBB,
    std::map<size_t, size_t> &mutableDetExclusions,
    IteratorT &mutableIterator) const {

  auto rangeDet = m_componentInfo->detectorRangeInSubtree(index);
  if (!rangeDet.empty()) {
    auto startIndex = *rangeDet.begin();
    auto endIndex = *(rangeDet.end() - 1);
    mutableBB.grow(componentBoundingBox(index, reference));
    mutableDetExclusions.insert(std::make_pair(startIndex, endIndex));
  }
  // No sub components (non detectors) in tube, so just increment iterator
  mutableIterator++;
}

/**
 * Grow the bounding box for a component by evaluating all detector bounding
 * boxes for detectors not within any limits described by excusion ranges.
 *
 * Map of exlusion ranges provided as input. There should be no intersection
 * between exclusion ranges, but that should already be guaranteed since
 * detector indices are unique. The key is the beginning of the exclusion range,
 * and therefore we have a guranteed ordering of exclusion ranges.
 *
 * @param index : Index of the component to get the bounding box for
 * @param reference : Reference bounding box.
 * @param mutableBB : Output bounding box. This will be grown.
 * @param detectorExclusions : ranges of detector indices NOT to consider.
 */
void ComponentInfo::growBoundingBoxByDetectors(
    size_t index, const BoundingBox *reference, BoundingBox &mutableBB,
    const std::map<size_t, size_t> &detectorExclusions) const {
  auto rangeDet = m_componentInfo->detectorRangeInSubtree(index);
  auto detIt = rangeDet.begin();
  auto exclIt = detectorExclusions.begin();
  while (detIt != rangeDet.end()) {
    auto detIndex = *detIt;
    if (exclIt != detectorExclusions.end() && detIndex >= exclIt->first &&
        detIndex <= exclIt->second) {
      // Move the detector iterator outside the current exclusion
      detIt += exclIt->second - exclIt->first + 1;
      // Move the exclusion iterator forward. Do not consider the same exclusion
      // range again.
      ++exclIt;
    } else {
      mutableBB.grow(componentBoundingBox(*detIt, reference));
      ++detIt;
    }
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
  if (!hasValidShape(index) ||
      componentType(index) == Beamline::ComponentType::Infinite) {
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
  if (isDetector(componentIndex) ||
      componentType(componentIndex) == Beamline::ComponentType::Infinite) {
    return componentBoundingBox(componentIndex, reference);
  }
  BoundingBox absoluteBB;
  auto rangeComp = m_componentInfo->componentRangeInSubtree(componentIndex);
  std::map<size_t, size_t> detExclusions{};
  auto compIterator = rangeComp.rbegin();
  while (compIterator != rangeComp.rend()) {
    const size_t index = *compIterator;
    const auto compFlag = componentType(index);
    if (hasSource() && index == source()) {
      ++compIterator;
    } else if (compFlag == Beamline::ComponentType::Rectangular) {
      growBoundingBoxAsRectuangularBank(index, reference, absoluteBB,
                                        detExclusions, compIterator);
    } else if (compFlag == Beamline::ComponentType::OutlineComposite) {
      growBoundingBoxAsOutline(index, reference, absoluteBB, detExclusions,
                               compIterator);
    } else {
      // General case
      absoluteBB.grow(componentBoundingBox(index, reference));
      ++compIterator;
    }
  }

  // Now deal with bounding boxes for detectors
  growBoundingBoxByDetectors(componentIndex, reference, absoluteBB,
                             detExclusions);
  return absoluteBB;
}

Beamline::ComponentType
ComponentInfo::componentType(const size_t componentIndex) const {
  return m_componentInfo->componentType(componentIndex);
}

void ComponentInfo::setScanInterval(
    const std::pair<int64_t, int64_t> &interval) {
  m_componentInfo->setScanInterval(interval);
}

void ComponentInfo::merge(const ComponentInfo &other) {
  m_componentInfo->merge(*other.m_componentInfo);
}

size_t ComponentInfo::scanSize() const { return m_componentInfo->scanSize(); }

} // namespace Geometry
} // namespace Mantid
