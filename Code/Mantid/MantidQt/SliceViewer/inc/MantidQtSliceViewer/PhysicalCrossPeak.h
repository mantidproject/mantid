#ifndef MANTID_SLICEVIEWER_PHYSICALCROSSPEAK_H_
#define MANTID_SLICEVIEWER_PHYSICALCROSSPEAK_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidQtSliceViewer/PeakTransform.h"
#include "MantidQtSliceViewer/PeakOverlayView.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    CrossPeakPrimitives Drawing primitive information.
    */
    struct CrossPeakPrimitives 
    {
      int peakHalfCrossWidth;
      int peakHalfCrossHeight;
      int peakLineWidth;
      double peakOpacityAtDistance;
      Mantid::Kernel::V3D peakOrigin;
    };

    /**
    @class PhysicalCrossPeak
    Represents the spacial and physical aspects of a cross peak. Used to handle all physical interactions with other spacial objects.
    */
    class DLLExport PhysicalCrossPeak 
    {
    public:
      /// Constructor
      PhysicalCrossPeak(const Mantid::Kernel::V3D& origin, const double& maxZ, const double& minZ);
      /// Destructor
      ~PhysicalCrossPeak();
      /// Setter for the slice point.
      void setSlicePoint(const double&);
      /// Transform the coordinates.
      void movePosition(PeakTransform_sptr peakTransform);
      /// Draw
      CrossPeakPrimitives draw(const double& windowHeight, const double& windowWidth) const;
      /// Get the bounding box.
      PeakBoundingBox getBoundingBox() const;
      /// Set the size of the cross peak in the viewing plane
      void setOccupancyInView(const double fraction);
      /// Set the size of the cross peak into the viewing plane
      void setOccupancyIntoView(const double fraction);
      /// Get the effective peak radius.
      double getEffectiveRadius() const;
      /// Get the width occupancy (fractional in the projection plane).
      double getOccupancyInView() const;
      /// Get the depth occupancy (fractional into the projection plane)
      double getOccupancyIntoView() const;

    private:
      /// Original origin x=h, y=k, z=l
      const Mantid::Kernel::V3D m_originalOrigin;
      /// Origin md-x, md-y, and md-z
      Mantid::Kernel::V3D m_origin;
      /// Fraction of the view considered for the effectiveRadius.
      double m_intoViewFraction;
      /// effective peak radius
      double m_effectiveRadius;
      /// Max opacity
      const double m_opacityMax;
      /// Min opacity
      const double m_opacityMin;
      /// Cached opacity gradient
      const double m_opacityGradient;
      /// Cross size percentage in y a fraction of the current screen height.
      double m_crossViewFraction;
      /// Cached opacity at the distance z from origin
      double m_opacityAtDistance;
      /// Current slice point.
      double m_slicePoint;

      DISABLE_COPY_AND_ASSIGN(PhysicalCrossPeak)
    };

    typedef boost::shared_ptr<PhysicalCrossPeak> PhysicalCrossPeak_stpr;
    typedef std::vector<PhysicalCrossPeak_stpr> VecPhysicalCrossPeak;
  }
}

#endif /* MANTID_SLICEVIEWER_PHYSICALCROSSPEAK_H_ */
