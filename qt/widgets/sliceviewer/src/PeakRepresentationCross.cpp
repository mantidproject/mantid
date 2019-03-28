// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/PeakRepresentationCross.h"
#include "MantidKernel/V2D.h"
#include "MantidQtWidgets/Common/NonOrthogonal.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"
#include "MantidQtWidgets/SliceViewer/PeakPrimitives.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"
#include <QPainter>

namespace MantidQt {
namespace SliceViewer {

PeakRepresentationCross::PeakRepresentationCross(
    const Mantid::Kernel::V3D &origin, const double &maxZ, const double &minZ)
    : m_intoViewFraction(0.015), m_crossViewFraction(0.015),
      m_originalOrigin(origin), m_origin(origin),
      m_effectiveRadius((maxZ - minZ) * m_intoViewFraction), m_opacityMax(0.8),
      m_opacityMin(0.0),
      m_opacityGradient((m_opacityMin - m_opacityMax) / m_effectiveRadius),
      m_opacityAtDistance(0.0), m_slicePoint(0.0) {}

/**
 *Set the distance between the plane and the center of the peak in md
 *coordinates
 *@param z : position of the plane slice in the z dimension.
 */
void PeakRepresentationCross::setSlicePoint(const double &z) {
  m_slicePoint = z;
  const double distanceAbs = std::abs(z - m_origin.Z());

  // Apply a linear transform to convert from a distance to an opacity between
  // opacityMin and opacityMax.
  m_opacityAtDistance = (m_opacityGradient * distanceAbs) + m_opacityMax;
}

/**
 * Move the peak position according the the transform.
 * @param peakTransform : Tranform to use.
 */
void PeakRepresentationCross::movePosition(
    Mantid::Geometry::PeakTransform_sptr peakTransform) {
  m_origin = peakTransform->transform(m_originalOrigin);
}

void PeakRepresentationCross::movePositionNonOrthogonal(
    Mantid::Geometry::PeakTransform_sptr peakTransform,
    NonOrthogonalAxis &info) {

  m_origin = m_originalOrigin; // reset to original peak point, then transform
                               // to skewed original peak point, then use
                               // peakTransform to take into account current
                               // dimensions
  API::transformLookpointToWorkspaceCoord(
      m_origin, info.fromHklToXyz, info.dimX, info.dimY, info.dimMissing);
  m_origin = peakTransform->transform(m_origin);
}
/**
 *
 *@return bounding box for peak in natural coordinates.
 */
PeakBoundingBox PeakRepresentationCross::getBoundingBox() const {
  using Mantid::Kernel::V2D;
  const Left left(m_origin.X() - m_effectiveRadius);
  const Right right(m_origin.X() + m_effectiveRadius);
  const Bottom bottom(m_origin.Y() - m_effectiveRadius);
  const Top top(m_origin.Y() + m_effectiveRadius);
  const SlicePoint slicePoint(m_origin.Z());

  return PeakBoundingBox(left, right, top, bottom, slicePoint);
}

void PeakRepresentationCross::setOccupancyInView(const double fraction) {
  m_crossViewFraction = fraction;
  setSlicePoint(m_slicePoint);
}

void PeakRepresentationCross::setOccupancyIntoView(const double fraction) {
  if (fraction != 0) {
    m_effectiveRadius *= (fraction / m_intoViewFraction);
    m_intoViewFraction = fraction;
    setSlicePoint(m_slicePoint);
  }
}

/**
 * Gets the effective peak radius of the cross representation
 * @return the effective radis
 */
double PeakRepresentationCross::getEffectiveRadius() const {
  return m_effectiveRadius;
}

std::shared_ptr<PeakPrimitives> PeakRepresentationCross::getDrawingInformation(
    PeakRepresentationViewInformation viewInformation) {
  auto drawingInformation = std::make_shared<PeakPrimitivesCross>(
      Mantid::Kernel::V3D() /*Peak Origin*/, 0.0 /*peakOpacityAtDistance*/,
      0 /*peakHalfCrossWidth*/, 0 /*peakHalfCrossHeight*/, 0 /*peakLineWidth*/);

  const auto halfCrossHeight =
      static_cast<int>(viewInformation.windowHeight * m_crossViewFraction);
  const auto halfCrossWidth =
      static_cast<int>(viewInformation.windowWidth * m_crossViewFraction);

  drawingInformation->peakHalfCrossHeight = halfCrossHeight;
  drawingInformation->peakHalfCrossWidth = halfCrossWidth;
  drawingInformation->peakLineWidth = 2;
  drawingInformation->peakOpacityAtDistance = m_opacityAtDistance;
  drawingInformation->peakOrigin = m_origin;

  return drawingInformation;
}

void PeakRepresentationCross::doDraw(
    QPainter &painter, PeakViewColor &foregroundColor,
    PeakViewColor & /*backgroundColor*/,
    std::shared_ptr<PeakPrimitives> drawingInformation,
    PeakRepresentationViewInformation viewInformation) {
  auto drawingInformationCross =
      std::static_pointer_cast<PeakPrimitivesCross>(drawingInformation);

  // Setup the QPainter
  painter.setRenderHint(QPainter::Antialiasing);

  // Add a pen with color, style and stroke
  auto foregroundColorCross = foregroundColor.colorCross;

  QPen pen(foregroundColorCross);
  pen.setWidth(drawingInformationCross->peakLineWidth);
  pen.setStyle(Qt::SolidLine);
  painter.setPen(pen);

  painter.setOpacity(drawingInformationCross->peakOpacityAtDistance);

  // Creat the actual lines and have them drawn by the painter
  const int halfCrossHeight = drawingInformationCross->peakHalfCrossHeight;
  const int halfCrossWidth = drawingInformationCross->peakHalfCrossWidth;

  const auto xOriginWindow = viewInformation.xOriginWindow;
  const auto yOriginWindow = viewInformation.yOriginWindow;

  QPoint bottomL(xOriginWindow - halfCrossWidth,
                 yOriginWindow - halfCrossHeight);
  QPoint bottomR(xOriginWindow + halfCrossWidth,
                 yOriginWindow - halfCrossHeight);
  QPoint topL(xOriginWindow - halfCrossWidth, yOriginWindow + halfCrossHeight);
  QPoint topR(xOriginWindow + halfCrossWidth, yOriginWindow + halfCrossHeight);

  painter.drawLine(bottomL, topR);
  painter.drawLine(bottomR, topL);
  painter.end();
}

const Mantid::Kernel::V3D &PeakRepresentationCross::getOrigin() const {
  return m_origin;
}

void PeakRepresentationCross::showBackgroundRadius(const bool /*show*/) {
  // Do nothing
}
} // namespace SliceViewer
} // namespace MantidQt
