// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"

#include <Eigen/Geometry>
#include <exception>
#include <iterator>
#include <string>

namespace Mantid::Geometry {

namespace {
/**
 * Rotate point by inverse of rotation held at componentIndex
 */
const Eigen::Vector3d undoRotation(const Eigen::Vector3d &point, const Beamline::ComponentInfo &compInfo,
                                   const size_t componentIndex) {
  auto unRotateTransform = Eigen::Affine3d(compInfo.rotation(componentIndex).inverse());
  return unRotateTransform * point;
}
/**
 * Put the point into the frame of the shape.
 * 1. Subtract component position (puts component pos at origin, same as shape
 * coordinate system).
 * 2. Apply inverse rotation of component to point. Unrotates the component into
 * shape coordinate frame, with observer reorientated.
 */
const Kernel::V3D toShapeFrame(const Kernel::V3D &point, const Beamline::ComponentInfo &compInfo,
                               const size_t componentIndex) {
  return Kernel::toV3D(
      undoRotation(Kernel::toVector3d(point) - compInfo.position(componentIndex), compInfo, componentIndex));
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
    std::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>> componentIds,
    std::shared_ptr<const std::unordered_map<Geometry::IComponent *, size_t>> componentIdToIndexMap,
    std::shared_ptr<std::vector<std::shared_ptr<const Geometry::IObject>>> shapes)
    : m_componentInfo(std::move(componentInfo)), m_componentIds(std::move(componentIds)),
      m_compIDToIndex(std::move(componentIdToIndexMap)), m_shapes(std::move(shapes)) {

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
std::unique_ptr<Geometry::ComponentInfo> ComponentInfo::cloneWithoutDetectorInfo() const {

  return std::unique_ptr<Geometry::ComponentInfo>(new Geometry::ComponentInfo(*this));
}

/** Copy constructor. Use with EXTREME CARE.
 *
 * Should not be public since proper links between DetectorInfo and
 * ComponentInfo must be set up. */
ComponentInfo::ComponentInfo(const ComponentInfo &other)
    : m_componentInfo(other.m_componentInfo->cloneWithoutDetectorInfo()), m_componentIds(other.m_componentIds),
      m_compIDToIndex(other.m_compIDToIndex), m_shapes(other.m_shapes) {}

// Defined as default in source for forward declaration with std::unique_ptr.
ComponentInfo::~ComponentInfo() = default;

std::vector<size_t> ComponentInfo::detectorsInSubtree(size_t componentIndex) const {
  return m_componentInfo->detectorsInSubtree(componentIndex);
}

std::vector<size_t> ComponentInfo::componentsInSubtree(size_t componentIndex) const {
  return m_componentInfo->componentsInSubtree(componentIndex);
}

const std::vector<size_t> &ComponentInfo::children(size_t componentIndex) const {
  return m_componentInfo->children(componentIndex);
}

size_t ComponentInfo::size() const { return m_componentInfo->size(); }

ComponentInfo::QuadrilateralComponent ComponentInfo::quadrilateralComponent(const size_t componentIndex) const {
  auto type = componentType(componentIndex);
  auto parentType = componentType(parent(componentIndex));
  if (!(type == Beamline::ComponentType::Structured || type == Beamline::ComponentType::Rectangular ||
        parentType == Beamline::ComponentType::Grid))
    throw std::runtime_error("ComponentType is not Structured or Rectangular "
                             "in ComponentInfo::quadrilateralComponent.");

  QuadrilateralComponent corners;
  const auto &innerRangeComp = m_componentInfo->children(componentIndex);
  corners.nX = innerRangeComp.size();
  const auto &firstCol = m_componentInfo->children(innerRangeComp[0]);
  const auto &lastCol = m_componentInfo->children(innerRangeComp[corners.nX - 1]);
  corners.nY = firstCol.size();
  corners.bottomLeft = firstCol.front();
  corners.topRight = lastCol.back();
  corners.topLeft = firstCol.back();
  corners.bottomRight = lastCol.front();

  return corners;
}

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const { return m_compIDToIndex->at(id); }

size_t ComponentInfo::indexOfAny(const std::string &name) const { return m_componentInfo->indexOfAny(name); }

bool ComponentInfo::uniqueName(const std::string &name) const { return m_componentInfo->uniqueName(name); }

bool ComponentInfo::isDetector(const size_t componentIndex) const {
  return m_componentInfo->isDetector(componentIndex);
}

bool ComponentInfo::hasValidShape(const size_t componentIndex) const {
  const auto *shapeAtIndex = (*m_shapes)[componentIndex].get();
  return shapeAtIndex != nullptr && shapeAtIndex->hasValidShape();
}

Kernel::V3D ComponentInfo::position(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->position(componentIndex));
}

Kernel::V3D ComponentInfo::position(const std::pair<size_t, size_t> &index) const {
  return Kernel::toV3D(m_componentInfo->position(index));
}

Kernel::Quat ComponentInfo::rotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo->rotation(componentIndex));
}

