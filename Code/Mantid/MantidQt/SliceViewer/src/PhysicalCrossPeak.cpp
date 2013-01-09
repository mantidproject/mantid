#include "MantidQtSliceViewer/PhysicalCrossPeak.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    Constructor
    @param peak origin (natural coordinates)
    @param z-axis max (natural coordinates)
    @param z-axis min (natural coordinates)
    */
    PhysicalCrossPeak::PhysicalCrossPeak(const Mantid::Kernel::V3D& origin, const double& maxZ, const double& minZ):
    m_originalOrigin(origin),
    m_origin(origin),
    m_effectiveRadius((maxZ - minZ)*0.015),
    m_opacityMax(0.8),
    m_opacityMin(0.0),
    m_opacityGradient((m_opacityMin - m_opacityMax)/m_effectiveRadius),
    m_crossViewFraction(0.015),
    m_opacityAtDistance(0.0)
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
    const double distanceAbs = std::abs(z - m_origin.Z());
    
    if(distanceAbs < m_effectiveRadius)
    {
      // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
      m_opacityAtDistance = (m_opacityGradient * distanceAbs)  + m_opacityMax;
    }
    else
    {
      m_opacityAtDistance =  m_opacityMin;
    }
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
  MantidQt::SliceViewer::CrossPeakPrimitives PhysicalCrossPeak::draw(const double& windowHeight, const double& windowWidth) const
  {
      CrossPeakPrimitives drawingObjects = {0,0,0,0.0,Mantid::Kernel::V3D()};
    if(isViewable())
    {
      const int halfCrossHeight = int(windowHeight * m_crossViewFraction);
      const int halfCrossWidth = int(windowWidth * m_crossViewFraction);

      // Create the return object.

      drawingObjects.peakHalfCrossHeight = halfCrossHeight;
      drawingObjects.peakHalfCrossWidth = halfCrossWidth;
      drawingObjects.peakLineWidth = 2;
      drawingObjects.peakOpacityAtDistance = m_opacityAtDistance;
      drawingObjects.peakOrigin = m_origin;
    }
    return drawingObjects;
  }

  /**
  @return bounding box for peak in natural coordinates.
  */
  RectangleType PhysicalCrossPeak::getBoundingBox() const
  {
    using Mantid::Kernel::V2D;
    V2D lowerLeft(m_origin.X() - m_effectiveRadius, m_origin.Y() - m_effectiveRadius);
    V2D upperRight(m_origin.X() + m_effectiveRadius, m_origin.Y() + m_effectiveRadius);
    return RectangleType(boost::make_tuple(lowerLeft, upperRight));
  }

  /**
  @param windowHeight : height of the window in px
  @param windowWidth : height of the window in px
  @param viewWidth : width of the view area in natural coodinates
  @param viewHeight : height of the view area in natural coordinates
  @return bounding box for peak in windows coordinates.
  */
  RectangleType PhysicalCrossPeak::getBoundingBox(const double& windowHeight, const double& windowWidth, const double& viewWidth, const double& viewHeight) const
  {
    using Mantid::Kernel::V2D;
    // Scale factor for going from viewX to windowX
    const double scaleYFactor = windowHeight/viewHeight;
    // Scale factor for going from viewY to windowY
    const double scaleXFactor = windowWidth/viewWidth;

    // Get the box in natural coordinates.
    RectangleType boxInNaturalCoords = this->getBoundingBox();

    const double expectedLowerLeftX = boxInNaturalCoords.get<0>().X() * scaleXFactor;
    const double expectedLowerLeftY = boxInNaturalCoords.get<0>().Y() * scaleYFactor;
    const double expectedUpperRightX = boxInNaturalCoords.get<1>().X() * scaleXFactor;
    const double expectedUpperRightY = boxInNaturalCoords.get<1>().Y() * scaleYFactor;

    // Apply scaling.
    return RectangleType(V2D(expectedLowerLeftX, expectedLowerLeftY), V2D(expectedUpperRightX, expectedUpperRightY));
  }

  }
}
