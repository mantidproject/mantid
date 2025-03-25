// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/DetermineGaugeVolume.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

namespace Mantid::Geometry {

using Kernel::V3D;

std::string SLIT = "Slit";
std::string CIRCLE = "Circle";
std::string BEAM_WIDTH = "beam-width";
std::string BEAM_HEIGHT = "beam-height";
std::string BEAM_RADIUS = "beam-radius";
std::string BEAM_SHAPE = "beam-shape";

BeamProfile::BeamProfile(IComponent_const_sptr source, const V3D beamDirection) {
  if (source->hasParameter(BEAM_SHAPE)) {
    std::string beamShape = source->getParameterAsString(BEAM_SHAPE);
    if (beamShape == CIRCLE) {
      shape = CIRCLE;
      radius = std::strtod(source->getParameterAsString(BEAM_RADIUS).c_str(), nullptr);
    } else if (beamShape == SLIT) {
      shape = SLIT;
      height = std::strtod(source->getParameterAsString(BEAM_HEIGHT).c_str(), nullptr);
      width = std::strtod(source->getParameterAsString(BEAM_WIDTH).c_str(), nullptr);
    } else {
      throw std::runtime_error("BeamProfile: \"beam-shape\" is not one of (Slit, Circle)\n");
    }
  } else {
    BoundingBox *sourceBB = new BoundingBox();
    if (sourceBB->isNull()) {
      throw std::runtime_error("BeamProfile: No bounding box found for source");
    }
    source->getBoundingBox(*sourceBB);
    shape = SLIT;
    width = sourceBB->xMax() - sourceBB->xMin();
    height = sourceBB->yMax() - sourceBB->yMin();
  }
  direction = beamDirection;
  center = source->getPos();
}

std::string cuboidXML(double width, double height, double depth, const V3D &centrePos) {

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
  leftFrontBottom += centrePos;
  leftFrontTop += centrePos;
  leftBackBottom += centrePos;
  rightFrontBottom += centrePos;

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

namespace GaugeVolume {
std::shared_ptr<IObject> determineGaugeVolume(const IObject &sample, const BeamProfile &beamProfile) {
  BoundingBox sampleBB = sample.getBoundingBox();
  const V3D &beamCenter = beamProfile.center;
  const V3D &beamDir = beamProfile.direction;
  V3D xAxis(1, 0, 0);
  V3D yAxis(0, 1, 0);

  if (std::abs(beamDir.scalar_prod(xAxis)) == 1) {
    xAxis = V3D(0, 1, 0);
  }

  // Make yAxis perpendicular to both the beam direction and xAxis
  yAxis = beamDir.cross_prod(xAxis);
  xAxis = yAxis.cross_prod(beamDir);
  xAxis.normalize();
  yAxis.normalize();

  // Set up the coordinate system for re-alignment
  std::vector<V3D> coordSystem = {beamCenter, xAxis, yAxis, beamDir};
  sampleBB.realign(&coordSystem);

  double xMax, yMax, xMin, yMin;
  if (beamProfile.shape == SLIT) {
    xMax = std::min(sampleBB.xMax(), beamCenter.X() + beamProfile.width / 2);
    yMax = std::min(sampleBB.yMax(), beamCenter.Y() + beamProfile.height / 2);
    xMin = std::max(sampleBB.xMin(), beamCenter.X() - beamProfile.width / 2);
    yMin = std::max(sampleBB.yMin(), beamCenter.Y() - beamProfile.height / 2);
  } else if (beamProfile.shape == CIRCLE) {
    xMax = std::min(sampleBB.xMax(), beamCenter.X() + beamProfile.radius);
    yMax = std::min(sampleBB.yMax(), beamCenter.Y() + beamProfile.radius);
    xMin = std::max(sampleBB.xMin(), beamCenter.X() - beamProfile.radius);
    yMin = std::max(sampleBB.yMin(), beamCenter.Y() - beamProfile.radius);
  } else {
    // Unsupported beam shape
    return nullptr;
  }

  if (xMin >= xMax || yMin >= yMax) {
    // No intersection
    return nullptr;
  }

  BoundingBox intersectionBox(xMax, yMax, sampleBB.zMax(), xMin, yMin, sampleBB.zMin());

  double height = yMax - yMin;
  double width = xMax - xMin;
  double depth = sampleBB.zMax() - sampleBB.zMin();

  std::string shapeXML = cuboidXML(width, height, depth, intersectionBox.centrePoint());

  return ShapeFactory().createShape(shapeXML);
}
} // namespace GaugeVolume
} // namespace Mantid::Geometry
