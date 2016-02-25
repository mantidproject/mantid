#include "MantidKernel/V2D.h"
#include "MantidQtSliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include "MantidQtSliceViewer/PeakRepresentationEllipsoid.h"

#include <QPainter>

namespace
{

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
                                       double scaleY, QPainterPath &painterPath)
{
    // Rotate the ellipse (around the z axis/ default) by the angle in the
    // drawingInformation
    // Then translate it to originWindow values
    // Then scale it by the scale factors
    QTransform transform;
    transform.rotateRadians(angle);
    transform.translate(transX,
                        transY);
    transform.scale(scaleX, scaleY);

    return transform.map(painterPath);
}
}

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
    auto projections
        = Mantid::SliceViewer::getProjections(directions, backgroundOuterRadii);

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
            = std::vector<double>{ellipsoidInfoBackgroundOuter.radiusMajorAxis,
                                  ellipsoidInfoBackgroundOuter.radiusMinorAxis};
        m_originEllipseBackgroundOuter = ellipsoidInfoBackgroundOuter.origin;

        // Handle the peak radius
        if (Mantid::SliceViewer::checkIfCutExists(m_directions, m_peakRadii,
                                                  m_origin, z)) {

            auto ellipsoidInfoPeaks = m_calculator->getSlicePlaneInfo(
                m_directions, m_peakRadii, m_origin, z);
            m_radiiEllipse
                = std::vector<double>{ellipsoidInfoPeaks.radiusMajorAxis,
                                      ellipsoidInfoPeaks.radiusMinorAxis};
            m_originEllipse = ellipsoidInfoPeaks.origin;
        }

        // Handle the inner background radius
        if (Mantid::SliceViewer::checkIfCutExists(
                m_directions, m_backgroundInnerRadii, m_origin, z)) {

            auto ellipsoidInfoBackgroundInner = m_calculator->getSlicePlaneInfo(
                m_directions, m_backgroundInnerRadii, m_origin, z);
            m_radiiEllipseBackgroundInner = std::vector<double>{
                ellipsoidInfoBackgroundInner.radiusMajorAxis,
                ellipsoidInfoBackgroundInner.radiusMinorAxis};
            m_originEllipseBackgroundInner
                = ellipsoidInfoBackgroundInner.origin;
        }

        const auto distanceAbs = std::abs(z - m_origin.Z());
        m_cachedOpacityAtDistance = m_cachedOpacityGradient[2] * distanceAbs
                                    + m_opacityMax;
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
    Mantid::Geometry::PeakTransform_sptr peakTransform)
{
    m_origin = peakTransform->transform(m_originalOrigin);

    m_directions[0] = peakTransform->transform(m_originalDirections[0]);
    m_directions[1] = peakTransform->transform(m_originalDirections[1]);
    m_directions[2] = peakTransform->transform(m_originalDirections[2]);

    m_cachedOpacityGradient
        = peakTransform->transform(m_originalCachedOpacityGradient);
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
    using namespace Mantid::SliceViewer;
    return getPeakBoundingBoxForEllipse(m_originEllipseBackgroundOuter,
                                        m_radiiEllipseBackgroundOuter,
                                        m_angleEllipse);
}

double PeakRepresentationEllipsoid::getEffectiveRadius() const
{
    return m_showBackgroundRadii ? m_backgroundOuterRadii[0] : m_peakRadii[0];
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
    return 0.0;
}

double PeakRepresentationEllipsoid::getOccupancyIntoView() const
{
    // DO NOTHING
    return 0.0;
}

const Mantid::Kernel::V3D &PeakRepresentationEllipsoid::getOrigin() const
{
    return m_originEllipseBackgroundOuter;
}

std::shared_ptr<PeakPrimitives>
PeakRepresentationEllipsoid::getDrawingInformation(
    PeakRepresentationViewInformation viewInformation)
{
    auto drawingInformation = std::make_shared<PeakPrimitivesEllipse>(
        Mantid::Kernel::V3D() /*peakOrigin*/, 0.0 /*peakOpacityAtDistance*/,
        0 /* peakLineWidth */, 0.0 /*peakInnerRadiusMajorAxis*/,
        0.0 /*peakInnerRadiusMinorAxis*/,
        0.0 /*backgroundOuterRadiusMajorAxis*/,
        0.0 /*backgroundOuterRadiusMinorAxis*/,
        0.0 /*backgroundInnerRadiusMajorAxis*/,
        0.0 /*backgroundInnerRadiusMinorAxis*/, 0.0 /*angle*/);

    // Add peak radius
    drawingInformation->peakInnerRadiusMajorAxis = m_radiiEllipse[0];
    drawingInformation->peakInnerRadiusMinorAxis = m_radiiEllipse[1];
    drawingInformation->angle = m_angleEllipse;

    // If the outer radius is selected, then add the outer radius
    if (this->m_showBackgroundRadii) {
        drawingInformation->backgroundOuterRadiusMajorAxis
            = m_radiiEllipseBackgroundOuter[0];
        drawingInformation->backgroundOuterRadiusMinorAxis
            = m_radiiEllipseBackgroundOuter[1];

        drawingInformation->backgroundInnerRadiusMajorAxis
            = m_radiiEllipseBackgroundInner[0];
        drawingInformation->backgroundInnerRadiusMinorAxis
            = m_radiiEllipseBackgroundInner[1];
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
    PeakRepresentationViewInformation viewInformation)
{
    // Scale factor for going from viewX to windowX
    const auto scaleY = viewInformation.windowHeight
                        / viewInformation.viewHeight;
    // Scale factor for going from viewY to windowY
    const auto scaleX = viewInformation.windowWidth / viewInformation.viewWidth;

    auto drawingInformationEllipse
        = std::static_pointer_cast<PeakPrimitivesEllipse>(drawingInformation);

    // Setup the QPainter
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(drawingInformationEllipse->peakOpacityAtDistance);

    // Add a pen with color, style and stroke, and a painter path
    auto foregroundColorSphere = foregroundColor.colorSphere;

    const QPointF zeroPoint(0.0, 0.0);

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
    QPen pen(foregroundColorSphere);
    pen.setWidth(drawingInformationEllipse->peakLineWidth);
    pen.setStyle(Qt::DashLine);
    painter.strokePath(transformedPeakRadiusInnerPath, pen);

    if (m_showBackgroundRadii) {
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
        QPainterPath backgroundRadiusFill
            = transformedBackgroundOuterPath.subtracted(transformedBackgroundInnerPath);

        painter.fillPath(backgroundRadiusFill, backgroundColor.colorSphere);
    }
    painter.end();
}
}
}
