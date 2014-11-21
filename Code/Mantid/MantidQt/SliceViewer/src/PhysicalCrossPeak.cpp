#include "MantidQtSliceViewer/PhysicalCrossPeak.h"

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    Constructor
    @param origin: peak origin (natural coordinates)
    @param maxZ: z-axis max (natural coordinates)
    @param minZ: z-axis min (natural coordinates)
    */
    PhysicalCrossPeak::PhysicalCrossPeak(const Mantid::Kernel::V3D& origin, const double& maxZ, const double& minZ):
    m_originalOrigin(origin),
    m_origin(origin),
    m_intoViewFraction(0.015),
    m_effectiveRadius((maxZ - minZ)*m_intoViewFraction),
    m_opacityMax(0.8),
    m_opacityMin(0.0),
    m_opacityGradient((m_opacityMin - m_opacityMax)/m_effectiveRadius),
    m_crossViewFraction(0.015),
    m_opacityAtDistance(0.0),
    m_slicePoint(0.0)
  {
  }

  /// Destructor
  PhysicalCrossPeak::~PhysicalCrossPeak()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set the distance between the plane and the center of the peak in md coordinates

  @param z : position of the plane slice in the z dimension.
  */
  void PhysicalCrossPeak::setSlicePoint(const double& z)
  {
    m_slicePoint = z;
    const double distanceAbs = std::abs(z - m_origin.Z());
    
    // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
    m_opacityAtDistance = (m_opacityGradient * distanceAbs)  + m_opacityMax;
  }

  /**
  Move the peak position according the the transform.
  @param peakTransform : Tranform to use.
  */
  void PhysicalCrossPeak::movePosition(PeakTransform_sptr peakTransform)
  {
    m_origin = peakTransform->transform(m_originalOrigin);
  }

  /**
  Peform calculations to draw the peak.
  @param windowHeight : Height of the window (pixels)
  @param windowWidth : Width of the window (pixels)
  @return structure of primitive information for drawing.
  */
    MantidQt::SliceViewer::CrossPeakPrimitives PhysicalCrossPeak::draw(const double& windowHeight,
        const double& windowWidth) const
    {
      CrossPeakPrimitives drawingObjects =
      { 0, 0, 0, 0.0, Mantid::Kernel::V3D() };

      const int halfCrossHeight = int(windowHeight * m_crossViewFraction);
      const int halfCrossWidth = int(windowWidth * m_crossViewFraction);

      // Create the return object.

      drawingObjects.peakHalfCrossHeight = halfCrossHeight;
      drawingObjects.peakHalfCrossWidth = halfCrossWidth;
      drawingObjects.peakLineWidth = 2;
      drawingObjects.peakOpacityAtDistance = m_opacityAtDistance;
      drawingObjects.peakOrigin = m_origin;

      return drawingObjects;
    }

  /**
  @return bounding box for peak in natural coordinates.
  */
  PeakBoundingBox PhysicalCrossPeak::getBoundingBox() const
  {
    using Mantid::Kernel::V2D;
    const Left left( m_origin.X() - m_effectiveRadius );
    const Right right( m_origin.X() + m_effectiveRadius );
    const Bottom bottom ( m_origin.Y() - m_effectiveRadius );
    const Top top ( m_origin.Y() + m_effectiveRadius );
    const SlicePoint slicePoint( m_origin.Z() );

    return PeakBoundingBox(left, right, top, bottom, slicePoint);
  }


  void PhysicalCrossPeak::setOccupancyInView(const double fraction)
  {
    m_crossViewFraction = fraction;
    setSlicePoint(m_slicePoint);
  }

    void PhysicalCrossPeak::setOccupancyIntoView(const double fraction)
    {
      if (fraction != 0)
      {
        m_effectiveRadius *= (fraction / m_intoViewFraction);
        m_intoViewFraction = fraction;
        setSlicePoint(m_slicePoint);
      }
    }

    /**
     * @return The effective peak radius.
     */
    double PhysicalCrossPeak::getEffectiveRadius() const
    {
      return m_effectiveRadius;
    }

    double PhysicalCrossPeak::getOccupancyInView() const
    {
      return m_crossViewFraction;
    }

    double PhysicalCrossPeak::getOccupancyIntoView() const
    {
      return m_intoViewFraction;
    }

  }
}
