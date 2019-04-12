// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/PeakRepresentationSphere.h"
#include "MantidKernel/V2D.h"
#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"

#include <QPainter>

namespace MantidQt {
namespace SliceViewer {

PeakRepresentationSphere::PeakRepresentationSphere(
    const Mantid::Kernel::V3D &origin, const double &peakRadius,
    const double &backgroundInnerRadius, const double &backgroundOuterRadius)
    : m_originalOrigin(origin), m_origin(origin), m_peakRadius(peakRadius),
      m_backgroundInnerRadius(backgroundInnerRadius),
      m_backgroundOuterRadius(backgroundOuterRadius), m_opacityMax(0.8),
      m_opacityMin(0.0), m_cachedOpacityAtDistance(0.0),
      m_peakRadiusAtDistance(
          peakRadius + 1), // Initialize such that physical peak is not visible
      m_cachedOpacityGradient((m_opacityMin - m_opacityMax) / m_peakRadius),
      m_peakRadiusSQ(m_peakRadius * m_peakRadius),
      m_backgroundInnerRadiusSQ(backgroundInnerRadius * backgroundInnerRadius),
      m_backgroundOuterRadiusSQ(backgroundOuterRadius * backgroundOuterRadius),
      m_showBackgroundRadius(false) {
  // This possibility can arise from IntegratePeaksMD.
  if (m_backgroundOuterRadiusSQ <= m_backgroundInnerRadiusSQ) {
    m_backgroundOuterRadius = m_backgroundInnerRadius;
    m_backgroundOuterRadiusSQ = m_backgroundInnerRadiusSQ;
  }
}

//----------------------------------------------------------------------------------------------
/** Set the distance between the plane and the center of the peak in md
coordinates

ASCII diagram below to demonstrate how dz (distance in z) is used to determine
the radius of the sphere-plane intersection at that point,
resloves both rx and ry. Also uses the distance to calculate the opacity to
apply.

@param z : position of the plane slice in the z dimension.

     /---------\
    /           \
---/---------rx--\---------------- plane
   |    dz|     /| peak
   |      |   /  |
   |      . /    |
   |             |
   \             /
    \           /
     \---------/
*/
void PeakRepresentationSphere::setSlicePoint(const double &z) {
  const double distance = z - m_origin.Z();
  const double distanceSQ = distance * distance;

  if (distanceSQ <= m_backgroundOuterRadiusSQ) {
    const double distanceAbs = std::sqrt(distanceSQ);
    m_peakRadiusAtDistance = std::sqrt(m_peakRadiusSQ - distanceSQ);
    m_backgroundInnerRadiusAtDistance =
        std::sqrt(m_backgroundInnerRadiusSQ - distanceSQ);
    m_backgroundOuterRadiusAtDistance =
        std::sqrt(m_backgroundOuterRadiusSQ - distanceSQ);
    // Apply a linear transform to convert from a distance to an opacity
    // between opacityMin and opacityMax.
    m_cachedOpacityAtDistance =
        m_cachedOpacityGradient * distanceAbs + m_opacityMax;
  } else {
    m_cachedOpacityAtDistance = m_opacityMin;
    m_backgroundOuterRadiusAtDistance.reset();
  }
}

/**
 *Move the peak origin according to the transform.
 *@param peakTransform : transform to use.
 */
void PeakRepresentationSphere::movePosition(
    Mantid::Geometry::PeakTransform_sptr peakTransform) {
  m_origin = peakTransform->transform(m_originalOrigin);
}

/**
 * Setter for showing/hiding the background radius.
 * @param show: Flag indicating what to do.
 */
void PeakRepresentationSphere::showBackgroundRadius(const bool show) {
  m_showBackgroundRadius = show;
}

/**
 *@return bounding box for peak in natural coordinates.
 */
PeakBoundingBox PeakRepresentationSphere::getBoundingBox() const {
  using Mantid::Kernel::V2D;

  auto zoomOutFactor = getZoomOutFactor();

  Left left(m_origin.X() - zoomOutFactor * m_backgroundOuterRadius);
  Bottom bottom(m_origin.Y() - zoomOutFactor * m_backgroundOuterRadius);
  Right right(m_origin.X() + zoomOutFactor * m_backgroundOuterRadius);
  Top top(m_origin.Y() + zoomOutFactor * m_backgroundOuterRadius);
  SlicePoint slicePoint(m_origin.Z());

  return PeakBoundingBox(left, right, top, bottom, slicePoint);
}

double PeakRepresentationSphere::getEffectiveRadius() const {
  return m_showBackgroundRadius ? m_backgroundOuterRadius : m_peakRadius;
}

void PeakRepresentationSphere::setOccupancyInView(const double /*fraction*/) {
  // DO NOTHING
}

void PeakRepresentationSphere::setOccupancyIntoView(const double /*fraction*/) {
  // DO NOTHING
}

const Mantid::Kernel::V3D &PeakRepresentationSphere::getOrigin() const {
  return m_origin;
}

std::shared_ptr<PeakPrimitives> PeakRepresentationSphere::getDrawingInformation(
    PeakRepresentationViewInformation viewInformation) {
  auto drawingInformation = std::make_shared<PeakPrimitiveCircle>(
      Mantid::Kernel::V3D() /*Peak Origin*/, 0.0 /*peakOpacityAtDistance*/,
      0 /* PeakLineWidth */, 0.0 /*peakInnerRadiusX*/, 0.0 /*peakInnerRadiusY*/,
      0.0 /*backgroundOuterRadiusX*/, 0.0 /*backgroundOuterRadiusY*/,
      0.0 /*backgroundInnerRadiusX*/, 0.0 /*backgroundInnerRadiusY*/);

  // Scale factor for going from viewX to windowX
  const auto scaleY = viewInformation.windowHeight / viewInformation.viewHeight;
  // Scale factor for going from viewY to windowY
  const auto scaleX = viewInformation.windowWidth / viewInformation.viewWidth;

  // Add the innder radius
  drawingInformation->peakInnerRadiusX = scaleX * m_peakRadiusAtDistance.get();
  drawingInformation->peakInnerRadiusY = scaleY * m_peakRadiusAtDistance.get();

  // If the outer radius is selected, and we actually have an integrated
  // background radius, then add the outer radius
  if (this->m_showBackgroundRadius && m_backgroundInnerRadiusAtDistance &&
      m_backgroundOuterRadiusAtDistance) {
    drawingInformation->backgroundOuterRadiusX =
        scaleX * m_backgroundOuterRadiusAtDistance.get();
    drawingInformation->backgroundOuterRadiusY =
        scaleY * m_backgroundOuterRadiusAtDistance.get();
    drawingInformation->backgroundInnerRadiusX =
        scaleX * m_backgroundInnerRadiusAtDistance.get();
    drawingInformation->backgroundInnerRadiusY =
        scaleY * m_backgroundInnerRadiusAtDistance.get();
  }

  drawingInformation->peakLineWidth = 2;
  drawingInformation->peakOpacityAtDistance = m_cachedOpacityAtDistance;
  drawingInformation->peakOrigin = m_origin;

  return drawingInformation;
}

void PeakRepresentationSphere::doDraw(
    QPainter &painter, PeakViewColor &foregroundColor,
    PeakViewColor &backgroundColor,
    std::shared_ptr<PeakPrimitives> drawingInformation,
    PeakRepresentationViewInformation viewInformation) {
  auto drawingInformationSphere =
      std::static_pointer_cast<PeakPrimitiveCircle>(drawingInformation);

  // Setup the QPainter
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setOpacity(drawingInformationSphere->peakOpacityAtDistance);

  // Add a pen with color, style and stroke, and a painter path
  auto foregroundColorSphere = foregroundColor.colorSphere;
  QPainterPath peakRadiusInnerPath;
  const QPointF originWindows(viewInformation.xOriginWindow,
                              viewInformation.yOriginWindow);
  peakRadiusInnerPath.addEllipse(originWindows,
                                 drawingInformationSphere->peakInnerRadiusX,
                                 drawingInformationSphere->peakInnerRadiusY);

  QPen pen(foregroundColorSphere);
  pen.setWidth(drawingInformationSphere->peakLineWidth);
  pen.setStyle(Qt::DashLine);
  painter.strokePath(peakRadiusInnerPath, pen);

  // Draw the background if this is requested
  if (m_showBackgroundRadius) {
    QPainterPath backgroundOuterPath;
    backgroundOuterPath.setFillRule(Qt::WindingFill);
    backgroundOuterPath.addEllipse(
        originWindows, drawingInformationSphere->backgroundOuterRadiusX,
        drawingInformationSphere->backgroundOuterRadiusY);
    QPainterPath backgroundInnerPath;
    backgroundInnerPath.addEllipse(
        originWindows, drawingInformationSphere->backgroundInnerRadiusX,
        drawingInformationSphere->backgroundInnerRadiusY);
    QPainterPath backgroundRadiusFill =
        backgroundOuterPath.subtracted(backgroundInnerPath);
    painter.fillPath(backgroundRadiusFill, backgroundColor.colorSphere);
  }
  painter.end();
}

double PeakRepresentationSphere::getZoomOutFactor() const {
  return zoomOutFactor;
}
} // namespace SliceViewer
} // namespace MantidQt
