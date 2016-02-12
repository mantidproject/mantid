#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_CROSS_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_CROSS_H

#include "MantidQtSliceViewer/PeakRepresentation.h"

namespace
{
struct PeakDrawInformationPeak {
    int peakHalfCrossWidth;
    int peakHalfCrossHeight;
    int peakLineWidth;
    double peakOpacityAtDistance;
    Mantid::Kernel::V3D peakOrigin;
};
}

namespace MantidQt
{
namespace SliceViewer
{

class PeakRepresentationCross : public PeakRepresentation
{
public:
    PeakRepresentationCross(const Mantid::Kernel::V3D& origin, const double& maxZ, const double& minZ);
    /// Setter for the slice point
    void setSlicePoint(const double &) override;
    /// Transform the coordinates.
    void
    movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) override;
    /// Get the bounding box.
    PeakBoundingBox getBoundingBox() const override;
    /// Set the size of the cross peak in the viewing plane
    void setOccupancyInView(const double fraction) override;
    /// Set the size of the cross peak into the viewing plane
    void setOccupancyIntoView(const double fraction) override;
    /// Get the effective peak radius.
    double getEffectiveRadius() const override;
    /// Get the width occupancy (fractional in the projection plane).
    double getOccupancyInView() const override;
    /// Get the depth occupancy (fractional into the projection plane)
    double getOccupancyIntoView() const override;
    /// Get the origin
    const Mantid::Kernel::V3D& getOrigin() const override;
    /// Show the background radius
    void showBackgroundRadius(const bool show) override;

protected:
    std::shared_ptr<PeakPrimitives> getDrawingInformation(
        PeakRepresentationViewInformation viewInformation) override;
    void doDraw(QPainter &painter, PeakViewColor& foregroundColor, PeakViewColor& backgroundColor,
                std::shared_ptr<PeakPrimitives> drawingInformation, PeakRepresentationViewInformation viewInformation) override;

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
};
}
}
#endif
