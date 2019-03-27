// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/PeakRepresentationEllipsoid.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/SliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"

namespace {
Mantid::Kernel::Logger g_log("PeakRepresentation");
}

#include <QPainter>

namespace {

/**
 * Handles the roation, translation and scaling of an ellipse in Qt
 * @param angle: the angle in radian by which to rotate
 * @param transX: x position
 * @param transY: y position
 * @param scaleX: x scale factor
 * @param scaleY: y scale factor
 * @param painterPath: the origin painter path
 * @return a transformed painter path
 */
QPainterPath getTransformedPainterPath(double angle, double transX,
                                       double transY, double scaleX,
                                       double scaleY,
                                       QPainterPath &painterPath) {
  // The ellipse which is passed in has its major axis along the x axis and
  // the minor axis
  // along the y axix. The origin is located at (0,0).
  // In principal we need to:
  // 1. Rotate the ellipse in radians
  // 2. Scale the ellipse
  // 3. Translate the ellipse
  // QTransform needs to be specified in reverse order!!!
  QTransform transform;
  transform.translate(transX, transY);
  transform.scale(scaleX, scaleY);

  // Important Note for the rotation: The rotation in Qt is done clock-wise,
  // since
  // the y axis is pointing downwards. The workspace plot does not reflect this
  // though
  // Hence we should be applying a counter-clockwise rotation which is achieved
  // by
  // prefixing with a minus sign.
  transform.rotateRadians(-angle);

  return transform.map(painterPath);
}
} // namespace

