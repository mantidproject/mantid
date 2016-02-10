#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_ELLIPSOID_H

#include "MantidQtSliceViewer/PeakRepresentation.h"

namespace MantidQt
{
namespace SliceViewer
{

class PeakRepresentationEllipsoid : public PeakRepresentation
{
public:
    /// Draw
    void draw(QPainter &painter) override;
    /// Setter for the slice point
    void setSlicePoint(const double &) override;
    /// Transform the coordinates.
    void movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) override;
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
};
}
}
