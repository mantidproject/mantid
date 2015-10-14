#ifndef MANTID_SLICEVIEWER_PHYSICALSPHERICALPEAK_H_
#define MANTID_SLICEVIEWER_PHYSICALSPHERICALPEAK_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidQtSliceViewer/PeakOverlayView.h"
#include <boost/optional.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {
    /// Alisas for a boost optional double.
    typedef boost::optional<double> optional_double;

    /**
    SphericalPeakPrimitives. Drawing information type.
    */
    struct SphericalPeakPrimitives 
    {
      double peakInnerRadiusX;
      double peakInnerRadiusY;
      double backgroundOuterRadiusX;
      double backgroundOuterRadiusY;
      double backgroundInnerRadiusX;
      double backgroundInnerRadiusY;
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
      PhysicalSphericalPeak(const Mantid::Kernel::V3D& origin, const double& peakRadius, const double& backgroundInnerRadius, const double& backgroundOuterRadius);
      /// Destructor
      ~PhysicalSphericalPeak();
      /// Setter for the slice point.
      void setSlicePoint(const double&);
      /// Transform the coordinates.
      void movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform);
      /// Draw
      SphericalPeakPrimitives draw(const double& windowHeight, const double& windowWidth, const double& viewWidth, const double& viewHeight) const;

      /// Setter to command whether the background radius should also be shown.
      void showBackgroundRadius(const bool show);

      /// Get the bounding box in natural coordinates.
      PeakBoundingBox getBoundingBox() const;

      /// Get the background outer radius.
      double getRadius() const;

      /// Show the background radius.
      bool getShowBackgroundRadius() const;

    private:
      /// Original origin x=h, y=k, z=l
      const Mantid::Kernel::V3D m_originalOrigin;
      /// Origin md-x, md-y, and md-z
      Mantid::Kernel::V3D m_origin;
      /// actual peak radius
      const double m_peakRadius;
      /// Peak background inner radius
      const double m_backgroundInnerRadius;
      /// Peak background outer radius
      double m_backgroundOuterRadius;
      /// Max opacity
      const double m_opacityMax;
      /// Min opacity
      const double m_opacityMin;
      /// Cached opacity at the distance z from origin
      double m_cachedOpacityAtDistance;
      /// Cached radius at the distance z from origin
      optional_double m_peakRadiusAtDistance;
      /// Cached opacity gradient.
      const double m_cachedOpacityGradient;
      /// Cached radius squared.
      const double m_peakRadiusSQ;
      /// Cached background inner radius sq.
      const double m_backgroundInnerRadiusSQ;
      /// Cached background outer radius sq.
      double m_backgroundOuterRadiusSQ;
      /// Flag to indicate that the background radius should be drawn.
      bool m_showBackgroundRadius;
      /// Inner radius at distance.
      optional_double m_backgroundInnerRadiusAtDistance;
      /// Outer radius at distance.
      optional_double m_backgroundOuterRadiusAtDistance;

      DISABLE_COPY_AND_ASSIGN(PhysicalSphericalPeak)
    };

    typedef boost::shared_ptr<PhysicalSphericalPeak> PhysicalSphericalPeak_sptr;
    typedef std::vector<PhysicalSphericalPeak_sptr> VecPhysicalSphericalPeak;

  }
}

#endif /* MANTID_SLICEVIEWER_PHYSICALSPHERICALPEAK_H_ */
