#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H

#include "MantidKernel/System.h"
#include "MantidGeometry/Crystal/PeakTransform.h"


class QPainter;

namespace MantidQt
{
namespace SliceViewer
{

class PeakBoundingBox;

class DLLExport PeakRepresentation {
public:
  /// Draw
  virtual void draw(QPainter& painter) = 0;
  /// Setter for the slice point
  virtual void setSlicePoint(const double&) = 0;
  /// Transform the coordinates.
  virtual void movePosition(Mantid::Geometry::PeakTransform_sptr peakTransform) = 0;
  /// Get the bounding box.
  virtual PeakBoundingBox getBoundingBox() const = 0;
  /// Set the size of the cross peak in the viewing plane
  virtual void setOccupancyInView(const double fraction) = 0;
  /// Set the size of the cross peak into the viewing plane
  virtual void setOccupancyIntoView(const double fraction) = 0;
  /// Get the effective peak radius.
  virtual double getEffectiveRadius() const = 0;
  /// Get the width occupancy (fractional in the projection plane).
  virtual double getOccupancyInView() const = 0;
  /// Get the depth occupancy (fractional into the projection plane)
  virtual double getOccupancyIntoView() const = 0;
};

}
}