namespace MantidQt {
namespace SliceViewer {

const double PeakRepresentationEllipsoid::zeroRadius = 0.0;

PeakRepresentationEllipsoid::PeakRepresentationEllipsoid(
    const Mantid::Kernel::V3D &origin, const std::vector<double> peakRadii,
    const std::vector<double> backgroundInnerRadii,
    const std::vector<double> backgroundOuterRadii,
    const std::vector<Mantid::Kernel::V3D> directions,
    std::shared_ptr<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>
        calculator)
    : m_originalOrigin(origin), m_originalDirections(directions),
      m_origin(origin), m_directions(directions), m_peakRadii(peakRadii),
      m_backgroundInnerRadii(backgroundInnerRadii),
      m_backgroundOuterRadii(backgroundOuterRadii), m_opacityMax(0.8),
      m_opacityMin(0.0), m_cachedOpacityAtDistance(0.0), m_angleEllipse(-1.0),
      m_showBackgroundRadii(false), m_calculator(calculator) {
  // Get projection lengths onto the xyz axes of the ellipsoid axes
  auto projections = Mantid::SliceViewer::getProjectionLengths(
      directions, backgroundOuterRadii);

  const auto opacityRange = m_opacityMin - m_opacityMax;

  // Get the opacity gradient in all directions
  int index = 0;
  for (const auto &projection : projections) {
    const auto gradient = opacityRange / projection;
    m_originalCachedOpacityGradient[index] = gradient;
    m_cachedOpacityGradient[index] = gradient;
    ++index;
  }
}

//----------------------------------------------------------------------------------------------
/** Set the distance between the plane and the center of the peak in md
coordinates.
@param z : position of the plane slice in the z dimension.
*/
void PeakRepresentationEllipsoid::setSlicePoint(const double &z) {

  // We check first the outer background. If there is no cut, then,
  // there should be nothing to left to do. Otherewise we do the inner
  // background and the peaks separately.
  if (Mantid::SliceViewer::checkIfCutExists(
          m_directions, m_backgroundOuterRadii, m_origin, z)) {

    // Handle the case of the outer background
    auto ellipsoidInfoBackgroundOuter = m_calculator->getSlicePlaneInfo(
        m_directions, m_backgroundOuterRadii, m_origin, z);

    // The angle should be the same for all
    m_angleEllipse = ellipsoidInfoBackgroundOuter.angle;
    m_radiiEllipseBackgroundOuter =
        std::vector<double>{ellipsoidInfoBackgroundOuter.radiusMajorAxis,
                            ellipsoidInfoBackgroundOuter.radiusMinorAxis};
    m_originEllipseBackgroundOuter = ellipsoidInfoBackgroundOuter.origin;

    // Handle the peak radius
    if (Mantid::SliceViewer::checkIfCutExists(m_directions, m_peakRadii,
                                              m_origin, z)) {

      auto ellipsoidInfoPeaks = m_calculator->getSlicePlaneInfo(
          m_directions, m_peakRadii, m_origin, z);
      m_radiiEllipse = std::vector<double>{ellipsoidInfoPeaks.radiusMajorAxis,
                                           ellipsoidInfoPeaks.radiusMinorAxis};
      m_originEllipse = ellipsoidInfoPeaks.origin;
    }

    // Handle the inner background radius
    if (Mantid::SliceViewer::checkIfCutExists(
            m_directions, m_backgroundInnerRadii, m_origin, z)) {

      auto ellipsoidInfoBackgroundInner = m_calculator->getSlicePlaneInfo(
          m_directions, m_backgroundInnerRadii, m_origin, z);
      m_radiiEllipseBackgroundInner =
          std::vector<double>{ellipsoidInfoBackgroundInner.radiusMajorAxis,
                              ellipsoidInfoBackgroundInner.radiusMinorAxis};
      m_originEllipseBackgroundInner = ellipsoidInfoBackgroundInner.origin;
    }

    const auto distanceAbs = std::abs(z - m_origin.Z());
    m_cachedOpacityAtDistance =
        m_cachedOpacityGradient[2] * distanceAbs + m_opacityMax;
  } else {
    m_cachedOpacityAtDistance = m_opacityMin;
    m_radiiEllipse.clear();
    m_radiiEllipseBackgroundInner.clear();
    m_radiiEllipseBackgroundOuter.clear();
  }
}

/**
 *Move the peak origin according to the transform. This affects
 * the origin but also the ellipsoid directions and the opacity gradient
 *@param peakTransform : transform to use.
 */
void PeakRepresentationEllipsoid::movePosition(
    Mantid::Geometry::PeakTransform_sptr peakTransform) {
  m_origin = peakTransform->transform(m_originalOrigin);

  m_directions[0] = peakTransform->transform(m_originalDirections[0]);
  m_directions[1] = peakTransform->transform(m_originalDirections[1]);
  m_directions[2] = peakTransform->transform(m_originalDirections[2]);

  m_cachedOpacityGradient =
      peakTransform->transform(m_originalCachedOpacityGradient);
}

/**
 * Setter for showing/hiding the background radius.
 * @param show: Flag indicating what to do.
 */
void PeakRepresentationEllipsoid::showBackgroundRadius(const bool show) {
  m_showBackgroundRadii = show;
}

/**
 *@return bounding box for peak in natural coordinates.
 */
PeakBoundingBox PeakRepresentationEllipsoid::getBoundingBox() const {
  using namespace Mantid::SliceViewer;
  return getPeakBoundingBoxForEllipsoid(m_directions, m_backgroundOuterRadii,
                                        m_origin);
}

double PeakRepresentationEllipsoid::getEffectiveRadius() const {
  return m_showBackgroundRadii ? m_backgroundOuterRadii[0] : m_peakRadii[0];
}

void PeakRepresentationEllipsoid::setOccupancyInView(const double /*fraction*/) {
  // DO NOTHING
}

void PeakRepresentationEllipsoid::setOccupancyIntoView(const double /*fraction*/) {
  // DO NOTHING
}

const Mantid::Kernel::V3D &PeakRepresentationEllipsoid::getOrigin() const {
  return m_originEllipseBackgroundOuter;
}

std::shared_ptr<PeakPrimitives>
PeakRepresentationEllipsoid::getDrawingInformation(
    PeakRepresentationViewInformation /*viewInformation*/) {
  auto drawingInformation = std::make_shared<PeakPrimitivesEllipse>(
      Mantid::Kernel::V3D() /*peakOrigin*/, 0.0 /*peakOpacityAtDistance*/,
      0 /* peakLineWidth */, 0.0 /*peakInnerRadiusMajorAxis*/,
      0.0 /*peakInnerRadiusMinorAxis*/, 0.0 /*backgroundOuterRadiusMajorAxis*/,
      0.0 /*backgroundOuterRadiusMinorAxis*/,
      0.0 /*backgroundInnerRadiusMajorAxis*/,
      0.0 /*backgroundInnerRadiusMinorAxis*/, 0.0 /*angle*/);

  // Add peak radius if it is available.
  if (m_radiiEllipse.size() > 0) {
    drawingInformation->peakInnerRadiusMajorAxis = m_radiiEllipse[0];
    drawingInformation->peakInnerRadiusMinorAxis = m_radiiEllipse[1];
  } else {
    drawingInformation->peakInnerRadiusMajorAxis = zeroRadius;
    drawingInformation->peakInnerRadiusMinorAxis = zeroRadius;
  }

  drawingInformation->angle = m_angleEllipse;

  // If the outer radius is selected, then add the outer radius
  if (this->m_showBackgroundRadii) {
    if (m_radiiEllipseBackgroundOuter.size() > 0) {
      drawingInformation->backgroundOuterRadiusMajorAxis =
          m_radiiEllipseBackgroundOuter[0];
      drawingInformation->backgroundOuterRadiusMinorAxis =
          m_radiiEllipseBackgroundOuter[1];
    } else {
      drawingInformation->backgroundOuterRadiusMajorAxis = zeroRadius;
      drawingInformation->backgroundOuterRadiusMinorAxis = zeroRadius;
    }

    // Add inner backgroudn radius if it is available. It might be that only a
    // cut through the outer
    // background radius exists
    if (m_radiiEllipseBackgroundInner.size() > 0) {
      drawingInformation->backgroundInnerRadiusMajorAxis =
          m_radiiEllipseBackgroundInner[0];
      drawingInformation->backgroundInnerRadiusMinorAxis =
          m_radiiEllipseBackgroundInner[1];
    } else {
      drawingInformation->backgroundInnerRadiusMajorAxis = zeroRadius;
      drawingInformation->backgroundInnerRadiusMinorAxis = zeroRadius;
    }
  }

  drawingInformation->peakLineWidth = 2;
  drawingInformation->peakOpacityAtDistance = m_cachedOpacityAtDistance;
  drawingInformation->peakOrigin = m_originEllipseBackgroundOuter;

  return drawingInformation;
}

void PeakRepresentationEllipsoid::doDraw(
    QPainter &painter, PeakViewColor &foregroundColor,
    PeakViewColor &backgroundColor,
    std::shared_ptr<PeakPrimitives> drawingInformation,
    PeakRepresentationViewInformation viewInformation) {
  // Scale factor for going from viewX to windowX
  const auto scaleY = viewInformation.windowHeight / viewInformation.viewHeight;
  // Scale factor for going from viewY to windowY
  const auto scaleX = viewInformation.windowWidth / viewInformation.viewWidth;

  auto drawingInformationEllipse =
      std::static_pointer_cast<PeakPrimitivesEllipse>(drawingInformation);

  // Setup the QPainter
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setOpacity(drawingInformationEllipse->peakOpacityAtDistance);

  // Add a pen with color, style and stroke, and a painter path
  auto foregroundColorEllipsoid = foregroundColor.colorEllipsoid;

  const QPointF zeroPoint(0.0, 0.0);

  // Only draw the ellipsoid if the axis are larger than 0
  if (drawingInformationEllipse->peakInnerRadiusMajorAxis != zeroRadius &&
      drawingInformationEllipse->peakInnerRadiusMinorAxis != zeroRadius) {
    // Add the ellipse at the origin (in order to rotate)
    QPainterPath peakRadiusInnerPath;

    peakRadiusInnerPath.addEllipse(
        zeroPoint, drawingInformationEllipse->peakInnerRadiusMajorAxis,
        drawingInformationEllipse->peakInnerRadiusMinorAxis);

    // Transform the painter path (rotate, translate, scale)
    auto transformedPeakRadiusInnerPath = getTransformedPainterPath(
        drawingInformationEllipse->angle, viewInformation.xOriginWindow,
        viewInformation.yOriginWindow, scaleX, scaleY, peakRadiusInnerPath);

    // Add the pen which draws the ellipse
    QPen pen(foregroundColorEllipsoid);
    pen.setWidth(drawingInformationEllipse->peakLineWidth);
    pen.setStyle(Qt::DashLine);

    painter.strokePath(transformedPeakRadiusInnerPath, pen);
  }

  if (m_showBackgroundRadii) {
    if (drawingInformationEllipse->backgroundOuterRadiusMajorAxis !=
            zeroRadius &&
        drawingInformationEllipse->backgroundOuterRadiusMinorAxis !=
            zeroRadius &&
        drawingInformationEllipse->backgroundInnerRadiusMajorAxis !=
            zeroRadius &&
        drawingInformationEllipse->backgroundInnerRadiusMinorAxis !=
            zeroRadius) {
      // Outer demarcation of the fill
      QPainterPath backgroundOuterPath;
      backgroundOuterPath.setFillRule(Qt::WindingFill);
      backgroundOuterPath.addEllipse(
          zeroPoint, drawingInformationEllipse->backgroundOuterRadiusMajorAxis,
          drawingInformationEllipse->backgroundOuterRadiusMinorAxis);
      auto transformedBackgroundOuterPath = getTransformedPainterPath(
          drawingInformationEllipse->angle, viewInformation.xOriginWindow,
          viewInformation.yOriginWindow, scaleX, scaleY, backgroundOuterPath);

      // Inner demarcation of the fill
      QPainterPath backgroundInnerPath;
      backgroundInnerPath.addEllipse(
          zeroPoint, drawingInformationEllipse->backgroundInnerRadiusMajorAxis,
          drawingInformationEllipse->backgroundInnerRadiusMinorAxis);
      auto transformedBackgroundInnerPath = getTransformedPainterPath(
          drawingInformationEllipse->angle, viewInformation.xOriginWindow,
          viewInformation.yOriginWindow, scaleX, scaleY, backgroundInnerPath);

      // Subtract inner fill from outer fill
      QPainterPath backgroundRadiusFill =
          transformedBackgroundOuterPath.subtracted(
              transformedBackgroundInnerPath);

      painter.fillPath(backgroundRadiusFill, backgroundColor.colorEllipsoid);
    }
  }
  painter.end();
}

double PeakRepresentationEllipsoid::getZoomOutFactor() const {
  return m_calculator->getZoomOutFactor();
}
} // namespace SliceViewer
} // namespace MantidQt