Kernel::Quat ComponentInfo::rotation(const std::pair<size_t, size_t> &index) const {
  return Kernel::toQuat(m_componentInfo->rotation(index));
}

Kernel::V3D ComponentInfo::relativePosition(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->relativePosition(componentIndex));
}

Kernel::Quat ComponentInfo::relativeRotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo->relativeRotation(componentIndex));
}

void ComponentInfo::setPosition(const std::pair<size_t, size_t> &index, const Kernel::V3D &newPosition) {
  m_componentInfo->setPosition(index, Kernel::toVector3d(newPosition));
}

void ComponentInfo::setRotation(const std::pair<size_t, size_t> &index, const Kernel::Quat &newRotation) {
  m_componentInfo->setRotation(index, Kernel::toQuaterniond(newRotation));
}

void ComponentInfo::scaleComponent(const size_t componentIndex, const Kernel::V3D &newScaling) {
  m_componentInfo->scaleComponent(componentIndex, Kernel::toVector3d(newScaling));
}

void ComponentInfo::scaleComponent(const std::pair<size_t, size_t> &index, const Kernel::V3D &newScaling) {
  m_componentInfo->scaleComponent(index, Kernel::toVector3d(newScaling));
}

size_t ComponentInfo::parent(const size_t componentIndex) const { return m_componentInfo->parent(componentIndex); }

bool ComponentInfo::hasParent(const size_t componentIndex) const { return m_componentInfo->hasParent(componentIndex); }

bool ComponentInfo::hasDetectorInfo() const { return m_componentInfo->hasDetectorInfo(); }

Kernel::V3D ComponentInfo::sourcePosition() const { return Kernel::toV3D(m_componentInfo->sourcePosition()); }

Kernel::V3D ComponentInfo::samplePosition() const { return Kernel::toV3D(m_componentInfo->samplePosition()); }

bool ComponentInfo::hasSource() const { return m_componentInfo->hasSource(); }

/*
 * @brief Check the sources of two componentInfo objects coincide
 *
 * @details check both objects either lack or have a source. If the latter,
 * check their positions differ by less than 1 nm = 1e-9 m.
 *
 * @returns true if sources are equivalent
 */
bool ComponentInfo::hasEquivalentSource(const ComponentInfo &other) const {
  return m_componentInfo->hasEquivalentSource(*(other.m_componentInfo));
}

bool ComponentInfo::hasSample() const { return m_componentInfo->hasSample(); }

/*
 * @brief Check the samples of two componentInfo objects coincide
 *
 * @details check both objects either lack or have a sample. If the latter,
 * check their positions differ by less than 1 nm = 1e-9 m.
 *
 * @returns true if sources are equivalent
 */
bool ComponentInfo::hasEquivalentSample(const ComponentInfo &other) const {
  return m_componentInfo->hasEquivalentSample(*(other.m_componentInfo));
}

bool ComponentInfo::hasDetectors(const size_t componentIndex) const {
  if (isDetector(componentIndex))
    return false;
  const auto range = m_componentInfo->detectorRangeInSubtree(componentIndex);
  return range.begin() < range.end();
}

size_t ComponentInfo::source() const { return m_componentInfo->source(); }

size_t ComponentInfo::sample() const { return m_componentInfo->sample(); }

double ComponentInfo::l1() const { return m_componentInfo->l1(); }

size_t ComponentInfo::root() const { return m_componentInfo->root(); }

void ComponentInfo::setPosition(const size_t componentIndex, const Kernel::V3D &newPosition) {
  m_componentInfo->setPosition(componentIndex, Kernel::toVector3d(newPosition));
}

