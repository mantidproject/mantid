#include "MantidQtSliceViewer/PhysicalCrossPeak.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    PhysicalCrossPeak::PhysicalCrossPeak(const Mantid::Kernel::V3D& origin, const double& maxZ, const double& minZ):
    m_originalOrigin(origin),
    m_origin(origin),
    m_effectiveRadius((maxZ - minZ)*0.015),
    m_opacityMax(0.8),
    m_opacityMin(0.0),
    m_crossViewFraction(0.015),
    m_opacityAtDistance(0.0)
  {
  }

  PhysicalCrossPeak::~PhysicalCrossPeak()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set the distance between the plane and the center of the peak in md coordinates

  @param z : position of the plane slice in the z dimension.
  */
  void PhysicalCrossPeak::setSlicePoint(const double& z)
  {
    const double distanceAbs = std::abs(z - m_origin.Z());

    // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
    m_opacityAtDistance = ((m_opacityMin - m_opacityMax)/m_effectiveRadius) * distanceAbs  + m_opacityMax;
    m_opacityAtDistance = m_opacityAtDistance >= m_opacityMin ? m_opacityAtDistance : m_opacityMin;
  }

  void PhysicalCrossPeak::movePosition(PeakTransform_sptr peakTransform)
  {
    m_origin = peakTransform->transform(m_originalOrigin);
  }

  MantidQt::SliceViewer::CrossPeakPrimitives PhysicalCrossPeak::draw(const double& windowHeight, const double& windowWidth) const
  {
    const int halfCrossHeight = int(windowHeight * m_crossViewFraction);
    const int halfCrossWidth = int(windowWidth * m_crossViewFraction);

    // Create the return object.
    CrossPeakPrimitives drawingObjects;
    drawingObjects.peakHalfCrossHeight = halfCrossHeight;
    drawingObjects.peakHalfCrossWidth = halfCrossWidth;
    drawingObjects.peakLineWidth = 2;
    drawingObjects.peakOpacityAtDistance = m_opacityAtDistance;
    drawingObjects.peakOrigin = m_origin;
    return drawingObjects;
  }
  }
}