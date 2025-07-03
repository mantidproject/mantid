// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/PanelsSurfaceCalculator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"

using namespace Mantid::Geometry;
using Mantid::Beamline::ComponentType;
using Mantid::Kernel::V3D;

namespace Mantid::API {
/**
 * Given the z axis, define the x and y ones.
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

Mantid::Kernel::V3D
PanelsSurfaceCalculator::calculatePanelNormal(const std::vector<Mantid::Kernel::V3D> &panelCorners) const {
  // find the normal
  auto xaxis = panelCorners[1] - panelCorners[0];
  auto yaxis = panelCorners[3] - panelCorners[0];
  const auto normal = normalize(xaxis.cross_prod(yaxis));
  return normal;
}

bool PanelsSurfaceCalculator::isBankFlat(const ComponentInfo &componentInfo, size_t bankIndex,
                                         const std::vector<size_t> &tubes, const Mantid::Kernel::V3D &normal) {
  for (auto tube : tubes) {
    const auto &children = componentInfo.children(tube);
    const auto vector = normalize(componentInfo.position(children[0]) - componentInfo.position(children[1]));
    if (fabs(vector.scalar_prod(normal)) > Mantid::Kernel::Tolerance) {
      this->g_log.warning() << "Assembly " << componentInfo.name(bankIndex) << " isn't flat.\n";
      return false;
    }
  }
  return true;
}

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

// Recursively set all detectors and subcomponents of a bank as visited
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

} // namespace Mantid::API
