#include "MantidQtSliceViewer/PhysicalSphericalPeak.h"

namespace MantidQt
{
  namespace SliceViewer
  {
      /**
      Constructor
      @param origin : peak origin (natural coordinates)
      @param radius : peak radius (natural coordinates)
      @param backgroundRadius : peak background radius (natural coordinates)
      */
      PhysicalSphericalPeak::PhysicalSphericalPeak(const Mantid::Kernel::V3D& origin, const double& peakRadius, const double& backgroundInnerRadius, const double& backgroundOuterRadius):
      m_originalOrigin(origin),
      m_origin(origin),
      m_peakRadius(peakRadius),
      m_backgroundInnerRadius(backgroundInnerRadius),
      m_backgroundOuterRadius(backgroundOuterRadius),
      m_opacityMax(0.8),
      m_opacityMin(0.0),
      m_cachedOpacityAtDistance(0.0),
      m_peakRadiusAtDistance(peakRadius+1), // Initialize such that physical peak is not visible
      m_backgroundInnerRadiusAtDistance(backgroundInnerRadius+1), // Initialize such that physical peak is not visible
      m_backgroundOuterRadiusAtDistance(backgroundOuterRadius+1), // Initialize such that physical peak is not visible
      m_cachedOpacityGradient((m_opacityMin - m_opacityMax)/m_peakRadius),
      m_peakRadiusSQ(m_peakRadius*m_peakRadius),
      m_backgroundInnerRadiusSQ(backgroundInnerRadius*backgroundInnerRadius),
      m_backgroundOuterRadiusSQ(backgroundOuterRadius*backgroundOuterRadius),
      m_showBackgroundRadius(false)
      {
      }

      /// Destructor
      PhysicalSphericalPeak::~PhysicalSphericalPeak()
      {
      }

        //----------------------------------------------------------------------------------------------
        /** Set the distance between the plane and the center of the peak in md coordinates

        ASCII diagram below to demonstrate how dz (distance in z) is used to determine the radius of the sphere-plane intersection at that point,
        resloves both rx and ry. Also uses the distance to calculate the opacity to apply.

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
      void PhysicalSphericalPeak::setSlicePoint(const double& z)
      {
        const double distance = z - m_origin.Z();
        const double distanceSQ = distance * distance;
        
        if(distanceSQ <= m_backgroundOuterRadiusSQ)
        {
          const double distanceAbs = std::sqrt(distanceSQ);
          m_peakRadiusAtDistance = std::sqrt( m_peakRadiusSQ - distanceSQ );
          m_backgroundInnerRadiusAtDistance = std::sqrt(m_backgroundInnerRadiusSQ - distanceSQ );
          m_backgroundOuterRadiusAtDistance = std::sqrt(m_backgroundOuterRadiusSQ - distanceSQ );
          // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
          m_cachedOpacityAtDistance = m_cachedOpacityGradient * distanceAbs  + m_opacityMax;
        }
        else
        {
          m_cachedOpacityAtDistance = m_opacityMin;
          m_backgroundOuterRadiusAtDistance.reset();
        }
      }

      /**
      Move the peak origin according to the transform.
      @param peakTransform : transform to use.
      */
      void PhysicalSphericalPeak::movePosition(PeakTransform_sptr peakTransform)
      {
        m_origin = peakTransform->transform(m_originalOrigin);
      }

      /**
      Peform calculations allowing the peak to be drawn.
      @param windowHeight : height of the window in px
      @param windowWidth : height of the window in px
      @param viewWidth : width of the view area in natural coodinates
      @param viewHeight : height of the view area in natural coordinates
      @return SphericalPeakPrimites. Information object for rendering.
      */
      MantidQt::SliceViewer::SphericalPeakPrimitives PhysicalSphericalPeak::draw(const double& windowHeight, const double& windowWidth, const double& viewWidth, const double& viewHeight) const
      {
        SphericalPeakPrimitives drawingObjects = {};

        // Scale factor for going from viewX to windowX
        const double scaleY = windowHeight/viewHeight;
        // Scale factor for going from viewY to windowY
        const double scaleX = windowWidth/viewWidth;
        drawingObjects.peakOpacityAtDistance = m_cachedOpacityAtDistance;
        drawingObjects.peakOrigin = m_origin;

        if(this->isViewablePeak())
        {
          // Create the return object.
          drawingObjects.peakInnerRadiusX = scaleX * m_peakRadiusAtDistance;
          drawingObjects.peakInnerRadiusY = scaleY * m_peakRadiusAtDistance;
        }
        if(this->isViewableBackground())
        {
          drawingObjects.backgroundOuterRadiusX = scaleX * m_backgroundOuterRadiusAtDistance.get();
          drawingObjects.backgroundOuterRadiusY = scaleY * m_backgroundOuterRadiusAtDistance.get();
          drawingObjects.backgroundInnerRadiusX = scaleX * m_backgroundInnerRadiusAtDistance;
          drawingObjects.backgroundInnerRadiusY = scaleY * m_backgroundInnerRadiusAtDistance;
        }
        return drawingObjects;
      }

      void PhysicalSphericalPeak::showBackgroundRadius(const bool show)
      {
        m_showBackgroundRadius = show;
      }

      bool PhysicalSphericalPeak::showBackgroundRadius() const
      {
        return m_showBackgroundRadius;
      }
  }
}