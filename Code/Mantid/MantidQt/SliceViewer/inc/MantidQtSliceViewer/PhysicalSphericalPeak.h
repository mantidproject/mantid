#ifndef MANTID_SLICEVIEWER_PHYSICALSPHERICALPEAK_H_
#define MANTID_SLICEVIEWER_PHYSICALSPHERICALPEAK_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidQtSliceViewer/PeakTransform.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class Spherical peak drawing primitive information.
    */
    struct SphericalPeakPrimitives 
    {
      double peakOuterRadiusX;
      double peakOuterRadiusY;
      double peakLineWidth;
      double peakOpacityAtDistance;
      Mantid::Kernel::V3D peakOrigin;
    };

    /**
    @class PhysicalSphericalPeak
    Represents the spacial and physical aspects of a spherical peak. Used to handle all physical interactions with other spacial objects.
    */
    class DLLExport PhysicalSphericalPeak 
    {
    public:
      /// Constructor
      PhysicalSphericalPeak(const Mantid::Kernel::V3D& origin, const double& radius);
      /// Destructor
      ~PhysicalSphericalPeak();
      /// Setter for the slice point.
      void setSlicePoint(const double&);
      /// Transform the coordinates.
      void movePosition(PeakTransform_sptr peakTransform);
      /// Draw
      SphericalPeakPrimitives draw(const double& windowHeight, const double& windowWidth, const double& viewWidth, const double& viewHeight) const;

      /**
      Determine whether the physical peak is visible. This means that the intesecting plane penetrates the sphere somehow. If the absolute
      distance between the plane and the origin is greater than the peak radius, then the peak is not visible.
      @return True if the peak is visible in the current configuration.
      */
      inline bool isViewable() const
      {
        return (m_cachedOpacityAtDistance != m_opacityMin);
      }

    private:
      /// Original origin x=h, y=k, z=l
      const Mantid::Kernel::V3D m_originalOrigin;
      /// Origin md-x, md-y, and md-z
      Mantid::Kernel::V3D m_origin;
      /// actual peak radius
      const double m_radius;
      /// Max opacity
      const double m_opacityMax;
      /// Min opacity
      const double m_opacityMin;
      /// Cached opacity at the distance z from origin
      double m_cachedOpacityAtDistance;
      /// Cached radius at the distance z from origin
      double m_cachedRadiusAtDistance;
      /// Cached opacity gradient.
      const double m_cachedOpacityGradient;
      /// Cached radius squared.
      const double m_cachedRadiusSQ;

      DISABLE_COPY_AND_ASSIGN(PhysicalSphericalPeak)
    };

  }
}

#endif /* MANTID_SLICEVIEWER_PHYSICALSPHERICALPEAK_H_ */