void ComponentInfo::setRotation(const size_t componentIndex, const Kernel::Quat &newRotation) {
  m_componentInfo->setRotation(componentIndex, Kernel::toQuaterniond(newRotation));
}

const IObject &ComponentInfo::shape(const size_t componentIndex) const { return *(*m_shapes)[componentIndex]; }

Kernel::V3D ComponentInfo::scaleFactor(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo->scaleFactor(componentIndex));
}

const std::string &ComponentInfo::name(const size_t componentIndex) const {
  return m_componentInfo->name(componentIndex);
}

void ComponentInfo::setScaleFactor(const size_t componentIndex, const Kernel::V3D &scaleFactor) {
  m_componentInfo->setScaleFactor(componentIndex, Kernel::toVector3d(scaleFactor));
}

double ComponentInfo::solidAngle(const size_t componentIndex, const Geometry::SolidAngleParams &params) const {
  if (!hasValidShape(componentIndex))
    throw Kernel::Exception::NullPointerException("ComponentInfo::solidAngle", "shape");
  // This is the observer position in the shape's coordinate system.
  const Kernel::V3D relativeObserver = toShapeFrame(params.observer(), *m_componentInfo, componentIndex);
  const Kernel::V3D scaleFactorAtIndex = this->scaleFactor(componentIndex);
  const auto paramsWithRelativeObserver = params.copyWithNewObserver(relativeObserver);
  if ((scaleFactorAtIndex - Kernel::V3D(1.0, 1.0, 1.0)).norm() < 1e-12)
    return shape(componentIndex).solidAngle(paramsWithRelativeObserver);
  else {
    // This function will scale the object shape when calculating the solid
    // angle.
    return shape(componentIndex).solidAngle(paramsWithRelativeObserver, scaleFactorAtIndex);
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
 * @param excludeMonitors : Optional flag to exclude monitors and choppers
 */
void ComponentInfo::growBoundingBoxAsRectuangularBank(size_t index, const Geometry::BoundingBox *reference,
                                                      Geometry::BoundingBox &mutableBB,
                                                      const bool excludeMonitors) const {

  auto panel = quadrilateralComponent(index);
  mutableBB.grow(componentBoundingBox(panel.bottomLeft, reference, excludeMonitors));
  mutableBB.grow(componentBoundingBox(panel.topRight, reference, excludeMonitors));
  mutableBB.grow(componentBoundingBox(panel.topLeft, reference, excludeMonitors));
  mutableBB.grow(componentBoundingBox(panel.bottomRight, reference, excludeMonitors));
}

/**
 * Grow the bounding box on the basis that the component described by index
 * has an outline, which describes the full bounding box of all components and
 * detectors held within.
 *
 * @param index : Index of the component to get the bounding box for
 * @param reference : Reference bounding box (optional)
 * @param mutableBB : Output bounding box. This will be grown.
 * @param excludeMonitors : Optional flag to exclude monitors and choppers
 */
void ComponentInfo::growBoundingBoxAsOutline(size_t index, const BoundingBox *reference, BoundingBox &mutableBB,
                                             const bool excludeMonitors) const {
  mutableBB.grow(componentBoundingBox(index, reference, excludeMonitors));
}

/**
 * Calculates the absolute bounding box for the leaf item at index
 *
 * @param index : Component index
 * @param reference : Optional reference for coordinate system for non-axis
 *aligned bounding boxes
 * @param excludeMonitors : Optional flag to exclude monitor and choppers
 * @return Absolute bounding box.
 */
BoundingBox ComponentInfo::componentBoundingBox(const size_t index, const BoundingBox *reference,
                                                const bool excludeMonitors) const {
  // Check that we have a valid shape here
  if (componentType(index) == Beamline::ComponentType::Infinite) {
    return BoundingBox(); // Return null bounding box
  }
  if (excludeMonitors) {
    // skip monitors
    if (isDetector(index) && m_componentInfo->isMonitor(index)) {
      return BoundingBox();
    }
    // skip other components such as choppers, etc
    if (componentType(index) == Beamline::ComponentType::Generic) {
      return BoundingBox();
    }
  }
  if (!hasValidShape(index)) {
    return BoundingBox(this->position(index).X(), this->position(index).Y(), this->position(index).Z(),
                       this->position(index).X(), this->position(index).Y(), this->position(index).Z());
  } else {
    const auto &s = this->shape(index);
    BoundingBox absoluteBB = s.getBoundingBox();

    // modify in place for speed
    const Eigen::Vector3d scaleFactorAtIndex = m_componentInfo->scaleFactor(index);
    // Scale
    absoluteBB.xMin() *= scaleFactorAtIndex[0];
    absoluteBB.xMax() *= scaleFactorAtIndex[0];
    absoluteBB.yMin() *= scaleFactorAtIndex[1];
    absoluteBB.yMax() *= scaleFactorAtIndex[1];
    absoluteBB.zMin() *= scaleFactorAtIndex[2];
    absoluteBB.zMax() *= scaleFactorAtIndex[2];
    // Rotate
    (this->rotation(index))
        .rotateBB(absoluteBB.xMin(), absoluteBB.yMin(), absoluteBB.zMin(), absoluteBB.xMax(), absoluteBB.yMax(),
                  absoluteBB.zMax());

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
      coordSystem.assign(reference->getCoordSystem().begin(), reference->getCoordSystem().end());

      // realign to reference coordinate system
      absoluteBB.realign(&coordSystem);
    }
    return absoluteBB;
  }
}

/**
 * Compute the bounding box for the component with componentIndex taking into
 *account
 * all sub components.
 *
 * @param componentIndex : Component index to get the bounding box for
 * @param reference : Optional reference for coordinate system for non-axis
 *aligned bounding boxes
 * @param excludeMonitors : Optional flag to exclude monitors and choppers
 * @return Absolute bounding box
 */
BoundingBox ComponentInfo::boundingBox(const size_t componentIndex, const BoundingBox *reference,
                                       const bool excludeMonitors) const {
  if (isDetector(componentIndex) || componentType(componentIndex) == Beamline::ComponentType::Infinite) {
    return componentBoundingBox(componentIndex, reference, excludeMonitors);
  }

  BoundingBox absoluteBB;
  const auto compFlag = componentType(componentIndex);

  auto parentFlag = Beamline::ComponentType::Generic;
  if (size() > 1)
    parentFlag = componentType(parent(componentIndex));

  if (hasSource() && componentIndex == source()) {
    // Do nothing. Source is not considered part of the beamline for bounding
    // box calculations.
  } else if (compFlag == Beamline::ComponentType::Unstructured) {
    for (const auto &childIndex : this->children(componentIndex)) {
      absoluteBB.grow(boundingBox(childIndex, reference, excludeMonitors));
    }
  } else if (compFlag == Beamline::ComponentType::Grid) {
    for (const auto &childIndex : this->children(componentIndex)) {
      growBoundingBoxAsRectuangularBank(childIndex, reference, absoluteBB, excludeMonitors);
    }
  } else if (compFlag == Beamline::ComponentType::Rectangular || compFlag == Beamline::ComponentType::Structured ||
             parentFlag == Beamline::ComponentType::Grid) {
    growBoundingBoxAsRectuangularBank(componentIndex, reference, absoluteBB, excludeMonitors);
  } else if (compFlag == Beamline::ComponentType::OutlineComposite) {
    growBoundingBoxAsOutline(componentIndex, reference, absoluteBB, excludeMonitors);
  } else {
    // General case
    absoluteBB.grow(componentBoundingBox(componentIndex, reference, excludeMonitors));
  }
  return absoluteBB;
}

Beamline::ComponentType ComponentInfo::componentType(const size_t componentIndex) const {
  return m_componentInfo->componentType(componentIndex);
}

void ComponentInfo::setScanInterval(const std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime> &interval) {
  m_componentInfo->setScanInterval({interval.first.totalNanoseconds(), interval.second.totalNanoseconds()});
}

size_t ComponentInfo::scanCount() const { return m_componentInfo->scanCount(); }

void ComponentInfo::merge(const ComponentInfo &other) { m_componentInfo->merge(*other.m_componentInfo); }

ComponentInfoIt ComponentInfo::begin() { return ComponentInfoIt(*this, 0, size()); }

ComponentInfoIt ComponentInfo::end() { return ComponentInfoIt(*this, size(), size()); }

const ComponentInfoConstIt ComponentInfo::cbegin() { return ComponentInfoConstIt(*this, 0, size()); }

const ComponentInfoConstIt ComponentInfo::cend() { return ComponentInfoConstIt(*this, size(), size()); }

} // namespace Mantid::Geometry
