// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
using Kernel::V3D;

/**
 * @brief Create XML string to define a cuboid for the beam and sample intersection
 *
 * @param width width of intersection area
 * @param height hight of intersection area
 * @param depth depth of intersection area
 * @param centerPos centre of the intersection area
 * @return std::string
 */
std::string cuboidXML(double width, double height, double depth, const V3D &centerPos) {

  // Convert full dimensions to half-lengths
  const double szX = width / 2.0;
  const double szY = height / 2.0;
  const double szZ = depth / 2.0;

  // Define corners of the cuboid
  V3D leftFrontBottom{szX, -szY, -szZ};
  V3D leftFrontTop{szX, -szY, szZ};
  V3D leftBackBottom{-szX, -szY, -szZ};
  V3D rightFrontBottom{szX, szY, -szZ};

  // Shift the points by the center position
  leftFrontBottom += centerPos;
  leftFrontTop += centerPos;
  leftBackBottom += centerPos;
  rightFrontBottom += centerPos;

  std::ostringstream xmlShapeStream;
  xmlShapeStream << " <cuboid id=\"gauge-volume\"> "
                 << "<left-front-bottom-point x=\"" << leftFrontBottom.X() << "\" y=\"" << leftFrontBottom.Y()
                 << "\" z=\"" << leftFrontBottom.Z() << "\"  /> "
                 << "<left-front-top-point  x=\"" << leftFrontTop.X() << "\" y=\"" << leftFrontTop.Y() << "\" z=\""
                 << leftFrontTop.Z() << "\"  /> "
                 << "<left-back-bottom-point  x=\"" << leftBackBottom.X() << "\" y=\"" << leftBackBottom.Y()
                 << "\" z=\"" << leftBackBottom.Z() << "\"  /> "
                 << "<right-front-bottom-point  x=\"" << rightFrontBottom.X() << "\" y=\"" << rightFrontBottom.Y()
                 << "\" z=\"" << rightFrontBottom.Z() << "\"  /> "
                 << "</cuboid>";

  return xmlShapeStream.str();
}

namespace Algorithms {
/**
 * @brief Construct a new IBeamProfile::IBeamProfile object
 *
 * @param center Center of the beam
 */
IBeamProfile::IBeamProfile(const V3D center) : m_beamCenter(center) {}

/**
 * @brief Get the intersection of the beam with the sample
 *
 * @param sample Sample object to be intersected with
 * @param beamDirection Direction from sample to beam to get correct intersection
 * @return Geometry::IObject_sptr
 */
Geometry::IObject_sptr IBeamProfile::getIntersectionWithSample(const Geometry::IObject &sample,
                                                               const V3D beamDirection) const {
  Geometry::BoundingBox sampleBB = sample.getBoundingBox();
  Geometry::BoundingBox intersectionBox;
  V3D xAxis(1, 0, 0);
  V3D yAxis(0, 1, 0);

  if (std::abs(beamDirection.scalar_prod(xAxis)) == 1) {
    xAxis = V3D(0, 1, 0);
  }

  // Make yAxis perpendicular to both the beam direction and xAxis
  yAxis = beamDirection.cross_prod(xAxis);
  xAxis = yAxis.cross_prod(beamDirection);
  xAxis.normalize();
  yAxis.normalize();

  // Set up the coordinate system for re-alignment
  std::vector<V3D> coordSystem = {m_beamCenter, xAxis, yAxis, beamDirection};
  sampleBB.realign(&coordSystem);

  try {
    intersectionBox = defineActiveRegion(sampleBB);
  } catch (...) {
    return nullptr;
  }

  double height = intersectionBox.yMax() - intersectionBox.yMin();
  double width = intersectionBox.xMax() - intersectionBox.xMin();
  double depth = intersectionBox.zMax() - intersectionBox.zMin();

  std::string shapeXML = cuboidXML(width, height, depth, intersectionBox.centrePoint());
  return Geometry::ShapeFactory().createShape(shapeXML);
}

} // namespace Algorithms
} // namespace Mantid
