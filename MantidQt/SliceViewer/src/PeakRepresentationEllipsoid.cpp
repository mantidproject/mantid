#include "MantidQtSliceViewer/PeakRepresentationEllipsoid.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include "MantidQtSliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidKernel/V2D.h"

#include <QPainter>

namespace MantidQt
{
namespace SliceViewer
{
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
      m_opacityMin(0.0), m_cachedOpacityAtDistance(0.0),
      m_showBackgroundRadii(false), m_calculator(calculator)
{
  // Get projection lengths onto the xyz axes of the ellipsoid axes
  auto projections = Mantid::SliceViewer::getProjections(directions,
                                                         backgroundOuterRadii);

  const auto opacityRange = m_opacityMin - m_opacityMax;

  // Get the opacity gradient in all directions
  int index = 0;
  for (const auto& projection : projections) {
    const auto gradient = opacityRange/projection;
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
void PeakRepresentationEllipsoid::setSlicePoint(const double &z)
{

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
        m_radiiEllipseBackgroundOuter
            = std::vector<double>(ellipsoidInfoBackgroundOuter.radiusMajorAxis,
                                  ellipsoidInfoBackgroundOuter.radiusMinorAxis);
        m_originEllipseBackgroundOuter = ellipsoidInfoBackgroundOuter.origin;

        // Handle the peak radius
        if (Mantid::SliceViewer::checkIfCutExists(m_directions, m_peakRadii,
                                                  m_origin, z)) {

            auto ellipsoidInfoPeaks = m_calculator->getSlicePlaneInfo(
                m_directions, m_peakRadii, m_origin, z);
            m_radiiEllipse
                = std::vector<double>(ellipsoidInfoPeaks.radiusMajorAxis,
                                      ellipsoidInfoPeaks.radiusMinorAxis);
            m_originEllipse = ellipsoidInfoPeaks.origin;
        }

        // Handle the inner background radius
        if (Mantid::SliceViewer::checkIfCutExists(
                m_directions, m_backgroundInnerRadii, m_origin, z)) {

            auto ellipsoidInfoBackgroundInner = m_calculator->getSlicePlaneInfo(
                m_directions, m_backgroundInnerRadii, m_origin, z);
            m_radiiEllipseBackgroundInner = std::vector<double>(
                ellipsoidInfoBackgroundInner.radiusMajorAxis,
                ellipsoidInfoBackgroundInner.radiusMinorAxis);
            m_originEllipseBackgroundInner
                = ellipsoidInfoBackgroundInner.origin;
        }


         // TODO apply opacity gradient
        const auto distanceAbs = std::abs(z - m_originEllipseBackgroundOuter[3]);
        //m_cachedOpacityAtDistance = m_cachedOpacityGradient*distanceAbs + m_opacityMax;

#if 0


        // TODO apply opacity gradient

#endif
    } else {
        m_cachedOpacityAtDistance = m_opacityMin;
        // m_backgroundOuterRadiusAtDistance.reset();
    }
}

/**
 *Move the peak origin according to the transform. This affects
 * the origin but also the ellipsoid directions
 *@param peakTransform : transform to use.
 */
void PeakRepresentationEllipsoid::movePosition(
    Mantid::Geometry::PeakTransform_sptr peakTransform)
{
    m_origin = peakTransform->transform(m_originalOrigin);
    m_directions[0] = peakTransform->transform(m_originalDirections[0]);
    m_directions[1] = peakTransform->transform(m_originalDirections[1]);
    m_directions[2] = peakTransform->transform(m_originalDirections[2]);
}

/**
 * Setter for showing/hiding the background radius.
 * @param show: Flag indicating what to do.
*/
void PeakRepresentationEllipsoid::showBackgroundRadius(const bool show)
{
    m_showBackgroundRadii = show;
}

/**
 *@return bounding box for peak in natural coordinates.
 */
PeakBoundingBox PeakRepresentationEllipsoid::getBoundingBox() const
{
#if 0
    using Mantid::Kernel::V2D;
    Left left(m_origin.X() - m_backgroundOuterRadius);
    Bottom bottom(m_origin.Y() - m_backgroundOuterRadius);
    Right right(m_origin.X() + m_backgroundOuterRadius);
    Top top(m_origin.Y() + m_backgroundOuterRadius);
    SlicePoint slicePoint(m_origin.Z());

    return PeakBoundingBox(left, right, top, bottom, slicePoint);
#endif
    return PeakBoundingBox();
}

double PeakRepresentationEllipsoid::getEffectiveRadius() const
{
    // return m_showBackgroundRadius ? m_backgroundOuterRadius[0] :
    // m_peakRadius[0];
}

void PeakRepresentationEllipsoid::setOccupancyInView(const double)
{
    // DO NOTHING
}

void PeakRepresentationEllipsoid::setOccupancyIntoView(const double)
{
    // DO NOTHING
}

double PeakRepresentationEllipsoid::getOccupancyInView() const
{
    // DO NOTHING
}

double PeakRepresentationEllipsoid::getOccupancyIntoView() const
{
    // DO NOTHING
}

const Mantid::Kernel::V3D &PeakRepresentationEllipsoid::getOrigin() const
{
    return m_originEllipseBackgroundOuter;
}

std::shared_ptr<PeakPrimitives>
PeakRepresentationEllipsoid::getDrawingInformation(
    PeakRepresentationViewInformation viewInformation)
{
#if 0
    auto drawingInformation = std::make_shared<PeakPrimitivesSphere>(
        Mantid::Kernel::V3D() /*Peak Origin*/, 0.0 /*peakOpacityAtDistance*/,
        0 /* PeakLineWidth */, 0.0 /*peakInnerRadiusX*/,
        0.0 /*peakInnerRadiusY*/, 0.0 /*backgroundOuterRadiusX*/,
        0.0 /*backgroundOuterRadiusY*/, 0.0 /*backgroundInnerRadiusX*/,
        0.0 /*backgroundInnerRadiusY*/);

    // Scale factor for going from viewX to windowX
    const auto scaleY = viewInformation.windowHeight
                        / viewInformation.viewHeight;
    // Scale factor for going from viewY to windowY
    const auto scaleX = viewInformation.windowWidth / viewInformation.viewWidth;

    // Add the innder radius
    drawingInformation->peakInnerRadiusX = scaleX
                                           * m_peakRadiusAtDistance.get();
    drawingInformation->peakInnerRadiusY = scaleY
                                           * m_peakRadiusAtDistance.get();

    // If the outer radius is selected, then add the outer radius
    if (this->m_showBackgroundRadius) {
        drawingInformation->backgroundOuterRadiusX
            = scaleX * m_backgroundOuterRadiusAtDistance.get();
        drawingInformation->backgroundOuterRadiusY
            = scaleY * m_backgroundOuterRadiusAtDistance.get();
        drawingInformation->backgroundInnerRadiusX
            = scaleX * m_backgroundInnerRadiusAtDistance.get();
        drawingInformation->backgroundInnerRadiusY
            = scaleY * m_backgroundInnerRadiusAtDistance.get();
    }

    drawingInformation->peakLineWidth = 2;
    drawingInformation->peakOpacityAtDistance = m_cachedOpacityAtDistance;
    drawingInformation->peakOrigin = m_origin;

    return drawingInformation;
#endif
}

void PeakRepresentationEllipsoid::doDraw(
    QPainter &painter, PeakViewColor &foregroundColor,
    PeakViewColor &backgroundColor,
    std::shared_ptr<PeakPrimitives> drawingInformation,
    PeakRepresentationViewInformation viewInformation)
{
#if 0
    auto drawingInformationSphere
        = std::static_pointer_cast<PeakPrimitivesSphere>(drawingInformation);

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
        QPainterPath backgroundRadiusFill
            = backgroundOuterPath.subtracted(backgroundInnerPath);
        painter.fillPath(backgroundRadiusFill, backgroundColor.colorSphere);
    }
    painter.end();
#endif
}
}
}
