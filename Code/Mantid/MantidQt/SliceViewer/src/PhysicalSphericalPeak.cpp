#include "MantidQtSliceViewer/PhysicalSphericalPeak.h"

namespace MantidQt
{
  namespace SliceViewer
  {
      /**
      Constructor
      @param origin : peak origin (natural coordinates)
      @param radius : peak radius (natural coordinates)
      */
      PhysicalSphericalPeak::PhysicalSphericalPeak(const Mantid::Kernel::V3D& origin, const double& radius):
      m_originalOrigin(origin),
      m_origin(origin),
      m_radius(radius),
      m_opacityMax(0.8),
      m_opacityMin(0.0),
      m_cachedOpacityAtDistance(0.0),
      m_cachedRadiusAtDistance(0.0),
      m_cachedOpacityGradient((m_opacityMin - m_opacityMax)/m_radius),
      m_cachedRadiusSQ(m_radius*m_radius)
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
        
        if(distanceSQ <= m_cachedRadiusSQ)
        {
          const double distanceAbs = std::sqrt(distanceSQ);
          m_cachedRadiusAtDistance = std::sqrt( m_cachedRadiusSQ - distanceSQ );
          // Apply a linear transform to convert from a distance to an opacity between opacityMin and opacityMax.
          m_cachedOpacityAtDistance = m_cachedOpacityGradient * distanceAbs  + m_opacityMax;
        }
        else
        {
          m_cachedRadiusAtDistance = 0;
          m_cachedOpacityAtDistance = m_opacityMin;
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
        SphericalPeakPrimitives drawingObjects;
        if(this->isViewable())
        {
          // Scale factor for going from viewX to windowX
          const double scaleY = windowHeight/viewHeight;
          // Scale factor for going from viewY to windowY
          const double scaleX = windowWidth/viewWidth;

          const double innerRadiusX = scaleX * m_cachedRadiusAtDistance;
          const double innerRadiusY = scaleY * m_cachedRadiusAtDistance;

          double outerRadiusX = scaleX * m_radius;
          double outerRadiusY = scaleY * m_radius;

          const double lineWidthX = outerRadiusX - innerRadiusX;
          const double lineWidthY = outerRadiusY - innerRadiusY;
          outerRadiusX -= lineWidthX/2;
          outerRadiusY -= lineWidthY/2;

          // Create the return object.
          drawingObjects.peakOuterRadiusX = outerRadiusX;
          drawingObjects.peakOuterRadiusY = outerRadiusY;
          drawingObjects.peakLineWidth = lineWidthX;
          drawingObjects.peakOpacityAtDistance = m_cachedOpacityAtDistance;
          drawingObjects.peakOrigin = m_origin;
        }
        return drawingObjects;
      }
  }
}
