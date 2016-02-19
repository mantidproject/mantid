#include "MantidQtSliceViewer/PeakRepresentationEllipsoid.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include "MantidKernel/V2D.h"

#include <QPainter>

namespace MantidQt
{
namespace SliceViewer
{

  PeakRepresentationEllipsoid::PeakRepresentationEllipsoid(const Mantid::Kernel::V3D &origin,
                                                           const std::vector<double> peakRadii,
                                                           const std::vector<double> backgroundInnerRadii,
                                                           const std::vector<double> backgroundOuterRadii,
                                                           const std::vector<Mantid::Kernel::V3D> directions)
    : m_originalOrigin(origin),
      m_originalDirections(directions),
      m_origin(origin),
      m_directions(directions),
      m_peakRadii(peakRadii),
      m_backgroundInnerRadii(backgroundInnerRadii),
      m_backgroundOuterRadii(backgroundOuterRadii),
      m_opacityMax(0.8),
      m_opacityMin(0.0),
      m_cachedOpacityAtDistance(0.0),
      m_showBackgroundRadii(false)
{



#if 0
    // This possibility can arise from IntegratePeaksMD.
    if (m_backgroundOuterRadiusSQ <= m_backgroundInnerRadiusSQ) {
        m_backgroundOuterRadius = m_backgroundInnerRadius;
        m_backgroundOuterRadiusSQ = m_backgroundInnerRadiusSQ;
    }
#endif
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
void PeakRepresentationEllipsoid::setSlicePoint(const double &z)
{
  const double distance = z - m_origin.Z();
  const double distanceSQ = distance*distance;

  // Need to find a good criterion when the
















#if 0
    const double distance = z - m_origin.Z();
    const double distanceSQ = distance * distance;

    if (distanceSQ <= m_backgroundOuterRadiusSQ) {
        const double distanceAbs = std::sqrt(distanceSQ);
        m_peakRadiusAtDistance = std::sqrt(m_peakRadiusSQ - distanceSQ);
        m_backgroundInnerRadiusAtDistance
            = std::sqrt(m_backgroundInnerRadiusSQ - distanceSQ);
        m_backgroundOuterRadiusAtDistance
            = std::sqrt(m_backgroundOuterRadiusSQ - distanceSQ);
        // Apply a linear transform to convert from a distance to an opacity
        // between opacityMin and opacityMax.
        m_cachedOpacityAtDistance = m_cachedOpacityGradient * distanceAbs
                                    + m_opacityMax;
    } else {
        m_cachedOpacityAtDistance = m_opacityMin;
        m_backgroundOuterRadiusAtDistance.reset();
    }
#endif
}

/**
 *Move the peak origin according to the transform.
 *@param peakTransform : transform to use.
 */
void PeakRepresentationEllipsoid::movePosition(
    Mantid::Geometry::PeakTransform_sptr peakTransform)
{
  // TODO need to roate the directions as well
#if 0
    m_origin = peakTransform->transform(m_originalOrigin);
#endif
}

/**
 * Setter for showing/hiding the background radius.
 * @param show: Flag indicating what to do.
*/
void PeakRepresentationEllipsoid::showBackgroundRadius(const bool show)
{
    m_showBackgroundRadii= show;
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
#if 0
    return m_showBackgroundRadius ? m_backgroundOuterRadius : m_peakRadius;
#endif
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
    return m_origin;
}

std::shared_ptr<PeakPrimitives> PeakRepresentationEllipsoid::getDrawingInformation(
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
