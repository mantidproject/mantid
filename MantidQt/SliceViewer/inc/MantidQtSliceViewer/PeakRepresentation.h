#ifndef MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H
#define MANTID_SLICEVIEWER_PEAK_REPRESENTATION_H

#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/PeakPrimitives.h"
#include "MantidQtSliceViewer/PeakViewPalette.h"
#include "MantidGeometry/Crystal/PeakTransform.h"


class QPainter;

namespace MantidQt
{
namespace SliceViewer
{
struct PeakRepresentationViewInformation {
  double windowHeight;
  double windowWidth;
  double viewHeight;
  double viewWidth;
  int xOriginWindow;
  int yOriginWindow;
};

class PeakBoundingBox;

class DLLExport PeakRepresentation {
public:
  /// Draw template method
  void draw(QPainter& painter, PeakViewColor& foregroundColor, PeakViewColor& backgroundColor, PeakRepresentationViewInformation viewInformation);
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
  /// Gets the origin
  virtual const Mantid::Kernel::V3D& getOrigin() const = 0;

protected:
  virtual std::shared_ptr<PeakPrimitives> getDrawingInformation(PeakRepresentationViewInformation viewInformation) = 0;
  virtual void doDraw(QPainter& painter, PeakViewColor& foregroundColor, PeakViewColor& backgroundColor, std::shared_ptr<PeakPrimitives> drawingInformation, PeakRepresentationViewInformation viewInformation) = 0;
};

typedef std::shared_ptr<PeakRepresentation> PeakRepresentation_sptr;
typedef std::vector<PeakRepresentation_sptr> VecPeakRepresentation;

}
}

#endif
