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

namespace {
/**
 * @brief Create XML string to define a cuboid for the beam and sample intersection
 *
 * @param xExtent xExtent of intersection area
 * @param yExtent yExtent of intersection area
 * @param zExtent zExtent of intersection area
 * @param centerPos centre of the intersection area
 * @return std::string
 */
std::string cuboidXML(double xExtent, double yExtent, double zExtent, const V3D &centerPos) {

  // Convert full dimensions to half-lengths
  const double szX = xExtent / 2.0;
  const double szY = yExtent / 2.0;
  const double szZ = zExtent / 2.0;

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
} // namespace

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
 * @return Geometry::IObject_sptr
 */
Geometry::IObject_sptr IBeamProfile::getIntersectionWithSample(const Geometry::IObject &sample) const {
  Geometry::BoundingBox sampleBB = sample.getBoundingBox();
  Geometry::BoundingBox intersectionBox;

  try {
    intersectionBox = defineActiveRegion(sampleBB);
  } catch (const std::invalid_argument &) {
    // Exception means the beam missed the object and cannot create an intersection BoundingBox
    return nullptr;
  }

  // If the intersection volume is the same as the sample volume use the sample volume instead of creating a new shape
  // V3D operator== comparison is done with a 1.0e-6 tolerance
  if ((sampleBB.minPoint() == intersectionBox.minPoint()) && (sampleBB.maxPoint() == intersectionBox.maxPoint())) {
    return std::shared_ptr<Geometry::IObject>(sample.clone());
  }

  double yExtent = intersectionBox.yMax() - intersectionBox.yMin();
  double xExtent = intersectionBox.xMax() - intersectionBox.xMin();
  double zExtent = intersectionBox.zMax() - intersectionBox.zMin();

  std::string shapeXML = cuboidXML(xExtent, yExtent, zExtent, intersectionBox.centrePoint());
  return Geometry::ShapeFactory().createShape(shapeXML);
}

} // namespace Algorithms
} // namespace Mantid
