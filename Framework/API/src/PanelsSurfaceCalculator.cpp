// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/PanelsSurfaceCalculator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"

#include <cmath>
#include <numeric>

using namespace Mantid::Geometry;
using Mantid::Beamline::ComponentType;
using Mantid::Kernel::V3D;

namespace Mantid::API {
/**
 * Given the z axis, define the x and y ones.
 *
 * @param zaxis :: A given vector in 3d space to become the z axis of a
 * coordinate system.
 * @param xaxis :: An output arbitrary vector perpendicular to zaxis.
 * @param yaxis :: An output arbitrary vector perpendicular to both zaxis and
 * xaxis.
 */
void PanelsSurfaceCalculator::setupBasisAxes(const Mantid::Kernel::V3D &zaxis, Mantid::Kernel::V3D &xaxis,
                                             Mantid::Kernel::V3D &yaxis) const {
  double R, theta, phi;
  zaxis.getSpherical(R, theta, phi);
  if (theta <= 45.0) {
    xaxis = Mantid::Kernel::V3D(1, 0, 0);
  } else if (phi <= 45.0) {
    xaxis = Mantid::Kernel::V3D(0, 1, 0);
  } else {
    xaxis = Mantid::Kernel::V3D(0, 0, 1);
  }
  yaxis = zaxis.cross_prod(xaxis);
  yaxis.normalize();
  xaxis = yaxis.cross_prod(zaxis);
}

/**
 * Returns the four corners of the specified panel
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param rootIndex :: Index of panel
 * @returns :: Panel Corners
 */
std::vector<Mantid::Kernel::V3D>
PanelsSurfaceCalculator::retrievePanelCorners(const Mantid::Geometry::ComponentInfo &componentInfo,
                                              const size_t rootIndex) const {
  auto panel = componentInfo.quadrilateralComponent(rootIndex);
  auto child = rootIndex;

  // Find shapeInfo for one of the children
  do {
    const auto &children = componentInfo.children(child);
    child = children[0];
  } while (!componentInfo.isDetector(child));

  const auto &shape = componentInfo.shape(child);
  const auto &shapeInfo = shape.getGeometryHandler()->shapeInfo();
  const auto &points = shapeInfo.points();
  double xoff = 0.0;
  double yoff = 0.0;

  // Find x and y widths of detectors and treat as offsets;
  for (size_t i = 0; i < points.size() - 1; ++i) {
    double xdiff = std::abs(points[i + 1].X() - points[i].X());
    double ydiff = std::abs(points[i + 1].Y() - points[i].Y());
    if (xdiff != 0)
      xoff = xdiff * 0.5;
    if (ydiff != 0)
      yoff = ydiff * 0.5;
  }

  std::vector<Mantid::Kernel::V3D> corners{
      componentInfo.position(panel.bottomLeft), componentInfo.position(panel.bottomRight),
      componentInfo.position(panel.topRight), componentInfo.position(panel.topLeft)};

  // Find xmin, xmax, ymin and ymax
  double xmin = corners[0].X();
  double xmax = corners[0].X();
  double ymin = corners[0].Y();
  double ymax = corners[0].Y();

  for (const auto &corner : corners) {
    xmin = corner.X() < xmin ? corner.X() : xmin;
    xmax = corner.X() > xmax ? corner.X() : xmax;
    ymin = corner.Y() < ymin ? corner.Y() : ymin;
    ymax = corner.Y() > ymax ? corner.Y() : ymax;
  }

  // apply offsets
  for (auto &corner : corners) {
    auto x = corner.X();
    auto y = corner.Y();
    corner.setX(x == xmin ? x - xoff : x + xoff);
    corner.setY(y == ymin ? y - yoff : y + yoff);
  }

  return corners;
}

/**
 * Calculate the normal vector to a panel
 *
 * @param panelCorners :: The four panel corner locations
 * @returns :: Normal vector
 */
Mantid::Kernel::V3D
PanelsSurfaceCalculator::calculatePanelNormal(const std::vector<Mantid::Kernel::V3D> &panelCorners) const {
  // find the normal
  auto xaxis = panelCorners[1] - panelCorners[0];
  auto yaxis = panelCorners[3] - panelCorners[0];
  const auto normal = normalize(xaxis.cross_prod(yaxis));
  return normal;
}

/**
 * Do all the detectors lie in a plane?
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param bankIndex :: Component index of bank
 * @param tubes :: Tube component indices
 * @param normal :: Panel normal vector
 * @returns Do the detectors lie flat in a plane?
 */
bool PanelsSurfaceCalculator::isBankFlat(const ComponentInfo &componentInfo, size_t bankIndex,
                                         const std::vector<size_t> &tubes, const Mantid::Kernel::V3D &normal) {
  for (auto tube : tubes) {
    const auto &children = componentInfo.children(tube);
    const auto vector = normalize(componentInfo.position(children[0]) - componentInfo.position(children[1]));
    if (std::abs(vector.scalar_prod(normal)) > Mantid::Kernel::Tolerance) {
      this->g_log.warning() << "Assembly " << componentInfo.name(bankIndex) << " isn't flat.\n";
      return false;
    }
  }
  return true;
}

/**
 * Calculate the normal vector of a bank of detectors
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param tubes :: Component indices of the tubes in the bank
 * @returns Bank normal vector
 */
Mantid::Kernel::V3D PanelsSurfaceCalculator::calculateBankNormal(const ComponentInfo &componentInfo,
                                                                 const std::vector<size_t> &tubes) {
  // calculate normal from first two tubes in bank as before
  const auto &tube0 = componentInfo.children(tubes[0]);
  const auto &tube1 = componentInfo.children(tubes[1]);
  auto pos = componentInfo.position(tube0[0]);
  auto x = componentInfo.position(tube0[1]) - pos;
  x.normalize();

  auto y = componentInfo.position(tube1[0]) - pos;
  y.normalize();
  auto normal = x.cross_prod(y);

  if (normal.nullVector()) {
    y = componentInfo.position(tube1[1]) - componentInfo.position(tube1[0]);
    y.normalize();
    normal = x.cross_prod(y);
  }

  normal.normalize();

  if (normal.nullVector())
    this->g_log.warning() << "Colinear Assembly.\n";

  return normal;
}

/**
 * Recursively set all detectors and subcomponents of a bank as visited
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param bankIndex :: Component index of the bank
 * @param visitedComponents :: Vector keeping track of which components have been visited
 */
void PanelsSurfaceCalculator::setBankVisited(const ComponentInfo &componentInfo, size_t bankIndex,
                                             std::vector<bool> &visitedComponents) const {
  const auto &children = componentInfo.children(bankIndex);
  visitedComponents[bankIndex] = true;
  for (auto child : children) {
    const auto &subChildren = componentInfo.children(child);
    if (subChildren.size() > 0)
      setBankVisited(componentInfo, child, visitedComponents);
    else
      visitedComponents[child] = true;
  }
}

/**
 * How many detectors are there in the given list of component indices?
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param components :: Component indices to check
 * @return :: Number of detectors
 */
size_t PanelsSurfaceCalculator::findNumDetectors(const ComponentInfo &componentInfo,
                                                 const std::vector<size_t> &components) const {
  return std::accumulate(
      components.cbegin(), components.cend(), std::size_t{0u},
      [&componentInfo](size_t lhs, const auto &comp) { return componentInfo.isDetector(comp) ? lhs + 1u : lhs; });
}

/**
 * Calculate the rotation needed around the bank's local x and y axes to place a bank on the projection plane
 * Perform the rotation in two stages to avoid any twist about the normal
 *
 * @param detPos :: Position of a detector of the bank.
 * @param normal :: Normal to the bank's plane.
 * @param zAxis ::  Direction to the viewer
 * @param yAxis ::  Perpendicular axis
 * @param samplePosition :: Sample position
 * @returns :: Rotation
 */
Mantid::Kernel::Quat PanelsSurfaceCalculator::calcBankRotation(const V3D &detPos, V3D normal, const V3D &zAxis,
                                                               const V3D &yAxis, const V3D &samplePosition) const {
  V3D directionToViewer = zAxis;
  V3D bankToOrigin = samplePosition - detPos;
  // signed shortest distance from the bank's plane to the origin (m_pos)
  double a = normal.scalar_prod(bankToOrigin);
  // if a is negative the origin is on the "back" side of the plane
  // (the "front" side is facing in the direction of the normal)
  if (a < 0.0) {
    // we need to flip the normal to make the side looking at the origin to be
    // the front one
    normal *= -1;
  }
  double b = zAxis.scalar_prod(bankToOrigin);
  if (b < 0.0) {
    // if the bank is at positive z then we need to rotate the normal to point in negative z direction
    directionToViewer *= -1;
  }
  if (directionToViewer == -normal) {
    return Mantid::Kernel::Quat(0, yAxis.X(), yAxis.Y(), yAxis.Z()); // 180 degree rotation about y axis
  } else if (normal.cross_prod(directionToViewer).nullVector()) {
    return Mantid::Kernel::Quat();
  }

  Mantid::Kernel::Quat requiredRotation;
  if (normal.cross_prod(yAxis).nullVector()) {
    requiredRotation = Mantid::Kernel::Quat(normal, directionToViewer);
  } else {
    V3D normalInXZPlane = {normal.X(), 0., normal.Z()};
    normalInXZPlane.normalize();
    auto rotationLocalX = Mantid::Kernel::Quat(normal, normalInXZPlane);
    auto rotAboutY180 = Mantid::Kernel::Quat(0, yAxis.X(), yAxis.Y(), yAxis.Z());
    auto rotationLocalY =
        normalInXZPlane == -directionToViewer ? rotAboutY180 : Mantid::Kernel::Quat(normalInXZPlane, directionToViewer);
    requiredRotation = rotationLocalY * rotationLocalX;
  }
  return requiredRotation;
}

/**
 * Transforms bounding box of a detector
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param detectorIndex :: Component index of the detector
 * @param refPos :: Reference position
 * @param rotation :: Rotation to apply
 * @param xaxis :: X axis
 * @param yaxis :: Y axis
 * @returns Transformed bounding box points
 */
std::vector<Mantid::Kernel::V2D>
PanelsSurfaceCalculator::transformedBoundingBoxPoints(const ComponentInfo &componentInfo, size_t detectorIndex,
                                                      const V3D &refPos, const Mantid::Kernel::Quat &rotation,
                                                      const V3D &xaxis, const V3D &yaxis) const {
  auto bb = componentInfo.boundingBox(detectorIndex);
  auto bbMinPoint = bb.minPoint() - refPos;
  auto bbMaxPoint = bb.maxPoint() - refPos;
  rotation.rotate(bbMinPoint);
  rotation.rotate(bbMaxPoint);
  bbMinPoint += refPos;
  bbMaxPoint += refPos;
  Mantid::Kernel::V2D bb0(xaxis.scalar_prod(bbMinPoint), yaxis.scalar_prod(bbMinPoint));
  Mantid::Kernel::V2D bb1(xaxis.scalar_prod(bbMaxPoint), yaxis.scalar_prod(bbMaxPoint));
  return {bb0, bb1};
}

/**
 * Perform a specified operation on all the components
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param operation :: Operation to perform on each component
 * @returns Detector IDs for each component
 */
std::vector<std::vector<size_t>> PanelsSurfaceCalculator::examineAllComponents(
    const ComponentInfo &componentInfo,
    std::function<std::vector<size_t>(const ComponentInfo &, size_t, std::vector<bool> &)> operation) {
  std::vector<bool> visited(componentInfo.size(), false);
  std::vector<std::vector<size_t>> detectorIDs;

  for (int64_t i = static_cast<int64_t>(componentInfo.root() - 1); i > 0; --i) {
    auto children = componentInfo.children(i);

    if (children.size() > 0 && !visited[i]) {
      visited[i] = true;
      detectorIDs.push_back(operation(componentInfo, i, visited));
    } else if (children.size() == 0 && componentInfo.parent(i) == componentInfo.root()) {
      visited[i] = true;
    }
  }
  return detectorIDs;
}

/**
 * Parent indices of tubes
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param rootIndex :: Component index to check
 * @param visited :: Vector tracking which components have been checked
 * @returns Parent IDs
 */
std::vector<size_t> PanelsSurfaceCalculator::tubeDetectorParentIDs(const ComponentInfo &componentInfo, size_t rootIndex,
                                                                   std::vector<bool> &visited) {
  const auto componentType = componentInfo.componentType(rootIndex);
  if (componentType != ComponentType::OutlineComposite)
    return {};

  const auto bankIndex0 = componentInfo.parent(rootIndex);
  auto tubes = std::vector<size_t>();
  bool foundFlatBank = false;
  V3D normal;
  size_t bankIndex;
  auto addTubes = [&componentInfo](size_t parentIndex, std::vector<size_t> &tubes) {
    const auto &children = componentInfo.children(parentIndex);
    for (auto child : children) {
      if (componentInfo.componentType(child) == ComponentType::OutlineComposite)
        // tube must have more than one detector to enable normal to be calculated
        if (componentInfo.children(child).size() > 1)
          tubes.emplace_back(child);
    }
  };

  // The main use case for this method has an assembly containing a set of
  // individual assemblies each of which has a single tube but together
  // these tubes make a flat structure.
  // Try grandparent of the tube supplied tube initially
  if (componentInfo.hasParent(bankIndex0)) {
    bankIndex = componentInfo.parent(bankIndex0);
    const auto &bankChildren = componentInfo.children(bankIndex);

    // Go down the tree to find all the tubes.
    for (const auto index : bankChildren)
      addTubes(index, tubes);
    if (tubes.empty()) {
      this->setBankVisited(componentInfo, bankIndex, visited);
      return tubes;
    }
    // Now we found all the tubes that may form a flat struture.
    // Use two of the tubes to calculate the normal to the plain of that structure
    normal = tubes.size() > 1 ? this->calculateBankNormal(componentInfo, tubes) : V3D();
    // If some of the tubes are not perpendicular to the normal the structure
    // isn't flat
    if (!normal.nullVector() && this->isBankFlat(componentInfo, bankIndex, tubes, normal))
      foundFlatBank = true;
  }

  if (!foundFlatBank) {
    // Try the next level down - parent of tube supplied
    tubes.clear();
    bankIndex = bankIndex0;
    addTubes(bankIndex, tubes);
    normal = tubes.size() > 1 ? this->calculateBankNormal(componentInfo, tubes) : V3D();
    if (normal.nullVector() || !this->isBankFlat(componentInfo, bankIndex, tubes, normal))
      this->setBankVisited(componentInfo, componentInfo.parent(rootIndex), visited);
  }

  this->setBankVisited(componentInfo, bankIndex, visited);
  return tubes;
}

/**
 * Gives the specified side-by-side view position from the IDF
 *
 * @param componentInfo :: ComponentInfo object from the workspace
 * @param instrument :: Instrument object from the workspace
 * @param componentIndex :: Component index to check
 * @returns :: Side-by-side position from the IDF
 */
std::optional<Kernel::V2D> PanelsSurfaceCalculator::getSideBySideViewPos(const ComponentInfo &componentInfo,
                                                                         const Instrument_const_sptr &instrument,
                                                                         const size_t componentIndex) const {
  const auto *componentID = componentInfo.componentID(componentIndex);
  const auto component = instrument->getComponentByID(componentID);
  if (!component) {
    return std::optional<Kernel::V2D>();
  }

  return component->getSideBySideViewPos();
}

} // namespace Mantid::API
